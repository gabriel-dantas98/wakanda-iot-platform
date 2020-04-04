// ########## BIBLIOTECAS ########## //

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0]))) //WHAT https://forum.arduino.cc/index.php?topic=527619.0

const char* ssid = "###########";
const char* password = "#############";
const char* WAKANDA_API = "192.168.2.136:5000";
const char* BROKER_ADDRESS = "m24.cloudmqtt.com";
const char* BROKER_USER = "##########";
const char* BROKER_PASSWORD = "##########";
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
HTTPClient http;
PubSubClient MQTT(espClient);

Device device;

//Prototypes
Device getDeviceConfig(String name, const char* deviceName, int port, IPAddress localIp, String publicIp, String macAddres, IPAddress gateway, String mask, String hostname, IPAddress dns, String ssid);
String getPublicIp();
String request(String method, String uri, String payload = "");
void WiFiConnect();
PubSubClient MQTTConnect();

void setup() {

  Serial.begin(9600);
  WiFi.begin(ssid, password);
  MQTT.setServer(BROKER_ADDRESS, BROKER_PORT);
  MQTT.setCallback(MQTTCallback);

  WiFiConnect();
  PubSubClient mqttConnection = MQTTConnect();
  
  Actuator actuators[] = {
    {"lampada_quarto", "Lampada quarto", "relay", LED_BUILTIN, ""}
  };
  
  Serial.print("Number of actuators: ");
  Serial.println(ARRAY_SIZE(actuators));
  
  Sensor sensors[] = {
    {"presenca_quarto", "Presenca Quarto", "proximity", 15, ""}
  };
  
  Serial.print("Number of sensors: ");
  Serial.println(ARRAY_SIZE(sensors));
  
  //Verify and registry device in Wakanda Orchestractor
  device = getDeviceConfig(WiFi.hostname(), DISPLAY_DEVICE_NAME, 8080, WiFi.localIP(), 
                           getPublicIp(), WiFi.macAddress(), WiFi.gatewayIP(), WiFi.subnetMask(), 
                           WiFi.hostname(), WiFi.dnsIP(), WiFi.SSID(), actuators, sensors);

  Serial.println("RUNNING SETUP....");
  String response = registryWakandaDevice(device);

  Device wakandaDevice = getWakandaDevice(response);
  Serial.println(wakandaDevice.displayName);
  Serial.println(wakandaDevice.network.localIP);
  Serial.println(wakandaDevice.sensors[0].topic);
  Serial.println(wakandaDevice.actuators[0].topic);
  
  pinMode(wakandaDevice.actuators[0].pin, OUTPUT);
  mqttConnection.subscribe(wakandaDevice.actuators[0].topic);
}

void loop() {
  //Get device information and subscribe in your mqtt topic
  Serial.println("RUNNING LOOP....");
  MQTT.loop();
  delay(2000);
}

void WiFiConnect() {
  
  while (WiFi.status() != WL_CONNECTED) {
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
      digitalWrite(LED_BUILTIN, HIGH);
    } else if (payload.equals("OFF") || payload.equals("TurnOFF")) {
      Serial.println("Turning OFF");
      digitalWrite(LED_BUILTIN, LOW);      
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
  
  String response = request("POST", registryEndpoint, payloadString);

  return response;
}

String getPublicIp() {

  return request("GET", "checkip.amazonaws.com");
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

String request(String method, String uri, String payload) {

  http.useHTTP10(true);
  http.addHeader("Content-Type", "application/json");
  int responseCode;
  String responseContent;

  http.begin("http://" + uri);

  if (method == "GET") {

    responseCode = http.GET();
    Serial.print("Responde code GET request code: ");
    Serial.println(responseCode);

    if (responseCode == HTTP_CODE_OK) {
      Serial.println("Request succeed!");
      responseContent = http.getString();
    }

  } else if ( method == "POST") {

    String json;
    //serializeJson(payload, json);

    responseCode = http.POST(payload);
    Serial.print("Responde code POST request code: ");
    Serial.println(responseCode);

    if (responseCode == HTTP_CODE_OK) {
      Serial.println("Request succeed!");
      responseContent = http.getString();
    }

  } else {
    Serial.println("This method not supported...");
  }


  http.end();

  return responseContent;
}

String byteMessageToString(byte* byteMessage, unsigned int length) {
     String msg;
 
    for(int i = 0; i < length; i++) 
    {
       char c = (char)byteMessage[i];
       msg += c;
    }

    return msg;
}
