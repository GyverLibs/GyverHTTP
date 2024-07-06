#pragma once
#include <Arduino.h>
#include <StringUtils.h>

#include "utils/cfg.h"

// ==================== READER ====================
class StreamReader : public Printable {
    class WritableBuffer {
       public:
        WritableBuffer(uint8_t* buffer) : buffer(buffer) {}

        size_t write(uint8_t* data, size_t len) {
            memcpy(buffer, data, len);
            buffer += len;
            return len;
        }

       private:
        uint8_t* buffer;
    };

    class WritableString {
       public:
        WritableString(String& s) : s(s) {}

        size_t write(uint8_t* data, size_t len) {
            s.concat((char*)data, len);
            return len;
        }

       private:
        String& s;
    };

   public:
    StreamReader(Stream* stream = nullptr, size_t len = 0, bool chunked = false) : stream(stream), _len(len), _chunked(chunked) {}

    // установить таймаут
    void setTimeout(size_t tout) {
        _tout = tout;
    }

    // установить размер блока
    void setBlockSize(size_t bsize) {
        _bsize = bsize;
    }

    // http chunked response
    bool isChunked() {
        return _chunked;
    }

    // прочитать в буфер, вернёт true при успехе
    bool readBytes(uint8_t* buf) const {
        WritableBuffer buffer(buf);
        return writeTo(buffer);
    }

    // прочитать в строку
    bool readString(String& s) {
        WritableString wr(s);
        return writeTo(wr);
    }

    // прочитать в строку
    String readString() {
        String s;
        readString(s);
        return s;
    }

    // прочитать байт
    uint8_t read() {
        if (available() && !_chunked) {
            int res = stream->read();
            if (res >= 0) {
                _len--;
                return res;
            } else {
                _len = 0;
            }
        }
        return 0;
    }

    // вывести в write(uint8_t*, size_t)
    template <typename T>
    bool writeTo(T& p) const {
        return _writeTo(p) == _len;
    }

    size_t printTo(Print& p) const {
        return _writeTo(p);
    }

    // оставшийся размер входящих данных
    size_t length() const {
        return stream ? _len : 0;
    }

    // оставшийся размер входящих данных
    size_t available() const {
        return length();
    }

    // корреткность ридера
    operator bool() const {
        return stream && (_len || _chunked);
    }

    Stream* stream = nullptr;

   private:
    size_t _len;
    size_t _bsize = 128;
    size_t _tout = 2000;
    bool _chunked = false;

    template <typename T>
    size_t _writeTo(T& p) const {
        if (!stream) return 0;

        if (_chunked) {
            size_t readAmount = 0;
            char lenstr[10];
            while (1) {
                delay(1);
                GHTTP_ESP_YIELD();

                size_t len = stream->readBytesUntil('\n', lenstr, 10);
                if (!len || lenstr[len - 1] != '\r') break;

                len = su::strToIntHex(lenstr, len - 1);
                if (!len) break;

                StreamReader reader(stream, len);
                size_t read = reader._writeTo(p);
                if (read != len) break;

                readAmount += read;
                len = stream->readBytesUntil('\n', lenstr, 10);
                if (len != 1 || lenstr[0] != '\r') break;
            }
            return readAmount;

        } else {
            if (!_len) return 0;

            size_t left = _len;
            uint8_t* buf = new uint8_t[min(_bsize, _len)];
            if (!buf) return 0;

            while (left) {
                delay(1);
                GHTTP_ESP_YIELD();
                if (!stream->available()) {
                    uint32_t ms = millis();
                    while (!stream->available()) {
                        delay(1);
                        GHTTP_ESP_YIELD();
                        if (millis() - ms >= _tout) goto terminate;
                    }
                }
                size_t len = min(min(left, (size_t)stream->available()), _bsize);
                size_t read = stream->readBytes(buf, len);
                GHTTP_ESP_YIELD();

                if (read != len) break;
                if (len != p.write(buf, len)) break;
                left -= len;
            }

        terminate:
            delete[] buf;
            return _len - left;
        }
    }
};