#ifndef _RFIDSERVICE_H_
#define _RFIDSERVICE_H_

#include "JsonEventInterface.h"
#include "SimpleTimer.h"

#define NEW_TARGET_TIMEOUT 5000

class RFIDService
{
public:
    RFIDService(JsonEventInterface& evtif);

    void OnTagTransceive(const JsonObject& args);
    void OnReleaseTarget(const JsonObject& args);

    void AcquireNewTarget();
    void Update();

private:
    JsonEventInterface& _evtif;
    int8_t _target; // Current nfc target
    int32_t _releaseTimer;
    SimpleTimer _timer;
};

#endif
