#pragma once
#include <Arduino.h>
#include <Client.h>

#ifndef __AVR__
#include <functional>
#endif

#include "HeadersParser.h"
#include "StreamReader.h"

#define HC_DEF_TIMEOUT 2000     // таймаут по умолчанию
#define HC_HEADER_BUF_SIZE 150  // буфер одной строки хэдера
#define HC_FLUSH_BLOCK 64       // блок очистки

#define HC_USE_LOG Serial

#ifdef HC_USE_LOG
#define HC_LOG(x) HC_USE_LOG.println(x)
#else
#define HC_LOG(x)
#endif

namespace ghttp {

class Client : public Print {
   public:
    class Response {
       public:
        Response() {}
        Response(const String& type, Stream* stream = nullptr, size_t len = 0) : _type(type), _reader(stream, len) {}

        // тип контента
        su::Text type() const {
            return _type;
        }

        // тело ответа
        StreamReader& body() {
            return _reader;
        }

        // ответ существует
        operator bool() {
            return _reader;
        }

       private:
        String _type;
        StreamReader _reader;
    };

   private:
#ifdef __AVR__
    typedef void (*ResponseCallback)(Response& resp);
#else
    typedef std::function<void(Response& resp)> ResponseCallback;
#endif

   public:
    Client(::Client& client, const char* host, uint16_t port) : client(client), _host(host), _port(port) {
        setTimeout(HC_DEF_TIMEOUT);
    }
    Client(::Client& client, const IPAddress& ip, uint16_t port) : client(client), _host(nullptr), _ip(ip), _port(port) {
        setTimeout(HC_DEF_TIMEOUT);
    }

    size_t write(uint8_t data) {
        if (!client.connected()) {
            _init();
            return 0;
        }
        _waiting = 1;
        _lastSend = millis();
        return client.write(data);
    }
    size_t write(const uint8_t* buffer, size_t size) {
        if (!client.connected()) {
            _init();
            return 0;
        }
        size_t w = client.write(buffer, size);
        client.flush();
        _waiting = 1;
        _lastSend = millis();
        return w;
    }

    // ==========================

    // установить новый хост и порт
    void setHost(const char* host, uint16_t port) {
        stop();
        _host = host;
        _port = port;
    }

    // установить новый хост и порт
    void setHost(const IPAddress& ip, uint16_t port) {
        setHost(nullptr, port);
        _ip = ip;
    }

    // установить новый клиент для связи
    void setClient(::Client& client) {
        stop();
        this->client = client;
        client.setTimeout(_timeout);
    }

    // установить таймаут ответа сервера, умолч. 2000 мс
    void setTimeout(uint16_t tout) {
        client.setTimeout(tout);
        _timeout = tout;
    }

    // обработчик ответов, требует вызова tick() в loop()
    void onResponse(ResponseCallback cb) {
        _resp_cb = cb;
    }

    // ==========================

    // подключиться
    bool connect() {
        if (!client.connected()) {
            _host ? client.connect(_host, _port) : client.connect(_ip, _port);
        }
        return client.connected();
    }

    // отправить запрос
    bool request(const su::Text& path, const su::Text& method, const su::Text& headers, const su::Text& payload) {
        return request(path, method, headers, (uint8_t*)payload.str(), payload.length());
    }

    // отправить запрос
    bool request(const su::Text& path, const su::Text& method = "GET", const su::Text& headers = su::Text(), const uint8_t* payload = nullptr, size_t length = 0) {
        if (!beginSend()) return 0;

        String req;
        req.reserve(50 + path.length() + headers.length());
        method.addString(req);
        req += ' ';
        path.addString(req);
        req += F(" HTTP/1.1\r\nHost: ");
        if (_host) req += _host;
        else req += _ip.toString();
        req += F("\r\n");
        headers.addString(req);
        if (payload && length) {
            req += F("Content-Length: ");
            req += length;
            req += F("\r\n");
        }
        req += F("\r\n");
        print(req);
        if (payload && length) write(payload, length);
        return 1;
    }

    // начать отправку. Дальше нужно вручную print
    bool beginSend() {
        flush();
        return connect();
    }

    // клиент ждёт ответа
    bool isWaiting() {
        if (!client.connected()) {
            _init();
            return 0;
        }
        return _waiting;
    }

    // есть ответ от сервера (асинхронно)
    bool available() {
        return (isWaiting() && client.available());
    }

    // дождаться и прочитать ответ сервера (по available если long poll)
    Response getResponse(HeadersCollector* collector = nullptr) {
        if (!isWaiting()) return Response();

        if (!_wait()) {
            flush();
            return Response();
        }

        HeadersParser headers(client, HC_HEADER_BUF_SIZE, collector);

        if (headers) {
            _close = headers.close;
            _waiting = 0;
            return Response(headers.contentType, &client, headers.length);
        } else {
            flush();
            return Response();
        }
    }

    // тикер, вызывать в loop для работы с коллбэком
    void tick() {
        if (available() && _resp_cb) {
            Response resp = getResponse();
            if (resp) _resp_cb(resp);
        }
    }

    // остановить клиента
    void stop() {
        HC_LOG("client stop");
        client.stop();
        _init();
    }

    // пропустить ответ, снять флаг ожидания, остановить если connection close
    void flush() {
        if (client.connected()) {
            _wait();
            uint8_t bytes[HC_FLUSH_BLOCK];
            while (client.available()) {
                yield();
                client.readBytes(bytes, min(client.available(), HC_FLUSH_BLOCK));
            }
            if (_close) {
                HC_LOG("connection close");
                client.stop();
            }
        }
        _init();
    }

    ::Client& client;

   private:
    ResponseCallback _resp_cb = nullptr;
    const char* _host = nullptr;
    IPAddress _ip;
    uint16_t _port;
    uint16_t _timeout;
    uint32_t _lastSend;
    bool _close = 0;
    bool _waiting = 0;

    void _init() {
        _close = 0;
        _waiting = 0;
    }
    bool _wait() {
        if (!_waiting) return 0;
        while (!client.available()) {
            delay(1);
            if (millis() - _lastSend >= _timeout) {
                HC_LOG("client timeout");
                stop();
                return 0;
            }
            if (!client.connected()) {
                HC_LOG("client disconnected");
                return 0;
            }
        }
        return 1;
    }
};

}  // namespace ghttp