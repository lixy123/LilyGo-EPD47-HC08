#include "HTTPClient.h"

//https://arduinojson.org/v6/doc/deserialization/?utm_source=github&utm_medium=readme
#include <ArduinoJson.h>
#include "config.h"

class cityWeather
{
  public:
    String date_now;
    String weather;
    float low_temperature;
    float high_temperature;
    float rainfall;
    String wind_direction;
    float wind_speed;
    float humidity;
    String last_update;
    String toString();
};

class GetWeather
{
  private:
    const char *http_host = "api.seniverse.com";
    const char *host = "https://api.seniverse.com"; //心知天气APIhost
    const char *city = "beijing";                   //查询的城市
    String req_url = "";
    HTTPClient http_client;
    WiFiClient client;

    //缓存天气数据，正常约500字节左右
    uint8_t buff[1024];

  public:
    GetWeather();
    ~GetWeather();
    //开蓝牙时可以用wificlient对象
    int getnow_weather_wifihttp(cityWeather* objcityWeather);
    //开蓝牙时不可用httpclient对象，不要用此函数！
    int getnow_weather(cityWeather* objcityWeather);
};
