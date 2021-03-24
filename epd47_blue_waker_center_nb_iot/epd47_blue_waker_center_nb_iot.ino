#include <HardwareSerial.h>
#include "RTClib.h"
#include "weather_seniverse_7020.h"
#include "weather_multiday_7020.h"
#include "ble_to_hc08.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

int reset_pin=15;

HardwareSerial mySerial(1);
hw_timer_t *timer = NULL;
int loop_num = 0;
RTC_DS3231 rtc;
/*
  编译文件大小: 1.0M

  1.虚拟串口使用的是9600,  sim7020需要提前用 AT+IPR=9600 命令将sim7020波特率改成9600
  2.华丽版本天气获取的JSON数据长达5K,天气json解析需要较大内存。
    最好用带psram的esp32,且编译时打开param:enabled,否则json解析天气会不稳定，
    如果是无psram的esp32开发板，def_weather_time_table 变量赋值为空串，不要使用本功能
    仅使用文本串显示天气。
  3.表格版本天气因为传输数据多(约1KB)，蓝牙传输偶有发现丢失字符现象, 之前没用nb-iot时较少出现，怀疑nb-iot信号与蓝牙有干扰？
  4. no-iot不如wifi稳定，查询天气会尝试3次，间隔1分钟。

  引脚连接:
  ESP32 Sim7020c
  5V    5v
  GND   GND
  12    TX
  13    RX
  15    RESET (用于拉低引脚复位）
*/


bool net_connect_succ = false;

String waker_blue_machine = "34:14:b5:90:89:17";

//上次返回天气时间，防止同一分钟多次获取时间
String last_weather_show_time = "";
String last_weather_show_time_table = "";
String last_wake_blue_time = "";

//蓝牙发送接收因为信号问题，不能保证每次发送的天气都能被墨水屏接收到，可通过增加发送次数降低失败机率

//1.定义返回天气的时间
String def_weather_time = "06:10;18:20";
//String def_weather_time = "13:30;14:30;15:00;16:00;17:25";

//2.定义返回天气的时间(带表格显示多天的华丽版本天气)
String def_weather_time_table = "06:45;18:55";

//3.定义唤醒远程单片机的时间
String def_wake_blue_time = "";  //考虑到蓝牙不稳定，未必连接正常，可间隔20分钟连续两次


char daysOfTheWeek[7][12] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

String g_ink_showtxt = "";


String weather_data = "";      //文本串天气,简单字串天气显示用
String weather_data_table = ""; //json格式的文本串天气,华丽表格天气显示用

uint32_t check_down_time = 0; //每小时AT命令检查一次，如果sim7020关机，启动它

String buff_split[10];

Manager_blue_to_hc08* objManager_blue_to_hc08;
GetWeather* objGetWeather;
cityWeather* objcityWeather;
Weather_multidayManager * objWeather_multidayManager;

int try_num_weather = 0;
int try_num_weather_table = 0;

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

void rebootESP() {
  Serial.print("Rebooting ESP32: ");
  delay(100);
  //ESP.restart();  左边的方法重启后连接不上esp32
  esp_restart();
}

/*
  String send_at_httpget(String p_char, int delay_sec) {
  String ret_str = "";
  String tmp_str = "";
  if (p_char.length() > 0)
  {
    Serial.println(String("cmd=") + p_char);
    mySerial.println(p_char);
  }
  mySerial.setTimeout(1000);
  uint32_t start_time = millis() / 1000;
  while (millis() / 1000 - start_time < delay_sec)
  {

    if (mySerial.available() > 0)
    {
      tmp_str = mySerial.readStringUntil('\n');
      Serial.println(tmp_str);

      //结束标志，退出
      if (tmp_str.indexOf("+CHTTPERR: 0,-2") > -1) break;

      //遇到这个标志，其它数据清除
      if (tmp_str.indexOf("+CHTTPNMIC: 0,0,") > -1)
      {
        //Serial.println("&");
        ret_str = tmp_str;
      }
      else
        ret_str = ret_str + tmp_str;

    }
    delay(10);
  }

  return ret_str;
  }
*/

String send_at_httpget(String p_char, int delay_sec) {
  String ret_str = "";
  String tmp_str = "";
  if (p_char.length() > 0)
  {
    Serial.println(String("cmd=") + p_char);
    mySerial.println(p_char);
  }
  ret_str = "";
  mySerial.setTimeout(5000);
  uint32_t start_time = millis() / 1000;
  while (millis() / 1000 - start_time < delay_sec)
  {

    if (mySerial.available() > 0)
    {
      tmp_str = mySerial.readStringUntil('\n');
      Serial.println(tmp_str);

      //结束标志，退出
      if (tmp_str.indexOf("+CHTTPERR: 0,-2") > -1) break;

      //遇到这个标志，其它数据清除
      if (tmp_str.indexOf("+CHTTPNMIC: 0,0,") > -1)
      {
        //Serial.println("&");
        ret_str = ret_str + parse_CHTTPNMIC(tmp_str);
      }
      else if (tmp_str.indexOf("+CHTTPNMIC: 0,1,") > -1)
      {
        //Serial.println("&");
        ret_str = ret_str + parse_CHTTPNMIC(tmp_str);
      }

    }
    delay(10);
  }

  return ret_str;
}

/*
  //不会因为readStringUntil 阻塞
  String send_at_old(String p_char, String break_str, int delay_sec) {
  char ch;
  String ret_str = "";
  if (p_char.length() > 0)
  {
    Serial.println(String("cmd=") + p_char);
    mySerial.println(p_char);
  }

  //发完命令立即退出
  //if (break_str=="") return "";

  uint32_t start_time = millis() / 1000;
  while (millis() / 1000 - start_time < delay_sec)
  {

    if (mySerial.available() > 0)
    {
      while (mySerial.available() > 0)
      {
        ch = mySerial.read();
        ret_str = ret_str + ch;
        delay(5);
      }
      if (break_str.length() > 0 && ret_str.indexOf(break_str) > -1)
        break;
    }
    delay(10);
  }

  return ret_str;
  }
*/

//要防止readStringUntil阻塞，确认有新数据再调用此函数
String send_at(String p_char, String break_str, int delay_sec) {

  String ret_str = "";
  String tmp_str = "";
  if (p_char.length() > 0)
  {
    Serial.println(String("cmd=") + p_char);
    mySerial.println(p_char);
  }

  //发完命令立即退出
  //if (break_str=="") return "";

  mySerial.setTimeout(1000);

  uint32_t start_time = millis() / 1000;
  while (millis() / 1000 - start_time < delay_sec)
  {
    if (mySerial.available() > 0)
    {
      //Serial.println(".");

      //此句容易被阻塞
      tmp_str = mySerial.readStringUntil('\n');
      Serial.println(tmp_str);
      ret_str = ret_str + tmp_str;
      if (break_str.length() > 0 && ret_str.indexOf(break_str) > -1)
        break;
    }
    delay(10);
  }
  return ret_str;
}

//重启7020
void reset_7020()
{
  digitalWrite(reset_pin, LOW);
  delay(1000);
  digitalWrite(reset_pin, HIGH);
  Serial.println("reset_7020...");
  clear_uart(20000);
}

//sec秒内不接收串口数据，并清缓存
void clear_uart(int ms_time)
{
  //唤醒完成后就可以正常接收串口数据了
  uint32_t starttime = 0;
  char ch;
  //5秒内有输入则输出
  starttime = millis();
  //临时接收缓存，防止无限等待
  while (true)
  {
    if  (millis()  - starttime > ms_time)
      break;
    while (mySerial.available())
    {
      ch = (char) mySerial.read();
      Serial.print(ch);
    }
    yield();
    delay(10);
  }
}



void free_http()
{
  String ret;
  ret = send_at("AT+CHTTPDISCON=0", "OK", 5);
  Serial.println("ret=" + ret);
  Serial.println(">>> 断开http连接  ok ...");

  ret = send_at("AT+CHTTPDESTROY=0", "OK", 5);
  Serial.println("ret=" + ret);


  Serial.println(">>> 释放 HTTP ok ...");
}

bool get_weather()
{
  bool succ_flag = false;

  String ret;

  ret = send_at("AT+CHTTPCREATE=\"" + objGetWeather->req_host + "\"", "+CHTTPCREATE: 0", 30);
  Serial.println("ret=" + ret);
  if (not (ret.indexOf("+CHTTPCREATE: 0") > -1))
    return false;

  Serial.println(">>> 创建HTTP Host ok ...");
  delay(200);

  ret = send_at("AT+CHTTPCON=0", "OK", 30);
  Serial.println("ret=" + ret);
  if (not (ret.indexOf("OK") > -1))
    return false;

  Serial.println(">>> 连接 http  ok ...");

  delay(200);
  //最长60秒内获得数据
  ret = send_at_httpget("AT+CHTTPSEND=0,0,\"" + objGetWeather->req_url + "\"", 60);
  //Serial.println("ret=" + ret);
  if (ret.length() > 10)
  {
    succ_flag = true;
    weather_data = ret;
    Serial.println("weather_data=" + weather_data);
  }

  return succ_flag;
}


//获得天气，表格版
bool get_weather_table()
{
  bool succ_flag = false;

  String ret;

  ret = send_at("AT+CHTTPCREATE=\"" + objWeather_multidayManager->req_host + "\"", "+CHTTPCREATE: 0", 30);
  Serial.println("ret=" + ret);
  if (not (ret.indexOf("+CHTTPCREATE: 0") > -1))
    return false;

  Serial.println(">>> 创建HTTP Host ok ...");
  delay(200);

  ret = send_at("AT+CHTTPCON=0", "OK", 30);
  Serial.println("ret=" + ret);
  if (not (ret.indexOf("OK") > -1))
    return false;

  Serial.println(">>> 连接 http  ok ...");

  delay(200);
  //最长120秒内获得数据
  ret = send_at_httpget("AT+CHTTPSEND=0,0,\"" + objWeather_multidayManager->req_url + "\"", 120);
  //Serial.println("ret=" + ret);
  if  (ret.length() > 100)
  {
    succ_flag = true;
    weather_data_table = ret;
    Serial.println("weather_data_table=" + weather_data_table);
  }

  return succ_flag;
}

bool check_net()
{
  bool  ret_bool = false;
  String ret = "";
  //查询网络状态
  ret = send_at("AT+CGCONTRDP", "\"cmnbiot\"", 1);

  //分配到IP
  if (ret.indexOf("\"cmnbiot\"") > -1)
  {
    ret_bool = true;
  }

  return ret_bool;
}

//仅检查是否关机状态
bool check_waker_7020()
{
  String ret = "";
  delay(1000);
  int cnt = 0;
  bool check_ok = false;
  //通过AT命令检查是否关机，共检查3次
  while (true)
  {
    cnt++;
    ret = send_at("AT", "", 2);
    Serial.println("ret=" + ret);

    //此标志表示关闭状态
    if (ret.indexOf("NORMAL POWER DOWN") > -1)
      break;

    //返回OK，说明正常
    if (ret.indexOf("OK") > -1)
    {
      check_ok = true;
      break;
    }
    if (cnt >= 3) break;
    delay(1000);
  }
  return check_ok;
}


bool connect_nb()
{
  bool  ret_bool = false;

  int error_cnt = 0;
  String ret;

  error_cnt = 0;
  //网络信号质量查询，返回信号值
  while (true)
  {
    ret = send_at("AT+CPIN?", "+CPIN: READY", 1);
    Serial.println("ret=" + ret);
    if (ret.indexOf("+CPIN: READY") > -1)
      break;
    delay(2000);

    error_cnt++;
    if (error_cnt >= 5)
      return false;
  }
  Serial.println(">>> SIM 卡状态 ok ...");

  error_cnt = 0;
  //网络信号质量查询，返回信号值
  while (true)
  {
    ret = send_at("AT+CSQ", "+CSQ: 0,0", 1);
    Serial.println("ret=" + ret);
    if (ret.indexOf("+CSQ: 0,0") == -1)
      break;
    delay(2000);

    error_cnt++;
    if (error_cnt >= 5)
      return false;
  }
  Serial.println(">>> 射频信号 ok ...");
  error_cnt = 0;
  //查询网络注册状态
  while (true)
  {
    ret = send_at("AT+CGREG?", "+CGREG: 0,1", 1);
    Serial.println("ret=" + ret);

    if (ret.indexOf("+CGREG: 0,1") > -1)
      break;
    delay(2000);

    error_cnt++;
    if (error_cnt >= 5)
      return false;
  }
  Serial.println(">>> PS 业务附着 ok ...");
  error_cnt = 0;
  //查询PDP状态
  while (true)
  {
    ret = send_at("AT+CGACT?", "+CGACT: 1,1", 1);
    Serial.println("ret=" + ret);
    if (ret.indexOf("+CGACT: 1,1") > -1)
      break;
    delay(2000);

    error_cnt++;
    if (error_cnt >= 5)
      return false;
  }
  Serial.println(">>> PDN 激活 OK ...");
  error_cnt = 0;
  //查询网络信息
  while (true)
  {
    ret = send_at("AT+COPS?", "+COPS: 0,2,\"46000\",9", 1);
    Serial.println("ret=" + ret);
    if (ret.indexOf("+COPS: 0,2,\"46000\",9") > -1)
      break;
    delay(2000);

    error_cnt++;
    if (error_cnt >= 5)
      return false;
  }
  Serial.println(">>> 网络信息，运营商及网络制式 OK...");
  error_cnt = 0;
  while (true)
  {
    //查询网络状态
    ret = send_at("AT+CGCONTRDP", "cmnbiot", 1);
    Serial.println("ret=" + ret);

    //分配到IP
    if (ret.indexOf("\"cmnbiot\"") > -1)
    {
      ret_bool = true;
      break;
    }
    delay(2000);

    error_cnt++;
    if (error_cnt >= 5)
      return false;
  }

  Serial.println(">>> 获取IP OK...");
  return ret_bool;
}
String Strhex_char(char ch) {

  return String(ch, HEX);;
}

String Strhex_convert(String data_str) {
  String tmpstr = "";

  for (int loop1 = 0; loop1 < data_str.length() ; loop1++)
  {
    tmpstr = tmpstr + Strhex_char(data_str[loop1]);
  }
  return tmpstr;
}

char hexStr_char(String data_str) {
  //int tmpint = data_str.toInt();
  //Serial.println("tmpint="+String(tmpint));
  //String(data[i], HEX);

  char ch;
  sscanf(data_str.c_str(), "%x", &ch);
  return ch;
}

String hexStr_convert(String data_str) {

  //如果数据不是整数值，返回空
  if (data_str.length() == 0) return "";
  if ((data_str.length() % 2) != 0) return "";

  char ch;
  String  tmpstr = "";
  for (int loop1 = 0; loop1 < data_str.length() / 2; loop1++)
  {
    ch = hexStr_char(data_str.substring(loop1 * 2, loop1 * 2 + 2));
    //Serial.print("ch="+String(ch));
    tmpstr = tmpstr + ch;
  }
  return tmpstr;
}


String parse_CHTTPNMIC(String in_str)
{
  //+CHTTPNMIC: 0,0,462,462,7b22726573756c7473223a5b7b226c6f636174696f6e223a7b226964223a2257583446425858464b453446222c226e616d65223a22e58c97e4baac222c22636f756e747279223a22434e222c2270617468223a22e58c97e4baac2ce58c97e4baac2ce4b8ade59bbd222c2274696d657a6f6e65223a22417369612f5368616e67686169222c2274696d657a6f6e655f6f6666736574223a222b30383a3030227d2c226461696c79223a5b7b2264617465223a22323032312d30332d3133222c22746578745f646179223a22e99cbe222c22636f64655f646179223a223331222c22746578745f6e69676874223a22e998b4222c22636f64655f6e69676874223a2239222c2268696768223a223136222c226c6f77223a2238222c227261696e66616c6c223a22302e30222c22707265636970223a22222c2277696e645f646972656374696f6e223a22e58d97222c2277696e645f646972656374696f6e5f646567726565223a22313830222c2277696e645f7370656564223a22382e34222c2277696e645f7363616c65223a2232222c2268756d6964697479223a223539227d5d2c226c6173745f757064617465223a22323032312d30332d31335430383a30303a30302b30383a3030227d5d7d

  String out_str = "";
  int cnt = 0;
  splitString(in_str, ",", buff_split, 5);
  //注意： 要乘2
  cnt = buff_split[3].toInt() * 2;
  out_str = buff_split[4].substring(0, cnt);
  //Serial.println("out_str1=" + out_str);
  out_str = hexStr_convert(out_str);
  //Serial.println("out_str2=" + out_str);
  return out_str;
}

void splitString(String message, String dot, String outmsg[], int len)
{
  int commaPosition, outindex = 0;
  for (int loop1 = 0; loop1 < len; loop1++)
    outmsg[loop1] = "";
  do {
    commaPosition = message.indexOf(dot);
    if (commaPosition != -1)
    {
      outmsg[outindex] = message.substring(0, commaPosition);
      outindex = outindex + 1;
      message = message.substring(commaPosition + 1, message.length());
    }
    if (outindex >= len) break;
  }
  while (commaPosition >= 0);

  if (outindex < len)
    outmsg[outindex] = message;
}



void IRAM_ATTR resetModule() {
  ets_printf("resetModule reboot\n");
  delay(100);
  //esp_restart_noos(); 旧api
  esp_restart();
}

void setup() {
  Serial.begin(115200);
  //                               RX, TX
  mySerial.begin(9600, SERIAL_8N1, 12, 13);

  pinMode(reset_pin, OUTPUT);
  digitalWrite(reset_pin, HIGH);
  
  //为防意外，n秒后强制复位重启，一般用不到。。。
  //n秒如果任务处理不完，看门狗会让esp32自动重启,防止程序跑死...
  //如果串口有新数据，时间会重新计算
  int wdtTimeout = 10 * 60 * 1000; //设置10分钟 watchdog

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000 , false); //set time in us
  timerAlarmEnable(timer);

  // 初始化时钟模块
  // 判断不出是否连接时钟模块，如果不连接，也返回True
  // 可通过返回的时间的有效性判断！
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    rebootESP();
  }

  Serial.println("boot ...");

  //10秒时间是等待主控初始化
  //不着急打开 sim7020c
  delay(10000);

  //boot_time = millis() / 1000;
  check_down_time = millis() / 1000;

  Serial.println(">>> 开启 nb-iot ...");


  delay(10000);  //等待一会，确保网络连接上

  Serial.println(">>> 检查网络连接 ...");
  net_connect_succ = false;

  net_connect_succ = connect_nb();

  Serial.println("net_connect_succ:" + String(net_connect_succ));

  objManager_blue_to_hc08 = new Manager_blue_to_hc08();
  objGetWeather = new GetWeather();
  objcityWeather = new cityWeather();
  objWeather_multidayManager = new Weather_multidayManager;

  g_ink_showtxt = "";
  loop_num = 0;
  try_num_weather = 1; //开机后获取天气到墨水屏
  try_num_weather_table = 0; //开机后获取天气到墨水屏
}


void loop() {
  //英文资料说millis（) 在大约50天后清零，
  delay(1000);

  //每30秒 feed dog 一次
  loop_num = loop_num + 1;
  if (loop_num > 30)
  {
    // Serial.println("feed watchdog...");
    loop_num = 0;
    timerWrite(timer, 0); //reset timer (feed watchdog)
  }

  if ( millis() / 1000 < check_down_time )
    check_down_time = millis() / 1000;


  //如果setup时网络连接失败，重新再试
  if (net_connect_succ == false)
  {
    delay(5000);
    Serial.println(">>> 检查网络连接 ...");
    net_connect_succ = connect_nb();
    return;
  }

  //每1小时检查sim7020是否alive, 如果关闭则重启sim7020
  if ( millis() / 1000 - check_down_time >   3600)
  {
    if (check_waker_7020() == false)
    {
      Serial.println("AT 命令不响应，reset sim7020");
      reset_7020();
      net_connect_succ = false;
    }

    check_down_time = millis() / 1000 ;
    return ;
  }


  if (net_connect_succ )
  {

    //前一次获取天气不成功，可再试2次
    if (try_num_weather > 0)
    {
      Serial.println("上次获取天气不成功，还有" + String(try_num_weather) + "次尝试机会...");
      try_num_weather = try_num_weather - 1;
      bool weatherok = get_weather();
      //释放获取天气时创建的资源，必须要！
      free_http();
      if (weatherok)
      {
        int http_code = objGetWeather->getnow_weather_7020(weather_data, objcityWeather);
        if (http_code == 0)
        {
          String weather_info  = objcityWeather->toString() ;
          g_ink_showtxt = Get_ds3231_time_weather(3) + " " + weather_info + " " + Get_ds3231_time_weather(1);
          last_weather_show_time = Get_ds3231_time_weather(2);
          try_num_weather = 0;
        }
      }
      //如果没成功获取天气，下一分钟再试
      if (try_num_weather > 0)
        delay(60000);
    }


    //前一次获取天气不成功，可再试2次
    if (try_num_weather_table > 0)
    {
      Serial.println("上次获取天气不成功，还有" + String(try_num_weather_table) + "次尝试机会...");
      try_num_weather_table = try_num_weather_table - 1;
      //不能保证每次都能获取到天气数据！
      bool weatherok = get_weather_table();
      //释放获取天气时创建的资源，必须要！
      free_http();
      if (weatherok)
      {
        int http_code = objWeather_multidayManager->getnow_weather_7020(weather_data_table);
        if (http_code == 0)
        {
          g_ink_showtxt = objWeather_multidayManager->resp_new;
          last_weather_show_time_table = Get_ds3231_time_weather(2);
          try_num_weather_table = 0;
        }
      }
      //如果没成功获取天气，下一分钟再试
      if (try_num_weather_table > 0)
        delay(60000);
    }


    //1.是否在设定时间
    String weather_time = Get_ds3231_time_weather(1);
    if (try_num_weather == 0 and last_weather_show_time != Get_ds3231_time_weather(2) and  def_weather_time.indexOf(weather_time) > -1)
    {
      try_num_weather = 2; //最多允许试3次.
      //不能保证每次都能获取到天气数据！
      bool weatherok = get_weather();
      //释放获取天气时创建的资源，必须要！
      free_http();
      if (weatherok)
      {
        int http_code = objGetWeather->getnow_weather_7020(weather_data, objcityWeather);
        if (http_code == 0)
        {
          String weather_info  = objcityWeather->toString() ;
          g_ink_showtxt = Get_ds3231_time_weather(3) + " " + weather_info + " " + Get_ds3231_time_weather(1);

          //记录上一次日期
          last_weather_show_time = Get_ds3231_time_weather(2);
          try_num_weather = 0;
        }
      }

      //如果没成功获取天气，下一分钟再试
      if (try_num_weather > 0)
        delay(60000);
    }


    //2 判断是否在设定"小时:分钟"时间，返回信息设置为当前天气_表格版
    weather_time = Get_ds3231_time_weather(1);
    if (last_weather_show_time_table != Get_ds3231_time_weather(2) and  def_weather_time_table.indexOf(weather_time) > -1)
    {
      try_num_weather_table = 2; //最多允许试3次.
      //不能保证每次都能获取到天气数据！
      bool weatherok = get_weather_table();
      //释放获取天气时创建的资源，必须要！
      free_http();
      if (weatherok)
      {
        int http_code = objWeather_multidayManager->getnow_weather_7020(weather_data_table);
        if (http_code == 0)
        {
          g_ink_showtxt = objWeather_multidayManager->resp_new;
          last_weather_show_time_table = Get_ds3231_time_weather(2);
          try_num_weather_table = 0;
        }
      }

      //如果没成功获取天气，下一分钟再试
      if (try_num_weather_table > 0)
        delay(60000);
    }


    //3 判断是否在设定"小时:分钟"时间，唤醒远程hc08的蓝牙设备
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


    if (g_ink_showtxt.length() > 0)
    {
      //正常300字节内5秒发送完成
      //1KB字节数据发送需要更长时间，发送中间需要定时delay，约35-40秒左右
      bool ret_bool = objManager_blue_to_hc08->blue_connect_sendmsg(g_ink_showtxt, false);
      if (ret_bool)
        g_ink_showtxt = "";
      delay(10000);
      //调试用
      //delay(60000);
    }
  }
}
