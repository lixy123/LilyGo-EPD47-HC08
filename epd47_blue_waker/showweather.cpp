#include "showweather.h"


weatherManager::weatherManager()
{
  readbuff = (byte*)ps_malloc(10 * 1024);

  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    Serial.println("alloc memory failed !!!");
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}




weatherManager::~weatherManager()
{

}


String weatherManager::readData(char * myFileName) {

  Serial.println("weatherManager::readData");
  if (!SPIFFS.exists(myFileName)) {
    Serial.println(String(myFileName) + " not exists");
    return "";
  }

  long readnum = 0;
  File file = SPIFFS.open(myFileName, FILE_READ);

  readnum = file.read(readbuff, 10 * 1024);
  file.close();
  readbuff[readnum] = '\0';
  Serial.println("readnum=" + String(readnum));
  //Serial.println("readData=" +  String((char *)readbuff));
  return String((char *)readbuff);

}

void weatherManager::ShowLine(int x0, int y0, int x1, int y1)
{
  epd_draw_line(x0, y0, x1, y1, 0, framebuffer);
}


void weatherManager::ShowRec(int x0, int y0, int x1, int y1)
{
  epd_draw_rect(x0, y0, abs(x1 - x0), abs(y1 - y0), 0, framebuffer);
}



uint8_t * weatherManager::get_icon(String no)
{

  uint8_t * ret_p = (uint8_t *)w0_data;
  if (no == "0")   // your hand is on the sensor
    ret_p = (uint8_t *)w0_data;
  else if (no == "1")
    ret_p = (uint8_t *) w1_data;
  else if (no == "2")
    ret_p = (uint8_t *) w2_data;
  else if (no == "3")
    ret_p = (uint8_t *) w3_data;
  else if (no == "4")
    ret_p = (uint8_t *) w4_data;
  else if (no == "5")
    ret_p = (uint8_t *) w5_data;
  else if (no == "6")
    ret_p = (uint8_t *) w6_data;
  else if (no == "7")
    ret_p = (uint8_t *) w7_data;
  else if (no == "8")
    ret_p = (uint8_t *) w8_data;
  else if (no == "9")
    ret_p = (uint8_t *) w9_data;
  else if (no == "10")
    ret_p = (uint8_t *) w10_data;
  else if (no == "11")
    ret_p = (uint8_t *) w11_data;
  else if (no == "12")
    ret_p = (uint8_t *) w12_data;
  else if (no == "1")
    ret_p = (uint8_t *) w1_data;
  else if (no == "13")
    ret_p = (uint8_t *) w13_data;
  else if (no == "14")
    ret_p = (uint8_t *) w14_data;
  else if (no == "15")
    ret_p = (uint8_t *) w15_data;
  else if (no == "16")
    ret_p = (uint8_t *) w16_data;
  else if (no == "17")
    ret_p = (uint8_t *) w17_data;
  else if (no == "1")
    ret_p = (uint8_t *) w1_data;
  else if (no == "18")
    ret_p = (uint8_t *) w18_data;
  else if (no == "19")
    ret_p = (uint8_t *) w19_data;
  else if (no == "20")
    ret_p = (uint8_t *) w20_data;
  else if (no == "21")
    ret_p = (uint8_t *) w21_data;
  else if (no == "22")
    ret_p = (uint8_t *) w22_data;
  else if (no == "23")
    ret_p = (uint8_t *) w23_data;
  else if (no == "24")
    ret_p = (uint8_t *) w24_data;
  else if (no == "25")
    ret_p = (uint8_t *) w25_data;
  else if (no == "26")
    ret_p = (uint8_t *) w26_data;
  else if (no == "27")
    ret_p = (uint8_t *) w27_data;
  else if (no == "28")
    ret_p = (uint8_t *) w28_data;
  else if (no == "29")
    ret_p = (uint8_t *) w29_data;
  else if (no == "30")
    ret_p = (uint8_t *) w30_data;
  else if (no == "31")
    ret_p = (uint8_t *) w31_data;
  else if (no == "32")
    ret_p = (uint8_t *) w32_data;
  else if (no == "39")
    ret_p = (uint8_t *) w39_data;
  else if (no == "49")
    ret_p = (uint8_t *) w49_data;
  else if (no == "53")
    ret_p = (uint8_t *) w53_data;
  else if (no == "54")
    ret_p = (uint8_t *) w54_data;
  else if (no == "55")
    ret_p = (uint8_t *) w55_data;
  else if (no == "56")
    ret_p = (uint8_t *) w56_data;
  else if (no == "57")
    ret_p = (uint8_t *) w57_data;
  else if (no == "58")
    ret_p = (uint8_t *) w58_data;
  else if (no == "99")
    ret_p = (uint8_t *) w99_data;
  else if (no == "301")
    ret_p = (uint8_t *) w301_data;
  else if (no == "302")
    ret_p = (uint8_t *) w302_data;

  return ret_p;
}


void weatherManager::draw_weather(String weather_result)
{
  Serial.println("begin draw_weather");
  if (weather_result.length() < 512)
  {
    Serial.println("无效的天气数据");
    return;
  }
  //约4-5kB
  /*
    StaticJsonBuffer<10 * 1024> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(weather_result);
  */

  //字符长度过大，必须用 DynamicJsonDocument
  DynamicJsonDocument  root(10 * 1024);
  deserializeJson(root, weather_result);

  int status = root["status"].as<int>();
  if (status != 0)
    return;

  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  ShowRec(10, 10, EPD_WIDTH - 20, EPD_HEIGHT - 8);

  ShowLine(10, EPD_HEIGHT / 5, EPD_WIDTH - 20, EPD_HEIGHT / 5);
  ShowLine(10, EPD_HEIGHT / 5 * 3, EPD_WIDTH - 20, EPD_HEIGHT / 5 * 3);
  ShowLine(10, EPD_HEIGHT - 70, EPD_WIDTH - 20,  EPD_HEIGHT - 70);

  ShowLine(EPD_WIDTH / 2, EPD_HEIGHT / 5, EPD_WIDTH / 2, EPD_HEIGHT - 70);
  ShowLine(EPD_WIDTH / 4, EPD_HEIGHT / 5 * 3, EPD_WIDTH / 4, EPD_HEIGHT - 70);
  ShowLine(EPD_WIDTH / 4 * 3, EPD_HEIGHT / 5 * 3, EPD_WIDTH / 4 * 3, EPD_HEIGHT - 70);

  ShowStr(root["result"]["date"].as<String>(), 20, 20, 150, framebuffer);
  ShowStr(root["result"]["week"].as<String>(), 520, 20, 150, framebuffer);

  uint8_t * icon_data;
  
  Rect_t area = {
    .x = EPD_WIDTH / 16,
    .y = EPD_HEIGHT / 5 + 120,
    .width = 70,
    .height = 60,
  };
 icon_data = get_icon(root["result"]["img"].as<String>());
  epd_copy_to_framebuffer(area, icon_data, framebuffer);

  //温度
  //   ShowPicture("pictures_weather/"+result["img"]+".png",EPD_WIDTH/16,EPD_HEIGHT/5+120);
  ShowStr(root["result"]["city"].as<String>() + " " + root["result"]["weather"].as<String>(), 20, EPD_HEIGHT / 5 + 20, 130, framebuffer);

  ShowStr(root["result"]["temp"].as<String>() + "℃", EPD_WIDTH / 4 + 140, EPD_HEIGHT / 5 + 20, 160, framebuffer);

  ShowStr("最高" + root["result"]["temphigh"].as<String>() + "℃" + " " + "最低" + root["result"]["templow"].as<String>() + "℃", EPD_WIDTH / 4 - 40, EPD_HEIGHT / 5 * 2 - 20, 48, framebuffer);
  ShowStr("湿度:" + root["result"]["humidity"].as<String>() + "%", EPD_WIDTH / 4 - 40, EPD_HEIGHT / 5 * 2 + 20, 48, framebuffer);

  ShowStr(root["result"]["winddirect"].as<String>(), EPD_WIDTH / 2 + 20, EPD_HEIGHT / 5 + 20, 130, framebuffer);
  ShowStr(root["result"]["windpower"].as<String>(), EPD_WIDTH / 2 + 20,  EPD_HEIGHT / 5 + 110, 130, framebuffer);

  //空气质量
  ShowStr("PM2.5:" + root["result"]["aqi"]["pm2_5"].as<String>(),   EPD_WIDTH / 2 + 200, EPD_HEIGHT / 5 * 2 - 20, 48, framebuffer);
  ShowStr("空气质量:" + root["result"]["aqi"]["quality"].as<String>(), EPD_WIDTH / 2 + 200, EPD_HEIGHT / 5 * 2 + 20, 48, framebuffer);


  //连续4天天气...

  ShowStr(root["result"]["daily"][1]["date"].as<String>(), EPD_WIDTH / 32, EPD_HEIGHT / 5 * 3 - 15 , 50, framebuffer)   ;
  ShowStr(root["result"]["daily"][1]["day"]["windpower"].as<String>(), EPD_WIDTH / 32 + 110, EPD_HEIGHT / 5 * 3 + 30, 64, framebuffer);
  ShowStr(root["result"]["daily"][1]["night"]["templow"].as<String>() + "~" + root["result"]["daily"][1]["day"]["temphigh"].as<String>() + "℃", EPD_WIDTH / 32 + 90, EPD_HEIGHT / 5 * 3 + 70, 64, framebuffer);

  area = {
    .x = EPD_WIDTH / 32,
    .y = EPD_HEIGHT / 5 * 3 + EPD_HEIGHT / 10,
    .width = 70,
    .height = 60,
  };
  icon_data = get_icon(root["result"]["daily"][1]["day"]["img"].as<String>());
  epd_copy_to_framebuffer(area, icon_data, framebuffer);



  ShowStr(root["result"]["daily"][2]["date"].as<String>(), EPD_WIDTH / 4 + EPD_WIDTH / 32, EPD_HEIGHT / 5 * 3 - 15 , 50, framebuffer)   ;
  ShowStr(root["result"]["daily"][2]["day"]["windpower"].as<String>(), EPD_WIDTH / 4 + EPD_WIDTH / 32 + 110, EPD_HEIGHT / 5 * 3 + 30, 64, framebuffer);
  ShowStr(root["result"]["daily"][2]["night"]["templow"].as<String>() + "~" + root["result"]["daily"][2]["day"]["temphigh"].as<String>() + "℃", EPD_WIDTH / 4 + EPD_WIDTH / 32 + 90, EPD_HEIGHT / 5 * 3 + 70, 64, framebuffer);

  area = {
    .x = EPD_WIDTH / 4 + EPD_WIDTH / 32,
    .y = EPD_HEIGHT / 5 * 3 + EPD_HEIGHT / 10,
    .width = 70,
    .height = 60,
  };
  icon_data = get_icon(root["result"]["daily"][2]["day"]["img"].as<String>());
  epd_copy_to_framebuffer(area, icon_data, framebuffer);

  ShowStr(root["result"]["daily"][3]["date"].as<String>(), EPD_WIDTH / 4 * 2 + EPD_WIDTH / 32, EPD_HEIGHT / 5 * 3 - 15 , 50, framebuffer) ;
  ShowStr(root["result"]["daily"][3]["day"]["windpower"].as<String>(), EPD_WIDTH / 4 * 2 + EPD_WIDTH / 32 + 110, EPD_HEIGHT / 5 * 3 + 30, 64, framebuffer);
  ShowStr(root["result"]["daily"][3]["night"]["templow"].as<String>() + "~" + root["result"]["daily"][3]["day"]["temphigh"].as<String>() + "℃", EPD_WIDTH / 4 * 2 + EPD_WIDTH / 32 + 90, EPD_HEIGHT / 5 * 3 + 70, 64, framebuffer);

  area = {
    .x = EPD_WIDTH / 4 * 2+ EPD_WIDTH / 32,
    .y = EPD_HEIGHT / 5 * 3 + EPD_HEIGHT / 10,
    .width = 70,
    .height = 60,
  };
  icon_data = get_icon(root["result"]["daily"][3]["day"]["img"].as<String>());
  epd_copy_to_framebuffer(area, icon_data, framebuffer);

  ShowStr(root["result"]["daily"][4]["date"].as<String>(), EPD_WIDTH / 4 * 3 + EPD_WIDTH / 32, EPD_HEIGHT / 5 * 3 - 15 , 50, framebuffer);
  ShowStr(root["result"]["daily"][4]["day"]["windpower"].as<String>(), EPD_WIDTH / 4 * 3 + EPD_WIDTH / 32 + 100, EPD_HEIGHT / 5 * 3 + 30, 64, framebuffer);
  ShowStr(root["result"]["daily"][4]["night"]["templow"].as<String>() + "~" + root["result"]["daily"][4]["day"]["temphigh"].as<String>() + "℃", EPD_WIDTH / 4 * 3 + EPD_WIDTH / 32 + 90 , EPD_HEIGHT / 5 * 3 + 70, 64, framebuffer);
  area = {
    .x = EPD_WIDTH / 4 * 3 + EPD_WIDTH / 32,
    .y = EPD_HEIGHT / 5 * 3 + EPD_HEIGHT / 10,
    .width = 70,
    .height = 60,
  };
  icon_data = get_icon(root["result"]["daily"][4]["day"]["img"].as<String>());
  epd_copy_to_framebuffer(area, icon_data, framebuffer);

  ShowStr("Last update:" + root["result"]["updatetime"].as<String>(), 250, EPD_HEIGHT - 85, 60, framebuffer);

  epd_poweron();
  //uint32_t t1 = millis();
  //全局刷
  epd_clear();




  //前面不要用writeln,有一定机率阻塞,无法休眠
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);

  //delay(500);
  epd_poweroff();

  Serial.println("end draw_weather"  );
}
