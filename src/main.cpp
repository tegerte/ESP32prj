
#include <WiFiManager.h>
#include "time.h"

const char *ntpServer = "pool.ntp.org";
// const long gmtOffset_sec = 3600;
// const int daylightOffset_sec = 3600;
//  put function declarations here:
void printLocalTime(void);
void setTZ(String);
void init_time(String);
void setup_wifi(void);

void setup()
{
  Serial.begin(921600);
  setup_wifi();
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  init_time("CET-1CEST,M3.5.0,M10.5.0/3"); // Europe/Berlin https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
}

void loop()
{
  
  delay(1000);
  printLocalTime();
}

// put function definitions here:
void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Connect to NTP and then set timezone
void init_time(String timezone)
{
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org"); // First connect to NTP server, with 0 TZ offset
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTZ(timezone);
}

// Set timezone when connected to NTP-Service
void setTZ(String timezone)
{
  Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
  setenv("TZ", timezone.c_str(), 1); //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

// init WiFi
void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  // wm.resetSettings();
  bool res;
  res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  if (!res)
  {
    Serial.println("Failed to connect");
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}
