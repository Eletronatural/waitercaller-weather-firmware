bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  StaticJsonDocument<512> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println(F("Failed to read file, using default configuration"));
    false;
  }  

  // Copy values from the JsonDocument to the Config
  strcpy(ssid, doc["wifi_network_name"]);
  Serial.print("Loaded Wifi Network Name: "); Serial.println(ssid);
  strcpy(password, doc["wifi_network_password"]);
  Serial.print("Loaded Wifi Network Password: "); Serial.println(password);
  strcpy(mqttServer, doc["mqtt_server_address"]);
  Serial.print("Loaded MQTT server: "); Serial.println(mqttServer);
  mqttPort = doc["mqtt_server_port"].as<int16_t>();
  Serial.print("Loaded MQTT port: "); Serial.println(mqttPort);
  strcpy(mqttUser, doc["mqtt_server_username"]);
  Serial.print("Loaded MQTT User: "); Serial.println(mqttUser);
  strcpy(mqttPassword, doc["mqtt_server_password"]);
  Serial.print("Loaded MQTT Password: "); Serial.println(mqttPassword);
  
  return true;
}
