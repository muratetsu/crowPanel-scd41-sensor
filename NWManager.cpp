#include "NWManager.h"
#include <TFT_eSPI.h>

extern TFT_eSPI lcd;

time_t NWManager::lastNtpSyncTime = 0;

// ...
void NWManager::init() {
    WiFi.mode(WIFI_STA);
    // connect=falseを指定してハードウェア初期化のみ行う（LCD初期化前のピン破壊をここで済ませる）
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 0, NULL, false);
}

void NWManager::connectWiFi() {
    Serial.println("Connecting to WiFi");
    
    // 接続プロセスを実際に開始する
    // すでにinit()で情報は渡っているため、再接続または接続開始をトリガー
    WiFi.reconnect(); 

    lcd.println("Connecting WiFi...");
    
    WiFi.setAutoReconnect(true);

    // Attempt to connect
    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            break;
        } else {
            delay(500);
            Serial.print('.');
            lcd.print(".");
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\r\nWiFi connected");
        lcd.println(" Connected!");
        
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        // Wait a bit to show the IP address
        delay(2000);
        
        lcd.println("IP: " + WiFi.localIP().toString());

        // setup time
        Serial.println("Setup time");
        lcd.println("Syncing Time...");
        if (setupTime()) {
            lastNtpSyncTime = time(NULL);
        }

        // WiFiをONにしたままでも消費電力はほとんど変わらなさそう
        // WiFi.disconnect(true);
        // WiFi.mode(WIFI_OFF);
    } else {
        Serial.println("\nWiFi connection failed.");
        lcd.println(" Failed!");
        delay(2000);
    }
}

bool NWManager::setupTime() {
    Serial.println("Setting up time...");
    // Configure time with JST (UTC+9)
    configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com");

    struct tm timeinfo;
    // getLocalTime waits for the time to be synchronized (up to 5s by default)
    if (getLocalTime(&timeinfo)) {
        Serial.println("Time synchronized");
        Serial.printf("Now: %04d/%02d/%02d %02d:%02d:%02d\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        
        return true;
    } else {
        Serial.println("Failed to obtain time");
        lcd.println("Time Sync Failed");
        
        // Fallback: Set time to 1970/1/1 manually
        Serial.println("Setting default time (1970/01/01)");
        struct timeval tv;
        tv.tv_sec = 0; // 1970-01-01 00:00:00 UTC
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
        
        return true; // Return true as time is now valid (though incorrect)
    }
}
