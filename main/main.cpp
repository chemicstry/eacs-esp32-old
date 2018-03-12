#include "WebSocketsClient.h"
#include "SimpleTimer.h"
#include "ServiceManager.h"
#include "PN532Instance.h"
#include "JSONRPC/Client.h"

#if CONFIG_USE_WIFI
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#endif

#if CONFIG_USE_ETH
#include <ETH.h>
#endif

JsonBidirectionalDataInterface jif;
ServiceManager svcmgr(jif.Downstream);

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
        {
            Serial.printf("[WSc] Disconnected!\n");
            svcmgr.Reset();
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
#if CONFIG_USE_ETH
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
#endif
        default:
            break;
    }
}

JSONRPC::Client c;

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    c.SetHandler(JSONRPC::JSONTransportHandler([](const json& o) {
        json resp;
        resp["jsonrpc"] = "2.0";
        resp["id"] = o["id"];
        resp["result"] = "labas as krabas";
        c.HandleData(resp);
    }));

    std::future<json> o = c.CallAsync("test", 1, "2");
    Serial.println(o.get().dump().c_str());

    // Start PN532
    NFC.begin();

    GetFirmwareVersionResponse version;
    if (!NFC.GetFirmwareVersion(version)) {
        Serial.print("Didn't find PN53x board");
        ESP.restart();
    }
    
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println(version.IC, HEX);
    Serial.print("Firmware version: "); Serial.println(version.Ver, DEC);
    Serial.print("Firmware revision: "); Serial.println(version.Rev, DEC);
    Serial.print("Supports ISO18092: "); Serial.println((bool)version.Support.ISO18092);
    Serial.print("Supports ISO14443 Type A: "); Serial.println((bool)version.Support.ISO14443_TYPEA);
    Serial.print("Supports ISO14443 Type B: "); Serial.println((bool)version.Support.ISO14443_TYPEB);
    
    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is
    // the default behaviour of the PN532.
    NFC.SetPassiveActivationRetries(0x00);
    
    // configure board to read RFID tags
    NFC.SAMConfig();

    // Enable wifi event callbacks
    WiFi.onEvent(WiFiEvent);

    #if CONFIG_USE_WIFI
        wifiMulti.addAP(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);

        Serial.println("Connecting Wifi...");
        if(wifiMulti.run() == WL_CONNECTED) {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
        }
    #endif

    #if CONFIG_USE_ETH
        ETH.begin();
    #endif

    #if CONFIG_SERVER_SSL
        webSocket.beginSSL(CONFIG_SERVER_HOST, CONFIG_SERVER_PORT);
    #else
        webSocket.begin(CONFIG_SERVER_HOST, CONFIG_SERVER_PORT);
    #endif

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
    svcmgr.Update();
}
