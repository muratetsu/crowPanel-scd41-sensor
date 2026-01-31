#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <SensirionI2cScd4x.h> 

// Color defined in RGB565 format
#define COLOR_CO2   0x9FF3 // Light Green (approx 0x99ff99)
#define COLOR_TEMP  0xFCB3 // Light Red   (approx 0xff9999)
#define COLOR_HUMID 0x9CDF // Light Blue  (approx 0x9999ff)

class UIManager {
public:
    static void drawInitialLabels();
    static void updateTime(struct tm *timeinfo);
    static void updateMeasurement(uint16_t co2, float temperature, float humidity);
    static void showStatus(const char *str, uint16_t scd4xError);
    static void clearStatus();
    
private:
    static const char MONTH[12][4];
};

#endif
