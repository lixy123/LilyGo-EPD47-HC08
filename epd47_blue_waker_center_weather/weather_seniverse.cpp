#include "weather_seniverse.h"



//{"results":[{"location":{"id":"WX4FBXXFKE4F","name":"北京","country":"CN","path":"北京,北京,中国","timezone":"Asia/Shanghai","timezone_offset":"+08:00"},
//"daily":[{"date":"2021-01-18","text_day":"晴","code_day":"0","text_night":"阴","code_night":"9","high":"4","low":"-6","rainfall":"0.0","precip":"","wind_direction":"东南","wind_direction_degree":"135","wind_speed":"8.4","wind_scale":"2","humidity":"27"}],"last_update":"2021-01-18T11:00:00+08:00"}]}

String cityWeather::toString() {
  String weather_info = weather + " 温度:" + String(int(low_temperature)) + "至" +  String(int(high_temperature)) + "度 " + wind_direction + "风" +
                        String(int(wind_speed)) + "级";
  return weather_info;
}

GetWeather::GetWeather() {
  req_url = (String)host + "/v3/weather/daily.json?key=";
  req_url += seniverse_key;
  req_url += "&location=";
  req_url += city;
  req_url += "&language=zh-Hans&unit=c&start=0&days=1";
  Serial.println(req_url);
  http_client.begin(req_url);
}

GetWeather::~GetWeather() {
  //client.stop();
  //WiFi.disconnect();
}


int GetWeather::getnow_weather_wifihttp(cityWeather* objcityWeather)
{
  int http_code = 0;
  Serial.println("begin getnow_weather_wifihttp");
  while (!client.connect(http_host, 80)) {
    Serial.println("connection failed");
  }
  Serial.println("connect ok");

  String line = "";
  bool http_ok = false;
  bool head_ok = false;
  uint32_t resultsize = 0;
  String findresult = "";

  String tmpstr = "";

  uint32_t all_starttime = 0;
  uint32_t starttime = 0;
  uint32_t stoptime = 0;
  starttime = millis() / 1000;
  all_starttime = starttime;

  //以下信息通过 Fiddler 工具分析协议，Raw部分获得
  String  HttpHeader = String("GET ") + String(req_url) +
                       String(" HTTP/1.1\r\nHost: ") + http_host + String("\r\nConnection: keep-alive") + String("\r\n\r\n");
  client.print(HttpHeader);

  //等待几秒，直到有数据
  while (!client.available())
  {
    if (millis() / 1000 - starttime < 0)
      starttime = millis() / 1000;
      
    if (millis() / 1000 - starttime >= 5)
    {
      Serial.println("timeout >5s");
      return 0;
    }
    delay(10);
    yield();
  }
  Serial.println("处理返回数据 ...");

  while (client.available())
  {
    if (head_ok == true)
    {
      //Serial.println("head_ok quit");
      if (http_ok  && resultsize > 0)
      {
        //正常500字节
        if (resultsize <= 1024)
        {
          // Serial.println(resultsize);
          client.read(buff, resultsize);
          char tmparray[resultsize + 1] ;
          memcpy(tmparray, buff, resultsize);

          tmparray[resultsize] = 0;
          // Serial.println(resultsize);
          findresult =  String(tmparray);
        }
      }
      break;
    }
    line = client.readStringUntil('\n');

    if (line.startsWith("HTTP/1.1"))
    {
      Serial.println(">" + line);
      if (line.startsWith("HTTP/1.1 200 OK"))
        http_ok = true;

    }
    if (line.startsWith("Content-Length: "))
    {
      tmpstr = line.substring(String("Content-Length: ").length(), line.length());

      Serial.println(">Content-Length: " + tmpstr);

      //如果超过65535可能会有问题!
      resultsize = tmpstr.toInt();
    }
    if (line.length() == 1 && line.startsWith("\r"))
    {
      head_ok = true;
      //Serial.println(">end");
    }
    //Serial.println(line);

    yield();
  }

  Serial.println("end getnow_weather_wifihttp");
  delay(10);
  client.stop();
  Serial.println("getnow_weather_wifihttp:" + String(millis() / 1000 - all_starttime) + "秒！");

  if (findresult.length() > 0)
  {
    Serial.println("> " + findresult);
    StaticJsonBuffer<1024> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(findresult);
    objcityWeather->weather = root["results"][0]["daily"][0]["text_day"].as<String>();

    objcityWeather->date_now = root["results"][0]["daily"][0]["date"].as<String>();
    objcityWeather->low_temperature = root["results"][0]["daily"][0]["low"].as<float>();
    objcityWeather->high_temperature = root["results"][0]["daily"][0]["high"].as<float>();
    objcityWeather->rainfall = root["results"][0]["daily"][0]["rainfall"].as<float>();
    objcityWeather->wind_direction = root["results"][0]["daily"][0]["wind_direction"].as<String>();
    objcityWeather->wind_speed = root["results"][0]["daily"][0]["wind_speed"].as<float>();

    objcityWeather->humidity =  root["results"][0]["daily"][0]["humidity"].as<float>();
    objcityWeather->last_update = root["results"][0]["last_update"].as<String>();

    http_code = HTTP_CODE_OK;
  }
  return (http_code);
}

int GetWeather::getnow_weather(cityWeather* objcityWeather) {
  uint32_t start_time = millis() / 1000;
  int http_code = http_client.GET();
  String rsp;
  Serial.println(http_code);
  if (http_code > 0)
  {
    //Serial.printf("HTTP get code: %d\n", http_code);
    if (http_code == HTTP_CODE_OK)
    {
      rsp = http_client.getString();
      Serial.println(rsp);
    }
  }
  return http_code;
}
