#include <Arduino.h>
#include <GamepadServer.h>

GamepadServer server("GamePadServer", 80);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  server.init();
  pinMode(22, OUTPUT);

  server.setOnConnectCallback([](AsyncWebSocketClient *client)
  {
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
  });

  server.setOnDisconnectCallback([](AsyncWebSocketClient *client)
  {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  });

  server.setonGamepadEventCallback([](InputInfo info, ParseStatus status)
  {
    if (status == ParseStatus::Success) {
        Serial.println("Type: " + String(info.type));
        Serial.println("Index: " + String(info.index));
        Serial.println("Value: " + String(info.value));
        if (info.type == 'B' && info.index == 0){
          digitalWrite(22, info.value/2000);
        }
    } else if (status == ParseStatus::FormatError) {
        Serial.println("Format error in input data.");
    } else if (status == ParseStatus::ValueError) {
        Serial.println("Invalid values in input data.");
    }
  });
}

void loop()
{
  // put your main code here, to run repeatedly:
  server.handle();
}