// CO2 Sensor with Sensirion SCD41
// Crow Panel 2.4対応版
//
// Board:
//   CrowPanel ESP32 HMI 2.4-inch Display
// Library:
//   TFT_eSPI 2.5.43
//   Sensirion I2C SCD4x 1.1.0
//
// January 10, 2026
// Tetsu Nishimura

#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <SensirionI2cScd4x.h>
#include "Graph.h"
#include "NWManager.h"
#include "UIManager.h"

// Graph size
#define GRAPH_WIDTH 268
#define GRAPH_HIGHT 160
#define GRAPH_XMIN  26
#define GRAPH_YMIN  38
#define GRAPH_YGRID 20

// Backlight definitions
#define BACKLIGHT_PIN 27
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

TFT_eSPI lcd = TFT_eSPI();
SensirionI2cScd4x scd4x;
Graph graph;

// Variables for accumulation and state tracking
int co2Sum = 0;
float tempSum = 0;
float humidSum = 0;
int numSum = 0;

/**
 * @brief Print SCD4x serial number
 */
void printSerialNumber(uint64_t serialNumber) {
  lcd.print("Serial: 0x");
  lcd.print((uint32_t)(serialNumber >> 32), HEX);
  lcd.print((uint32_t)(serialNumber & 0xFFFFFFFF), HEX);
  lcd.println();
}

/**
 * @brief Initialize and start SCD4x measurement
 */
 void scd4xInit(void) {
   int16_t error;
  char errorMessage[256];
  uint64_t serialNumber;

  Wire.begin();
  scd4x.begin(Wire, SCD41_I2C_ADDR_62);

  // Ensure sensor is in clean state
  error = scd4x.wakeUp();
  if (error) {
    lcd.println("Error trying to execute wakeUp(): ");
    errorToString(error, errorMessage, 256);
    lcd.println(errorMessage);
  }

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    lcd.println("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    lcd.println(errorMessage);
  }

  error = scd4x.reinit();
  if (error) {
    lcd.println("Error trying to execute reinit(): ");
    errorToString(error, errorMessage, 256);
    lcd.println(errorMessage);
  }

  error = scd4x.getSerialNumber(serialNumber);
  if (error) {
    lcd.println("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    lcd.println(errorMessage);
  } else {
    printSerialNumber(serialNumber);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    lcd.println("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    lcd.println(errorMessage);
  }
}

/**
 * @brief Set backlight brightness
 * @param brightness 0-255
 */
void setBacklightBrightness(uint8_t brightness) {
  ledcWrite(BACKLIGHT_PIN, brightness);
}

void setup() {
  Serial.begin(115200);

  // WiFi.begin()を実行するとなぜかLCDが正常に動作しなくなるので先にWiFiを接続する
  NWManager::connectWiFi();

  //Display Prepare
  lcd.begin();
  lcd.setRotation(3); 
  lcd.fillScreen(TFT_BLACK);

  // Configure PWM for backlight (Must be done AFTER lcd.begin() because TFT_eSPI resets the pin)
  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(BACKLIGHT_PIN, 96); // Initial brightness (0-255)
  
  lcd.setTextSize(1);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.println("Initialize SCD41");

  scd4xInit();

  // Draw display
  UIManager::drawInitialLabels();
  graph.begin(&lcd, GRAPH_WIDTH, GRAPH_HIGHT, GRAPH_XMIN, GRAPH_YMIN, GRAPH_YGRID, COLOR_CO2, COLOR_TEMP, COLOR_HUMID);
}

/**
 * @brief センサーデータの取得と蓄積 (5秒毎)
 */
void processSensorData(struct tm *timeinfo) {
  static int prevSecond = 0;

  if ((timeinfo->tm_sec - prevSecond + 60) % 60 >= 5) {
    prevSecond = timeinfo->tm_sec;
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    uint16_t error;

    UIManager::clearStatus();

    // Read Measurement
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
      UIManager::showStatus("Error: ", error);
    } else if (co2 == 0) {
      UIManager::showStatus("Invalid sample detected", 0);
    } else {
      UIManager::updateMeasurement(co2, temperature, humidity);

      co2Sum += co2;
      tempSum += temperature;
      humidSum += humidity;
      numSum++;
    }
  }
}

/**
 * @brief 定期的な更新処理 (1分毎)
 * グラフ更新、ログ保存、平均値計算
 */
void processMinuteUpdate(struct tm *timeinfo) {
  static int prevMinute = 0;

  if (prevMinute != timeinfo->tm_min) {
    prevMinute = timeinfo->tm_min;
    UIManager::updateTime(timeinfo);

    if (numSum != 0) {
      uint16_t co2 = co2Sum / numSum;
      float temperature = tempSum / numSum;
      float humidity = humidSum / numSum;
      char strBuf[256];

      graph.add(co2, temperature, humidity);
      graph.plot(timeinfo, 7);

      co2Sum = 0;
      tempSum = 0;
      humidSum = 0;
      numSum = 0;
    }
  }
}

void loop() {
  struct tm timeinfo;

  getLocalTime(&timeinfo);

  // センサーデータ処理
  processSensorData(&timeinfo);

  // 1分毎の更新処理
  processMinuteUpdate(&timeinfo);

  delay(1000);
}
