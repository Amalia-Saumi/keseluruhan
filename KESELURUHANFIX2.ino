#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Konfigurasi Sensor DHT22
#define DHTTYPE DHT22
#define DHTPIN 2
DHT dht(DHTPIN, DHTTYPE);

// Konfigurasi Sensor Debu GP2Y1010AU0F
int measurePin = A0;   // Hubungkan sensor debu ke pin A0 Arduino
int ledPower = 3;      // Hubungkan pin LED driver sensor debu ke pin D3 Arduino
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

// Konfigurasi Driver Motor L298N
int ENA = 5;   // PWM pin untuk kipas
int ENB = 8;   // PWM pin untuk blower
int IN1 = 6;   // Kontrol arah kipas
int IN2 = 7;   // Kontrol arah kipas
int IN3 = 9;   // Kontrol arah blower
int IN4 = 10;  // Kontrol arah blower

// Konfigurasi Relay Humidifier
const int relayPin = 4;   // Pin digital untuk mengontrol relay
const int humidifierTriggerLevel = 60; // Level kelembaban untuk mengaktifkan humidifier

// Konfigurasi LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Sesuaikan alamat I2C jika perlu

// Variabel untuk mengatur tampilan bergantian di LCD
unsigned long previousMillis = 0;
const long interval = 3000; // Interval waktu 3 detik untuk ganti tampilan
int displayState = 0;       // State untuk tampilan bergantian

// Fungsi untuk menghitung rata-rata beberapa pembacaan sensor debu
float getAverageDustDensity(int samples) {
  float totalDensity = 0;
  for (int i = 0; i < samples; i++) {
    digitalWrite(ledPower, LOW);    // Nyalakan LED
    delayMicroseconds(samplingTime);
  
    voMeasured = analogRead(measurePin);   // Baca nilai debu
  
    delayMicroseconds(deltaTime);
    digitalWrite(ledPower, HIGH);   // Matikan LED
    delayMicroseconds(sleepTime);
  
    // Hitung tegangan dari nilai analog
    calcVoltage = voMeasured * (5.0 / 1024.0);

    // Hitung densitas debu
    dustDensity = 170 * calcVoltage - 0.1;
    totalDensity += dustDensity;
  }
  return totalDensity / samples;
}

void setup() {
  // Inisialisasi komunikasi serial
  Serial.begin(9600);
  
  // Inisialisasi sensor DHT22
  dht.begin();
  
  // Inisialisasi pin LED sensor debu
  pinMode(ledPower, OUTPUT);
  
  // Inisialisasi pin Driver Motor
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Inisialisasi pin relay untuk humidifier
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Memastikan relay dalam keadaan OFF (HIGH = off, LOW = on)

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Inisialisasi Sistem");
  delay(2000);
}

void loop() {
  // Membaca data dari sensor DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Cek kegagalan pembacaan sensor DHT
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Gagal membaca sensor DHT!");
  } else {
    Serial.print("Kelembaban: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Suhu: ");
    Serial.print(temperature);
    Serial.println(" *C");
  }

  // Mengontrol relay berdasarkan kelembaban
  if (humidity < humidifierTriggerLevel) {
    digitalWrite(relayPin, LOW); // Aktifkan relay (ON)
    Serial.println("Humidifier ON");
  } else {
    digitalWrite(relayPin, HIGH); // Nonaktifkan relay (OFF)
    Serial.println("Humidifier OFF");
  }

  // Membaca data dari sensor debu dengan filter rata-rata (10 sampel)
  dustDensity = getAverageDustDensity(10);

  // Cetak pembacaan sensor debu ke Serial Monitor
  Serial.print("Densitas Debu: ");
  Serial.print(dustDensity);
  Serial.println(" µg/m3");

  // Mengontrol kipas berdasarkan suhu
  if (temperature > 30) {
    // Nyalakan kipas jika suhu lebih dari 30°C
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 255); // Kecepatan maksimal
    Serial.println("Kipas ON");
  } else if (temperature <= 25) {
    // Matikan kipas jika suhu kurang dari atau sama dengan 25°C
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
    Serial.println("Kipas OFF");
  }

  // Mengontrol blower berdasarkan densitas debu
  if (dustDensity > 150) {
    // Nyalakan blower jika densitas debu lebih dari 150 µg/m³
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 255); // Kecepatan maksimal
    Serial.println("Blower ON");
  } else if (dustDensity < 30) {
    // Matikan blower jika debu tidak terdeteksi (densitas debu kurang dari 30 µg/m³)
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);
    Serial.println("Blower OFF");
  }

  // Menampilkan data pada LCD secara bergantian
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Bergantian tampilan di LCD
    lcd.clear();
    switch (displayState) {
      case 0:
        lcd.setCursor(0, 0);
        lcd.print("Suhu: ");
        lcd.print(temperature);
        lcd.print("C");
        lcd.setCursor(0, 1);
        lcd.print("Kelemb: ");
        lcd.print(humidity);
        lcd.print("%");
        displayState = 1;
        break;
        
      case 1:
        lcd.setCursor(0, 0);
        lcd.print("Debu: ");
        lcd.print(dustDensity);
        lcd.print(" ug/m3");
        displayState = 2;
        break;

      case 2:
        lcd.setCursor(0, 0);
        lcd.print("Tegangan:");
        lcd.setCursor(0, 1);
        lcd.print(calcVoltage);
        lcd.print(" V");
        displayState = 0;
        break;
    }
  }

  delay(100); // Perbarui dengan jeda singkat untuk stabilitas
}
