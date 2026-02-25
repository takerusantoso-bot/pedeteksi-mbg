#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Helper wajib untuk Firebase
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// --- KONFIGURASI OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- KONFIGURASI WIFI & FIREBASE ---
#define WIFI_SSID "iniwifi"      
#define WIFI_PASSWORD "12345678"   
#define API_KEY "AIzaSyATxmP784qq4qXuZaEv0rFK1AWMQriEj8I"
#define DATABASE_URL "https://makanan-basi-default-rtdb.asia-southeast1.firebasedatabase.app/" // Tanpa "/" di akhir

// --- KONFIGURASI PIN ---
#define MQ_PIN 34
#define BUZZER_PIN 25
#define LED_RED 14
#define LED_YELLOW 27
#define LED_GREEN 26

// --- OBJEK FIREBASE ---
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

int gasValue = 0;
// Threshold bertingkat
int thresholdWaspada = 1500; 
int thresholdBahaya = 2500; 

unsigned long lastMillisWifi = 0;
unsigned long intervalReboot = 300000; 

void setup() {
  Serial.begin(115200);
  
  // Setup Output
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED Gagal!"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Traffic Light Init...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); 
  delay(1000);
  WiFi.setSleep(false); 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 30) {
    delay(500);
    Serial.print(".");
    counter++;
  }

  // Firebase Setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // Auto Reboot jika WiFi putus
  if (WiFi.status() != WL_CONNECTED) {
    if (lastMillisWifi == 0) lastMillisWifi = millis();
    if (millis() - lastMillisWifi > intervalReboot) ESP.restart();
  } else {
    lastMillisWifi = 0;
  }

  gasValue = analogRead(MQ_PIN);
  String statusMsg = "";
  
  // --- LOGIKA TRAFFIC LIGHT & BUZZER ---
  if (gasValue >= thresholdBahaya) {
    // MERAH
    statusMsg = "BAHAYA!";
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
  } 
  else if (gasValue >= thresholdWaspada && gasValue < thresholdBahaya) {
    // KUNING
    statusMsg = "WASPADA";
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  } 
  else {
    // HIJAU
    statusMsg = "AMAN";
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Tampilan OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("STATUS: ");
  display.print(WiFi.status() == WL_CONNECTED ? "[ON]" : "[OFF]");
  
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("GAS:");
  display.print(gasValue);

  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(statusMsg);
  display.display();

  // Kirim ke Firebase
  if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
    Firebase.RTDB.setInt(&fbdo, "/foodweb/gas_value", gasValue);
    Firebase.RTDB.setString(&fbdo, "/foodweb/status", statusMsg);
  }

  delay(1000); 
}