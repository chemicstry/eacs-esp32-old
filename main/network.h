#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "Arduino.h"

#if CONFIG_USE_WIFI
#include <WiFi.h>
#include <WiFiMulti.h>
#endif

#if CONFIG_USE_ETH
#include <ETH.h>
#endif

void WiFiEvent(WiFiEvent_t event);

void network_setup();
void network_loop();

#endif
