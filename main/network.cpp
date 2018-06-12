#include "network.h"
#include "esp_log.h"

static const char* TAG = "NETWORK";

WiFiMulti wifiMulti;

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

void network_setup()
{
    // Enable wifi event callbacks
    WiFi.onEvent(WiFiEvent);

    #if CONFIG_USE_WIFI
        wifiMulti.addAP(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);

        ESP_LOGI(TAG, "Connecting Wifi...");
        if (wifiMulti.run() == WL_CONNECTED)
            ESP_LOGI(TAG, "WiFi connected. IP: %s", WiFi.localIP().toString().c_str());
    #endif

    #if CONFIG_USE_ETH
        ETH.begin();
    #endif
}

bool network_loop()
{
    #if CONFIG_USE_WIFI
        if (wifiMulti.run() != WL_CONNECTED) {
            ESP_LOGE(TAG, "WiFi not connected!");
            delay(1000);
            return false;
        }
    #endif

    return true;
}
