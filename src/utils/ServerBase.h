#pragma once
#include <Arduino.h>
#include <Client.h>
#include <StringUtils.h>

#include "HeadersParser.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "cfg.h"

#ifndef __AVR__
#include <functional>
#endif

#if defined(ESP8266) || defined(ESP32)
#include <FS.h>
#endif

#define HS_HEADER_BUF_SIZE 150  // буфер одной строки хэдера
#define HS_BLOCK_SIZE 256       // размер блока выгрузки из файла и PROGMEM
#define HS_FLUSH_BLOCK 64       // блок очистки
#define HS_CACHE_PRD "604800"   // период кеширования

namespace ghttp {

class ServerBase {
   private:
    class Response : public Printable {
       public:
        Response(size_t code) {
            s.reserve(200);
            s += F("HTTP/1.1 ");
            s += code;
            if (code == 200) s += (code == 200) ? F(" OK\r\n") : F(" ERROR\r\n");
        }

        size_t printTo(Print& p) const {
            return p.println(s);
        }

        void cache(bool enabled) {
            if (enabled) s += F("Cache-Control: max-age=" HS_CACHE_PRD);
            else s += F(
                     "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                     "Pragma: no-cache\r\n"
                     "Expires: 0\r\n");
        }
        void type(const su::Text& t) {
            s += F("Content-Type: ");
            if (t) t.addString(s);
            else s += F("text/plain");
            clrf();
        }
        void gzip(bool enabled) {
            if (enabled) s += F("Content-Encoding: gzip\r\n");
        }
        void cors(bool use = true) {
            if (use) {
                s += F(
                    "Access-Control-Allow-Origin:*\r\n"
                    "Access-Control-Allow-Private-Network: true\r\n"
                    "Access-Control-Allow-Methods:*\r\n");
            }
        }
        void length(size_t len) {
            s += F("Content-Length: ");
            s += len;
            clrf();
        }
        void header(const su::Text& name, const su::Text& value) {
            name.addString(s);
            s += F(": ");
            value.addString(s);
            clrf();
        }

       private:
        String s;
        void clrf() {
            s += F("\r\n");
        }
    };

   public:
    class Request {
       public:
        Request(const su::Text& method, const su::Text& url, Stream* stream, size_t len) : _reader(stream, len), _method(method), _url(url) {
            _q = _url.indexOf('?');
        }

        // метод запроса
        const su::Text& method() const {
            return _method;
        }

        // полный урл
        const su::Text& url() const {
            return _url;
        }

        // путь (без параметров)
        su::Text path() const {
            return (_q > 0) ? _url.substring(0, _q) : su::Text();
        }

        // получить значение параметра по ключу
        su::Text param(const su::Text& key) const {
            if (_q < 0) return su::Text();

            su::Text params = _url.substring(_q + 1);
            int p = 0;

            while (1) {
                p = params.indexOf(key, p);
                if (p < 0) return su::Text();

                p += key.length();
                if (params[p] == '=') {
                    p++;
                    break;
                }
            }
            int end = params.indexOf('&', p);
            if (end < 0) end = params.length();
            return params.substring(p, end);
        }

        // получить тело запроса. Может выводиться в Print
        StreamReader& body() {
            return _reader;
        }

       private:
        StreamReader _reader;
        const su::Text _method;
        const su::Text _url;
        int16_t _q = -1;
    };

#ifdef __AVR__
    typedef void (*RequestCallback)(Request req);
#else
    typedef std::function<void(Request req)> RequestCallback;
#endif

    // ==================== SERVER ====================
   public:
    // подключить обработчик запроса
    void onRequest(RequestCallback callback) {
        _req_cb = callback;
    }

    // отправить клиенту. Можно вызывать несколько раз подряд
    void send(const su::Text& text, uint16_t code = 200, su::Text type = su::Text()) {
        if (!_clientp) return;

        _flush();
        if (!_handled) {
            _handled = true;
            Response resp(code);
            resp.type(type);
            resp.cors(_cors);
            _clientp->print(resp);
        }
        _clientp->print(text);
    }

    // отправить клиенту код. Должно быть единственным ответом
    void send(uint16_t code) {
        if (!_clientp) return;

        _flush();
        Response resp(code);
        resp.cors(_cors);
        _clientp->print(resp);
        _handled = true;
        _clientp = nullptr;
    }

#ifdef FS_H
    // отправить файл
    void sendFile(File& file, su::Text type = su::Text(), bool cache = false, bool gzip = false) {
        if (!_clientp) return;
        StreamWriter sender(file, file.size());
        _sendFile(sender, type, cache, gzip);
    }
#endif
    // отправить файл из буфера
    void sendFile(const uint8_t* buf, size_t len, su::Text type = su::Text(), bool cache = false, bool gzip = false) {
        if (!_clientp) return;
        StreamWriter sender(buf, len);
        _sendFile(sender, type, cache, gzip);
    }

    // отправить файл из PROGMEM
    void sendFile_P(const uint8_t* buf, size_t len, su::Text type = su::Text(), bool cache = false, bool gzip = false) {
        if (!_clientp) return;
        StreamWriter sender(buf, len, true);
        _sendFile(sender, type, cache, gzip);
    }

    // пометить запрос как выполненный
    void handle() {
        _handled = true;
    }

    // использовать CORS хэдеры (умолч. включено)
    void useCors(bool use) {
        _cors = use;
    }

    // получить mime тип файла по его пути
    const __FlashStringHelper* getMime(const su::Text& path) {
        int16_t pos = path.lastIndexOf('.');
        if (pos > 0) {
            switch (path.substring(pos + 1).hash()) {
                case su::SH("avi"):
                    return F("video/x-msvideo");
                case su::SH("bin"):
                    return F("application/octet-stream");
                case su::SH("bmp"):
                    return F("image/bmp");
                case su::SH("css"):
                    return F("text/css");
                case su::SH("csv"):
                    return F("text/csv");
                case su::SH("gz"):
                    return F("application/gzip");
                case su::SH("gif"):
                    return F("image/gif");
                case su::SH("html"):
                    return F("text/html");
                case su::SH("jpeg"):
                case su::SH("jpg"):
                    return F("image/jpeg");
                case su::SH("js"):
                    return F("text/javascript");
                case su::SH("json"):
                    return F("application/json");
                case su::SH("png"):
                    return F("image/png");
                case su::SH("svg"):
                    return F("image/svg+xml");
                case su::SH("wav"):
                    return F("audio/wav");
                case su::SH("xml"):
                    return F("application/xml");
            }
        }
        return F("text/plain");
    }

    // обработать запрос
    void handleRequest(::Client& client, HeadersCollector* collector = nullptr) {
        String lineStr = client.readStringUntil('\n');
        su::Text lineTxt(lineStr);
        su::Text lines[3];
        size_t n = lineTxt.split(lines, 3, ' ');
        if (n != 3) return;

        HeadersParser headers(client, HS_HEADER_BUF_SIZE, collector);

        if (!headers || !_req_cb) return send(400);

        _clientp = &client;
        _handled = false;

        if (headers.contentType.startsWith(F("multipart")) && headers.length) {
            bool eol = false;
            size_t boundlen = 0;
            while (client.connected()) {
                GHTTP_ESP_YIELD();
                String s = client.readStringUntil('\n');
                if (!s.length() || s[s.length() - 1] != '\r') break;

                if (!boundlen) boundlen = s.length();
                headers.length -= s.length() + 1;  // + \n
                if (s.length() == 1) {
                    eol = 1;
                    break;
                }
            }
            if (eol && headers.length >= boundlen + 2 + 3) {
                _req_cb(Request(lines[0], lines[1], &client, headers.length - (boundlen + 2 + 3)));  // \r\n + --
            }
            _flush();
        } else {
            _req_cb(Request(lines[0], lines[1], &client, headers.length));
        }

        if (!_handled) send(500);
        _clientp = nullptr;
    }

   private:
    RequestCallback _req_cb = nullptr;
    ::Client* _clientp = nullptr;
    bool _handled = false;
    bool _cors = true;

    void _sendFile(StreamWriter& sender, const su::Text& type, bool cache, bool gzip) {
        _flush();
        sender.setBlockSize(HS_BLOCK_SIZE);
        Response resp(200);
        resp.length(sender.length());
        resp.type(type);
        resp.cache(cache);
        resp.gzip(gzip);
        resp.cors(_cors);
        _clientp->print(resp);

        _clientp->print(sender);
        _handled = true;
        _clientp = nullptr;
    }
    void _flush() {
        uint8_t bytes[HS_FLUSH_BLOCK];
        while (_clientp && _clientp->available()) {
            delay(1);
            GHTTP_ESP_YIELD();
            _clientp->readBytes(bytes, min(_clientp->available(), HS_FLUSH_BLOCK));
        }
    }
};

}  // namespace ghttp