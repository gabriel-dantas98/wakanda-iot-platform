// ########## BIBLIOTECAS ########## //

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>

#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0]))) //WHAT https://forum.arduino.cc/index.php?topic=527619.0

const char fingerprintAWS[] PROGMEM = "1E AB 2B 7B 17 4B 7D B1 6C A4 F8 69 30 E4 71 B6 99 1F C3 0C";
const char fingerprintWAKANDA[] PROGMEM = "5C BF B8 76 32 AB C1 D9 79 63 6D 30 F1 71 F0 5D 92 AF 02 A7";

const char* ssid = "#################";
const char* password = "#####################";
const char* WAKANDA_API = "#####################";
const char* BROKER_ADDRESS = "#####################";
const char* BROKER_USER = "#####################";
const char* BROKER_PASSWORD = "#####################";
const int BROKER_PORT = 12671;
const char* DISPLAY_DEVICE_NAME = "NodemcuQuarto01";

struct NetworkConfig {
  int    port;
  String localIP;
  String publicIP;
  String macAddres;
  String gateway;
  String mask;
  String hostname;
  String dns;
  String ssid;
};

struct Actuator {
  const char* name;
  const char* displayName;
  const char* type;
  int pin;
  const char* topic;
};

struct Sensor {
  const char* name;
  const char* displayName;
  const char* type;
  int pin;
  const char* topic;
};

struct Device {
  String name;
  const char* displayName;
  NetworkConfig network;
  Actuator* actuators;
  Sensor* sensors;
};

WiFiClient espClient;
ESP8266WiFiMulti WiFiMulti;
PubSubClient MQTT(espClient);

Device device;

//Prototypes
Device getDeviceConfig(String name, const char* deviceName, int port, IPAddress localIp, String publicIp, String macAddres, IPAddress gateway, String mask, String hostname, IPAddress dns, String ssid);
String getPublicIp();
String request(String method, String uri, String payload = "", const char* fingerprint = "");
void WiFiConnect();
PubSubClient MQTTConnect();
String byteMessageToString(byte* byteMessage, unsigned int length);

void setup() {

  Serial.begin(9600);
  //WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  WiFiConnect();
  
  MQTT.setServer(BROKER_ADDRESS, BROKER_PORT);
  MQTT.setCallback(MQTTCallback);

  PubSubClient mqttConnection = MQTTConnect();
  
  Actuator actuators[] = {
    {"lampada_quarto", "Lampada quarto", "relay", 15, ""}
  };
  
  Serial.print("Number of actuators: ");
  Serial.println(ARRAY_SIZE(actuators));
  
  Sensor sensors[] = {
    {"presenca_quarto", "Presenca Quarto", "proximity", 9, ""}
  };
  
  Serial.print("Number of sensors: ");
  Serial.println(ARRAY_SIZE(sensors));
  
  //Verify and registry device in Wakanda Orchestractor
  device = getDeviceConfig(WiFi.hostname(), DISPLAY_DEVICE_NAME, 8080, WiFi.localIP(), 
                           getPublicIp(), WiFi.macAddress(), WiFi.gatewayIP(), WiFi.subnetMask(), 
                           WiFi.hostname(), WiFi.dnsIP(), WiFi.SSID(), actuators, sensors);

  Serial.println("RUNNING SETUP....");
  String response = registryWakandaDevice(device);
  Serial.println("Device registred!");
  
  Device wakandaDevice = getWakandaDevice(response);
  Serial.println(wakandaDevice.displayName);
  Serial.println(wakandaDevice.network.localIP);
  Serial.println(wakandaDevice.sensors[0].topic);
  Serial.println(wakandaDevice.actuators[0].topic);
  
  Serial.println("Activating device actuator");
  pinMode(wakandaDevice.actuators[0].pin, OUTPUT);

  Serial.print("Subcribing in topic: ");
  Serial.println(wakandaDevice.actuators[0].topic);
  mqttConnection.subscribe(wakandaDevice.actuators[0].topic);
  Serial.print("MQTT topic activated!");
}

void loop() {
  //Get device information and subscribe in your mqtt topic
  Serial.println("RUNNING LOOP....");
  MQTT.loop();
  delay(2000);
}

void WiFiConnect() {

  while ((WiFiMulti.run() != WL_CONNECTED)) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
}

PubSubClient MQTTConnect() {
  while (!MQTT.connected())
  {
    Serial.println("Connecting MQTT Broker...");

    if (MQTT.connect(DISPLAY_DEVICE_NAME, BROKER_USER, BROKER_PASSWORD))
    {
      Serial.println("Connected MQTT Broker!");
      return MQTT;
    }
    else
    {
      Serial.println("Failed connect MQTT Broker!");
    }
  }
}

void MQTTCallback(char* topic, byte* bytePayload, unsigned int lengthPayload) {
    Serial.println("Message MQTT received!");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    String payload = byteMessageToString(bytePayload, lengthPayload);
    Serial.println(payload);

    Serial.print("Size topic: ");
    Serial.println(sizeof(topic));
    Serial.print("Custom size topic: ");
    Serial.println(ARRAY_SIZE(topic));  
  
    if (payload.equals("ON") || payload.equals("TurnON")) {
      Serial.println("Turning ON");
      digitalWrite(LED_BUILTIN, LOW);
    } else if (payload.equals("OFF") || payload.equals("TurnOFF")) {
      Serial.println("Turning OFF");
      digitalWrite(LED_BUILTIN, HIGH);      
    }
}

Device getWakandaDevice(String registryResponse) {
    
  DynamicJsonDocument wakandaDeviceJsonDocument(2048);
  
  deserializeJson(wakandaDeviceJsonDocument, registryResponse);
  
  NetworkConfig network {
    wakandaDeviceJsonDocument["network"]["port"],
    wakandaDeviceJsonDocument["network"]["localIP"],
    wakandaDeviceJsonDocument["network"]["publicIP"],
    wakandaDeviceJsonDocument["network"]["macAddres"],
    wakandaDeviceJsonDocument["network"]["gateway"],
    wakandaDeviceJsonDocument["network"]["mask"],
    wakandaDeviceJsonDocument["network"]["hostname"],
    wakandaDeviceJsonDocument["network"]["dns"],
    wakandaDeviceJsonDocument["network"]["ssid"]
  };
    
  Actuator actuators[] = {
    { 
      wakandaDeviceJsonDocument["actuators"][0]["name"], 
      wakandaDeviceJsonDocument["actuators"][0]["displayName"], 
      wakandaDeviceJsonDocument["actuators"][0]["type"], 
      wakandaDeviceJsonDocument["actuators"][0]["pin"], 
      wakandaDeviceJsonDocument["actuators"][0]["topic"]
    }
  };
  
  Sensor sensors[] = {
     { 
      wakandaDeviceJsonDocument["sensors"][0]["name"], 
      wakandaDeviceJsonDocument["sensors"][0]["displayName"], 
      wakandaDeviceJsonDocument["sensors"][0]["type"], 
      wakandaDeviceJsonDocument["sensors"][0]["pin"], 
      wakandaDeviceJsonDocument["sensors"][0]["topic"]
    }
  };
 
  Device device {
    wakandaDeviceJsonDocument["name"],
    wakandaDeviceJsonDocument["displayName"],
    network,
    actuators,
    sensors
  };
  
  return device;
}

String registryWakandaDevice(Device device) {
  DynamicJsonDocument payload(2048);
  String pathEndpoint = "/devices";
  String registryEndpoint = WAKANDA_API + pathEndpoint;

  String payloadString;

  payload["name"] = device.name;

  JsonObject network = payload.createNestedObject("network");
  network["port"] = device.network.port;
  network["localIP"] = device.network.localIP;
  network["publicIP"] = device.network.publicIP;

  JsonArray sensors = payload.createNestedArray("sensors");
  
  // ISSO AQ NAO TA FUNFANDO --'
  //Serial.println(ARRAY_SIZE(device.actuators));
  //Serial.println(ARRAY_SIZE(device.sensors));
 
  for (int i = 0; i < 1; i++){

      JsonObject sensors_chield = sensors.createNestedObject();
      sensors_chield["name"] = device.sensors[i].name;
      sensors_chield["displayName"] = device.sensors[i].displayName;
      sensors_chield["type"] = device.sensors[i].type;
      sensors_chield["pin"] = device.sensors[i].pin;
  }
  
  JsonArray actuators = payload.createNestedArray("actuators");

   
  for (int i = 0; i < 1; i++){
  
    JsonObject actuators_chield = actuators.createNestedObject();
    actuators_chield["name"] = device.actuators[i].name;
    actuators_chield["displayName"] = device.actuators[i].displayName;
    actuators_chield["type"] = device.actuators[i].type;
    actuators_chield["pin"] = device.actuators[i].pin;
  }
  
  serializeJson(payload, payloadString);
  
  Serial.print("Serialized: ");
  Serial.println(payloadString);
  
  String response = request("POST", registryEndpoint, payloadString, fingerprintWAKANDA);

  return response;
}

String getPublicIp() {
  String publicIP = request("GET", "checkip.amazonaws.com", "", fingerprintAWS);
  publicIP.replace("\n","");
  
  return publicIP;
}

Device getDeviceConfig(String name, const char* deviceName, int port, IPAddress localIP, String publicIP, String macAddres, IPAddress gateway, IPAddress mask, String hostname, IPAddress dns, String ssid, Actuator actuators[], Sensor sensors[]) {

  NetworkConfig networkConfig {
    port,
    localIP.toString(),
    publicIP,
    macAddres,
    gateway.toString(),
    mask.toString(),
    hostname,
    dns.toString(),
    ssid
  };

  
  Device deviceConfig {
    name,
    deviceName,
    networkConfig,
    actuators,
    sensors
  };

  return deviceConfig;
}



String request(String method, String uri, String payload, const char* fingerprint) {
  
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

  client->setFingerprint(fingerprint);
  
  HTTPClient https;
  
  https.addHeader("Content-Type", "application/json");
  
  int responseCode;
  String responseContent = "";

  https.begin(*client, "https://" + uri);

  if (method == "GET") {

    responseCode = https.GET();
    Serial.print("Responde code GET request code: ");
    Serial.println(responseCode);

    if (responseCode == HTTP_CODE_OK) {
      Serial.println("Request succeed!");
      responseContent = https.getString();
    } else {
      Serial.println("Status code not OK...");
      Serial.println(responseCode);      
    }

  } else if ( method == "POST") {

    String json;

    responseCode = https.POST(payload);
    Serial.print("Responde code POST request code: ");
    Serial.println(responseCode);

    if (responseCode == HTTP_CODE_OK) {
      Serial.println("Request succeed!");
      responseContent = https.getString();
    } else if (responseCode == 502) {
      Serial.println("Request 502 device already created!");
      responseContent = https.getString();      
    } else {
      Serial.println("This method not supported...");
    }
  }
  
  https.end();
  return responseContent;
}

String byteMessageToString(byte* byteMessage, unsigned int length) {
     String msg;
 
    for(int i = 0; i < length; i++) {
       char c = (char)byteMessage[i];
       msg += c;
    }

    return msg;
}
