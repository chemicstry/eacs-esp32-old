#include "src/arduinoWebSockets/WebSocketsClient.h"
#include "src/SimpleTimer/SimpleTimer.h"
#include "WebsocketService.h"
#include "PN532Instance.h"

#define USE_ETH
//#define USE_WIFI

#ifdef USE_WIFI
#define WIFI_SSID "ssid"
#define WIFI_PASS "password"
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#endif

#ifdef USE_ETH
#include <ETH.h>
#endif

JsonBidirectionalDataInterface jif;
WebsocketService wservice(jif.Downstream);

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
        {
            Serial.printf("[WSc] Disconnected!\n");
            wservice.Reset();
            break;
        }
        case WStype_CONNECTED:
        {
            Serial.printf("[WSc] Connected to url: %s\n",  payload);
            break;
        }
        case WStype_TEXT:
        {
            StaticJsonBuffer<512> jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(payload);
            jif.Upstream.Send(root);
            break;
        }
        case WStype_BIN:
        {
            Serial.printf("[WSc] get binary length: %u\n", length);
            break;
        }
    }
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event) {
        case SYSTEM_EVENT_ETH_START:
            Serial.println("ETH Started");
            //set eth hostname here
            ETH.setHostname("esp32-ethernet");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            Serial.println("ETH Connected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            Serial.print("ETH MAC: ");
            Serial.print(ETH.macAddress());
            Serial.print(", IPv4: ");
            Serial.print(ETH.localIP());
            if (ETH.fullDuplex()) {
                Serial.print(", FULL_DUPLEX");
            }
            Serial.print(", ");
            Serial.print(ETH.linkSpeed());
            Serial.println("Mbps");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH Disconnected");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            Serial.println("ETH Stopped");
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    // Start PN532
    NFC.begin();

    uint32_t versiondata = NFC.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("Didn't find PN53x board");
        ESP.restart();
    }
    
    // Print PN532 chip data
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    
    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is
    // the default behaviour of the PN532.
    NFC.setPassiveActivationRetries(0xFF);
    
    // configure board to read RFID tags
    NFC.SAMConfig();

    // Enable wifi event callbacks
    WiFi.onEvent(WiFiEvent);

#ifdef USE_WIFI
    wifiMulti.addAP(WIFI_SSID, WIFI_PASS);

    Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
#endif

#ifdef USE_ETH
    ETH.begin();
#endif

    //webSocket.beginSSL("rfidtest.gaben.win", 443);
    webSocket.begin("78.63.23.25", 25687);
    webSocket.onEvent(webSocketEvent);
  
    // Bind upstream callback to relay data to websocket
    jif.Upstream.SetCb([](const JsonObject& data) {
        static char buf[200];
        data.printTo(buf);
        webSocket.sendTXT(buf);
    });
}

void loop() {
    webSocket.loop();
    wservice.Update();
}
