/*
   Waiter Caller Weather Firmware
   Validated for devices: ESP8266 ESP-01 connected to BME280 sensor
   Created at 2020-06-21 by Gustavo Rubin (gusrubin@gmail.com)
*/

#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <BME280I2C.h>
#include <Wire.h>

WiFiClient espClient;
PubSubClient client(espClient);

char sensorName[30] = "";
char ssid[30] = "";
char password[30] = "";
char mqttServer[17] = "";
int mqttPort = 0;
char mqttUser[30] = "";
char mqttPassword[30] = "";
char topicWeather[52] = "waitercaller/weather/";

// BME280I2C configuration;
// Default : forced mode, standby time = 1000 ms, Oversampling = pressure ×1,
// temperature ×1, humidity ×1, filter off

BME280I2C::Settings settings(
  BME280::OSR_X1,
  BME280::OSR_X1,
  BME280::OSR_X1,
  BME280::Mode_Forced,
  BME280::StandbyTime_1000ms,
  BME280::Filter_Off,
  BME280::SpiEnable_False,
  0x76
);

BME280I2C bme(settings);

float tem(NAN), hum(NAN), pre(NAN), alt(NAN);

BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_hPa);

const int capacity = JSON_OBJECT_SIZE(100);
StaticJsonDocument<capacity> json;

void publishData() {
  bme.read(pre, tem, hum, tempUnit, presUnit);
  alt = 44330 * (1 - pow(pre / 1013.25, 0.190294957));  
  
  json["temperature"] = roundFloat(tem);
  json["humidity"] = roundFloat(hum);
  json["pression"] = roundFloat(pre);
  json["altitude"] = roundFloat(alt);

  char jsonOutput[100];
  serializeJson(json, jsonOutput);

  client.publish(topicWeather, jsonOutput);
}

float roundFloat(float var) 
{ 
    // 37.66666 * 100 =3766.66 
    // 3766.66 + .5 =3767.16    for rounding off value 
    // then type cast to int so value is 3767 
    // then divided by 100 so the value converted into 37.67 
    float value = (int)(var * 100 + .5); 
    return (float)value / 100; 
} 

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  Serial.println("Load Config");
  if (!loadConfig()) {
    Serial.println("Failed to load config");
  } else {
    Serial.println("Config loaded");
  }

  Serial.println("Setup Network");
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  reconnect();

  Serial.println("Setup GPIOs");
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output

  while (!Serial) {} // Wait

  Wire.begin(0, 2);
  while (!bme.begin())
  {
    client.publish(topicWeather, "BME280 begin error");
    delay(1000);
  }

  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      client.publish(topicWeather, "BME280 detected");
      break;
    case BME280::ChipModel_BMP280:
      client.publish(topicWeather, "BMP280 detected");
      break;
    default:
      client.publish(topicWeather, "Unknow sensor detected" );
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  publishData();

  delay(60000);  
}
