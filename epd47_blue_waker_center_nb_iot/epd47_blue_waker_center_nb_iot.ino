#include <HardwareSerial.h>
#include "RTClib.h"
#include "weather_seniverse_7020.h"
#include "ble_to_hc08.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

HardwareSerial mySerial(1);
hw_timer_t *timer = NULL;
int loop_num = 0;
RTC_DS3231 rtc;
/*
  编译文件大小: 1.0M
*/

int led = 15;
bool net_connect_succ = false;


//上次返回天气时间，防止同一分钟多次获取时间
String last_weather_show_time = "";
String last_weather_show_time_table = "";
String last_wake_blue_time = "";

//蓝牙发送接收因为信号问题，不能保证每次发送的天气都能被墨水屏接收到，可通过增加发送次数降低失败机率
//定义几点几分返回天气的时间
//def_weather_time 文本字串天气
//def_weather_time_table 带表格显示多天的华丽版本天气
String def_weather_time = "06:25;12:30;17:25";
String def_weather_time_table = "";
String def_wake_blue_time = "";  //考虑到蓝牙不稳定，未必连接正常，可间隔20分钟连续两次


char daysOfTheWeek[7][12] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

String g_ink_showtxt = "";


String weather_data = "";
uint32_t check_down_time = 0; //每小时AT命令检查一次，如果sim7020关机，启动它

String buff_split[10];

Manager_blue_to_hc08* objManager_blue_to_hc08;
GetWeather* objGetWeather;
cityWeather* objcityWeather;


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

String send_at_httpget(String p_char, int delay_sec) {
  //char  j = 0;
  //char ch;
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


String send_at(String p_char, String break_str, int delay_sec) {
  char  j = 0;
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
        delay(10);
      }
      if (break_str.length() > 0 && ret_str.indexOf(break_str) > -1)
        break;
    }
    delay(50);
  }

  return ret_str;
}

/*
  readStringUntil 有阻塞，不好用
  String send_at(String p_char, String break_str, int delay_sec) {
  //char  j = 0;
  //char ch;
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
      Serial.println(".");

      //此句容易被阻塞
      tmp_str = mySerial.readStringUntil('\n');
      Serial.println(tmp_str);
      ret_str = ret_str + tmp_str;
      if (break_str.length() > 0 && ret_str.indexOf(break_str) > -1)
        break;
    }
    delay(10);
  }
  Serial.println("@");
  return ret_str;
  }
*/


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
    delay(5);
  }
}

void powerUp() {
  Serial.println("powerUp");
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);

  delay(2000);
  clear_uart(500);  //刚启动的数据无效，丢掉
}

//void powerDown() {
//  digitalWrite(led, HIGH);
//  delay(2000);
//  digitalWrite(led, LOW);
//  delay(2000);
//}

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

  ret = send_at("AT+CHTTPCREATE=\"" + objGetWeather->req_host + "\"", "OK", 30);
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
  ret = send_at_httpget("AT+CHTTPSEND=0,0,\"" + objGetWeather->req_url + "\"", 30);
  Serial.println("ret=" + ret);
  if (ret.indexOf("+CHTTPNMIC:") > -1 and ret.length() > 10)
  {
    succ_flag = true;
    weather_data = parse_CHTTPNMIC(ret);
    Serial.println("weather_data=" + weather_data);
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

void waker_7020()
{
  String ret = "";
  delay(1000);
  //通过AT命令
  while (true)
  {
    ret = send_at("AT", "", 2);

    Serial.println("ret=" + ret);

    //返回数据中无OK
    //则判断为关机状态，进行开机
    if ( ret.indexOf("OK") > -1)
      break;
    else
    {
      //Serial.println("powerUp");
      powerUp();
    }
    delay(2000);
  }
  Serial.println(">>> AT ok ...");
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

  //为防意外，n秒后强制复位重启，一般用不到。。。
  //n秒如果任务处理不完，看门狗会让esp32自动重启,防止程序跑死...
  //如果串口有新数据，时间会重新计算
  int wdtTimeout = 10 * 60 * 1000; //设置10分钟 watchdog

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000 , false); //set time in us
  timerAlarmEnable(timer);

  // 初始化时钟模块
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    rebootESP();
  }

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Serial.println("boot ...");

  //10秒时间是等待主控初始化
  //不着急打开 sim7020c
  delay(10000);

  //boot_time = millis() / 1000;
  check_down_time = millis() / 1000;

  Serial.println(">>> 开启 nb-iot ...");

  //通过AT命令，检查是否未开机，如果关机，自动打开
  waker_7020();

  delay(10000);  //等待一会，确保网络连接上

  Serial.println(">>> 检查网络连接 ...");
  net_connect_succ = false;

  net_connect_succ = connect_nb();

  Serial.println("net_connect_succ:" + String(net_connect_succ));

  objManager_blue_to_hc08 = new Manager_blue_to_hc08();
  objGetWeather = new GetWeather();
  objcityWeather = new cityWeather();
  g_ink_showtxt = "";
  loop_num = 0;
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
      Serial.println("AT 命令不响应，开启sim7020");
      powerUp();
      net_connect_succ = false;
    }

    check_down_time = millis() / 1000 ;
    return ;
  }


  if (net_connect_succ )
  {
    //是否在设定时间
    String weather_time = Get_ds3231_time_weather(1);
    if (last_weather_show_time != Get_ds3231_time_weather(2) and  def_weather_time.indexOf(weather_time) > -1)
    {
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
        }

      }

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
