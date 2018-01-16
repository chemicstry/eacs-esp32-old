#ifndef _JSONDATAINTERFACE_H_
#define _JSONDATAINTERFACE_H_

#include "DataInterface.h"
#include "ArduinoJson.h"

typedef DataInterface<JsonObject> JsonDataInterface;
typedef BidirectionalDataInterface<JsonObject> JsonBidirectionalDataInterface;

#endif
