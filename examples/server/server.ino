#include <Arduino.h>
#include <GyverHTTP.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#define WIFI_SSID ""
#define WIFI_PASS ""

ghttp::Server<WiFiServer, WiFiClient> server(80);

const char html_p[] PROGMEM = R"raw(
<!DOCTYPE html>
<html lang="en">

<body>
    <button onclick="url()">url</button>
    <button onclick="json()">json</button>
    <input type="file">
    <button onclick="file()">send file</button>
</body>

<script>
    async function url() {
        const res = await fetch('/qs?kek=pek&lol=kek', {
            method: 'POST',
        });
    }
    async function json() {
        const res = await fetch('/json', {
            method: 'POST',
            body: JSON.stringify({ test: 123 }),
        });
    }
    async function file() {
        let input = document.querySelector('input[type="file"]');
        let data = new FormData();
        data.append('upload', input.files[0], 'upload')

        const res = await fetch('/upload', {
            method: 'POST',
            body: data,
        });
    }
</script>

</html>
)raw";

void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.println(WiFi.localIP());

    server.begin();

    server.onRequest([](ghttp::ServerBase::Request req) {
        // URL
        Serial.println(req.method());
        Serial.println(req.url());
        Serial.println(req.path());
        Serial.println(req.param("kek"));
        Serial.println(req.param("lol"));

        // BODY
        Serial.println(req.body());
        // req.body().writeTo(Serial);
        // req.body().writeTo(file);
        // req.body().stream.readBytes(buf, req.length());

        // RESPONSE
        if (req.url() == "/") {
            server.sendFile_P((uint8_t*)html_p, strlen_P(html_p), "text/html");

            // server.sendFile((uint8_t*)"hello text!", 11);
            // File f = LittleFS.open("lorem.txt", "r");
            // server.sendFile(f);
        }
        // server.send("hello ");
        // server.send("kek ");
        // server.send("pek");
    });
}

void loop() {
    server.tick();
}