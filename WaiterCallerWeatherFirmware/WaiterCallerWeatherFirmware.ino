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

char ssid[30] = "";
char password[30] = "";
char mqttServer[17] = "";
int mqttPort = 0;
char mqttUser[30] = "";
char mqttPassword[30] = "";

char strBuffer[128];

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

// BME280I2C bme280; // Default : forced mode, standby time = 1000 ms
// Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

BME280I2C bme(settings);

struct BME280Data {
  // Temperature degC
  float t;
  // Pressure hPa
  float p;
  // Humidity %
  float h;
};
BME280Data bme280Data;

float tem(NAN), hum(NAN), pre(NAN);

BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_Pa);

void readBME280Data() {
  bme.read(
    bme280Data.t,
    bme280Data.h,
    bme280Data.p,
    tempUnit,
    presUnit);
}

char temp[8] = "";
char humi[8] = "";
char pres[8] = "";

// Reverses a string 'str' of length 'len'
void reverse(char* str, int len)
{
  int i = 0, j = len - 1, temp;
  while (i < j) {
    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
    i++;
    j--;
  }
}

// Converts a given integer x to string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
  int i = 0;
  while (x) {
    str[i++] = (x % 10) + '0';
    x = x / 10;
  }

  // If number of digits required is more, then
  // add 0s at the beginning
  while (i < d)
    str[i++] = '0';

  reverse(str, i);
  str[i] = '\0';
  return i;
}

// Converts a floating-point/double number to a string.
void ftoa(float n, char* res, int afterpoint)
{
  // Extract integer part
  int ipart = (int)n;

  // Extract floating part
  float fpart = n - (float)ipart;

  // convert integer part to string
  int i = intToStr(ipart, res, 0);

  // check for display option after point
  if (afterpoint != 0) {
    res[i] = '.'; // add dot

    // Get the value of fraction part upto given no.
    // of points after dot. The third parameter
    // is needed to handle cases like 233.007
    fpart = fpart * pow(10, afterpoint);

    intToStr((int)fpart, res + i + 1, afterpoint);
  }
}

// Provide sufficiently large buffer
void floatToStr(float value, char* buffer) {
  sprintf(buffer, "%d.%1d", (uint32_t)value, (uint32_t)(value * 10) % 10);
}

void publishBME280Data() {
  //doubleToStr(bme280Data.t, strBuffer);
  memset(temp, 0, sizeof temp);
  //strBuffer[] = 0;
  strcpy(temp, "Temperadure: ");
  ftoa(bme280Data.t, strBuffer, 0);
  strcat(temp, String(bme280Data.t).c_str());
  client.publish("waitercaller/weather-notify", temp);

  memset(humi, 0, sizeof humi);
  strcpy(humi, "Humidity: ");
  ftoa(bme280Data.h, strBuffer, 0);
  strcat(humi, String(strBuffer).c_str());
  client.publish("waitercaller/weather-notify", humi);

  memset(pres, 0, sizeof pres);
  strcpy(pres, "Pression: ");
  ftoa(bme280Data.p, strBuffer, 0);
  strcat(pres, String(strBuffer).c_str());
  client.publish("waitercaller/weather-notify", pres);
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
  //client.setCallback(callback);
  reconnect();

  Serial.println("Setup GPIOs");
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output

  readBME280Data();

  client.publish("waitercaller/weather-notify", "starting");

  while (!Serial) {} // Wait

  client.publish("waitercaller/weather-notify", "serial ok");

  Wire.begin(0, 2);
  while (!bme.begin())
  {
    client.publish("waitercaller/weather-notify", "begin error");
    Serial.println("Could not find BME280 sensor!");
    client.publish("waitercaller/weather-notify", "No BME280");
    delay(1000);
  }

  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      client.publish("waitercaller/weather-notify", "BME280 Ok");
      break;
    case BME280::ChipModel_BMP280:
      client.publish("waitercaller/weather-notify", "No Humidity");
      break;
    default:
      client.publish("waitercaller/weather-notify", "Unknow Sensor");
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(1000);
  // client.publish("waitercaller/weather-notify", String(bmeconnected).c_str());

  readBME280Data();
  publishBME280Data();
}
