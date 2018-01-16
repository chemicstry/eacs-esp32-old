#include "JsonEventInterface.h"

using namespace std::placeholders;

JsonEventInterface::JsonEventInterface(JsonDataInterface& dataif): _dataif(dataif)
{
    dataif.SetCb(std::bind(&JsonEventInterface::_OnReceiveData, this, _1));
}

void JsonEventInterface::Send(const std::string name, const JsonObject& args)
{
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& object = jsonBuffer.createObject();
    object["event"] = name.c_str();
    object["args"] = args;

    _dataif.Send(object);
}

void JsonEventInterface::_OnReceiveData(const JsonObject& data)
{
    Emit(data["event"].as<char*>(), data["args"]);
}