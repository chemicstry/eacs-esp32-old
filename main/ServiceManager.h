#ifndef _SERVICEMANAGER_H_
#define _SERVICEMANAGER_H_

#include "JsonDataInterface.h"
#include "ArduinoJson.h"
#include "JsonEventInterface.h"
#include "RFIDService.h"
#include "SimpleTimer.h"

class ServiceManager : public JsonEventInterface
{
public:
    ServiceManager(JsonDataInterface& dataif);
    void OnServerHello(const JsonObject& args);
    void OnUnlock(const JsonObject& args);

    void Reset();
    void Update();

private:
    RFIDService _rfidService;
    SimpleTimer _timer;
};

#endif
