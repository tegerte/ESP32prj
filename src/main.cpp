
#include <WiFiManager.h>
#include "time.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12

#define CLK_PIN 18
#define DATA_PIN 23
#define CS_PIN 5
// set to 1 if we are implementing the user interface pot
#define USE_UI_CONTROL 0

#if USE_UI_CONTROL
#define SPEED_IN A5
uint8_t frameDelay = 25; // default frame delay value
#endif                   // USE_UI_CONTROL

// Hardware SPI connection

#define SPEED_TIME 90
#define PAUSE_TIME 1000

// Turn on debug statements to the serial output
#define DEBUG 0

#if DEBUG
#define PRINT(s, x)     \
  {                     \
    Serial.print(F(s)); \
    Serial.print(x);    \
  }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif

// Hardware SPI connection
// MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary output pins

#define DHTPIN 22     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// const long gmtOffset_sec = 3600;
// const int daylightOffset_sec = 3600;
//  put function declarations here:
String displayLocalTime(void);
void setTZ(String);
void init_time(String);
void setup_wifi(void);
void print_data_1h(ArduinoJson::V730PB22::JsonObject &data_block);
void print_my_weather_data(ArduinoJson::V730PB22::JsonObject &data_block, int days_to_show);
String read_temp_humidity_new(void);
void switchAnimation(int, char const*, char const*);

const char *ntpServer = "pool.ntp.org";
String lat = "48.620253";
String lon = "8.779804";
String asl = "505";
// String URL = "https://my.meteoblue.com/packages/basic-1h_basic-day?lat=47.558&lon=7.573&apikey=DEMOKEY?";
String URL_basic = "https://my.meteoblue.com/packages/basic-1h_basic-day?";
String ApiKey = "";
int days_to_show = 3;

// Initialize DHT sensor.
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
uint8_t curText;
// Initialize the Parola library using the hardware SPI pins
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(921600);
  setup_wifi();
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  init_time("CET-1CEST,M3.5.0,M10.5.0/3"); // Europe/Berlin https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  dht.begin();

  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  P.begin();
  P.setInvert(false);
  switchAnimation(0, "Los gehts!", "Los gehts!");
}

void loop()
{
  String curr_loc_tm = displayLocalTime();
  static int animationId = 0;
  //P.print(curr_loc_tm);
  //Do_the_HTTP_stuff();
  String temp = read_temp_humidity_new();
  Serial.println(curr_loc_tm);
  const char* tm =  curr_loc_tm.c_str(); 
  const char* temp_c = temp.c_str();
  if (P.displayAnimate())
  {
    P.displayReset();
    Serial.println(curr_loc_tm+ " from animation loop");
    switchAnimation(++animationId % 2, tm, temp_c);
  };
  delay(100);
}

// Do the HTTP stuff, query weather API
void Do_the_HTTP_stuff()
{
  HTTPClient http;
  http.begin(URL_basic + "lat=" + lat + "&lon=" + lon + "&asl=" + asl + "&apikey=" + ApiKey);
  // Serial.println(URL_basic + "lat=" + lat + "&lon=" + lon + "&asl=" + asl + "&apikey=" + ApiKey);
  int httpCode = http.GET();
  Serial.println("HTTP Code: " + String(httpCode));
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    String JSON_Data = http.getString();
    // Serial.println(JSON_Data);

    JsonDocument doc;
    deserializeJson(doc, JSON_Data);
    JsonObject obj = doc.as<JsonObject>();

    // const float temp = doc["data_1h"]["temperature"][0];
    // Serial.print("Temperature: "+ String(temp) + "°C\n");
    JsonObject data_1h = obj["data_1h"].as<JsonObject>();
    // print_data_1h(data_1h);
    JsonObject data_day = obj["data_day"].as<JsonObject>();
    print_data_1h(data_day);
    print_my_weather_data(data_day, days_to_show);
  }
  else
  {
    Serial.print("Error! HTTP return code: " + httpCode);
  }
  http.end();
}

// Toggle between animations
void switchAnimation(int animationId, char const* the_tm, const char* the_temp)
{
  switch (animationId)
  {
  case 0:
    //P.displayText("NTP", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    P.displayText(the_tm, PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    break;
  case 1:
    P.displayText(the_temp, PA_CENTER, 100, 0, PA_SCROLL_RIGHT, PA_SCROLL_RIGHT);
    //P.displayText("PTN", PA_CENTER, 100, 0, PA_SCROLL_RIGHT, PA_SCROLL_RIGHT);
    break;
  }
}
// Read temperature and humidity from DHT sensor
String read_temp_humidity_new()
{
  String temp_humid_str;
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    Serial.println(F("Error reading temperature!"));
    return  String();
  }
  else
  {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    temp_humid_str= String(event.temperature) + "°C";
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println(F("Error reading humidity!"));
    return String();
  }
  else
  {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    temp_humid_str = temp_humid_str + ", "  + String(event.relative_humidity) + "%";
    return temp_humid_str;
  }
}
// extract the  interesting values from the JSON-Block
void print_my_weather_data(ArduinoJson::V730PB22::JsonObject &data_block, int days_to_show)
{
  for (JsonObject::iterator it = data_block.begin(); it != data_block.end(); ++it)
  {
    Serial.print(it->value()["time"].as<String>() + ",");
    Serial.print(it->value()["temperature_max"][0].as<String>() + ",");

    Serial.println();
  }

  // for (JsonObject::iterator it = data_block.begin(); it != data_block.end(); ++it)
  // {
  //   Serial.print(it->value()["time"].as<String>() + ",");
  //   std::vector<String> list_of_interesting_vals = { "temperature_max", "felttemperature_min", "uvindex" };
  //   for (const auto& entry : list_of_interesting_vals)
  //   {

  //     for (int i = 0; i < days_to_show; i++)
  //     {
  //       Serial.print(it->value()[entry][i].as<String>() + ",");
  //     }

  //     Serial.println();
  //   }
  // }
}

// print Weather data for a JSON-Block
void print_data_1h(ArduinoJson::V730PB22::JsonObject &data_block)
{
  for (JsonObject::iterator it = data_block.begin(); it != data_block.end(); ++it)
  {
    Serial.println("\n\n" + String(it->key().c_str()));
    if (it->value().is<JsonArray>())
    {
      for (JsonVariant v : it->value().as<JsonArray>())
      {
        Serial.print(v.as<String>() + ",");
      }
    }
    else
    {
      Serial.println(it->value().as<String>());
    }
  }
} // put function definitions here:
// print the local time nicely formatted
String displayLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return String();
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  char timeStringBuff[50]; // Buffer to hold the formatted time
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
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
