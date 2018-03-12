#include "ServiceManager.h"
#include "Arduino.h"

using namespace std::placeholders;

ServiceManager::ServiceManager(JsonDataInterface& dataif): JsonEventInterface(dataif), _rfidService(*this)
{
    On("hello", std::bind(&ServiceManager::OnServerHello, this, _1));
    On("unlock", std::bind(&ServiceManager::OnUnlock, this, _1));

    pinMode(32, OUTPUT);
}

void ServiceManager::OnServerHello(const JsonObject& data)
{
    Serial.println("Server hello!");

    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& args = jsonBuffer.createObject();
    args["hello"] = "world";
    Send("hello", args);
}

void ServiceManager::OnUnlock(const JsonObject& args)
{
    digitalWrite(32, HIGH);
    _timer.setTimeout(200, []() {
        digitalWrite(32, LOW);
    });
}

void ServiceManager::Reset()
{
}

void ServiceManager::Update()
{
    _timer.run();
    _rfidService.Update();
}
