#pragma once
#include <Arduino.h>

#include "utils/cfg.h"

#if defined(ESP8266) || defined(ESP32)
#include <FS.h>
#endif

// ==================== SENDER ====================
class StreamSender : public Printable {
   public:
    StreamSender() {}

#ifdef FS_H
    StreamSender(File& file) : _file(&file), _len(file.size()) {}
#endif

    StreamSender(const uint8_t* buf, size_t len, bool pgm = 0) : _buf(buf), _len(len), _pgm(pgm) {}

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

#ifdef FS_H
        if (_file) {
            if (!*_file) return 0;
            uint8_t* buf = new uint8_t[min(_bsize, _len)];
            if (!buf) return 0;
            while (left) {
                size_t len = min(_bsize, left);
                size_t read = _file->read(buf, len);
                GHTTP_ESP_YIELD();
                printed += p.write(buf, read);
                left -= len;
            }
            delete[] buf;
            return printed;
        }
#endif
        if (_buf) {
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

                return printed;
            } else {
                // const uint8_t* bytes = _buf;
                // while (left) {
                //     size_t len = min(_bsize, left);
                //     printed += p.write(bytes, len);
                //     bytes += len;
                //     left -= len;
                // }
                // return printed;

                return p.write(_buf, _len);
            }
        }
        return 0;
    }

   protected:
#ifdef FS_H
    File* _file = nullptr;
#endif
    const uint8_t* _buf = nullptr;
    size_t _len = 0;
    bool _pgm = 0;

   private:
    size_t _bsize = 128;
};