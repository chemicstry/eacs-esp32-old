#ifndef _JSONEVENTINTERFACE_H_
#define _JSONEVENTINTERFACE_H_

#include "JsonEventEmitter.h"
#include "JsonDataInterface.h"

class JsonEventInterface : public JsonEventEmitter
{
public:
    JsonEventInterface(JsonDataInterface& dataif);
    void Send(const std::string name, const JsonObject& args);

private:
    void _OnReceiveData(const JsonObject& data);
    JsonDataInterface& _dataif;
};

#endif
