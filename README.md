# ESP32 Health Monitor & Control System

## 🧠 Tổng Quan

Dự án này xây dựng một hệ thống giám sát sức khỏe sử dụng **ESP32** với các chức năng:
- Đo nhịp tim qua cảm biến **MAX30102**
- Đo nhiệt độ và độ ẩm bằng **AHT20**
- Giao tiếp không dây qua **WiFi** hoặc **Bluetooth**
- Gửi dữ liệu dưới dạng **JSON**
- Nhận lệnh để điều khiển **servo** hoặc bật/tắt cảm biến
- Tương thích với chế độ **multitasking (FreeRTOS)** trên ESP32

---

## 📦 Phần Cứng

| Thành phần         | Mô tả                                       |
|--------------------|----------------------------------------------|
| ESP32 Dev Board    | Vi điều khiển chính                         |
| MAX30102           | Cảm biến nhịp tim và SpO2                   |
| AHT20/AHT21        | Cảm biến nhiệt độ và độ ẩm                  |
| Servo (SG90/MG90)  | Động cơ servo điều khiển góc (0–180 độ)     |
| Dây nối, nguồn     | Kết nối và cấp nguồn cho toàn mạch          |

**Kết nối I2C:**
- SDA: GPIO 25  
- SCL: GPIO 26

**Servo:**
- Chân điều khiển servo: GPIO 21

---

## 🛠️ Cài Đặt

### 1. Yêu cầu thư viện (Arduino IDE):
Cài đặt các thư viện sau:
- `MAX30105` của SparkFun
- `heartRate.h` (đi kèm thư viện SparkFun MAX30105)
- `Adafruit AHTX0`
- `WiFi` (ESP32 mặc định)
- `WebServer` (ESP32 mặc định)
- `BluetoothSerial` (nếu dùng Bluetooth)
- `Servo`

## 🧩 Thư viện cần thiết

- `MAX30105` (SparkFun)
- `heartRate.h` (kèm theo MAX30105)
- `Adafruit AHTX0`
- `WiFi` (mặc định trên ESP32)
- `WebServer` (ESP32 core)
- `BluetoothSerial` (ESP32 core)
- `Servo`

---

## ⚙️ Cấu hình

### Serial

- **Baudrate**: `115200`

---
### Chọn chế độ giao tiếp

-Trong đầu code `main.cpp`, chọn một trong hai:
-#define ENABLE_WIFI        // Bật giao tiếp WiFi
-#define ENABLE_BLUETOOTH // Hoặc bật Bluetooth (KHÔNG dùng cả hai)

---

### Giao tiếp WiFi (nếu bật `#define ENABLE_WIFI`)

- SSID: `AP`  
- Password: `12345678`
- Trên Serial hiện địa chỉ IP
- Truy cập `http://<IP>/data` để lấy dữ liệu dạng JSON:
```json
{
  "IR": 56342,
  "BPM": 78.5,
  "Temp": 26.3,
  "Hum": 58.7
}
```
---
### Giao tiếp Bluetooth (Nếu bật #define `ENABLE_BLUETOOTH`)

-Sử dụng app Bluetooth Serial Terminal
-Kết nối với esp32
-Trên terminal sẽ hiện các thông số
-Có thể gửi 1 trong 3 lệnh
| Lệnh    | Chức năng                               |
| ------- | --------------------------------------- |
| `GYMAX` | Bật/tắt cảm biến nhịp tim MAX30102      |
| `AHT`   | Bật/tắt cảm biến nhiệt độ - độ ẩm AHT20 |
| `0-180` | Xoay servo đến góc tương ứng            |
