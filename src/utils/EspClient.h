#pragma once
#include <Arduino.h>

#if defined(ESP8266) || defined(ESP32)

#include "Client.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#else
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif

namespace ghttp {

class EspInsecureClient : public ghttp::Client {
   public:
    EspInsecureClient(const char* host, uint16_t port) : ghttp::Client(_client, host, port) {
#ifdef ESP8266
        // _client.setBufferSizes(512, 512);
#endif
        _client.setInsecure();
    }

   private:
#if defined(ESP8266)
    BearSSL::WiFiClientSecure _client;
#else
    WiFiClientSecure _client;
#endif
};

}  // namespace ghttp
#endif