#include "HTTPClient.h"

//https://arduinojson.org/v6/doc/deserialization/?utm_source=github&utm_medium=readme
#include <ArduinoJson.h>
#include "config.h"


class Weather_multidayManager
{
  private:
    const char *http_host = "api.jisuapi.com";
    const char *host = "http://api.jisuapi.com"; //jisuapi
    const char *city = "1";                   //查询的城市
    String req_url = "";
    HTTPClient http_client;
    WiFiClient client;

  public:
    String resp_new;  //精简后的json,便于蓝
    String resp;      //原始的json 
    Weather_multidayManager();
    ~Weather_multidayManager();

    int getnow_weather();
};
