#ifndef NW_MANAGER_H
#define NW_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "Secrets.h"

class NWManager {
public:
    static void connectWiFi();
    static bool setupTime();

private:
    static time_t lastNtpSyncTime;
};

#endif
