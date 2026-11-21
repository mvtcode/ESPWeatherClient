#include <Arduino.h>
#include <WiFi.h>
#include <ESPWeatherClient.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Dang ket noi WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");

    // Khoi tao thanh pho TP. Ho Chi Minh (10.8231, 106.6297)
    ESPWeatherClient::init("TP. Hồ Chí Minh", 10.8231f, 106.6297f);

    Serial.println("Dang tai du lieu thoi tiet tu Open-Meteo...");
    if (ESPWeatherClient::fetchNow()) {
        WeatherInfo w = ESPWeatherClient::getWeather();
        Serial.println("=== THOI TIET HIEN TAI ===");
        Serial.printf("Thanh pho: %s\n", w.city_name);
        Serial.printf("Nhiet do: %.1f C (Cam giac: %.1f C)\n", w.temperature, w.feels_like);
        Serial.printf("Trang thai: %s (Ma code: %d)\n", w.condition_text, w.weather_code);
        Serial.printf("Do am: %d%%\n", w.humidity);
        Serial.printf("Toc do gio: %.1f km/h\n", w.wind_speed);
        Serial.printf("Chi so UV: %.1f\n", w.uv_index);
    } else {
        Serial.println("Lay du lieu thoi tiet that bai!");
    }
}

void loop() {
    ESPWeatherClient::update(WiFi.status() == WL_CONNECTED);
    delay(1000);
}
