
//https://arduinojson.org/v6/doc/deserialization/?utm_source=github&utm_medium=readme
#include <ArduinoJson.h>
#include "config.h"


class Weather_multidayManager
{
  private:

  public:
    String req_url = "";
    String req_host = "http://api.jisuapi.com/"; //心知天气APIhost
    String resp_new;  //精简后的json,便于蓝
    String resp;      //原始的json
    Weather_multidayManager();
    ~Weather_multidayManager();
    int getnow_weather_7020(String weather_data);
};
