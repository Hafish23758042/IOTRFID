#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>

// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbykDnsA1dzwNhhhw3sBnXbNmjVJqD-JmFufEtyVfJLKLGota90-dzIdG5G8D8B03U5yJA";

// Enter network credentials:
const char* ssid     = "POPO";
const char* password = "12345678";

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

// Google Sheets setup (do not edit)
const char* host        = "script.google.com";
const int   httpsPort   = 443;
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

// RFID setup
#define RST_PIN  0   //D3
#define SS_PIN   2   //D4
#define BUZZER   16  //D0

MFRC522 mfrc522(SS_PIN, RST_PIN); // Inisialisasi modul RFID
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;

// Blok RFID yang akan dibaca
int blocks[] = {4, 5, 6, 8, 9};
#define total_blocks (sizeof(blocks) / sizeof(blocks[0]))

byte readBlockData[18];
String nama;

void setup() {
  Serial.begin(9600);        
  delay(10);
  Serial.println('\n');
  SPI.begin();
  mfrc522.PCD_Init(); // Inisialisasi RFID
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for(int i = 0; i < 5; i++) { 
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      String msg = "Connected. OK";
      Serial.println(msg);
      delay(1000);
      break;
    } else {
      Serial.println("Connection failed. Retrying...");
    }
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    delay(3000);
    return;
  }
}

void loop() {
  Serial.println("Scan your Tag");

  // Cek apakah ada kartu baru
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Membaca data dari blok RFID
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));
  String values = "", data;

  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);
    data = String((char*)readBlockData);
    data.trim();
    
    if (i == 0) {
      nama = data; // Nama diambil dari blok pertama
      values = "\"" + data + ",";
    } else if (i == total_blocks - 1) {
      values += data + "\"}";
    } else {
      values += data + ",";
    }
  }

  payload = payload_base + values;

  // Publikasikan data ke Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);

  if (client != nullptr && client->connected()) {
    if (client->POST(url, host, payload)) {
      Serial.println("Data sent successfully!");
      Serial.println("Hallo, " + nama);

      // Indikator berhasil
      digitalWrite(BUZZER, HIGH);
      delay(500);
      digitalWrite(BUZZER, LOW);
    } else {
      Serial.println("Failed to send data");
    }
  } else {
    Serial.println("Client not connected, retrying...");
    if (client) {
      delete client;
    }
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    client->connect(host, httpsPort);
  }

  // Hentikan kartu
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // Delay sebelum membaca kartu berikutnya
  delay(2000);
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) { 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Autentikasi blok
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  } else {
    Serial.println("Authentication success");
  }
  
  // Deklarasi bufferLen
  byte bufferLen = 18;

  // Membaca data dari blok
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");  
  }
}
