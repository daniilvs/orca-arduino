#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     9
#define OLED_DC        8
#define OLED_CS        10

const int VRx = A0;  // Ось X для джойстика
const int VRy = A1;  // Ось Y для джойстика
const int SW = 2;    // Кнопка джойстика

SoftwareSerial RFID(2, 3); // RX на D2 и TX (не используется) на D3

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

int menuIndex = 0;
int totalTags = 0;
char savedTags[10][20]; // Память для 10 меток по 20 символов каждая

// Прототипы функций
void displayMenu();
void executeMenuAction(int index);
void readTag(char *rfidTag);
void saveTag();
// void emulateTag();
void deleteTag();
void saveTagsToEEPROM();
void loadTagsFromEEPROM();

void setup() {
  Serial.begin(9600);
  RFID.begin(9600);
  
  pinMode(SW, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true); // Остановите выполнение, если инициализация не удалась
  }

  display.clearDisplay();
  loadTagsFromEEPROM();
  displayMenu();
}

void loop() {
  int xValue = analogRead(VRx);
  int yValue = analogRead(VRy);
  bool buttonPressed = digitalRead(SW) == LOW;

  // Навигация по меню
  if (yValue < 400 && menuIndex > 0) { // Вверх
    menuIndex--;
    displayMenu();
    delay(200);
  } else if (yValue > 600 && menuIndex < 3) { // Вниз
    menuIndex++;
    displayMenu();
    delay(200);
  }
  
  // Подтверждение выбора
  if (buttonPressed) {
    delay(200);
    executeMenuAction(menuIndex);
  }
}

// Функция для отображения меню
void displayMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 1);

  for (int i = 0; i < 4; i++) {
    if (i == menuIndex) display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Выделение выбранного пункта
    else display.setTextColor(SSD1306_WHITE);

    switch(i) {
      case 0:
        display.println(F("1. Read Tag"));
        break;
      case 1:
        display.println(F("2. Save Tag"));
        break;
      // case 2:
      //   display.println(F("3. Emulate Tag"));
      //   break;
      // case 3:
      //   display.println(F("4. Delete Tag"));
      //   break;
      case 2:
        display.println(F("3. Delete Tag"));
        break;
    }
  }

  display.display();
}

// Выполнение действия на основе выбора в меню
void executeMenuAction(int index) {
  switch (index) {
    case 0: {
      char rfidTag[20];
      readTag(rfidTag);
      break;
    }
    case 1:
      saveTag();
      break;
    // case 2:
    //   emulateTag();
    //   break;
    // case 3:
    //   deleteTag();
    //   break;
    case 2:
      deleteTag();
      break;
  }
  displayMenu(); // Возвращаемся к меню после выполнения действия
}

// Чтение метки
void readTag(char *rfidTag) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Reading Tag..."));
  display.display();

  int index = 0;
  while (RFID.available() && index < 19) {
    char c = RFID.read();
    rfidTag[index++] = c;
  }
  rfidTag[index] = '\0'; // Завершающий символ строки

  display.clearDisplay();
  display.setCursor(0, 0);
  if (index == 0) {
    display.println(F("No Tag Found"));
  } else {
    display.println(F("Tag ID:"));
    display.println(rfidTag);
  }
  display.display();
  delay(4000);
}

// Функция для сохранения метки
void saveTag() {
  char rfidTag[20];
  readTag(rfidTag);

  if (rfidTag[0] == '\0') {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("No Tag to Save"));
    display.display();
    delay(4000);
    return;
  }

  if (totalTags < 10) {
    strncpy(savedTags[totalTags], rfidTag, 20); // Сохранение в массив
    totalTags++;
    saveTagsToEEPROM(); // Сохранение в EEPROM
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Tag Saved!"));
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Memory Full!"));
  }
  display.display();
  delay(10000);
}

// Эмуляция метки (выводит сохраненные метки на дисплей)
// void emulateTag() {
//   if (totalTags == 0) {
//     display.clearDisplay();
//     display.setCursor(0, 0);
//     display.println(F("No saved tags!"));
//     display.display();
//     delay(10000);
//     return;
//   }
  
//   for (int i = 0; i < totalTags; i++) {
//     display.clearDisplay();
//     display.setCursor(0, 0);
//     display.print(F("Tag "));
//     display.print(i + 1);
//     display.print(F(": "));
//     display.println(savedTags[i]);
//     display.display();
//     delay(1000);
//   }
// }

// Удаление метки
void deleteTag() {
  if (totalTags == 0) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("No saved tags!"));
    display.display();
    delay(2000);
    return;
  }
  
  totalTags--;
  saveTagsToEEPROM();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Tag Deleted"));
  display.display();
  delay(2000);
}

// Сохранение меток в EEPROM
void saveTagsToEEPROM() {
  for (int i = 0; i < totalTags; i++) {
    for (int j = 0; j < 20; j++) {
      EEPROM.write(i * 20 + j, savedTags[i][j]);
    }
  }
}

// Загрузка меток из EEPROM
void loadTagsFromEEPROM() {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 20; j++) {
      savedTags[i][j] = EEPROM.read(i * 20 + j);
    }
    if (savedTags[i][0] != '\0') {
      totalTags++;
    }
  }
}
