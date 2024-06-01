#pragma once
#include <Arduino.h>

// ==================== READER ====================
class StreamReader : public Printable {
   public:
    StreamReader(Stream* stream = nullptr, size_t len = 0) : stream(stream), _len(len) {}

    // установить таймаут
    void setTimeout(size_t tout) {
        _tout = tout;
    }

    // установить размер блока
    void setBlockSize(size_t bsize) {
        _bsize = bsize;
    }

    // прочитать в буфер, вернёт true при успехе
    bool readBytes(uint8_t* buf) const {
        return (stream && buf) ? (stream->readBytes(buf, _len) == _len) : 0;
    }

    // вывести в write(uint8_t*, size_t)
    template <typename T>
    bool writeTo(T& p) const {
        return _writeTo(p) == _len;
    }

    size_t printTo(Print& p) const {
        return _writeTo(p);
    }

    // общий размер входящих данных
    size_t length() const {
        return stream ? _len : 0;
    }

    // корреткность ридера
    operator bool() const {
        return length();
    }

    Stream* stream = nullptr;

   private:
    size_t _len;
    size_t _bsize = 128;
    size_t _tout = 2000;

    template <typename T>
    size_t _writeTo(T& p) const {
        if (!_len || !stream) return 0;

        size_t left = _len;
        uint8_t* buf = new uint8_t[min(_bsize, _len)];
        if (!buf) goto terminate;

        while (left) {
            delay(1);
            if (!stream->available()) {
                uint32_t ms = millis();
                while (!stream->available()) {
                    yield();
                    if (millis() - ms >= _tout) goto terminate;
                }
            }
            size_t len = min(min(left, (size_t)stream->available()), _bsize);
            size_t read = stream->readBytes(buf, len);

            delay(1);
            if (read != len) break;
            if (len != p.write(buf, len)) break;
            left -= len;
        }

    terminate:
        delete[] buf;
        return _len - left;
    }
};