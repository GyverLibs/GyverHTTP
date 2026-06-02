This is an automatic translation and may be incorrect in some places. See the source README and examples for authoritative information.

[![latest](https://img.shields.io/github/v/release/GyverLibs/GyverHTTP.svg?color=brightgreen)](https://github.com/GyverLibs/GyverHTTP/releases/latest/download/GyverHTTP.zip)
[![PIO](https://badges.registry.platformio.org/packages/gyverlibs/library/GyverHTTP.svg)](https://registry.platformio.org/libraries/gyverlibs/GyverHTTP)
[![Foo](https://img.shields.io/badge/Website-AlexGyver.ru-blue.svg?style=flat-square)](https://alexgyver.ru/)
[![Foo](https://img.shields.io/badge/%E2%82%BD%24%E2%82%AC%20%D0%9F%D0%BE%D0%B4%D0%B4%D0%B5%D1%80%D0%B6%D0%B0%D1%82%D1%8C-%D0%B0%D0%B2%D1%82%D0%BE%D1%80%D0%B0-orange.svg?style=flat-square)](https://alexgyver.ru/support_alex/)
[![Foo](https://img.shields.io/badge/README-ENGLISH-blueviolet.svg?style=flat-square)](https://github-com.translate.goog/GyverLibs/GyverHTTP?_x_tr_sl=ru&_x_tr_tl=en)  

[![Foo](https://img.shields.io/badge/ПОДПИСАТЬСЯ-НА%20ОБНОВЛЕНИЯ-brightgreen.svg?style=social&logo=telegram&color=blue)](https://t.me/GyverLibs)

# GyverHTTP
Very simple and lightweight HTTP server and semi-asynchronous HTTP client
- Fast sending and receiving files
- Convenient minimalist API

### Compatibility
Compatible with all Arduino platforms (Arduino features are used)

## Contents
- [Use of use](#usage)
- [Versions](#versions)
- [Installation](#install)
- [Bugs and feedback](#feedback)

<a id="usage"></a>

## Use of use
### StreamWriter
Fast data sender in Print, supports work with files and PROGMEM. Reads to the buffer and sends in blocks, which is many times faster than normal sending.
```cpp
StreamWriter(Stream* stream, size_t size);
StreamWriter(const uint8_t* buf, size_t len, bool pgm = 0);

// size
size_t length();

// set-up
void setBlockSize(size_t bsize);

// print
size_t printTo(Print& p);
```

### StreamReader
A quick reader of data from a stream of known length. Buffers and writes in blocks to the consumer, which is many times faster than normal reading.
```cpp
StreamReader(Stream* stream = nullptr, size_t len = 0);

// line up
String readString();

// time-out
void setTimeout(size_t tout);

// block size
void setBlockSize(size_t bsize);

// Read in the buffer, will return true on success
bool readBytes(uint8_t* buf);

// write(uint8 t*, size t)
template <typename T>
bool writeTo(T& p);

// totality
size_t length();

// readership
operator bool();

Stream* stream;
```

### Client
```cpp
size_t write(uint8_t data);
size_t write(const uint8_t* buffer, size_t size);

// ==========================

// install a new host and port
void setHost(const char* host, uint16_t port);

// install a new host and port
void setHost(const IPAddress& ip, uint16_t port);

// Establish a new client for communication
void setClient(::Client& client);

// set the server response timeout, shut up. 2000 ms.
void setTimeout(uint16_t tout);

// The answer processor requires a tick() call to loop()
void onResponse(ResponseCallback cb);

// ==========================

// plug in
bool connect();

// request
bool request(Text path, Text method, Text headers, FormData& data);
bool request(Text path, Text method, Text headers, Text payload);
bool request(Text path, Text method = "GET", Text headers = Text(), const uint8_t* payload = nullptr, size_t length = 0);

// start shipping. Next, you need to print manually.
bool beginSend();

// client
bool isWaiting();

// There is a response from the server (asynchronous)
bool available();

// wait and read the server response (on available if long poll)
Response getResponse(HeadersCollector* collector = nullptr);

// ticker, call in the loop to work with callback
void tick();

// stop
void stop();

// skip the answer, remove the waiting flag if connection close
void flush();
```

### Client::Response
```cpp
// Content type (from Content Type Header)
Text type();

// code
uint16_t code();

// Response Body (length from Content-Length Header)
StreamReader& body();

// answer
operator bool();
```

### Client::FormData
// Buildinger form data
```cpp
void add(Text name, Text filename, Text type, Text data);
```

### Client::Headers
// Header builder
```cpp
void add(Text name, Text value);
```

### Server
```cpp
Server(uint16_t port);

// launch
void begin();

// loop
void tick(HeadersCollector* collector = nullptr);

// plug in
void onRequest(RequestCallback callback);

// start answering. In Headers you can specify custom headers
void beginResponse(Headers& resp);

// begin
void beginResponse(uint16_t code = 200);

// client access
Client* client();

// Send the code to the client. Must be the only answer.
void send(uint16_t code);

// Send to the client and end the session. Should be the only answer, use without startResponse
void sendSingle(const Text& text, uint16_t code = 200, Text type = Text());

// Send it to the client. You can call several times in a row
void print(Printable& p);
void send(Text text);
void send(Text text, uint16_t code, Text type = Text());

// file
void sendFile(File& file, Text type = Text(), bool cache = false, bool gzip = false);

// Send the file line as text
void sendFile(const Text& text, Text type = Text(), bool cache = false);

// buffer
void sendFile(const uint8_t* buf, size_t len, Text type = Text(), bool cache = false, bool gzip = false);

// send a file from PROGMEM
void sendFile_P(const uint8_t* buf, size_t len, Text type = Text(), bool cache = false, bool gzip = false);

// send the file line from PROGMEM
void sendFile_P(const char* pstr, Text type = Text(), bool cache = false);

// mark the request as completed
void handle();

// use CORS headers (silent included)
void useCors(bool use);

// Get a mime file type along the way
const __FlashStringHelper* getMime(Text path);
```

### ServerBase::Request
```cpp
// method
Text method();

// urlful
Text url();

// path (without parameters)
Text path();

// get the key value
// parameter without value will return a valid empty string
Text param(Text key);

// Get the request body. Could be in Print.
StreamReader& body();
```

### ServerBase::Headers
```cpp
// start with the answer code
Headers(uint16_t code);

// header
void add(Text name, Text value);
```

### ghttp::HeadersCollector
The interface for manual handling of headers. Used as follows:

We create our class based on it. For example, let him bring the series.
```cpp
class Collector : public ghttp::HeadersCollector {
   public:
    void header(Text& name, Text& value) {
        Serial.print(name);
        Serial.print(": ");
        Serial.println(value);
    }
};
```

Header interception in the case of HTTP client
```cpp
if (http.available()) {
    Collector collector;
    ghttp::Client::Response resp = http.getResponse(&collector);
    // At this point, we have broken heads.
    if (resp) ...
}
```

Header interception in the case of HTTP server
```cpp
Collector collector;

void onrequest(...) {
}

void loop() {
    // Headers will be processed before calling callback
    server.tick(&collector);
}
```

<a id="versions"></a>

## Versions
- v1.0
- 1.0.8 - Improvements and additions

<a id="install"></a>
## Installation
- The library can be found under the name **GyverHTTP** and installed through the library manager in:
    - Arduino IDE
    - Arduino IDE v2
    - PlatformIO
- [Download the library](https://github.com/GyverLibs/GyverHTTP/archive/refs/heads/main.zip).zip archive for manual installation:
    - Unpack and put in *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Unpack and put in *C:\Program Files\Arduino\libraries* (Windows x32)
    - Unpack and put in *Documents/Arduino/libraries/ *
    - (Arduino IDE) Automatic installation from .zip: *Sketch/Connect library/Add .ZIP library...* and specify downloaded archive
- Read more detailed instructions for installing libraries[here](https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)
### Update
- I recommend always updating the library: new versions fix errors and bugs, as well as optimize and add new features.
- Through the library manager IDE: find the library as when installing and click "Update"
- Manually: **Delete the folder with the old version** and then put the new one in its place. “Replacement” can not be done: sometimes new versions delete files that will remain when replaced and can lead to errors!

<a id="feedback"></a>

## Bugs and feedback
If you find bugs, create **Issue**, or better write to the mail immediately.[alex@alexgyver.ru](mailto:alex@alexgyver.ru)  
The library is open for revision and your **Pull Requests*!

When reporting bugs or incorrect work of the library, it is necessary to specify:
- Library version
- What is used by the IC
- SDK version (for ESP)
- Arduino IDE version
- Are embedded examples that use features and designs that cause bugs in your code working correctly?
- What code was downloaded, what work was expected from it and how it works in reality
- Ideally, attach the minimum code in which the bug is observed. Not a canvas of a thousand lines, but a minimum code.
