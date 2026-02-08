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
#include <SD.h>
#include <FS.h>

#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18
#define SD_CS_PIN 5

SPIClass SD_SPI;

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

/**
 * @brief Initialize SD card
 */
void initSD() {
  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS_PIN, SD_SPI, 40000000)) {
    Serial.println("SD Card Mount Failed");
  } else {
    Serial.println("SD Card Initialized");
  }
}

/**
 * @brief Write logs to SD card
 */
void writeLog(struct tm *timeinfo, uint16_t co2, float temperature, float humidity) {
  char logFileName[24];
  strftime(logFileName, sizeof(logFileName), "/%Y%m%d.csv", timeinfo);

  File file = SD.open(logFileName, FILE_APPEND);
  if (file) {
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    file.printf("%s, %d, %.2f, %.2f\n", timeStr, co2, temperature, humidity);
    file.close();
  } else {
    Serial.print("Failed to open log file: ");
    Serial.println(logFileName);
  }
}

void setup() {
  Serial.begin(115200);

  // 1. WiFiのハードウェア初期化のみ行う (LCDのピン設定が破壊される可能性があるため最初に行う)
  NWManager::init();
  
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

  // 2. WiFi接続開始 (LCDに状況を表示する)
  // すでにハードウェア初期化は済んでいるためピン設定は破壊されないはず
  NWManager::connectWiFi();

  // Draw display
  UIManager::drawInitialLabels();
  graph.begin(&lcd, GRAPH_WIDTH, GRAPH_HIGHT, GRAPH_XMIN, GRAPH_YMIN, GRAPH_YGRID, COLOR_CO2, COLOR_TEMP, COLOR_HUMID);

  initSD();

  // Wait for time synchronization and load history
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
      loadHistoryFromSD(&timeinfo);
  } else {
      Serial.println("Failed to obtain time");
  }
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

      // Save log to SD card
      writeLog(timeinfo, co2, temperature, humidity);

      co2Sum = 0;
      tempSum = 0;
      humidSum = 0;
      numSum = 0;
    }
  }
}

/**
 * @brief Load history data from SD card (last 5 hours)
 */
void loadHistoryFromSD(struct tm *now) {
  time_t t_now = mktime(now);
  time_t t_scan = t_now - (5 * 3600); // Start from 5 hours ago
  time_t t_last_added = t_scan - 60;  // Track last added time
  
  // We need to check potentially two files: Yesterday and Today
  // Logic: Iterate from start time to now, checking files
  
  // Simple approach: Iterate through minutes? No, too slow.
  // Better: Identify relevant files and scan them.
  
  time_t t_current_day = t_scan;
  while (t_current_day <= t_now) {
    struct tm *tm_current = localtime(&t_current_day);
    char logFileName[24];
    strftime(logFileName, sizeof(logFileName), "/%Y%m%d.csv", tm_current);
    
    // Store current scan year/month/day to calculating next day later
    struct tm start_of_day = *tm_current;

    File file = SD.open(logFileName, FILE_READ);
    if (file) {
      Serial.print("Loading log: ");
      Serial.println(logFileName);
      
      while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() < 20) continue; // Skip empty or short lines
        
        // Parse line: YYYY-MM-DD HH:MM:SS, co2, temp, humid
        // Example: 2026-02-08 10:10:02, 800, 25.50, 45.20
        int year, month, day, hour, minute, second;
        int co2;
        float temp, humid;
        
        // Use sscanf to parse
        char timeStr[24];
        // We can just parse the first part as string if needed, or parse all
        // Log format: "%s, %d, %.2f, %.2f\n" where %s is "%Y-%m-%d %H:%M:%S"
        // sscanf format: "%d-%d-%d %d:%d:%d, %d, %f, %f"
        
        int items = sscanf(line.c_str(), "%d-%d-%d %d:%d:%d, %d, %f, %f", 
                           &year, &month, &day, &hour, &minute, &second, 
                           &co2, &temp, &humid);
                           
        if (items >= 9) {
          struct tm tm_line = {0};
          tm_line.tm_year = year - 1900;
          tm_line.tm_mon = month - 1;
          tm_line.tm_mday = day;
          tm_line.tm_hour = hour;
          tm_line.tm_min = minute;
          tm_line.tm_sec = second;
          tm_line.tm_isdst = -1;
          
          time_t t_line = mktime(&tm_line);
          
          if (t_line < t_scan) continue; // Too old
          if (t_line > t_now) break;     // Future (relative to now)
          
          // Fill gaps with invalid data
          // If t_line is more than 1 minute after t_last_added
          while (difftime(t_line, t_last_added) > 90) { // Tolerance > 1.5 min
             t_last_added += 60;
             if (t_last_added >= t_line) break;
             graph.add(GRAPH_INVALID_VALUE, GRAPH_INVALID_VALUE, GRAPH_INVALID_VALUE);
          }
          
          // Add valid data
          graph.add((float)co2, temp, humid);
          t_last_added = t_line;
        }
      }
      file.close();
    }
    
    // Advance to next day approx (add 20 hours to be safe to cross midnight from any start point)
    // Actually, just incrementing day is safer logic-wise outside.
    // simpler: if file was yesterday, check today.
    
    // Move to next day (00:00:00)
    start_of_day.tm_mday += 1;
    start_of_day.tm_hour = 0;
    start_of_day.tm_min = 0;
    start_of_day.tm_sec = 0;
    start_of_day.tm_isdst = -1;
    t_current_day = mktime(&start_of_day);
  }
  
  // Fill remaining gap to now
  while (difftime(t_now, t_last_added) > 90) {
     t_last_added += 60;
     graph.add(GRAPH_INVALID_VALUE, GRAPH_INVALID_VALUE, GRAPH_INVALID_VALUE);
  }
  
  // Update graph once
  graph.plot(now, 7);
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
