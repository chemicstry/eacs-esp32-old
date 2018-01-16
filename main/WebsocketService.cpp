#include "WebsocketService.h"
#include "Arduino.h"

using namespace std::placeholders;

WebsocketService::WebsocketService(JsonDataInterface& dataif): JsonEventInterface(dataif), _rfidService(*this)
{
    On("hello", std::bind(&WebsocketService::OnServerHello, this, _1));
    On("unlock", std::bind(&WebsocketService::OnUnlock, this, _1));

    pinMode(32, OUTPUT);
}

void WebsocketService::OnServerHello(const JsonObject& data)
{
    Serial.println("Server hello!");

    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& args = jsonBuffer.createObject();
    args["hello"] = "world";
    Send("hello", args);
}

void WebsocketService::OnUnlock(const JsonObject& args)
{
    digitalWrite(32, HIGH);
    _timer.setTimeout(200, []() {
        digitalWrite(32, LOW);
    });
}

void WebsocketService::Reset()
{
}

void WebsocketService::Update()
{
    _timer.run();
    _rfidService.Update();
}
