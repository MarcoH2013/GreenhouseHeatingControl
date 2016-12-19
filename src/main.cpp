#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#include <DNSServer.h>          // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>   // https://github.com/esp8266/Arduino
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <SimpleTimer.h>        // https://github.com/jfturcot/SimpleTimer

#include <FS.h>

#include <PubSubClient.h>       // http://pubsubclient.knolleary.net

#include <DHT.h>                // https://github.com/adafruit/DHT-sensor-library

#define DBG_OUTPUT_PORT Serial

// Define MQTT Server
char const* mqtt_server = "nibbler";

// Define DHT
#define DHTPIN1 12     // what digital pin we're connected to
#define DHTTYPE1 DHT22   // DHT 22  (AM2302), AM2321

// Define DHT
#define DHTPIN2 13     // what digital pin we're connected to
#define DHTTYPE2 DHT22   // DHT 22  (AM2302), AM2321

#define OUTPIN1 4 // Relais IN1
#define OUTPIN2 5 // Relais IN2

// Declare timer
SimpleTimer timer;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient espClient;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Declare mqtt client
PubSubClient client(espClient);

// Declare dht
DHT dht1(DHTPIN1, DHTTYPE1);
DHT dht2(DHTPIN2, DHTTYPE2);

byte hol;

void configModeCallback (WiFiManager *myWiFiManager) {
    DBG_OUTPUT_PORT.println("Entered config mode");
    DBG_OUTPUT_PORT.println(WiFi.softAPIP());
    DBG_OUTPUT_PORT.println(myWiFiManager->getConfigPortalSSID());
}

void publishMessage(char topicName[], float value) {
    char msg[50];
    char topic[100];

    sprintf(topic, "/ESPDHT/%i/%s", ESP.getChipId(), topicName);
    dtostrf(value, 4, 2, msg);
    client.publish(topic, msg);
}

void readSensor1() {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht1.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht1.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
        //DBG_OUTPUT_PORT.println("Failed to read from DHT sensor!");
        return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht1.computeHeatIndex(t, h, false);

    publishMessage("sensor1/humidity", h);
    publishMessage("sensor1/temperature", t);
    publishMessage("sensor1/heatindex", hic);

    DBG_OUTPUT_PORT.print("Sensor 1: ");
    DBG_OUTPUT_PORT.print("Humidity: ");
    DBG_OUTPUT_PORT.print(h);
    DBG_OUTPUT_PORT.print(" % ");
    DBG_OUTPUT_PORT.print("Temperature: ");
    DBG_OUTPUT_PORT.print(t);
    DBG_OUTPUT_PORT.print(" *C ");
    DBG_OUTPUT_PORT.print("Heat index: ");
    DBG_OUTPUT_PORT.print(hic);
    DBG_OUTPUT_PORT.println(" *C");
}

void readSensor2() {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht2.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht2.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
        //DBG_OUTPUT_PORT.println("Failed to read from DHT sensor!");
        return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht2.computeHeatIndex(t, h, false);

    publishMessage("sensor2/humidity", h);
    publishMessage("sensor2/temperature", t);
    publishMessage("sensor2/heatindex", hic);

    DBG_OUTPUT_PORT.print("Sensor 2: ");
    DBG_OUTPUT_PORT.print("Humidity: ");
    DBG_OUTPUT_PORT.print(h);
    DBG_OUTPUT_PORT.print(" % ");
    DBG_OUTPUT_PORT.print("Temperature: ");
    DBG_OUTPUT_PORT.print(t);
    DBG_OUTPUT_PORT.print(" *C ");
    DBG_OUTPUT_PORT.print("Heat index: ");
    DBG_OUTPUT_PORT.print(hic);
    DBG_OUTPUT_PORT.println(" *C");
}

void callback(char const* topic, byte* payload, unsigned int length) {
    DBG_OUTPUT_PORT.print("Message arrived [");
    DBG_OUTPUT_PORT.print(topic);
    DBG_OUTPUT_PORT.print("] ");
    for (int i = 0; i < length; i++) {
        DBG_OUTPUT_PORT.print((char)payload[i]);
    }
    DBG_OUTPUT_PORT.println();

    if (String(topic).endsWith("/reboot")) {
      DBG_OUTPUT_PORT.print("Reboot...");
      ESP.restart();
    } else if (String(topic).endsWith("/reset")) {
      DBG_OUTPUT_PORT.print("Reset setting...");
      WiFiManager wifiManager;
      wifiManager.resetSettings();
    } else if (String(topic).endsWith("/relay1")) {
      if ((char)payload[0] == '1') {
          digitalWrite(OUTPIN1, HIGH);
      } else {
          digitalWrite(OUTPIN1, LOW);
      }
    } else if (String(topic).endsWith("/relay2")) {
      if ((char)payload[0] == '1') {
          digitalWrite(OUTPIN2, HIGH);
      } else {
          digitalWrite(OUTPIN2, LOW);
      }
    }
}

void reconnect() {
    char topic[100];
    // Loop until we're reconnected
    while (!client.connected()) {
        DBG_OUTPUT_PORT.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESPDHT-";
        clientId += String(ESP.getChipId());

        // Attempt to connect
        if (client.connect(clientId.c_str())) {
            DBG_OUTPUT_PORT.println("connected");
            // Once connected, publish an announcement...
            sprintf(topic, "/ESPDHT/%i", ESP.getChipId());
            client.publish(topic, "hello world");
            // ... and resubscribe
            sprintf(topic, "/ESPDHT/%i/reboot", ESP.getChipId());
            client.subscribe(topic);
            //delay(100);
            sprintf(topic, "/ESPDHT/%i/reset", ESP.getChipId());
            client.subscribe(topic);
            //delay(100);
            sprintf(topic, "/ESPDHT/%i/relay1", ESP.getChipId());
            client.subscribe(topic);
            //delay(100);
            sprintf(topic, "/ESPDHT/%i/relay2", ESP.getChipId());
            client.subscribe(topic);
        } else {
            DBG_OUTPUT_PORT.print("failed, rc=");
            DBG_OUTPUT_PORT.print(client.state());
            DBG_OUTPUT_PORT.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    DBG_OUTPUT_PORT.begin(74880);
    DBG_OUTPUT_PORT.println("Booting up...");

    pinMode(OUTPIN1, OUTPUT);
    pinMode(OUTPIN2, OUTPUT);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset saved settings
    //wifiManager.resetSettings();
    wifiManager.setDebugOutput(true);
    wifiManager.setAPCallback(configModeCallback);

    wifiManager.autoConnect("myesp8266");

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    dht1.begin();
    dht2.begin();

    timer.setInterval(5000, readSensor1);
    timer.setInterval(5000, readSensor2);
}

void loop() {
    //ArduinoOTA.handle();

    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    timer.run();
}
