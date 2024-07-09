#pragma once
#include <Arduino.h>
#include <StringUtils.h>

#include "utils/cfg.h"

#define GHTTP_LENSTR_LEN 10

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

    // вывести в write(uint8_t*, size_t). Вернёт количество записанных или 0 при ошибке
    template <typename T>
    size_t writeTo(T& p) const {
        return _chunked ? _writeTo(p) : (_writeTo(p) == _len ? _len : 0);
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
        uint8_t* buf = new uint8_t[_chunked ? _bsize : min(_bsize, _len)];
        if (!buf) return 0;

        size_t n = 0;
        if (_chunked) {
            char lenstr[GHTTP_LENSTR_LEN];
            while (1) {
                GHTTP_ESP_YIELD();

                bool last = 0;
                size_t len = stream->readBytesUntil('\n', lenstr, GHTTP_LENSTR_LEN);
                if (!len || lenstr[len - 1] != '\r') {
                    n = 0;
                    break;
                }

                len = su::strToIntHex(lenstr, len - 1);
                if (len) {
                    size_t w = _writeBuffered(len, buf, p);
                    if (w != len) {
                        n = 0;
                        break;
                    }
                    n += w;
                } else {
                    last = 1;
                }

                len = stream->readBytesUntil('\n', lenstr, GHTTP_LENSTR_LEN);
                if (len != 1 || lenstr[0] != '\r') {
                    n = 0;
                    break;
                }

                if (last) break;
            }

        } else {
            if (_len) n = _writeBuffered(_len, buf, p);
        }

        delete[] buf;
        return n;
    }

    template <typename T>
    size_t _writeBuffered(size_t len, uint8_t* buffer, T& p) const {
        size_t left = len;
        while (left) {
            GHTTP_ESP_YIELD();
            if (!_waitStream()) break;

            size_t block = min(min(left, (size_t)stream->available()), _bsize);
            size_t read = stream->readBytes(buffer, block);
            GHTTP_ESP_YIELD();

            if (read != block) break;
            if (block != p.write(buffer, block)) break;
            left -= block;
        }
        return len - left;
    }

    bool _waitStream() const {
        if (!stream->available()) {
            int ms = _tout;
            while (!stream->available()) {
                delay(1);
                if (!--ms) return 0;
            }
        }
        return 1;
    }
};