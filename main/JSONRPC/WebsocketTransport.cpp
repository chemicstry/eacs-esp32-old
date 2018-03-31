#include "WebsocketTransport.h"
#include "esp_log.h"

static const char* TAG = "WST";

using namespace JSONRPC;
using namespace std::placeholders;

WebsocketTransport::WebsocketTransport()
{
    setUpstreamHandler(JSONRPC::TransportHandler([this](const json& j) {
        std::string s = j.dump();
        ESP_LOGI(TAG, "Sent: %s", s.c_str());
        {
            std::lock_guard<std::recursive_mutex> l(_mutex);
            sendTXT(s.c_str());
        }
    }));

    onEvent(std::bind(&WebsocketTransport::eventHandler, this, _1, _2, _3));
}

void WebsocketTransport::eventHandler(WStype_t type, uint8_t* payload, size_t length)
{
    switch(type)
    {
        case WStype_DISCONNECTED:
        {
            Serial.printf("[WSc] Disconnected!\n");
            ESP_LOGI(TAG, "Disconnected");
            break;
        }
        case WStype_CONNECTED:
        {
            ESP_LOGI(TAG, "Connected to url: %s", (char*)payload);
            break;
        }
        case WStype_TEXT:
        {
            ESP_LOGI(TAG, "Received: %s", (char*)payload);
            {
                std::lock_guard<std::recursive_mutex> l(_mutex);
                sendDownstream(json::parse((char*)payload));
            }
            break;
        }
        case WStype_BIN:
        {
            ESP_LOGI(TAG, "Received binary. Len: %zu", length);
            break;
        }
    }
}
