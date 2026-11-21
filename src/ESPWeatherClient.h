#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

struct WeatherInfo {
    float temperature = 0.0f;
    int humidity = 0;
    int weather_code = 0;
    char condition_text[32] = "N/A";
    float feels_like = 0.0f;
    float wind_speed = 0.0f;
    float uv_index = 0.0f;

    char temp_str[16] = "--";
    char feels_like_str[16] = "--";
    char humidity_str[16] = "--";
    char wind_speed_str[16] = "--";
    char uv_index_str[16] = "--";

    char city_name[48] = "Hà Nội";
    bool is_valid = false;
    uint32_t last_update_time = 0;
};

class ESPWeatherClient {
private:
    static WeatherInfo current_weather;
    static SemaphoreHandle_t weatherMutex;
    static float latitude;
    static float longitude;
    static char city[48];
    static uint32_t last_fetch_time;
    static bool is_fetching;
    static uint32_t update_interval_ms;

    static void fetchWeatherTask(void *param);

public:
    static void init(const char *cityName = "Hà Nội", float lat = 21.0285f, float lon = 105.8542f);
    static void setLocation(const char *cityName, float lat, float lon);
    static void setUpdateInterval(uint32_t intervalMinutes);
    static void update(bool wifiConnected, bool force = false, uint32_t intervalMinutes = 0);
    static bool fetchNow();
    static WeatherInfo getWeather();
    static bool isFetching() { return is_fetching; }
    static const char *getWeatherDescription(int code);
};

// Alias tuong thich nguoc
using WeatherService = ESPWeatherClient;
