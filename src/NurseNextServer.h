#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ESPAsyncWebServer.h"
#include <DNSServer.h>
#include "CaptiveRequestHandler.h"

struct InputInfo
{
    char type;
    int index;
    int value;
};

enum class ParseStatus
{
    Success,
    FormatError,
    ValueError
};

ParseStatus parseInputData(const char *data, InputInfo &info)
{
    char type;
    int index, value;

    if (sscanf(data, "%c%d.%d", &type, &index, &value) != 3)
    {
        return ParseStatus::FormatError;
    }

    if (!(type == 'B' || type == 'A') || index < 0 || value < 0)
    {
        return ParseStatus::ValueError;
    }

    info.type = type;
    info.index = index;
    info.value = value;

    return ParseStatus::Success;
}

class GamepadServer
{
private:
    char *name;
    int port;
    AsyncWebServer server;
    AsyncWebSocket socket;
    IPAddress apIP;
    DNSServer dnsServer;
    const byte DNS_PORT = 53;

    // Callback functions for WebSocket events
    std::function<void(AsyncWebSocketClient *)> onConnectCallback;
    std::function<void(AsyncWebSocketClient *)> onDisconnectCallback;
    std::function<void(InputInfo, ParseStatus)> onGamepadEvent;

public:
    GamepadServer(char *name, int port) : server(port), socket("/ws"), apIP(8, 8, 4, 4)
    {
        this->name = name;
        this->port = port;
    }

    void init()
    {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(this->name);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

        // if DNSServer is started with "*" for domain name, it will reply with
        // provided IP to all DNS request
        dnsServer.start(DNS_PORT, "*", apIP);

        socket.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
                       { onEvent(server, client, type, arg, data, len); });
        server.addHandler(&socket);
        server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
        server.begin();
    }

    // New method to set the onConnect callback
    void setOnConnectCallback(std::function<void(AsyncWebSocketClient *)> callback)
    {
        onConnectCallback = callback;
    }

    // New method to set the onDisconnect callback
    void setOnDisconnectCallback(std::function<void(AsyncWebSocketClient *)> callback)
    {
        onDisconnectCallback = callback;
    }

    // New method to set the onMessage callback
    void setonGamepadEventCallback(std::function<void(InputInfo, ParseStatus)> callback)
    {
        onGamepadEvent = callback;
    }

    void handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
    {
        AwsFrameInfo *ws_info = (AwsFrameInfo *)arg;
        if (ws_info->final && ws_info->index == 0 && ws_info->len == len && ws_info->opcode == WS_TEXT)
        {
            data[len] = 0;
            Serial.println((char *)data);

            // Parse message
            InputInfo info;
            ParseStatus status = parseInputData((char *)data, info);

            if (onGamepadEvent)
            {
                onGamepadEvent(info, status);
            }
        }
    }

    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
        switch (type)
        {
        case WS_EVT_CONNECT:
            if (onConnectCallback)
            {
                onConnectCallback(client);
            }
            break;
        case WS_EVT_DISCONNECT:
            if (onDisconnectCallback)
            {
                onDisconnectCallback(client);
            }
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(client, arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        }
    }

    void handle()
    {
        socket.cleanupClients();
        dnsServer.processNextRequest();
    }
};
