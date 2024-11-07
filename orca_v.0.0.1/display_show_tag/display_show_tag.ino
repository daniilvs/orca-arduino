#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// ДИСПЛЕЙ
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     9
#define OLED_DC        8
#define OLED_CS        10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// RFID МОДУЛЬ
const int BUFFER_SIZE = 14; // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const int DATA_SIZE = 10; // 10byte data (2byte version + 8byte tag)
const int DATA_VERSION_SIZE = 2; // 2byte version (actual meaning of these two bytes may vary)
const int DATA_TAG_SIZE = 8; // 8byte tag
const int CHECKSUM_SIZE = 2; // 2byte checksum
SoftwareSerial ssrfid = SoftwareSerial(2,8);

uint8_t buffer[BUFFER_SIZE]; // used to store an incoming data frame
int buffer_index = 0;

// ДЖОЙСТИК
const int VRx = A0;  // Ось X для джойстика
const int VRy = A1;  // Ось Y для джойстика
const int SW = 2;    // Кнопка джойстика

// SoftwareSerial RFID(2, 3); // RX на D2 и TX на D3

int menuIndex = 0;
int totalTags = 0;
char savedTags[10][20]; // Память для 10 меток по 20 символов каждая

void setup() {
  Serial.begin(9600);
  ssrfid.begin(9600);
  ssrfid.listen();
  Serial.println("INIT DONE");
  
  pinMode(SW, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true); // Остановите выполнение, если инициализация не удалась
  }
  display.display();
  delay(2000);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Reading Tag..."));
  display.display();
}

void loop() {
  if (ssrfid.available() > 0){
    bool call_extract_tag = false;
    int ssvalue = ssrfid.read(); // read
  if (ssvalue == -1) { // no data was read
    return;
  }

  if (ssvalue == 2) { // RDM630/RDM6300 found a tag => tag incoming
    buffer_index = 0;
  } else if (ssvalue == 3) { // tag has been fully transmitted
    call_extract_tag = true; // extract tag at the end of the function call
  }
  if (buffer_index >= BUFFER_SIZE) { // checking for a buffer overflow (It's very unlikely that an buffer overflow comes up!)
    Serial.println("Error: Buffer overflow detected!");
    return;
  }
  buffer[buffer_index++] = ssvalue; // everything is alright => copy current value to buffer
  if (call_extract_tag == true) {
    if (buffer_index == BUFFER_SIZE) {
      unsigned tag = extract_tag();
    } else { // something is wrong... start again looking for preamble (value: 2) 
      buffer_index = 0;
      return;
      }
    }
  }
}

unsigned extract_tag() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  // display.setCursor(0, 0);
  // display.println(F("Reading Tag..."));
  // display.display();

  uint8_t msg_head = buffer[0];
  uint8_t *msg_data = buffer + 1; // 10 byte => data contains 2byte version + 8byte tag
  uint8_t *msg_data_version = msg_data;
  uint8_t *msg_data_tag = msg_data + 2;
  uint8_t *msg_checksum = buffer + 11; // 2 byte
  uint8_t msg_tail = buffer[13];
  // print message that was sent from RDM630/RDM6300
  Serial.println("--------");
  Serial.print("Message-Head: ");
  Serial.println(msg_head);
  Serial.println("Message-Data (HEX): ");
  for (int i = 0; i < DATA_VERSION_SIZE; ++i) {
    Serial.print(char(msg_data_version[i]));
  }
  Serial.println(" (version)");
  for (int i = 0; i < DATA_TAG_SIZE; ++i) {
    Serial.print(char(msg_data_tag[i]));
  }
  Serial.println(" (tag)");
  Serial.print("Message-Checksum (HEX): ");
  for (int i = 0; i < CHECKSUM_SIZE; ++i) {
    Serial.print(char(msg_checksum[i]));
  }
  Serial.println("");
  Serial.print("Message-Tail: ");
  Serial.println(msg_tail);
  Serial.println("--");
  long tag = hexstr_to_value(msg_data_tag, DATA_TAG_SIZE);
  Serial.print("Extracted Tag: ");
  Serial.println(tag);
  long checksum = 0;
  for (int i = 0; i < DATA_SIZE; i+= CHECKSUM_SIZE) {
    long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);
    checksum ^= val;
  }
  Serial.print("Extracted Checksum (HEX): ");
  Serial.print(checksum, HEX);
  if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) { // compare calculated checksum to retrieved checksum
    Serial.print(" (OK)"); // calculated checksum corresponds to transmitted checksum!
  } else {
    Serial.print(" (NOT OK)"); // checksums do not match
  }
  Serial.println("");
  Serial.println("--------");

  display.clearDisplay();

  display.print("Head: ");
  display.println(msg_head);
  display.println("Data (HEX): ");
  for (int i = 0; i < DATA_VERSION_SIZE; ++i) {
    display.print(char(msg_data_version[i]));
  }
  display.println(" (version)");
  for (int i = 0; i < DATA_TAG_SIZE; ++i) {
    display.print(char(msg_data_tag[i]));
  }
  display.println(" (tag)");
  display.print("Checksum(HEX): ");
  for (int i = 0; i < CHECKSUM_SIZE; ++i) {
    display.print(char(msg_checksum[i]));
  }
  display.println("");
  display.print("Tail: ");
  display.println(msg_tail);
  display.print("Tag: ");
  display.println(tag);
  display.print("Checksum(HEX): ");
  display.print(checksum, HEX);
  if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) { // compare calculated checksum to retrieved checksum
    display.print("OK"); // calculated checksum corresponds to transmitted checksum!
  } else {
    display.print("!OK"); // checksums do not match
  }
  display.display();

  return tag;
}
long hexstr_to_value(char *str, unsigned int length) { // converts a hexadecimal value (encoded as ASCII string) to a numeric value
  char* copy = malloc((sizeof(char) * length) + 1); 
  memcpy(copy, str, sizeof(char) * length);
  copy[length] = '\0';
  // the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
  long value = strtol(copy, NULL, 16); // strtol converts a null-terminated string to a long value
  free(copy); // clean up
  return value;
}
