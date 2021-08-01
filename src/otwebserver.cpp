#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "otwebserver.h"

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
</head>
<body>
<H1>Just Testing</H2>
</body>
</html>
)rawliteral";

String processor(const String &)
{
    return "";
}

void wssetup()
{
    // Handle Web Server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    // Handle Web Server Events

    // events.onConnect([](AsyncEventSourceClient *client) {
    //  if (client->lastId())
    //  {
    //    serr.println("Client reconnected");
    //  }
    //});
    // server.addHandler(&events);
    server.begin();
}