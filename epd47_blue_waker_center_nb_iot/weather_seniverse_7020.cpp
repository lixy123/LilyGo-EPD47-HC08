#include "weather_seniverse_7020.h"


//{"results":[{"location":{"id":"WX4FBXXFKE4F","name":"北京","country":"CN","path":"北京,北京,中国","timezone":"Asia/Shanghai","timezone_offset":"+08:00"},
//"daily":[{"date":"2021-01-18","text_day":"晴","code_day":"0","text_night":"阴","code_night":"9","high":"4","low":"-6","rainfall":"0.0","precip":"","wind_direction":"东南","wind_direction_degree":"135","wind_speed":"8.4","wind_scale":"2","humidity":"27"}],"last_update":"2021-01-18T11:00:00+08:00"}]}

String cityWeather::toString() {
  String weather_info = weather + " 温度:" + String(int(low_temperature)) + "至" +  String(int(high_temperature)) + "度 " + wind_direction + "风" +
                        String(int(wind_speed)) + "级";
  return weather_info;
}

GetWeather::GetWeather() {
  req_url = String("/v3/weather/daily.json?key=");
  req_url += seniverse_key;
  req_url += "&location=";
  req_url += seniverse_city;
  req_url += "&language=zh-Hans&unit=c&start=0&days=1";

}

GetWeather::~GetWeather() {

}


int GetWeather::getnow_weather_7020(String weather_data, cityWeather* objcityWeather)
{
  int http_code=-1;
  if (weather_data.length() > 0)
  {
    //Serial.println("> " + findresult);
    //StaticJsonBuffer<1024> jsonBuffer;
    //JsonObject& root = jsonBuffer.parseObject(findresult);

    //StaticJsonDocument<1024> root;
    DeserializationError error = deserializeJson(root, weather_data);

    //F:\esp32\libraries\ArduinoJson\src\ArduinoJson\Deserialization\DeserializationError.hpp
    if (error) {
      Serial.println("Failed to parse string,error=" + String(error.c_str()));
      return http_code;
    }


    objcityWeather->weather = root["results"][0]["daily"][0]["text_day"].as<String>();

    objcityWeather->date_now = root["results"][0]["daily"][0]["date"].as<String>();
    objcityWeather->low_temperature = root["results"][0]["daily"][0]["low"].as<float>();
    objcityWeather->high_temperature = root["results"][0]["daily"][0]["high"].as<float>();
    objcityWeather->rainfall = root["results"][0]["daily"][0]["rainfall"].as<float>();
    objcityWeather->wind_direction = root["results"][0]["daily"][0]["wind_direction"].as<String>();
    objcityWeather->wind_speed = root["results"][0]["daily"][0]["wind_speed"].as<float>();

    objcityWeather->humidity =  root["results"][0]["daily"][0]["humidity"].as<float>();
    objcityWeather->last_update = root["results"][0]["last_update"].as<String>();

    http_code = 0;
    root.clear();
  }
  return (http_code);
}
