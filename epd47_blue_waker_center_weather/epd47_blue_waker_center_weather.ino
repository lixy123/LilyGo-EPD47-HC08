#include "ble_to_hc08.h"
#include "weather_seniverse.h"  //这个是早期用的，只能显示文本
#include "weather_multiday.h"   //这个显示效果较华丽
#include "smartconfigManager.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "webpages.h"

#include <Wire.h>
#include "RTClib.h"


#define FILESYSTEM SPIFFS

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */


//说明：weather_multiday 使用 华丽天气数据推送功能需要打开PSRAM
//     原因是获取的天气JSON达到5kB,在Arduinojson解析时容易内存不足失败
//     打开PSRAM能减少内存不足异常的机率
//编译后固件大小: 1.8M
//  编译分区：HUGE APP

//定时唤醒的蓝牙服务器地址，仅仅连接并断开用，不发信息
//应该做成配置式
//注意：用小写字母
String waker_blue_machine = "34:14:b5:90:89:17";

hw_timer_t *timer = NULL;

uint32_t loop_num = 0; //dog用

GetWeather* objGetWeather;
cityWeather* objcityWeather;
smartconfigManager* objsmartconfigManager;
Weather_multidayManager * objWeather_multidayManager;
Manager_blue_to_hc08* objManager_blue_to_hc08;

//上次返回天气时间，防止同一分钟多次获取时间
String last_weather_show_time = "";
String last_weather_show_time_table = "";
String last_wake_blue_time = "";

//蓝牙发送接收因为信号问题，不能保证每次发送的天气都能被墨水屏接收到，可通过增加发送次数降低失败机率
//定义几点几分返回天气的时间
//def_weather_time 文本字串天气
//def_weather_time_table 带表格显示多天的华丽版本天气
String def_weather_time = "06:10,18:10";
String def_weather_time_table = "06:30,18:30";
String def_wake_blue_time = "05:20,05:50";  //考虑到蓝牙不稳定，未必连接正常，可间隔20分钟连续两次

const int default_webserverporthttp = 80;

char daysOfTheWeek[7][12] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

String g_ink_showtxt = "";


bool rebootESP_flag = false;
AsyncWebServer *server;
RTC_DS3231 rtc;

//蓝牙开启可以和ESPAsyncWebServer共存，
//可以用wificlient对象，但不可用HTTPClient对象！
void rebootESP() {
  Serial.print("Rebooting ESP32: ");
  delay(100);
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
  ets_printf("resetModule reboot\n");
  delay(100);
  //esp_restart_noos(); 旧api
  esp_restart();
}

void setup() {
  Serial.begin(115200);

  //为防意外，n秒后强制复位重启，一般用不到。。。
  //n秒如果任务处理不完，看门狗会让esp32自动重启,防止程序跑死...
  //如果串口有新数据，时间会重新计算
  int wdtTimeout = 10 * 60 * 1000; //设置10分钟 watchdog

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

  objWeather_multidayManager = new Weather_multidayManager;


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
  Serial.println("time:" + Get_ds3231_time_weather(1));
}


void loop()
{
  //scan_time=100;

  delay(1000); // Delay a second between loops.

  //每30秒 feed dog 一次
  loop_num = loop_num + 1;
  if (loop_num > 30)
  {
    // Serial.println("feed watchdog...");
    loop_num = 0;
    timerWrite(timer, 0); //reset timer (feed watchdog)
  }


  //1.连接wifi
  objsmartconfigManager->connectwifi();

  //2.检查重启信号
  if (rebootESP_flag)
  {
    Serial.println("go to reboot...");
    delay(1000);
    rebootESP();
  }

  //3 第一个是字符串时钟，第二个是华丽带表格的时钟，两个的时间要求至少间隔5分钟以上
  //3.1 判断是否在设定"小时:分钟"时间，返回信息设置为当前天气
  String weather_time = Get_ds3231_time_weather(1);
  if (last_weather_show_time != Get_ds3231_time_weather(2) and  def_weather_time.indexOf(weather_time) > -1)
  {
    int http_code = objGetWeather->getnow_weather_wifihttp(objcityWeather);
    //int http_code = objGetWeather->getnow_weather(objcityWeather);
    // C:\Users\32446\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.5-rc6\libraries\HTTPClient\src\HTTPClient.h
    // HTTP_CODE_OK=200
    if (http_code == HTTP_CODE_OK)
    {
      String weather_info  = Get_ds3231_time_weather(3) + " " + objcityWeather->toString() + " " + Get_ds3231_time_weather(1);
      Serial.println("get weather:" + weather_info);
      g_ink_showtxt = weather_info;
      last_weather_show_time = Get_ds3231_time_weather(2);
    }
  }

  //3.2 判断是否在设定"小时:分钟"时间，返回信息设置为当前天气_表格版
  weather_time = Get_ds3231_time_weather(1);
  if (last_weather_show_time_table != Get_ds3231_time_weather(2) and  def_weather_time_table.indexOf(weather_time) > -1)
  {
    int http_code = objWeather_multidayManager->getnow_weather();
    // C:\Users\32446\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.5-rc6\libraries\HTTPClient\src\HTTPClient.h
    // HTTP_CODE_OK=200
    if (http_code == HTTP_CODE_OK)
    {
      g_ink_showtxt = objWeather_multidayManager->resp_new;
      last_weather_show_time_table = Get_ds3231_time_weather(2);
    }
  }


  //3.3 判断是否在设定"小时:分钟"时间，唤醒远程hc08的蓝牙设备
  //目前只有一台蓝牙设备需要唤醒，可考虑增加多台的情况。。。
  //目前设计被唤醒的设备是树莓派，hc08的蓝牙的state引脚通过继电器，
  //当hc08蓝牙设备被连接，蓝牙设备的引脚state为高电平，触发继电器让树莓派上的GPIO03与GND短接，从而遥控树莓派开机
  //树莓派用电量约300ma,平时接市电，用电极少，目的不是为了省电，是为了减少树莓派SD卡磨损
  weather_time = Get_ds3231_time_weather(1);
  if (last_wake_blue_time != Get_ds3231_time_weather(2) and  def_wake_blue_time.indexOf(weather_time) > -1)
  {
    //最长蓝牙扫描时间60秒,连接上蓝牙后停顿1秒
    objManager_blue_to_hc08->waker_remote_blue(waker_blue_machine, 60, 1);
    delay(5000);
    last_wake_blue_time = Get_ds3231_time_weather(2);
  }


  //4.如果有发送数据，进行发送
  //数据来源：天气，华丽表格版天气，通过网页输入的文字
  if (g_ink_showtxt.length() > 0)
  {
    //正常300字节内5秒发送完成
    //1KB字节数据发送需要更长时间，发送中间需要定时delay，约35-40秒左右
    bool ret_bool = objManager_blue_to_hc08->blue_connect_sendmsg(g_ink_showtxt, false);
    if (ret_bool)
      g_ink_showtxt = "";
    delay(5000);

    //调试用
    //delay(60000);
  }

}
