#include "page.h"

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  String processor(const String& var) const{
    if (var == "IP"){
      String ipString = WiFi.softAPIP().toString(); // Create a local copy
      return ipString;
    }
    return String();
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", page, [this](const String& var) {
        return this->processor(var);
    });

  }
};