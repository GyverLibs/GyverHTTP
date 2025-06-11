#pragma once
#include <Arduino.h>
#include <StringUtils.h>

// #define GHTTP_HEADERS_LOG Serial

#include "cfg.h"

namespace ghttp {

class HeadersCollector {
   public:
    virtual void header(Text& name, Text& value) = 0;
};

class HeadersParser {
   public:
    template <typename client_t>
    HeadersParser(client_t& client, HeadersCollector* collector = nullptr) {
        contentType.reserve(50);
        String buf;

        while (client.connected()) {
            GHTTP_ESP_YIELD();
            buf = client.readStringUntil('\n');
            size_t n = buf.length();

            if (!n || buf[n - 1] != '\r') break;  // пустая или не оканчивается на \r
            if (n == 1) {                         // == \r
                valid = true;
                break;
            }

            Text header(buf.c_str(), n - 1);

#ifdef GHTTP_HEADERS_LOG
            GHTTP_HEADERS_LOG.println(header);
#endif

            int16_t colon = header.indexOf(':');
            if (colon > 0) {
                Text name = header.substring(0, colon);
                Text value = header.substring(colon + 1).trim();

                if (collector) collector->header(name, value);

                switch (name.hash()) {
                    case SH("Content-Type"): value.addString(contentType); break;
                    case SH("Content-Length"): length = value.toInt32(); break;
                    case SH("Transfer-Encoding"): chunked = (value == F("chunked")); break;
                    case SH("Connection"): close = (value == F("close")); break;
                }
            }
        }
    }

    // legacy
    template <typename client_t>
    HeadersParser(client_t& client, size_t, HeadersCollector* collector = nullptr) : HeadersParser(client, collector) {}

    String contentType;
    size_t length = 0;
    bool close = false;
    bool valid = false;
    bool chunked = false;

    operator bool() {
        return valid;
    }
};

}  // namespace ghttp