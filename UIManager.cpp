#include <TFT_eSPI.h>
#include "UIManager.h"

extern TFT_eSPI lcd;

const char UIManager::MONTH[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void UIManager::drawInitialLabels() {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(COLOR_CO2);
    lcd.drawString("ppm", 80, 220, 2);
    lcd.setTextColor(COLOR_TEMP);
    lcd.drawCircle(190, 220, 2, COLOR_TEMP);
    lcd.drawString("C", 193, 220, 2);
    lcd.setTextColor(COLOR_HUMID);
    lcd.drawString("%", 280, 220, 2);
}

void UIManager::updateTime(struct tm *timeinfo) {
  char timeStrbuff[32];

  lcd.fillRect(0, 0, 200, 26, TFT_BLACK);  // Clear
  lcd.setTextColor(TFT_WHITE);
  sprintf(timeStrbuff, "%s %d   %d:%02d", MONTH[timeinfo->tm_mon], timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);
  lcd.setTextDatum(0);
  lcd.drawString(timeStrbuff, 2, 0, 4);
}

void UIManager::updateMeasurement(uint16_t co2, float temperature, float humidity) {
  char buf[12];

  lcd.setTextDatum(2);

  // CO2
  lcd.setTextColor(COLOR_CO2, TFT_BLACK);
  sprintf(buf, "%6d", co2);
  lcd.drawRightString(buf, 75, 215, 4);
  // Temperature
  lcd.setTextColor(COLOR_TEMP, TFT_BLACK);
  sprintf(buf, "%5.1f", temperature);
  lcd.drawRightString(buf, 185, 215, 4);
  // Humidity
  lcd.setTextColor(COLOR_HUMID, TFT_BLACK);
  sprintf(buf, "%3.0f", humidity);
  lcd.drawRightString(buf, 275, 215, 4);
}

void UIManager::showStatus(const char *str, uint16_t scd4xError) {
  char strBuf[256];

  clearStatus();

  lcd.setCursor(0, 22, 2);
  lcd.setTextColor(TFT_RED);
  lcd.print(str);

  if (scd4xError) {
    errorToString(scd4xError, strBuf, 256);
    lcd.print(strBuf);
  }
}

void UIManager::clearStatus() {
  lcd.setCursor(0, 22, 2);
  lcd.print("                                      ");
}
