#pragma once
#include <Arduino.h>

#include "utils/cfg.h"

// ==================== SENDER ====================
class StreamWriter : public Printable {
   public:
    StreamWriter() {}
    StreamWriter(Stream& stream, size_t len) : _stream(&stream), _len(len) {}
    StreamWriter(const uint8_t* buf, size_t len, bool pgm = 0) : _buf(buf), _len(len), _pgm(pgm) {}

    // размер данных
    size_t length() const {
        return _len;
    }

    // установить размер блока отправки
    void setBlockSize(size_t bsize) {
        _bsize = bsize;
    }

    // напечатать в принт
    size_t printTo(Print& p) const {
        if (!_len) return 0;

        size_t left = _len;
        size_t printed = 0;

        if (_stream) {
            if (!_stream->available()) return 0;
            uint8_t* buf = new uint8_t[min(_bsize, _len)];
            if (!buf) return 0;
           
            while (left) {
                size_t len = min(min(left, (size_t)_stream->available()), _bsize)
                size_t read = _stream->read(buf, len);
                GHTTP_ESP_YIELD();
                printed += p.write(buf, read);
                if (len != read) break;
                left -= len;
            }
            delete[] buf;
           
        } else if (_buf) {
            if (_pgm) {
                const uint8_t* bytes = _buf;
                uint8_t* buf = new uint8_t[min(_bsize, _len)];
                if (!buf) return 0;

                while (left) {
                    GHTTP_ESP_YIELD();
                    size_t len = min(_bsize, left);
                    memcpy_P(buf, bytes, len);
                    printed += p.write(buf, len);
                    bytes += len;
                    left -= len;
                }
                delete[] buf;
               
            } else {
                printed = p.write(_buf, _len);
            }
        }
        return printed;
    }

   protected:
    Stream* _stream = nullptr;
    const uint8_t* _buf = nullptr;
    size_t _len = 0;
    bool _pgm = 0;

   private:
    size_t _bsize = 128;
};
