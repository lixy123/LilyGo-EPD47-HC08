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
    //天气字符串数据，正常约500字节左右
    StaticJsonDocument<1028> root;  //放在此处，减少函数申请内存异常的机率。
  public:
    String req_host = "https://api.seniverse.com/"; //心知天气APIhost
    String req_url = "";
    GetWeather();
    ~GetWeather();

    int getnow_weather_7020(String weather_data, cityWeather* objcityWeather);
};
