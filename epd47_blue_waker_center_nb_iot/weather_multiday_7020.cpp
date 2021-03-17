#include "weather_multiday_7020.h"

/*
  https://arduinojson.org/v6/how-to/use-external-ram-on-esp32/  如何调用 arduinojson库时使用psram
  //注意：为了解析天气json,
  struct SpiRamAllocator {
  void* allocate(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }

  void deallocate(void* pointer) {
    heap_caps_free(pointer);
  }

  void* reallocate(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
  };

  using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;
*/

Weather_multidayManager::Weather_multidayManager() {
  req_url = String("/weather/query?appkey=");
  req_url += jisuapi_key;
  req_url += "&cityid=";
  req_url += jisuapi_city;
  resp = "";
  resp_new = "";
}

Weather_multidayManager::~Weather_multidayManager() {
  //client.stop();
  //WiFi.disconnect();
}


//获取原始url的json，并精简
//注:原始json约5k,精简成1K,lbe蓝牙传输约需30秒.
//申请变量时有富余,加倍
int Weather_multidayManager::getnow_weather_7020(String weather_data)
{

  int http_code=-1;
  resp = weather_data;
  resp_new = "";
  if (resp.length() > 0)
  {
    //DynamicJsonDocument  root(15360);  //15*1024
    //SpiRamJsonDocument   root(15*1024);
    //https://arduinojson.org/v6/api/staticjsondocument/
    //字符长度过大，超过1k 字节要用 DynamicJsonDocument
    DynamicJsonDocument  root(15 * 1024);
    //申请15kb内存一般不容易报错，在此步deserializeJson时需要更大内存，很容易报错（no_memery），需要打开psram
    DeserializationError error = deserializeJson(root, resp);

    resp = "";
    resp_new = "";


    //F:\esp32\libraries\ArduinoJson\src\ArduinoJson\Deserialization\DeserializationError.hpp
    if (error) {
      Serial.println("Failed to parse string,error=" + String(error.c_str()));
      http_code=-1;
      return http_code;
    }

    int status = root["status"].as<int>();

    Serial.println("json status=" + String(status));

    if (status != 0)
    {
      http_code=-1;
      return http_code;
    }

    Serial.println("create new_root");


    DynamicJsonDocument  new_root(3072);  //3*1024 内存占用太大，不适合放在函数内
    //SpiRamJsonDocument new_root(3*1024);

    new_root["status"] = root["status"];

    JsonObject root_result = new_root.createNestedObject("result");


    root_result["city"] = root["result"]["city"];
    root_result["date"] = root["result"]["date"];
    root_result["week"] = root["result"]["week"];
    root_result["weather"] = root["result"]["weather"];
    root_result["temp"] = root["result"]["temp"];
    root_result["temphigh"] = root["result"]["temphigh"];
    root_result["templow"] = root["result"]["templow"];
    root_result["img"] = root["result"]["img"];
    root_result["humidity"] = root["result"]["humidity"];
    root_result["winddirect"] = root["result"]["winddirect"];
    root_result["windpower"] = root["result"]["windpower"];
    root_result["updatetime"] = root["result"]["updatetime"];

    JsonObject root_result_aqi = root_result.createNestedObject("aqi");
    root_result_aqi["pm2_5"] = root["result"]["aqi"]["pm2_5"];
    root_result_aqi["quality"] = root["result"]["aqi"]["quality"];

    JsonArray daily_array = root_result.createNestedArray("daily");

    JsonObject daily1 ;
    JsonObject daily1_day;
    JsonObject daily1_night;
    for (int loop1 = 0; loop1 < 5; loop1++)
    {
      //连续多天日历
      daily1 = daily_array.createNestedObject();
      daily1["date"] = root["result"]["daily"][loop1]["date"];
      daily1_day = daily1.createNestedObject("day");
      daily1_day["img"] =  root["result"]["daily"][loop1]["day"]["img"];
      daily1_day["weather"] = root["result"]["daily"][loop1]["day"]["weather"];
      daily1_day["winddirect"] = root["result"]["daily"][loop1]["day"]["winddirect"];
      daily1_day["windpower"] = root["result"]["daily"][loop1]["day"]["windpower"];
      daily1_day["temphigh"] = root["result"]["daily"][loop1]["day"]["temphigh"];
      daily1_night = daily1.createNestedObject("night");
      daily1_night["templow"] = root["result"]["daily"][loop1]["night"]["templow"];
    }
    serializeJson(new_root, resp_new);
    new_root.clear();
    root.clear();
    http_code = 0;
  }
  return http_code;
}
