#include <Arduino.h>
#include <ArduinoJson.h>
void writeData();
void connectMQTTserver();
void receiveCallback(char*, byte*, unsigned int);
void subscribeTopic();
// void connectWifi();
void clearData();
void saveConfigCallback();
void blinkLED();
bool saveConfig(DynamicJsonDocument json);
void readConfig();