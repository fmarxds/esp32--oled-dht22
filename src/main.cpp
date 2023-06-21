#include <Arduino.h>
#include <WiFiMulti.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DEVICE "ESP32"

#define WIFI_SSID "<WIFI_SSID>"
#define WIFI_PASSWORD "<WIFI_PASSWORD>"

#define INFLUXDB_URL "<INFLUXDB_URL>"
#define INFLUXDB_TOKEN "<INFLUXDB_TOKEN>"
#define INFLUXDB_ORG "<INFLUXDB_ORG>"
#define INFLUXDB_BUCKET "<INFLUXDB_BUCKET>"
#define TZ_INFO "UTC-3"

#define DHT22_PIN 19

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SLEEP_TIME 60000

WiFiMulti wifiMulti;
DHT dht(DHT22_PIN, DHT22);
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
InfluxDBClient influxDBClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point pointWeather("weather");

void setupWifi() {
    WiFiClass::mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi network.");
    while (wifiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\nConnected!");
}

void setupDht() {
    dht.begin();
}

void setupOledDisplay() {
    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Failed to start OLED Display");
    }
    delay(2000);
    oled.clearDisplay();
    oled.display();
}

void setupInfluxDB() {
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    if (influxDBClient.validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(influxDBClient.getServerUrl());
    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(influxDBClient.getLastErrorMessage());
    }
}

void setupInfluxDBDataPoints() {
    pointWeather.addTag("device", DEVICE);
    pointWeather.addTag("SSID", WiFi.SSID());
}

void setup() {
    Serial.begin(9600);
    setupDht();
    setupOledDisplay();
    setupWifi();
    setupInfluxDB();
    setupInfluxDBDataPoints();
}

void updateDisplay(float humidity, float heatIndex) {
    oled.clearDisplay();
    oled.setTextSize(2);
    oled.setTextColor(WHITE);
    oled.setCursor(0, 1);
    oled.printf("%.2f C\n%.2f g/m3", heatIndex, humidity);
    oled.display();
}

void updateInfluxDBDataPoints(float temperature, float humidity, float heatIndex) {
    pointWeather.clearFields();
    pointWeather.addField("temperature", temperature);
    pointWeather.addField("humidity", humidity);
    pointWeather.addField("heat_index", heatIndex);
    Serial.println(pointWeather.toLineProtocol());

    if (wifiMulti.run() != WL_CONNECTED) {
        Serial.println("Lost connection to Wi-Fi network.");
        setupWifi();
    }

    if (!influxDBClient.writePoint(pointWeather)) {
        Serial.print("InfluxDB send Point failed: ");
        Serial.println(influxDBClient.getLastErrorMessage());
    }
}

void loop() {

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

    updateDisplay(humidity, heatIndex);
    updateInfluxDBDataPoints(temperature, humidity, heatIndex);

    delay(SLEEP_TIME);

}