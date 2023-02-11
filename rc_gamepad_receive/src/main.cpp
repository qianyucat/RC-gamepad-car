#include <Arduino.h>
#include <FS.h>
#include <main.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

//设置项
char mqttServer[40] = "192.168.31.12";
char mqttPort[6] = "1883";
char mqttUser[20] = "rcCar";                //mqtt用户名
char mqttPassword[20] = "rcCarPassword";    //mqtt密码
char carName[16] = "rc-gamepad-car";       //设置不同的订阅主题，防止遥控串线

int directionMax = 2000;
int engineMax = 2000;
int directionTrim = 0;
int engineTrim = 0;

//是否启动调试模式
bool debug = false;
 
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

//舵机、电机控制
Servo direction;
Servo engine;

//设置电机引脚
const uint8_t ch3 = D3;
const uint8_t ch4 = D4;
//接受控制参数
uint16 ch[4] = {0, 0, 0, 0};
//json解析


Ticker ticker;
int clearFlag = 2;
Ticker ledTicker;
bool LEDon = 0;
//flag for saving data
bool shouldSaveConfig = false;

void setup() {
  Serial.begin(9600);               // 启动串口通讯

  //从 FS 读取json数据
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    readConfig();
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  pinMode(LED_BUILTIN, OUTPUT);     // 设置板上LED引脚为输出模式
  ledTicker.attach(0.5, blinkLED);     //服务器未连接时，LED闪烁

  //设置电机引脚
  direction.attach(D1);
  engine.attach(D2);
  //设置为中位点
  direction.writeMicroseconds(1500 + directionTrim);
  engine.writeMicroseconds(1500 + engineTrim);
  // direction.write(90);
  pinMode(ch3, OUTPUT);
  pinMode(ch4, OUTPUT);


  // wifiManager.resetSettings();
  // id/name, placeholder/prompt, default, length
  // 添加配置项到ap配网页面
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server (192.168.1.10)", mqttServer, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port (1883)", mqttPort, 5);
  WiFiManagerParameter custom_mqtt_user("mqttUser", "mqttUser", mqttUser, 20);
  WiFiManagerParameter custom_mqtt_password("mqttPassword", "mqttPassword", mqttPassword, 20);
  WiFiManagerParameter custom_car_name("carName", "car name", carName, 16);

  //自动配网
  WiFiManager wifiManager;
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //添加参数
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_car_name);
  //连接WiFi
  if (!wifiManager.autoConnect("RC-Car-AP")) {
    Serial.println("connect timeout");
    delay(5000);
    ESP.reset();
    delay(5000);
  }
  //连接成功
  Serial.println("WiFi Connected!"); 

  //读取参数
  strcpy(mqttServer, custom_mqtt_server.getValue());
  strcpy(mqttPort, custom_mqtt_port.getValue());
  strcpy(carName, custom_car_name.getValue());
  Serial.println(mqttServer);
  Serial.println(strlen(mqttServer));

  //保存配置-------------------------------------------------
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
  DynamicJsonDocument doc(256);
#else
  DynamicJsonBuffer jsonBuffer;
  JsonObject& doc = jsonBuffer.createObject();
#endif
  //MQTT服务器数据
  doc["server"] = mqttServer;
  doc["port"] = mqttPort;
  doc["user"] = mqttUser;
  doc["password"] = mqttPassword;
  doc["carName"] = carName;
  //遥控车矫正数据
  doc["dMax"] = directionMax;
  doc["eMax"] = engineMax;
  doc["dTrim"] = directionTrim;
  doc["eTrim"] = engineTrim;

  if (shouldSaveConfig) {
    saveConfig(doc);
  }
  //结束保存-------------------------------------------------------------

  // 设置MQTT服务器和端口号
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(receiveCallback);
 
  // 连接MQTT服务器
  Serial.println("connectMQTT\n");
  connectMQTTserver();

  //看门狗，防止失控
  ticker.attach(0.3, clearData);
}
 
void loop() {
  if (mqttClient.connected()) { // 如果开发板成功连接服务器
    mqttClient.loop();          // 处理信息以及心跳
  } else {                      // 如果开发板未能成功连接服务器
    connectMQTTserver();        // 则尝试连接服务器
  }
}

//写入控制数据到电机
void writeData() {
  if (ch[0] == 1) {
    direction.writeMicroseconds(ch[1] + directionTrim);
    engine.writeMicroseconds(ch[2] + engineTrim);
    // analogWrite(engine, ch[2]);
    // digitalWrite(engine, HIGH);
  } else {
    direction.writeMicroseconds(1500 + directionTrim);
    engine.writeMicroseconds(1500 + engineTrim);
  }
}

// 收到信息后的回调函数
void receiveCallback(char* topic, byte* payload, unsigned int length) {
  if (debug) {
    Serial.print("Message Received [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println("");
    Serial.print("Message Length(Bytes) ");
    Serial.println(length);
  }
  //收到motor主题时
  if (strcmp(topic, String("rcCar/" + String(carName) + "/control/motor").c_str()) == 0) {
    //从JSON中读取控制数据
    StaticJsonDocument<56> controlDoc;
    DeserializationError error = deserializeJson(controlDoc, payload, length);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    ch[0] = (int)controlDoc["0"]; // connect
    ch[1] = map((int)controlDoc["1"], 0, 1000, 3000 - directionMax, directionMax); // axes
    ch[2] = map((int)controlDoc["2"], 0, 1000, 3000 - engineMax, engineMax); // throttle 0~1000

    if (debug) {
      for (int i = 0; i < 3; i++) {
      Serial.println(ch[i]);
      }
    }
    writeData();
    //喂狗，防止清零
    clearFlag = 2;
  }
  //收到ch3主题时
  if (strcmp(topic, String("rcCar/" + String(carName) + "/control/ch3").c_str()) == 0) {
    if (length > 0) {
      ch[3] = (int)payload[0] - (int)'0';
      Serial.println(ch[3]);
      digitalWrite(ch3, ch[3]);
    }
  }
  //收到ch4主题时
  if (strcmp(topic, String("rcCar/" + String(carName) + "/control/ch4").c_str()) == 0) {
    if (length > 0) {
      ch[4] = (int)payload[0] - (int)'0';
      Serial.println(ch[4]);
      digitalWrite(ch4, ch[4]);
    }
  }
  //收到newConfig主题时
  if (strcmp(topic, String("rcCar/" + String(carName) + "/config/newConfig").c_str()) == 0) {
    //从JSON中读取设置数据
    DynamicJsonDocument json(256);
    DeserializationError error = deserializeJson(json, payload, length);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    if (json.containsKey("server")) strcpy(mqttServer, json["server"]);
    if (json.containsKey("port")) strcpy(mqttPort, json["port"]);
    if (json.containsKey("user")) strcpy(mqttUser, json["user"]);
    if (json.containsKey("password")) strcpy(mqttPassword, json["password"]);
    if (json.containsKey("carName")) strcpy(carName, json["carName"]);
    if (json.containsKey("dMax")) directionMax = json["dMax"];
    if (json.containsKey("eMax")) engineMax = json["eMax"];
    if (json.containsKey("dTrim")) directionTrim = json["dTrim"];
    if (json.containsKey("eTrim")) engineTrim = json["eTrim"];
    if(saveConfig(json)) mqttClient.publish(String("rcCar/" + String(carName) + "/config/saved").c_str(), "1");
    else mqttClient.publish(String("rcCar/" + String(carName) + "/config/saved").c_str(), "0");
  }
  //收到getConfig主题时
  if (strcmp(topic, String("rcCar/" + String(carName) + "/config/getConfig").c_str()) == 0) {
    StaticJsonDocument<256> doc;

    doc["server"] = mqttServer;
    doc["port"] = mqttPort;
    doc["user"] = mqttUser;
    doc["password"] = mqttPassword;
    doc["carName"] = carName;
    doc["dMax"] = directionMax;
    doc["eMax"] = engineMax;
    doc["dTrim"] = directionTrim;
    doc["eTrim"] = engineTrim;

    String output = String("");
    serializeJson(doc, output);

    //发送carConfig
    if(mqttClient.publish(String("rcCar/" + String(carName) + "/config/carConfig").c_str(), output.c_str())){
      Serial.println("Publish Topic:/config/carConfig"); 
    } else {
      Serial.println("Message Publish Failed."); 
    }
  }
}


// 连接MQTT服务器并订阅信息
void connectMQTTserver(){
  // 根据ESP8266的MAC地址生成客户端ID（避免与其它ESP8266的客户端ID重名）
  String clientId = "rcCar-" + WiFi.macAddress();
 
  // 连接MQTT服务器
  if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword)) { 
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address:");
    Serial.println(mqttServer);
    Serial.println("ClientId: ");
    Serial.println(clientId);
    subscribeTopic(); // 订阅指定主题
    ledTicker.detach();
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    ledTicker.attach(0.5, blinkLED);     //服务器未连接时，LED闪烁
    delay(5000);
  }   
}
 
// 订阅指定主题
void subscribeTopic(){
 
  // 建立订阅主题1
  // 这么做是为确保不同设备使用同一个MQTT服务器测试消息订阅时，所订阅的主题名称不同
  String topicString = "rcCar/" + String(carName) + "/control/+";
  char subTopic[topicString.length() + 1];  
  strcpy(subTopic, topicString.c_str());
  
  // 通过串口监视器输出是否成功订阅主题1以及订阅的主题1名称
  if(mqttClient.subscribe(subTopic)){
    Serial.println("Subscrib Topic:");
    Serial.println(subTopic);
  } else {
    Serial.print("Subscribe Fail...");
  }

  String topicString2 = "rcCar/" + String(carName) + "/config/+";
  char subTopic2[topicString2.length() + 1];  
  strcpy(subTopic2, topicString2.c_str());
  
  // 通过串口监视器输出是否成功订阅主题1以及订阅的主题1名称
  if(mqttClient.subscribe(subTopic2)){
    Serial.println("Subscrib Topic:");
    Serial.println(subTopic2);
  } else {
    Serial.print("Subscribe Fail...");
  }
}
 
// ESP8266连接wifi
// void connectWifi(){
 
//   WiFi.begin(ssid, password);
 
//   //等待WiFi连接,成功连接后输出成功信息
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     Serial.print(".");
//   }
//   Serial.println("");
//   Serial.println("WiFi Connected!");  
//   Serial.println(""); 
// }
void clearData() {
  clearFlag--;
  if (clearFlag <= 0) {
    ch[0] = 0;
    writeData();
    clearFlag = 0;
  }
}

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
void blinkLED() {
  LEDon = !LEDon;
  digitalWrite(LED_BUILTIN, LEDon);
}

bool saveConfig(DynamicJsonDocument json) {
  Serial.println("saving config");
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
    return false;
  }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
  // serializeJson(json, Serial);
  serializeJson(json, configFile);
#else
  json.printTo(Serial);
  json.printTo(configFile);
#endif
  configFile.close();
  return true;
}

void readConfig() {
  Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(256);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");
          if (json.containsKey("server")) strcpy(mqttServer, json["server"]);
          if (json.containsKey("port")) strcpy(mqttPort, json["port"]);
          if (json.containsKey("user")) strcpy(mqttUser, json["user"]);
          if (json.containsKey("password")) strcpy(mqttPassword, json["password"]);
          if (json.containsKey("carName")) strcpy(carName, json["carName"]);
          if (json.containsKey("dMax")) directionMax = json["dMax"];
          if (json.containsKey("eMax")) engineMax = json["eMax"];
          if (json.containsKey("dTrim")) directionTrim = json["dTrim"];
          if (json.containsKey("eTrim")) engineTrim = json["eTrim"];
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
}

