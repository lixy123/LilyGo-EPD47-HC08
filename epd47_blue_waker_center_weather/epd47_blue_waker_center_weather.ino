#include "ble_to_hc08.h"
#include "weather_seniverse.h"
#include "smartconfigManager.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "webpages.h"

#include <Wire.h>
#include "RTClib.h"

#define FILESYSTEM SPIFFS

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */


//编译后固件大小: 1.8M
hw_timer_t *timer = NULL;

uint32_t loop_num = 0; //dog用

GetWeather* objGetWeather;
cityWeather* objcityWeather;
smartconfigManager* objsmartconfigManager;

Manager_blue_to_hc08* objManager_blue_to_hc08;

//上次返回天气时间，防止同一分钟多次获取时间
String last_weather_show_time = "";
//定义几点几分返回天气的时间

String def_weather_time = "05:10,12:10,18:10,23:00";
//String def_weather_time = "20:25,20:30,20:35,20:40";

const int default_webserverporthttp = 80;

char daysOfTheWeek[7][12] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

String g_ink_showtxt = "";
String g_ink_showtxt2 = "";

bool rebootESP_flag = false;
AsyncWebServer *server;
RTC_DS3231 rtc;

//蓝牙开启可以和ESPAsyncWebServer共存，
//可以用wificlient对象，但不可用HTTPClient对象！
void rebootESP() {
  Serial.print("Rebooting ESP32: ");
  //ESP.restart();  左边的方法重启后连接不上esp32
  esp_restart();
}

String Get_ds3231_time()
{
  char buf[50];
  DateTime now = rtc.now();
  sprintf(buf, "%02d,%02d,%02d,%02d,%02d,%02d", now.year(), now.month() , now.day(), now.hour(), now.minute(), now.second());
  return String(buf);
}

//专用于判断是否该返回天气的时间
String Get_ds3231_time_weather(int flag)
{
  DateTime now = rtc.now();
  char buf[50];

  if (flag == 1)
  {
    sprintf(buf, "%02d:%02d", now.hour(), now.minute());
  }
  else if (flag == 2)
  {
    sprintf(buf, "%02d,%02d,%02d,%02d,%02d", now.year(), now.month() , now.day(), now.hour(), now.minute());

  }
  else if (flag == 3)
  {
    sprintf(buf, "%02d月%02d日%s",  now.month() , now.day(), daysOfTheWeek[now.dayOfTheWeek()]);
  }
  return String(buf);
}

String Get_ds3231_time_weather()
{
  DateTime now = rtc.now();

  String time_txt = String(  now.hour()) + ":" + String(  now.minute()) ;
  return time_txt;
}


void notFound(AsyncWebServerRequest *request) //回调函数
{
  request->send(404, "text/plain", "FileNotFound");

}


String processor(const String& var) {
  /*
    if (var == "FILELIST") {
    return listFiles(true);
    }
    if (var == "FREESPIFFS") {
    return humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
    }

    if (var == "USEDSPIFFS") {
    return humanReadableSize(SPIFFS.usedBytes());
    }

    if (var == "TOTALSPIFFS") {
    return humanReadableSize(SPIFFS.totalBytes());
    }
  */
  return String();
}


void configureWebServer() {
  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
    Serial.println(logmessage);
    request->send_P(200, "text/html", index_html, processor);
  });

  server->on("/post_msg", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
    Serial.println(logmessage);
    request->send_P(200, "text/html", index_html, processor);
  });


  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    request->send(200, "text/html", reboot_html);
    Serial.println(logmessage);
    rebootESP_flag = true;
    Serial.println("timer rebootESP");
  });



  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server->on("/get_msg", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam = "";
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam("txt_msg")) {
      inputMessage = request->getParam("txt_msg")->value();
      g_ink_showtxt = inputMessage;
      g_ink_showtxt2 = inputMessage;
      inputParam = "txt_msg";
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    //    inputMessage=inputMessage.substring(2);
    //    inputMessage=inputMessage.substring(0,5);
    Serial.println("Find Params(" + inputParam + ")=" + inputMessage);
    request->send(200, "text/html", "Find Params("
                  + inputParam + ")=" + inputMessage +
                  "<br><a href=\"/\">Return to Home Page</a>");
  });


  server->onNotFound(notFound); //用户访问未注册的链接时执行notFound()函数
}


void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  //esp_restart_noos(); 旧api
  esp_restart();
}

void setup() {
  Serial.begin(115200);

  //为防意外，n秒后强制复位重启，一般用不到。。。
  //n秒如果任务处理不完，看门狗会让esp32自动重启,防止程序跑死...
  //如果串口有新数据，时间会重新计算
  int wdtTimeout = 5 * 60 * 1000; //设置5分钟 watchdog

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000 , false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt


  //WiFi.mode(WIFI_OFF);
  //all_start_time = millis() / 1000;
  //last_send_time =  millis() / 1000;

  //1.初始化spiffs
  if (!SPIFFS.begin(true)) {
    // if you have not used SPIFFS before on a ESP32, it will show this error.
    // after a reboot SPIFFS will be configured and will happily work.
    Serial.println("ERROR: Cannot mount SPIFFS, Rebooting");
    rebootESP();
  }

  //ip_send_ok = false;
  // 2.初始化时钟模块
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    rebootESP();
  }

  //3.连接wifi
  objsmartconfigManager = new smartconfigManager();
  objGetWeather = new GetWeather();
  objcityWeather = new cityWeather();

  objsmartconfigManager->connectwifi();

  //4.startup web server
  Serial.println("Starting Webserver ...");
  //启动网页服务器
  Serial.println("\nConfiguring Webserver ...");
  server = new AsyncWebServer(default_webserverporthttp);
  server->begin();
  configureWebServer();

  //5.打开蓝牙
  objManager_blue_to_hc08 = new Manager_blue_to_hc08();

  Serial.println("setup end...");
  Serial.println("time:"+ Get_ds3231_time_weather(1));
}


void loop()
{
  delay(10); // Delay a second between loops.

  //每约10秒 feed dog 一次
  loop_num = loop_num + 1;
  if (loop_num > 1000)
  {
    // Serial.println("feed watchdog...");
    loop_num = 0;
    timerWrite(timer, 0); //reset timer (feed watchdog)
  }


  //if (millis() / 1000 < all_start_time)
  //  all_start_time = millis() ;


  //1.连接wifi
  objsmartconfigManager->connectwifi();

  //2.检查重启信号
  if (rebootESP_flag)
  {
    Serial.println("go to reboot...");
    delay(1000);
    rebootESP();
  }

  //3.判断是否在设定"小时:分钟"时间，返回信息设置为当前天气
  String weather_time = Get_ds3231_time_weather(1);
  if (last_weather_show_time != Get_ds3231_time_weather(2) and  def_weather_time.indexOf(weather_time) > -1)
  {
    int http_code = objGetWeather->getnow_weather_wifihttp(objcityWeather);
    if (http_code == HTTP_CODE_OK)
    {
      String weather_info  = Get_ds3231_time_weather(3) + " " + objcityWeather->toString() + " " + Get_ds3231_time_weather(1);
      Serial.println("get weather:" + weather_info);
      g_ink_showtxt = weather_info;
      g_ink_showtxt2 = weather_info;
      last_weather_show_time = Get_ds3231_time_weather(2);
    }
  }

  //一次未成功发送，再试
  if (g_ink_showtxt.length() > 0)
  {
    bool ret_bool = objManager_blue_to_hc08->blue_connect_sendmsg(g_ink_showtxt, false);
    if (ret_bool)
      g_ink_showtxt = "";
    delay(5000);
  }

}
