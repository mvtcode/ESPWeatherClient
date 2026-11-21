# ESPWeatherClient

Thư viện ESP32 / Arduino cập nhật dữ liệu thời tiết thời gian thực từ **Open-Meteo API** (nhiệt độ, độ ẩm, cảm giác thực tế, tốc độ gió, chỉ số UV và mô tả thời tiết tiếng Việt).

---

## ✨ Tính năng nổi bật

- 🌍 **Không cần API Key**: Dữ liệu lấy trực tiếp từ Open-Meteo hoàn toàn miễn phí và chính xác.
- 📍 **Linh hoạt toạ độ**: Dễ dàng đổi địa điểm theo Toạ độ (Vĩ độ `lat`, Kinh độ `lon`) và tên thành phố.
- 🇻🇳 **Mô tả Tiếng Việt**: Tự động ánh xạ mã thời tiết WMO (`weather_code`) sang tiếng Việt chuẩn (*Trời nắng, Ít mây, Sương mù, Mưa phùn, Mưa rào, Giông bão...*).
- ⚡ **Non-blocking FreeRTOS Task**: Chạy ngầm trong FreeRTOS task riêng biệt, không block luồng UI/LVGL.
- 🛡️ **Zero Dynamic Heap Copy**: Dữ liệu chuỗi được format trong mảng `char[]`, hạn chế tối đa phân mảnh bộ nhớ (Heap Fragmentation).

---

## 📦 Cài đặt

Thêm vào `platformio.ini`:
```ini
lib_deps =
    https://github.com/mvtcode/ESPWeatherClient.git
```

Hoặc gắn tag phiên bản:
```ini
lib_deps =
    https://github.com/mvtcode/ESPWeatherClient.git#v1.0.0
```

---

## 🚀 Hướng dẫn sử dụng nhanh

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <ESPWeatherClient.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    // Khoi tao mac dinh (Ha Noi: 21.0285, 105.8542)
    ESPWeatherClient::init("Hà Nội", 21.0285f, 105.8542f);
    ESPWeatherClient::setUpdateInterval(30); // 30 phut
}

void loop() {
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    ESPWeatherClient::update(isConnected);

    WeatherInfo w = ESPWeatherClient::getWeather();
    if (w.is_valid) {
        Serial.printf("[%s] Nhiet do: %s°C (Cam giac: %s°C)\n", w.city_name, w.temp_str, w.feels_like_str);
        Serial.printf("Thoi tiet: %s | Do am: %s%% | Gio: %s km/h | UV: %s\n",
                      w.condition_text, w.humidity_str, w.wind_speed_str, w.uv_index_str);
    }

    delay(5000);
}
```

---

## 📄 License
MIT License - Developed by [mvtcode](https://github.com/mvtcode).
