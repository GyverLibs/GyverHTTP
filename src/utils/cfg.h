#pragma once

#ifdef ESP8266
#define GHTTP_ESP_YIELD() esp_yield();//optimistic_yield(2000);
#else
#define GHTTP_ESP_YIELD()
#endif