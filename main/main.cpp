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
#include "driver/uart.h"

std::vector<std::string> hardcoded = {
    "046981BA703A80",
    "044438723D4B80",
    "044438723D4B80",
    "E54BA965",
    "043D1DB2504380",
    "046B85BA504380",
    "04054482504A80",
    "14CBC66C",
    "04353DBAD84E80",
    "04638C1A504380",
    "049A429A504A80",
    "044E668A3D4B80",
    "04550B524A4A80",
    "0439275A4A4A80",
    "0494431A504380",
    "045A0482664980",
    "04974E32DC4880",
    "04195AAA504380",
    "A6C73912056037",
    "041D7AAAD84E80"
};

static const char* TAG = "main";

static const char* tokenHeader = "token: eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZGVudGlmaWVyIjoiZnJvbnRfZG9vciIsInBlcm1pc3Npb25zIjpbImVhY3MtdXNlci1hdXRoOmF1dGhfdWlkIl19.RZmKDv2f-aNrwwrw_9GZEu1AI_svV24R2PVtzVi2jQAN-1bcjWQBw0lM_JYCxdX5oS5sx92C26qtpZSg1iGdng";

using namespace std::placeholders;
using json = nlohmann::json;

// tag auth
JSONRPC::WebsocketTransport tagAuthRPCTransport;
JSONRPC::Node tagAuthRPC(tagAuthRPCTransport);
const char* tagAuthRPCFingerprint = "E1:0D:E5:8E:A1:E7:61:3D:50:FF:BF:F3:C0:22:61:FC:14:BA:B8:34:9E:5B:C5:7B:65:0A:A3:7B:04:E6:30:4A";

// user auth
JSONRPC::WebsocketTransport userAuthRPCTransport;
JSONRPC::Node userAuthRPC(userAuthRPCTransport);
const char* userAuthRPCFingerprint = "E1:0D:E5:8E:A1:E7:61:3D:50:FF:BF:F3:C0:22:61:FC:14:BA:B8:34:9E:5B:C5:7B:65:0A:A3:7B:04:E6:30:4A";

// message bus
JSONRPC::WebsocketTransport messageBusRPCTransport;
JSONRPC::Node messageBusRPC(messageBusRPCTransport);
const char* messageBusRPCFingerprint = "E1:0D:E5:8E:A1:E7:61:3D:50:FF:BF:F3:C0:22:61:FC:14:BA:B8:34:9E:5B:C5:7B:65:0A:A3:7B:04:E6:30:4A";

#define RELAY_PIN 32

#define BUZZER_PIN 16
void buzz(int duration)
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
}

void tone(int pin, int freq, int duration)
{
    int channel = 0;
    int resolution = 8;

    ledcSetup(channel, 4000, resolution);
    ledcAttachPin(pin, channel);
    ledcWriteTone(channel, freq);
    ledcWrite(channel, 255);
    delay(duration);
    ledcWrite(channel, 0);
}

void setupNFC()
{
    // Start PN532
    NFC.begin();

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

void Open()
{
    digitalWrite(RELAY_PIN, HIGH);
    //buzz(100);
    //delay(3000);
    for (int i =0; i < 6; i++)
    {
        buzz(200);
        delay(400);
    }
    digitalWrite(RELAY_PIN, LOW);
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

        buzz(50);
        
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

        if (std::find(hardcoded.begin(), hardcoded.end(), UID) != hardcoded.end())
        {
            Open();
            continue;
        }
        
        // Initiate authentication
        try {
            if (!tagAuthRPC.call("auth", BuildTagInfo(tgdata)))
            {
                ESP_LOGE(TAG, "Tag auth failed!");
                buzz(500);
                continue;
            }

            if (!userAuthRPC.call("auth_uid", "doors", UID))
            {
                ESP_LOGE(TAG, "User auth failed!");
                buzz(500);
                continue;
            }
        } catch (const JSONRPC::RPCMethodException& e) {
            ESP_LOGE(TAG, "RPC call failed: %s", e.message.c_str());
            buzz(500);
            continue;
        } catch (const JSONRPC::TimeoutException& e) {
            ESP_LOGE(TAG, "RPC call timed out");
            buzz(500);
            continue;
        }

        ESP_LOGI(TAG, "Auth successful!");

        // Open doors
        Open();
    }
}

#define READER_BUTTON_PIN 14
#define READER_BUTTON_TIMEOUT 1000
#define EXIT_BUTTON_PIN 13
#define EXIT_BUTTON_TIMEOUT 1000

std::thread* GPIOThreadHandle = nullptr;
void GPIOThread()
{
    static uint64_t readerButtonTimeout = 0;
    static uint64_t exitButtonTimeout = 0;

    ESP_LOGI(TAG, "GPIO thread started");

    while(1)
    {
        if (!digitalRead(READER_BUTTON_PIN) && readerButtonTimeout+READER_BUTTON_TIMEOUT < millis())
        {
            readerButtonTimeout = millis();
            ESP_LOGI(TAG, "Reader button pressed");
            messageBusRPC.notify("publish", "button");
        }

        if (!digitalRead(EXIT_BUTTON_PIN) && exitButtonTimeout+EXIT_BUTTON_TIMEOUT < millis())
        {
            exitButtonTimeout = millis();
            ESP_LOGI(TAG, "Exit button pressed");
            Open();
        }

        delay(10);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    pinMode(BUZZER_PIN, OUTPUT);
    buzz(200);
    delay(200);
    buzz(200);
    delay(200);
    buzz(200);

    // Configures WiFi and/or Ethernet
    network_setup();

    // Configures task state reporting if enabled in menuconfig
    EnableTaskStats();

    // Setup button as input
    pinMode(READER_BUTTON_PIN, INPUT);
    pinMode(EXIT_BUTTON_PIN, INPUT);

    // Connects to RPC servers
    tagAuthRPCTransport.beginSSL("192.168.1.209", 3000, "/", tagAuthRPCFingerprint);
    userAuthRPCTransport.beginSSL("192.168.1.209", 3001, "/", userAuthRPCFingerprint);
    messageBusRPCTransport.beginSSL("192.168.1.209", 3002, "/", messageBusRPCFingerprint);

    // Sets authorisation tokens
    tagAuthRPCTransport.setExtraHeaders(tokenHeader);
    userAuthRPCTransport.setExtraHeaders(tokenHeader);
    messageBusRPCTransport.setExtraHeaders(tokenHeader);

    // Start GPIO thread
    GPIOThreadHandle = new std::thread(GPIOThread);

    // Starts main RFID thread
    RFIDThreadHandle = new std::thread(RFIDThread);
}

void loop() {
    if (network_loop())
    {
        tagAuthRPCTransport.update();
        userAuthRPCTransport.update();
        messageBusRPCTransport.update();
    }

    // Yield to kernel to prevent triggering watchdog
    delay(10);
}
