#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_AHTX0.h>
#include <Servo.h>
Servo myServo;
// ===== Chọn chế độ =====
#define ENABLE_WIFI
//#define ENABLE_BLUETOOTH
TaskHandle_t readMAXHandle = NULL;
TaskHandle_t readAHTHandle = NULL;

#if defined(ENABLE_WIFI) && defined(ENABLE_BLUETOOTH)
  #error "Only Wifi or Bluetooth"
#endif

#if defined(ENABLE_WIFI)
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer server(80);
  const char* ssid = "AP";
  const char* password = "12345678";
#endif

#if defined(ENABLE_BLUETOOTH)
  #include "BluetoothSerial.h"
  BluetoothSerial SerialBT;
#endif

// ==== GY-MAX30102 ====
MAX30105 particleSensor;
volatile long irValue = 0;
volatile float bpmValue = 0;

// ==== AHT ====
Adafruit_AHTX0 aht;
volatile float tempValue = 0;
volatile float humValue = 0;

// ==== Biến dữ liệu ====
String dataToSend = "";

// ==== /data cho web ====
#if defined(ENABLE_WIFI)
void handleData() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "application/json", dataToSend);

}
#endif

// ==== Task đọc GYMAX ====
void readMAXTask(void *pvParameters) {
  const byte RATE_SIZE = 4;
  byte rates[RATE_SIZE] = {0};
  byte rateSpot = 0;
  int64_t lastBeatUs = 0;
  float beatsPerMinute = 0;
  const int samplesPerCycle = 20;
  const int sampleIntervalMs = 10;

  for (;;) {
    bool beatDetectedInCycle = false;

    for (int i = 0; i < samplesPerCycle; i++) {
      int64_t nowUs = esp_timer_get_time(); // μs
      long ir = particleSensor.getIR();
      irValue = ir;

      if (checkForBeat(ir)) {
        beatDetectedInCycle = true;

        if (lastBeatUs > 0) {
          float delta = (nowUs - lastBeatUs) / 1000000.0;
          beatsPerMinute = 60.0 / delta;

          if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;

            int beatAvg = 0, count = 0;
            for (byte x = 0; x < RATE_SIZE; x++) {
              if (rates[x] > 0) {
                beatAvg += rates[x];
                count++;
              }
            }

            if (count > 0)
              bpmValue = beatAvg / (float)count;
          }
        }

        lastBeatUs = nowUs;
      }

      vTaskDelay(pdMS_TO_TICKS(sampleIntervalMs)); // Delay mỗi lần đọc
    }

    // Sau mỗi chu kỳ đo 20 mẫu
    int64_t now = esp_timer_get_time();

    // Nếu không phát hiện nhịp hoặc tín hiệu IR thấp thì reset BPM
    if (!beatDetectedInCycle || irValue < 10000 || (now - lastBeatUs > 3000000)) {
      bpmValue = 0;
      memset(rates, 0, RATE_SIZE);
      rateSpot = 0;
    }

    // Delay nhẹ để tiết kiệm CPU
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ==== Task đọc AHT ====
void readAHTTask(void *pvParameters) {
  sensors_event_t humidity, temp;
  for (;;) {
    aht.getEvent(&humidity, &temp);
    tempValue = temp.temperature;
    humValue = humidity.relative_humidity;
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ==== Task cập nhật dữ liệu ====
void updateDataTask(void *pvParameters) {
  for (;;) {
    String json = "{";
    json += "\"IR\":" + String(irValue);
    json += ",\"BPM\":" + String(bpmValue, 1);
    json += ",\"Temp\":" + String(tempValue, 1);
    json += ",\"Hum\":" + String(humValue, 1);
    json += "}";

    dataToSend = json;

    Serial.println(dataToSend);
    if (irValue < 50000) Serial.println("⚠️ Không phát hiện ngón tay.");

#if defined(ENABLE_BLUETOOTH)
    SerialBT.println(dataToSend);
#endif
#if defined(ENABLE_WIFI)
  server.handleClient();
#endif
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// === Xử lí lệnh ===
void handleCommand(String cmd) {
  cmd.toUpperCase();  // Chuẩn hóa chữ hoa để dễ so sánh

  if (cmd == "GYMAX") {
    // Bật/tắt task GYMAX
    if (eTaskGetState(readMAXHandle) == eSuspended) {
      vTaskResume(readMAXHandle);
      Serial.println("GYMAX sensor ENABLED");
    } else {
      vTaskSuspend(readMAXHandle);
      irValue = 0;
      bpmValue = 0;
      Serial.println("GYMAX sensor DISABLED");
    }
  } 
  else if (cmd == "AHT") {
    // Bật/tắt task AHT
    if (eTaskGetState(readAHTHandle) == eSuspended) {
      vTaskResume(readAHTHandle);
      Serial.println("AHT sensor ENABLED");
    } else {
      vTaskSuspend(readAHTHandle);
      tempValue = 0;
      humValue = 0;
      Serial.println("AHT sensor DISABLED");
    }
  } 
  else {
    // Nếu lệnh là số, điều khiển servo
    int angle = cmd.toInt();
    if (angle >= 0 && angle <= 180) {
      myServo.write(angle);
      Serial.print("Servo set to angle: ");
      Serial.println(angle);
    } else {
      Serial.println("Invalid command or angle");
    }
  }
}

// === Task điều khiển ===
void controlTask(void *pvParameters) {
  String cmd = "";
  for (;;) {
#if defined(ENABLE_BLUETOOTH)
    while (SerialBT.available()) {
      char c = SerialBT.read();
      if (c == '\n') {
        cmd.trim();
        handleCommand(cmd);
        cmd = "";
      } else {
        cmd += c;
      }
    }
#elif defined(ENABLE_WIFI)
    // Không xử lý ở đây, bỏ qua hoặc cho khác chức năng
#endif

    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') {
        cmd.trim();
        handleCommand(cmd);
        cmd = "";
      } else {
        cmd += c;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {


  myServo.attach(21);
  Serial.begin(115200);
  Wire.begin(25, 26);
  Serial.println("Initializing...");

  // Khởi động MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 not found");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
  Serial.println("MAX30102 started");

  // Khởi động AHT
  if (!aht.begin()) {
    Serial.println("Không tìm thấy cảm biến AHT");
    while (1);
  }

#if defined(ENABLE_WIFI)
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(); Serial.print("Connected! IP: "); Serial.println(WiFi.localIP());
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
#endif

#if defined(ENABLE_BLUETOOTH)
  SerialBT.begin("ESP32_BT");
  Serial.println("Bluetooth started");
#endif

  xTaskCreatePinnedToCore(readMAXTask, "ReadMAX", 4096, NULL, 1, &readMAXHandle, 1);
  xTaskCreatePinnedToCore(readAHTTask, "ReadAHT", 2048, NULL, 1, &readAHTHandle, 1);
  xTaskCreatePinnedToCore(updateDataTask, "UpdateData", 2048, NULL, 1, NULL, 1);  // thêm task updateDataTask
  xTaskCreatePinnedToCore(controlTask, "Control", 2048, NULL, 1, NULL, 1);
  vTaskSuspend(readMAXHandle);  // Mặc định tắt
  vTaskSuspend(readAHTHandle); 
}

void loop() {

}
