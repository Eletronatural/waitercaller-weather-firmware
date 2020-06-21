/*
   Waiter Caller Weather Firmware
   Validated for devices: ESP8266 ESP-01 connected to BME280 sensor
   Created at 2020-06-21 by Gustavo Rubin (gusrubin@gmail.com)
*/

#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BME280I2C.h>

WiFiClient espClient;
PubSubClient client(espClient);
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

char ssid[30] = "";
char password[30] = "";
char mqttServer[17] = "";
int mqttPort = 0;
char mqttUser[30] = "";
char mqttPassword[30] = "";

void printBME280Data (
   Stream* client
)
{
   float temp(NAN), hum(NAN), pres(NAN);

   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);

   bme.read(pres, temp, hum, tempUnit, presUnit);

   client->print("Temp: ");
   client->print(temp);
   client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
   client->print("\t\tHumidity: ");
   client->print(hum);
   client->print("% RH");
   client->print("\t\tPressure: ");
   client->print(pres);
   client->println("Pa");

   delay(1000);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  while(!Serial) {} // Wait

  Wire.begin();

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }

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
}

void loop() {  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  printBME280Data(&Serial);
   delay(500);

  // client.publish(topicDesk, "0");
}
