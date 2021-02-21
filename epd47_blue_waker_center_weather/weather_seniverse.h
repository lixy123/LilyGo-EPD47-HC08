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

    //缓存天气字符串数据，正常约500字节左右
    uint8_t buff[1024];
    StaticJsonDocument<1024> root;  //放在此处，减少函数申请内存异常的机率。
  public:
    GetWeather();
    ~GetWeather();
    //用这个
    int getnow_weather_wifihttp(cityWeather* objcityWeather);
    //用这函数访问会返回-1, 有可能是与开启蓝牙有关，有可能是因为访问对象是https类型, 不稳定，不好用
    //http_client对象访问https不好用
     //int getnow_weather(cityWeather* objcityWeather);
};
