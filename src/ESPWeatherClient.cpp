#include "ESPWeatherClient.h"
#include <WiFi.h>
#include <HTTPClient.h>

#if defined(DEBUG_ESP_WEATHER) || defined(ENABLE_SERIAL_LOG)
#define EWC_LOG_I(fmt, ...) Serial.printf("[Weather][I] " fmt "\n", ##__VA_ARGS__)
#define EWC_LOG_W(fmt, ...) Serial.printf("[Weather][W] " fmt "\n", ##__VA_ARGS__)
#define EWC_LOG_E(fmt, ...) Serial.printf("[Weather][E] " fmt "\n", ##__VA_ARGS__)
#define EWC_LOG_D(fmt, ...) Serial.printf("[Weather][D] " fmt "\n", ##__VA_ARGS__)
#else
#define EWC_LOG_I(...)
#define EWC_LOG_W(...)
#define EWC_LOG_E(...)
#define EWC_LOG_D(...)
#endif

WeatherInfo ESPWeatherClient::current_weather;
SemaphoreHandle_t ESPWeatherClient::weatherMutex = NULL;
float ESPWeatherClient::latitude = 21.0285f;
float ESPWeatherClient::longitude = 105.8542f;
char ESPWeatherClient::city[48] = "Hà Nội";
uint32_t ESPWeatherClient::last_fetch_time = 0;
bool ESPWeatherClient::is_fetching = false;
uint32_t ESPWeatherClient::update_interval_ms = 30 * 60 * 1000; // 30 phut

const char* ESPWeatherClient::getWeatherDescription(int code) {
    if (code == 0) return "Trời nắng";
    if (code == 1 || code == 2) return "Ít mây";
    if (code == 3) return "Nhiều mây";
    if (code == 45 || code == 48) return "Sương mù";
    if (code >= 51 && code <= 57) return "Mưa phùn";
    if (code >= 61 && code <= 67) return "Trời mưa";
    if (code >= 71 && code <= 77) return "Có tuyết";
    if (code >= 80 && code <= 82) return "Mưa rào";
    if (code >= 95 && code <= 99) return "Giông bão";
    return "Trời quang";
}

static bool extractJsonRawBuf(const char* str, int strLen, int startPos,
                               const char* key, char* outBuf, int outBufSize) {
    if (!str || !key || !outBuf || outBufSize <= 0) return false;
    int keyLen = strlen(key);
    const char* found = str + startPos;
    int remaining = strLen - startPos;
    const char* pos = nullptr;
    for (int i = 0; i <= remaining - keyLen; i++) {
        if (strncmp(found + i, key, keyLen) == 0) {
            pos = found + i + keyLen;
            break;
        }
    }
    if (!pos) return false;
    while (*pos == ' ' || *pos == ':') pos++;
    if (*pos == '"') {
        pos++;
        const char* end = strchr(pos, '"');
        if (!end) return false;
        int len = (int)(end - pos);
        if (len >= outBufSize) len = outBufSize - 1;
        strncpy(outBuf, pos, len);
        outBuf[len] = '\0';
        return true;
    }
    const char* end = pos;
    while (*end && *end != ',' && *end != '}' && *end != ']') end++;
    int len = (int)(end - pos);
    while (len > 0 && pos[len-1] == ' ') len--;
    if (len >= outBufSize) len = outBufSize - 1;
    strncpy(outBuf, pos, len);
    outBuf[len] = '\0';
    return (len > 0);
}

static bool executeWeatherFetch(float lat, float lon, const char *cityName, WeatherInfo &outWeather) {
    HTTPClient http;
    http.setTimeout(12000);
    http.setConnectTimeout(10000);

    char url[256];
    snprintf(url, sizeof(url),
             "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m,uv_index",
             lat, lon);

    EWC_LOG_D("Fetching Open-Meteo: %s", url);
    http.begin(url);
    http.setUserAgent("ESP32WeatherClient/1.0");
    int httpCode = http.GET();
    bool success = false;

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        const char* payloadBuf = payload.c_str();
        int payloadLen = (int)payload.length();

        const char* curKey = "\"current\":";
        const char* curPos = strstr(payloadBuf, curKey);
        int curIdx = curPos ? (int)(curPos - payloadBuf) : -1;

        if (curIdx != -1) {
            char tempStr[16] = {0};
            char humStr[16] = {0};
            char feelStr[16] = {0};
            char codeStr[16] = {0};
            char windStr[16] = {0};
            char uvStr[16] = {0};

            extractJsonRawBuf(payloadBuf, payloadLen, curIdx, "\"temperature_2m\":", tempStr, sizeof(tempStr));
            extractJsonRawBuf(payloadBuf, payloadLen, curIdx, "\"relative_humidity_2m\":", humStr, sizeof(humStr));
            extractJsonRawBuf(payloadBuf, payloadLen, curIdx, "\"apparent_temperature\":", feelStr, sizeof(feelStr));
            extractJsonRawBuf(payloadBuf, payloadLen, curIdx, "\"weather_code\":", codeStr, sizeof(codeStr));
            extractJsonRawBuf(payloadBuf, payloadLen, curIdx, "\"wind_speed_10m\":", windStr, sizeof(windStr));
            extractJsonRawBuf(payloadBuf, payloadLen, curIdx, "\"uv_index\":", uvStr, sizeof(uvStr));

            if (strlen(feelStr) == 0 && strlen(tempStr) > 0) {
                strncpy(feelStr, tempStr, sizeof(feelStr) - 1);
            }

            if (strlen(tempStr) > 0) {
                outWeather.temperature = atof(tempStr);
                strncpy(outWeather.temp_str, tempStr, sizeof(outWeather.temp_str) - 1);

                outWeather.humidity = atoi(humStr);
                strncpy(outWeather.humidity_str, strlen(humStr) > 0 ? humStr : "--", sizeof(outWeather.humidity_str) - 1);

                outWeather.feels_like = atof(feelStr);
                strncpy(outWeather.feels_like_str, feelStr, sizeof(outWeather.feels_like_str) - 1);

                outWeather.weather_code = atoi(codeStr);
                outWeather.wind_speed = atof(windStr);
                strncpy(outWeather.wind_speed_str, strlen(windStr) > 0 ? windStr : "--", sizeof(outWeather.wind_speed_str) - 1);

                outWeather.uv_index = atof(uvStr);
                strncpy(outWeather.uv_index_str, strlen(uvStr) > 0 ? uvStr : "--", sizeof(outWeather.uv_index_str) - 1);

                strncpy(outWeather.condition_text,
                        ESPWeatherClient::getWeatherDescription(outWeather.weather_code),
                        sizeof(outWeather.condition_text) - 1);

                if (cityName && strlen(cityName) > 0) {
                    strncpy(outWeather.city_name, cityName, sizeof(outWeather.city_name) - 1);
                }

                outWeather.is_valid = true;
                outWeather.last_update_time = millis();
                success = true;
            }
        }
    } else {
        EWC_LOG_W("HTTP Error: %d", httpCode);
    }

    http.end();
    return success;
}

void ESPWeatherClient::init(const char *cityName, float lat, float lon) {
    if (weatherMutex == NULL) {
        weatherMutex = xSemaphoreCreateMutex();
    }
    if (cityName && strlen(cityName) > 0) {
        strncpy(city, cityName, sizeof(city) - 1);
        city[sizeof(city) - 1] = '\0';
    }
    latitude = lat;
    longitude = lon;
    strncpy(current_weather.city_name, city, sizeof(current_weather.city_name) - 1);
    last_fetch_time = 0;
    is_fetching = false;
    EWC_LOG_I("Initialized for City: %s (%.4f, %.4f)", city, latitude, longitude);
}

void ESPWeatherClient::setLocation(const char *cityName, float lat, float lon) {
    if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (cityName && strlen(cityName) > 0) {
            strncpy(city, cityName, sizeof(city) - 1);
            city[sizeof(city) - 1] = '\0';
        }
        latitude = lat;
        longitude = lon;
        strncpy(current_weather.city_name, city, sizeof(current_weather.city_name) - 1);
        xSemaphoreGive(weatherMutex);
    }
    last_fetch_time = 0;
    EWC_LOG_I("Location changed to: %s (%.4f, %.4f)", cityName, lat, lon);
}

void ESPWeatherClient::setUpdateInterval(uint32_t intervalMinutes) {
    if (intervalMinutes < 5) intervalMinutes = 5;
    update_interval_ms = intervalMinutes * 60 * 1000;
}

void ESPWeatherClient::fetchWeatherTask(void *param) {
    WeatherInfo info;
    if (executeWeatherFetch(latitude, longitude, city, info)) {
        if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            current_weather = info;
            xSemaphoreGive(weatherMutex);
        }
    }
    is_fetching = false;
    vTaskDelete(NULL);
}

void ESPWeatherClient::update(bool wifiConnected, bool force, uint32_t intervalMinutes) {
    if (!wifiConnected) return;

    uint32_t interval = (intervalMinutes > 0) ? (intervalMinutes * 60 * 1000) : update_interval_ms;
    if (!current_weather.is_valid) {
        interval = 20 * 1000;
    }

    uint32_t now = millis();
    if (!is_fetching && (force || last_fetch_time == 0 || (now - last_fetch_time >= interval))) {
        last_fetch_time = now;
        is_fetching = true;

        BaseType_t res = xTaskCreatePinnedToCore(
            fetchWeatherTask,
            "WeatherTask",
            8 * 1024,
            NULL,
            1,
            NULL,
            0
        );
        if (res != pdPASS) {
            EWC_LOG_E("Task creation failed");
            is_fetching = false;
        }
    }
}

bool ESPWeatherClient::fetchNow() {
    WeatherInfo info;
    if (executeWeatherFetch(latitude, longitude, city, info)) {
        if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            current_weather = info;
            xSemaphoreGive(weatherMutex);
            return true;
        }
    }
    return false;
}

WeatherInfo ESPWeatherClient::getWeather() {
    WeatherInfo copy;
    if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        copy = current_weather;
        xSemaphoreGive(weatherMutex);
    }
    return copy;
}
