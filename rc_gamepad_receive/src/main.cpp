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

// 设置wifi接入信息(请根据您的WiFi信息进行修改)
// const char* ssid = "HUAWEI-LX5CET";
// const char* password = "lzy#197222";
char mqttServer[40];
char mqttPort[6] = "1883";

//是否启动调试模式
bool debug = false;
 
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

Servo direction;
int engine = D2;
int forward = D3;
//接受控制参数
uint16 control[4] = {0, 0, 0, 0};
//json解析
StaticJsonDocument<64> doc;

Ticker ticker;
int clearFlag = 2;
//flag for saving data
bool shouldSaveConfig = false;
void setup() {
  Serial.begin(9600);               // 启动串口通讯

  //从 FS json 读取数据
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
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
        DynamicJsonDocument json(1024);
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
          strcpy(mqttServer, json["mqttServer"]);
          strcpy(mqttPort, json["mqttPort"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  pinMode(LED_BUILTIN, OUTPUT);     // 设置板上LED引脚为输出模式
  digitalWrite(LED_BUILTIN, HIGH);  // 启动后关闭板上LED

  //设置电机引脚
  direction.attach(D1,500,2500);
  //舵机初始化为90度
  direction.write(90);
  pinMode(engine, OUTPUT);
  pinMode(forward, OUTPUT);


  // wifiManager.resetSettings();
  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqttServer, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqttPort, 5);

  //自动配网
  WiFiManager wifiManager;
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //添加参数
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
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

  Serial.println(mqttServer);
  Serial.println(strlen(mqttServer));

  //保存配置
  if (shouldSaveConfig) {
    Serial.println("saving config");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
  DynamicJsonDocument json(1024);
#else
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
#endif
  json["mqttServer"] = mqttServer;
  json["mqttPort"] = mqttPort;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
  serializeJson(json, Serial);
  serializeJson(json, configFile);
#else
  json.printTo(Serial);
  json.printTo(configFile);
#endif
  configFile.close();
  //end save
  }
  
  // 设置MQTT服务器和端口号
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(receiveCallback);
 
  // 连接MQTT服务器
  Serial.println("connectMQTT\n");
  connectMQTTserver();

  //看门狗，防止失控
  ticker.attach(0.2, clearData);
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
  if (control[0] == 1) {
    direction.write(control[2]);
    analogWrite(engine, control[3]);
    digitalWrite(forward, control[1]);
  } else {
    direction.write(90);
    analogWrite(engine, 0);
    digitalWrite(forward, HIGH);
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
  
  //从JSON中读取控制数据
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  control[0] = doc["A"]; // connect
  control[1] = doc["B"]; // forward
  control[2] = map(doc["C"], 0, 2000, 0, 180); // axes
  control[3] = map(doc["D"], 0, 1000, 0, 1023); // RT

  if (debug) {
    for (int i = 0; i < sizeof(control) / sizeof(control[0]); i++) {
    Serial.println(control[i]);
    }
  }

  writeData();
  //喂狗，防止清零
  clearFlag = 2;
}


// 连接MQTT服务器并订阅信息
void connectMQTTserver(){
  // 根据ESP8266的MAC地址生成客户端ID（避免与其它ESP8266的客户端ID重名）
  String clientId = "rcCar-" + WiFi.macAddress();
 
  // 连接MQTT服务器
  if (mqttClient.connect(clientId.c_str())) { 
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address:");
    Serial.println(mqttServer);
    Serial.println("ClientId: ");
    Serial.println(clientId);
    subscribeTopic(); // 订阅指定主题
  } else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    delay(5000);
  }   
}
 
// 订阅指定主题
void subscribeTopic(){
 
  // 建立订阅主题1。主题名称以Taichi-Maker-Sub为前缀，后面添加设备的MAC地址。
  // 这么做是为确保不同设备使用同一个MQTT服务器测试消息订阅时，所订阅的主题名称不同
  String topicString = "rcCar/" + WiFi.macAddress() + "/control";
  char subTopic[topicString.length() + 1];  
  strcpy(subTopic, topicString.c_str());
  
  // 通过串口监视器输出是否成功订阅主题1以及订阅的主题1名称
  if(mqttClient.subscribe(subTopic)){
    Serial.println("Subscrib Topic:");
    Serial.println(subTopic);
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
    control[0] = 0;
    writeData();
    clearFlag = 0;
  }
}

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

