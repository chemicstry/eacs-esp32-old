#ifndef _WEBSOCKETSERVICE_H_
#define _WEBSOCKETSERVICE_H_

#include "JsonDataInterface.h"
#include "ArduinoJson.h"
#include "JsonEventInterface.h"
#include "RFIDService.h"
#include "SimpleTimer.h"

class WebsocketService : public JsonEventInterface
{
public:
    WebsocketService(JsonDataInterface& dataif);
    void OnServerHello(const JsonObject& args);
    void OnUnlock(const JsonObject& args);

    void Reset();
    void Update();

private:
    RFIDService _rfidService;
    SimpleTimer _timer;
};

#endif
