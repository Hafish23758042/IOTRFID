#include <SPI.h>
#include <MFRC522.h>

const uint8_t RST_PIN = D3;
const uint8_t SS_PIN = D4;

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte bufferLen = 18;
byte readBlockData[18];

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Scan a MIFARE 1K Tag...");
}

void loop() {
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("\n**Card Detected**");
  Serial.print("UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  byte buffer[18];
  Serial.setTimeout(20000);

  inputAndWrite("Nama", buffer, 4);
  inputAndWrite("npm", buffer, 5);
  inputAndWrite("Jurusan", buffer, 6);
  inputAndWrite("Program Studi", buffer, 8);
  inputAndWrite("Jenis Kelamin", buffer, 9);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void inputAndWrite(const char *label, byte *buffer, int blockNum) {
  Serial.print(label);
  Serial.println(", akhiri dengan #:");
  byte len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';

  if (writeBlock(blockNum, buffer)) {
    readBlock(blockNum, readBlockData);
    dumpSerial(blockNum, readBlockData);
  }
}

bool writeBlock(int blockNum, byte *buffer) {
  if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
    Serial.println("Auth failed");
    return false;
  }
  if (mfrc522.MIFARE_Write(blockNum, buffer, 16) != MFRC522::STATUS_OK) {
    Serial.println("Write failed");
    return false;
  }
  return true;
}

bool readBlock(int blockNum, byte *buffer) {
  if (mfrc522.MIFARE_Read(blockNum, buffer, &bufferLen) != MFRC522::STATUS_OK) {
    Serial.println("Read failed");
    return false;
  }
  return true;
}

void dumpSerial(int blockNum, byte *buffer) {
  Serial.print("Data di blok ");
  Serial.print(blockNum);
  Serial.print(": ");
  for (int i = 0; i < 16; i++) Serial.write(buffer[i]);
  Serial.println();
}
