#include "WebSocketsClient.h"
#include "PN532Instance.h"
#include "JSONRPC/JSONRPC.h"
#include "JSONRPC/Node.h"
#include "JSONRPC/WebsocketTransport.h"
#include "Utils.h"
#include "esp_log.h"
#include <thread>
#include "Arduino.h"
#include "network.h"

static const char* TAG = "main";

using namespace std::placeholders;
using json = nlohmann::json;

// tag auth
JSONRPC::WebsocketTransport tagAuthRPCTransport;
JSONRPC::Node tagAuthRPC(tagAuthRPCTransport);

// user auth
JSONRPC::WebsocketTransport userAuthRPCTransport;
JSONRPC::Node userAuthRPC(userAuthRPCTransport);

#define RELAY_PIN 32

void setupNFC()
{
    // Start PN532
    NFC.begin();

    delay(5);

    // Get PN532 firmware version
    GetFirmwareVersionResponse version;
    if (!NFC.GetFirmwareVersion(version)) {
        ESP_LOGE(TAG, "Unable to fetch PN532 firmware version");
        ESP.restart();
    }

    // configure board to read RFID tags and prevent going to sleep
    if (!NFC.SAMConfig()) {
        ESP_LOGE(TAG, "NFC SAMConfig failed");
        ESP.restart();
    }
    
    // Print chip data
    ESP_LOGI(TAG, "Found chip PN5%02X", version.IC);
    ESP_LOGI(TAG, "Firmware version: %#04X", version.Ver);
    ESP_LOGI(TAG, "Firmware revision: %#04X", version.Rev);
    ESP_LOGI(TAG, "Features: ISO18092: %d, ISO14443A: %d, ISO14443B: %d",
        (bool)version.Support.ISO18092,
        (bool)version.Support.ISO14443_TYPEA,
        (bool)version.Support.ISO14443_TYPEB);
    
    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is
    // the default behaviour of the PN532.
    if (!NFC.SetPassiveActivationRetries(0xFF)) {
        ESP_LOGE(TAG, "NFC SetPassiveActivationRetries failed");
        ESP.restart();
    }
}

// Builds tagInfo JSON object
json BuildTagInfo(const PN532Packets::TargetDataTypeA& tgdata)
{
    json taginfo;
    taginfo["ATQA"] = tgdata.ATQA;
    taginfo["SAK"] = tgdata.SAK;
    // Hex encoded strings
    taginfo["ATS"] = BinaryDataToHexString(tgdata.ATS);
    taginfo["UID"] = BinaryDataToHexString(tgdata.UID);
    return taginfo;
}

// Waits for RFID tag and then authenticates
std::thread* RFIDThreadHandle = nullptr;
void RFIDThread()
{
    // Configures PN532
    setupNFC();

    // Door relay
    pinMode(RELAY_PIN, OUTPUT);

    // Holds index of the current active tag
    // Used in matching transceive RPC call to tag
    static int currentTg = 0;

    // Transceive RPC method
    tagAuthRPC.bind("transceive", [](std::string data) {
        // Parse hex string to binary array
        BinaryData buf = HexStringToBinaryData(data);

        // Create interface with tag
        TagInterface tif = NFC.CreateTagInterface(currentTg);

        // Send
        if (tif.Write(buf))
            throw JSONRPC::RPCMethodException(1, "Write failed");

        // Receive
        if (tif.Read(buf) < 0)
            throw JSONRPC::RPCMethodException(2, "Read failed");

        // Convert back to hex encoded string
        return BinaryDataToHexString(buf);
    });

    while(1)
    {
        // Finds nearby ISO14443 Type A tags
        InListPassiveTargetResponse resp = NFC.InListPassiveTarget(1, BRTY_106KBPS_TYPE_A);

        // Only read one tag at a time
        if (resp.NbTg != 1)
        {
            // Yield to kernel
            delay(10);
            continue;
        }
        
        ESP_LOGI(TAG, "Detected RFID tag");

        // For parsing response data
        ByteBuffer buf(resp.TgData);

        // Parse as ISO14443 Type A target
        PN532Packets::TargetDataTypeA tgdata;
        buf >> tgdata;

        // Set current active tag
        currentTg = tgdata.Tg;

        // Convert UID to hex encoded string
        std::string UID = BinaryDataToHexString(tgdata.UID);
        ESP_LOGI(TAG, "Tag UID: %s", UID.c_str());
        
        // Initiate authentication
        try {
            if (!tagAuthRPC.call("auth", BuildTagInfo(tgdata)))
            {
                ESP_LOGE(TAG, "Tag auth failed!");
                continue;
            }

            if (!userAuthRPC.call("auth_uid", "doors", UID))
            {
                ESP_LOGE(TAG, "User auth failed!");
                continue;
            }
        } catch (const JSONRPC::RPCMethodException& e) {
            ESP_LOGE(TAG, "RPC call failed: %s", e.message.c_str());
            continue;
        } catch (const JSONRPC::TimeoutException& e) {
            ESP_LOGE(TAG, "RPC call timed out");
            continue;
        }

        ESP_LOGI(TAG, "Auth successful!");

        // Open doors
        digitalWrite(RELAY_PIN, HIGH);
        delay(500);
        digitalWrite(RELAY_PIN, LOW);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    // Configures WiFi and/or Ethernet
    network_setup();

    // Configures task state reporting if enabled in menuconfig
    EnableTaskStats();

    // Connects to RPC servers
    tagAuthRPCTransport.begin("192.168.1.175", 3000);
    userAuthRPCTransport.begin("192.168.1.175", 3001);

    // Starts main RFID thread
    RFIDThreadHandle = new std::thread(RFIDThread);
}

void loop() {
    network_loop();
    tagAuthRPCTransport.update();
    userAuthRPCTransport.update();

    // Yield to kernel to prevent triggering watchdog
    delay(10);
}
