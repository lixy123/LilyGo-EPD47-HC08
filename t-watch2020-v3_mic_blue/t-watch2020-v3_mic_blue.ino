#include "CloudSpeechClient.h"
#include "I2S.h"
#include "time.h"
#include "esp_system.h"
#include <WebServer.h>
#include <Preferences.h>

#include "ble_to_hc08.h"

//文件编译大小: 2.2M

//点阵中文字库每行显示字数计算
//  240/36=6
//  240/24=8
//  240/18=9
//#include "u8g2_msyh36.h"
#include "u8g2_msyh18.h"

#include "memo_historyManager.h"


#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>  //必须加上,否则AP模式 配置参数会有问题

//百度短语音识别可以将 60 秒以下的音频识别为文字。适用于语音对话、语音控制、语音输入等场景。
//现在服务已不免费，如账号欠费，将报错:
//{"err_msg":"request pv too much","err_no":3305,"sn":"949924248261612088653"}

TFT_eSPI *tft;   // tft instance
U8G2_FOR_ADAFRUIT_GFX u8f;       // U8g2 font instance
AXP20X_Class *power;
memo_historyManager* objmemo_historyManager;
bool irq = false;

Manager_blue_to_hc08* objManager_blue_to_hc08;

String last_voice = "";
const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "ESP32SETUP";
boolean settingMode = false;
String ssidList1;
String ssidList2;

//Preferences 的参数重烧固件会仍会存在！
//配置参数：


String report_address = "";  //如果配置有值,识别到的文字传给树莓派...
String report_url = "";      //如果配置有值,识别到的文字传给树莓派...

String dog_delay;     //定时狗健康状态监控，多少秒如果程序不在主循环上 esp32自动重启

String baidu_key;     //百度语音账号key
String baidu_secert;  //百度语音账号秘密码 注意：名称过长会有问题


String volume_low; //静音音量标准   (暂未用)
String volume_high ; //噪音音量标准 (过滤杂音用，暂未用)
String volume_double ; //音量乘以倍数

String define_max1 ;  //音量波峰（最高的一个值)
String define_avg1 ;  //音量平均值

String define_max2 ;  //和上面的2值的含义一样，区别是唤醒的音量标准会更大一些，在录音时静音的标准略低
String define_avg2;

String pre_sound;  //预缓冲秒数 2秒足够
String skip_baidu; //声音长度过短跳过识别 1==yes 0==no
String loopsleep;  //每次调用百度文字识别后的休息时间,防止过度频繁调用百度服务

String wifi_ssid1 ;
String wifi_password1  ;


WebServer webServer(80);
Preferences preferences;


hw_timer_t *timer = NULL;
const char* ntpServer = "ntp1.aliyun.com";
const long  gmtOffset_sec = 3600 * 8;
const int   daylightOffset_sec = 0;
long  last_check = 0;

const int record_time = 10;  // 录音秒数
const int waveDataSize = record_time * 32000 ;
const int numCommunicationData = 8000;
//数组：8000字节缓冲区
char communicationData[numCommunicationData];   //1char=8bits 1byte=8bits 1int=8*2 1long=8*4
long writenum = 0;

struct tm timeinfo;

CloudSpeechClient* cloudSpeechClient;
TTGOClass           *ttgo = nullptr;


//文字显示到TFT上
//用lvgl结全自建中文字库显示有bug, 内部字库在汉字过多或者过大时，编显示汉字错位
//最终改成用U8g2_for_TFT_eSPI显示汉字，显示效果正常
void showtft(String rec_text, String log_text)
{
  //黑屏
  tft->fillScreen(TFT_BLACK);          // clear the graphics buffer

  //坐标位置,汉字左下角
  //u8f.setCursor(0, 40);               // start writing at this position
  //u8f.print(log_text + "\n" + rec_text);

  int cursor_x = 0;
  int cursor_y = 30;

  objmemo_historyManager->clear_list();
  objmemo_historyManager->multi_append_txt_list(log_text + "\n" + rec_text);

  String now_string;
  int i;
  //全部显示
  for ( i = 0; i < objmemo_historyManager->memolist.size(); i++)
  {
    now_string = objmemo_historyManager->memolist.get(i);
    if (now_string.length() > 0)
    {
      u8f.drawUTF8(cursor_x, cursor_y, now_string.c_str());
      cursor_y = cursor_y + 30;
    }
  }
}

void printparams()
{
  // return;
  Serial.println(" report_address: " + report_address);
  Serial.println(" report_url: " + report_url);
  Serial.println(" baidu_key: " + baidu_key);
  Serial.println(" baidu_secert: " + baidu_secert);

  Serial.println(" dog_delay: " + dog_delay);

  Serial.println(" volume_low: " + volume_low);
  Serial.println(" volume_high: " + volume_high);
  Serial.println(" volume_double: " + volume_double);

  Serial.println(" define_max1: " + define_max1);
  Serial.println(" define_avg1: " + define_avg1);

  Serial.println(" define_max2: " + define_max2);
  Serial.println(" define_avg2: " + define_avg2);

  Serial.println(" pre_sound: " + pre_sound);
  Serial.println(" skip_baidu: " + skip_baidu);
  Serial.println(" loopsleep: " + loopsleep);
  Serial.println(" wifi_ssid1: " + wifi_ssid1);
  Serial.println(" wifi_password1: " + wifi_password1);
}


void writeparams()
{
  Serial.println("Writing params to EEPROM...");

  printparams();


  preferences.putString("report_address", report_address);
  preferences.putString("report_url", report_url);


  preferences.putString("baidu_key", baidu_key);
  preferences.putString("baidu_secert", baidu_secert);


  preferences.putString("dog_delay", dog_delay);

  preferences.putString("volume_low", volume_low);
  preferences.putString("volume_high", volume_high);
  preferences.putString("volume_double", volume_double);


  preferences.putString("define_max1", define_max1);
  preferences.putString("define_avg1", define_avg1);

  preferences.putString("define_max2", define_max2);
  preferences.putString("define_avg2", define_avg2);


  preferences.putString("pre_sound", pre_sound);
  preferences.putString("skip_baidu", skip_baidu);
  preferences.putString("loopsleep", loopsleep);

  preferences.putString("wifi_ssid1", wifi_ssid1);
  preferences.putString("wifi_password1", wifi_password1);

  Serial.println("Writing params done!");
}

bool readparams()
{
  report_address = preferences.getString("report_address");

  //如果这个值还没有，说明本机器没有配置默认
  if (report_address == "")
  {
    Serial.println("首次运行，配置默认参数");

    report_address = "192.168.1.20";


    report_url = "/chat";

    baidu_key = "";
    baidu_secert =  "";


    volume_low = "15";    //静音音量值
    volume_high = "5000"; //噪音音量值
    volume_double = "15"; //音量乘以倍数  PDM的mic用40有问题，建议用10-15

    //实测平时正常
    //high_max:0 val_avg:0
    define_max1 = "250";
    define_avg1 = "20";

    define_max2 = "150";
    define_avg2 =  "15";


    dog_delay = "10";

    pre_sound = "2";
    skip_baidu = "1";
    loopsleep = "2";  //每识别完文字后等待时间

    wifi_ssid1 = "";
    wifi_password1 = "";
    writeparams();
    printparams();
    return false;
  }


  report_address = preferences.getString("report_address");
  report_url =  preferences.getString("report_url");
  baidu_key = preferences.getString("baidu_key");
  baidu_secert = preferences.getString("baidu_secert");
  dog_delay = preferences.getString("dog_delay");

  volume_low = preferences.getString("volume_low");
  volume_high = preferences.getString("volume_high");
  volume_double = preferences.getString("volume_double");

  define_max1 = preferences.getString("define_max1");
  define_avg1 = preferences.getString("define_avg1");

  define_max2 = preferences.getString("define_max2");
  define_avg2 = preferences.getString("define_avg2");


  pre_sound = preferences.getString("pre_sound");
  skip_baidu = preferences.getString("skip_baidu");
  loopsleep = preferences.getString("loopsleep");

  wifi_ssid1 = preferences.getString("wifi_ssid1");
  wifi_password1 = preferences.getString("wifi_password1");

  printparams();
  return true;
}




void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  //esp_restart_noos(); 旧api

  esp_restart();
}

String GetLocalTime()
{
  String timestr = "";
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (timestr);
  }
  timestr = String(timeinfo.tm_mon + 1) + "-" + String(timeinfo.tm_mday) + " " +
            String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min) + ":" + String(timeinfo.tm_sec) ;
  return (timestr);
}



int16_t max(int16_t a, int16_t b)
{
  if (a > b) return a;
  else return b;
}

int16_t min(int16_t a, int16_t b)
{
  if (b > a) return a;
  else return b;
}

//每半秒一次检测噪音
bool wait_loud()
{

  String timelong_str = "";
  float val_avg = 0;
  int16_t val_max = 0;

  int32_t tmpval = 0;
  int16_t val16 = 0;
  uint8_t val1, val2;
  bool aloud = false;
  Serial.println(">");
  int32_t loop_sound_rec = 0;
  while (true)
  {
    loop_sound_rec = loop_sound_rec + 1;
    //每25秒处理一次即可
    if (loop_sound_rec % 100 == 0)
      timerWrite(timer, 0); //reset timer (feed watchdog)

    //读满缓冲区8000字节
    //此函数会自动调节时间，只要后续的操作不要让缓冲区占满即可
    //1/4秒 8000字节 4000次
    I2S_Read(communicationData, numCommunicationData);

    for (int loop1 = 0; loop1 < numCommunicationData / 2 ; loop1++)
    {

      val1 = communicationData[loop1 * 2];
      val2 = communicationData[loop1 * 2 + 1] ;
      val16 = val1 + val2 *  256;

      //不确认所有这产品的偏差值是否都是1570
      //计算偏差（不要原样存入wav,否则播放有问题）
      val16 = val16 + 1570;

      //pdm 麦克风只分析负数值
      if (val16 < 0)
      {
        val_avg = val_avg + abs(val16);
        //正负数都比较，靠谱
        val_max = max( val_max, abs(val16));
      }


      //经验数据：乘以40 ：音量提升20db
      tmpval = val16 * volume_double.toInt();
      if (abs(tmpval) > 32767 )
      {
        if (val16 > 0)
          tmpval = 32767;
        else
          tmpval = -32767;
      }
      //Serial.println(String(val1) + " " + String(val2) + " " + String(val16) + " " + String(tmpval));
      communicationData[loop1 * 2] =  (byte)(tmpval & 0xFF);
      communicationData[loop1 * 2 + 1] = (byte)((tmpval >> 8) & 0xFF);

    }

    //填充声音信息到缓存区 (配置:2秒或n秒)
    //每个缓存环存满了换下一个缓存环
    cloudSpeechClient->pre_push_sound_buff((byte *)communicationData, numCommunicationData);


    //半秒检查一次  16000字节 8000次数据记录
    if (loop_sound_rec % 2 == 0 )
    {
      //乘以2,是正负对半原理
      val_avg = val_avg * 2 / numCommunicationData;

      //判断是否噪音指标： 峰值， 平均值
      if ( val_max > define_max1.toInt() && val_avg > define_avg1.toInt())
        //if ( val_max > define_max1.toInt())
        aloud = true;
      else
        aloud = false;

      if (aloud)
      {
#ifdef SHOW_DEBUG

        timelong_str = ">>>>> high_max:" + String(val_max) +  " high_avg:" + String(val_avg) ;
        Serial.println(timelong_str);
#endif
        last_voice =  String(val_max) +  "|" + String(val_avg)  ;
        break;
      }
      val_avg = 0;
      val_max = 0;


      //防止溢出
      if (loop_sound_rec >= 100000)
        loop_sound_rec = 0;
    }
  }
  last_check = millis() ;
  return (true);
}

int record_sound()
{
  uint32_t all_starttime;
  uint32_t all_endtime;
  uint32_t timelong = 0;
  String timelong_str = "";
  uint32_t last_starttime = 0;

  float val_avg = 0;
  int16_t val_max = 0;
  //int32_t all_val_zero = 0;
  int16_t val16 = 0;
  uint8_t val1, val2;
  bool aloud = false;
  int32_t tmpval = 0;
  int all_alound;
  writenum = 0;

  //int loop1=0;
  //初始化0
  cloudSpeechClient->sound_bodybuff_p = 0;

  //用双声道，32位并没什么关系，因为拷数据时间很快！完全不占用多少时间！
  //Serial.println("record start 16k,16位,单声道");
  Serial.println( ">" + GetLocalTime() + " 开始录音 声音格式:16khz 16位 单声道， 静音检测，最长10秒结束...");

  //Serial.println(GetLocalTime() + "> " + "record... 反应时间:" + String(millis() - last_check) + "毫秒");

  //Serial.println("");
  // last_press = millis() / 1000;
  all_starttime = millis() / 1000;
  last_starttime = millis() / 1000;

  timerWrite(timer, 0); //reset timer (feed watchdog)


  //反复循环最长时间I2S录音
  for (uint32_t j = 0; j < waveDataSize / numCommunicationData; ++j) {
    //timelong_str = "";
    // Serial.println("loop");
    //读满缓冲区8000字节
    //此函数会自动调节时间，只要后续的操作不要让缓冲区占满即可
    I2S_Read(communicationData, numCommunicationData);

    //timelong_str = timelong_str + "," + j;
    //Serial.println(timelong_str);

    //平均值，最大值记录，检测静音参数用
    for (int loop1 = 0; loop1 < numCommunicationData / 2 ; loop1++)
    {
      val1 = communicationData[loop1 * 2];
      val2 = communicationData[loop1 * 2 + 1] ;
      val16 = val1 + val2 *  256;


      //不确认所有这产品的偏差值是否都是1570
      //计算偏差（不要原样存入wav,否则播放有问题）
      val16 = val16 + 1570;

      //pdm麦克风只分析负值
      if (val16 < 0)
      {
        val_avg = val_avg + abs(val16);
        val_max = max( val_max, abs(val16));
      }



      //乘以40 ：音量提升20db
      tmpval = val16 * volume_double.toInt();
      if (abs(tmpval) > 32767 )
      {
        if (val16 > 0)
          tmpval = 32767;
        else
          tmpval = -32767;
      }
      communicationData[loop1 * 2] =  (byte)(tmpval & 0xFF);
      communicationData[loop1 * 2 + 1] = (byte)((tmpval >> 8) & 0xFF);

    }

    //声音信息保存至缓冲区
    cloudSpeechClient->push_bodybuff_buff((byte*)communicationData, numCommunicationData);

    writenum = writenum + numCommunicationData;
    //半秒检查一次静音  16000字节 8000次数据记录
    if (j % 2 == 1)
    {
      val_avg = val_avg * 2 / numCommunicationData ;

      //if ( val_max > define_max2.toInt() && val_avg > define_avg2.toInt()  )
      if ( val_max > define_max2.toInt() )
        aloud = true;
      else
        aloud = false;

      if (aloud)
      {
        all_alound = all_alound + 1;
        //录音过程中，调试输出不要轻易用，会影响识别率！
        //timelong_str = ">>>>> " + String( millis() / 1000 - all_starttime) + String("秒 ");
        //timelong_str = timelong_str + " high_max:" + String(val_max) +  " high_avg:" + String(val_avg) ;
        //Serial.println(timelong_str);
        last_starttime = millis() / 1000;
      }

      val_avg = 0;
      val_max = 0;

    }

    //3秒仍静音，中断退出
    if ( millis() / 1000 - last_starttime > 3)
    {
#ifdef SHOW_DEBUG
      Serial.println("检测到连续3秒静音，退出录音");
#endif
      break;
    }
  }
  all_endtime = millis() / 1000;

#ifdef SHOW_DEBUG
  Serial.println("文件字节数:" + String(writenum) + ",理论秒数:" + String(writenum / 32000) + "秒") ;
  Serial.println("录音结束,时长:" + String(all_endtime - all_starttime) + "秒" );
#endif
  return (all_alound);
}

//如果flag 1 必须连接才算over,  如果为0 只试30秒
bool connectwifi(int flag)
{

  if (wifi_ssid1.length() == 0)
  {
    showtft("", "路由连接参数没设置");
    delay(2000);
    return false;
  }

  if (WiFi.status() == WL_CONNECTED) return true;

  showtft("", "连接路由:" + wifi_ssid1 );

  while (true)
  {
    if (WiFi.status() == WL_CONNECTED) break;

    int trynum = 0;
    Serial.print("Connecting to ");

    Serial.println(wifi_ssid1);

    //静态IP有时会无法被访问，原因不明！
    WiFi.disconnect(true); //关闭网络
    WiFi.mode(WIFI_OFF);
    delay(1000);
    WiFi.mode(WIFI_STA);

    WiFi.begin(wifi_ssid1.c_str(), wifi_password1.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(2000);
      Serial.print(".");
      trynum = trynum + 1;
      //30秒 退出
      if (trynum > 14) break;
    }
    if (flag == 0) break;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected with IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.subnetMask());
    Serial.println(WiFi.dnsIP(0));
    Serial.println(WiFi.dnsIP(1));
    return true;
  }
  else
    return false;
}

//计算显示的单个字的长度
int GetCharwidth(String ch)
{
  int wTemp = u8f.getUTF8Width(ch.c_str());
  return (wTemp);
}




void begin_recordsound()
{
  Serial.println("最长录音10秒，检测静音会提前结束" );
  //调用录音函数，直到结束
  int rec_ok = record_sound();
  String retstr = "";

  //录入声音都是静音
  if (skip_baidu == "1" && rec_ok == 0)
  {
#ifdef SHOW_DEBUG
    Serial.println("无用声音信号...");
#endif
    return;
  }
  else
  {
#ifdef SHOW_DEBUG
    Serial.println("进行文字识别" );
#endif

    showtft("", "2.百度识别");

    uint32_t all_starttime = millis() / 1000;
    String VoiceText = cloudSpeechClient->getVoiceText();
    Serial.println("识别用时: " + String ( millis() / 1000 - all_starttime) + "秒" );

    int error = 0;

    if (VoiceText.indexOf("speech quality error") > -1)
    {
      Serial.println("find speech quality error");
      error = 1;
    }

    VoiceText.replace("speech quality error", "");
    VoiceText.replace("。", "");

    //如果识别到文字，对文字进行处理
    if (VoiceText.length() > 0)
    {
      record_succ(VoiceText);
    }
    else
      showtft("", "");
  }
}

//成功识别文字后的处理(VoiceText 不会为空)
void record_succ(String VoiceText)
{
  //if (VoiceText.length() == 0) return;

  String retstr = "";
  //每个汉字占3个长度
  Serial.println(String(">>>  识别结果:") + GetLocalTime() + "> " + VoiceText + " len=" + VoiceText.length());

  //1.文字输出到TFT
  showtft( VoiceText, "3.识别完成");

  //平均上传时间应该<1秒
  //wav文件备份（先不考虑做成配置)
  /*
    Serial.println("wav upload...");
    String machine_id = "1";
    bool ret = cloudSpeechClient->uploadfile(report_address, 9999, String(cloudSpeechClient->recordfile) + "_bak" + machine_id + ".wav");
    if (ret == true)
    {
      Serial.println("wav upload success");
     showtft( VoiceText, "4.wav上传成功");
    }
    else
    Serial.println("wav upload fail");
  */

  showtft( VoiceText, "4.蓝牙传送");
  Serial.println("sendmsg bluetooth...");
  objManager_blue_to_hc08->blue_connect_sendmsg(VoiceText, false);
  showtft( VoiceText, "5.蓝牙传送完成");
}




void setupMode() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("scanNetworks");

  ssidList1 = "";

  for (int i = 0; i < n; ++i) {
    ssidList1 += "<option value=\"";
    ssidList1 += WiFi.SSID(i);
    ssidList1 += "\"";

    if (WiFi.SSID(i) == wifi_ssid1)
      ssidList1 += " selected ";
    ssidList1 += ">";
    ssidList1 += WiFi.SSID(i);
    ssidList1 += "</option>";
  }
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  WiFi.mode(WIFI_MODE_AP);
  startWebServer();
  Serial.println("Starting Access Point at \"" + String(apSSID) + "\"");
  //ShowTft( "");
  showtft( "", "连接路由器ESP32SETUP,访问http://192.168.4.1设置参数");
}


String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String new_urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}


//配置模式在笔记本电脑win10上 192.168.4.1 ping不通，网页打不开
//但是使用手机可以连接此ip,可能是win10安全机制问题？
void startWebServer() {

  //设置模式
  //if (settingMode) {
  //if (true)
  //{
  Serial.print("Starting Web Server at ");
  if (settingMode)
    Serial.println(WiFi.softAPIP());
  else
    Serial.println(WiFi.localIP());
  //设置主页

  // readparams();
  webServer.on("/settings", []() {
    String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
    s += "<form method=\"get\" action=\"setap\">" ;

    s += " <hr>";
    s += "<label>SSID: </label><select style=\"width:200px\"  name=\"wifi_ssid1\" >" + ssidList1 +  "</select>";
    s += "Password: <input name=\"wifi_password1\" style=\"width:100px\"  value='" + wifi_password1 + "' type=\"text\">";
    s += "<hr>";

    s += "<br>report_address: <input name=\"report_address\" style=\"width:350px\" value='" + report_address + "'type=\"text\">";
    s += "<br>report_url: <input name=\"report_url\" style=\"width:350px\"  value='" + report_url + "'type=\"text\">";

    s += "<br>baidu_key: <input name=\"baidu_key\" style=\"width:350px\"  value='" + baidu_key + "'type=\"text\">";
    s += "<br>baidu_secert: <input name=\"baidu_secert\" style=\"width:350px\"  value='" + baidu_secert + "'type=\"text\">";


    s += " <hr>";
    s += "<br>dog_delay: <input name=\"dog_delay\" style=\"width:100px\"  value='" + dog_delay + "'type=\"text\">mins";

    s += "<br>volume_low: <input name=\"volume_low\" style=\"width:100px\"  value='" + volume_low + "'type=\"text\">";
    s += "volume_high: <input name=\"volume_high\" style=\"width:100px\"  value='" + volume_high + "'type=\"text\">";
    s += "volume_double: <input name=\"volume_double\" style=\"width:100px\"  value='" + volume_double + "'type=\"text\">";

    s += "<br>define_max1: <input name=\"define_max1\" style=\"width:100px\"  value='" + define_max1 + "'type=\"text\">";
    s += "define_avg1: <input name=\"define_avg1\" style=\"width:100px\"  value='" + define_avg1 + "'type=\"text\">";
    s += "<br>define_max2: <input name=\"define_max2\" style=\"width:100px\" value='" + define_max2 + "'type=\"text\">";
    s += "define_avg2: <input name=\"define_avg2\" style=\"width:100px\" value='" + define_avg2 + "'type=\"text\">";

    s += "<br>pre_sound:<input name=\"pre_sound\" style=\"width:100px\" value='" + pre_sound + "'type=\"text\">";
    if (skip_baidu == "")
      s += "skip_baidu: <select name=\"skip_baidu\" ><option  value=\"1\" selected>yes</option> <option  value=\"0\">no</option>  </select>";
    else if (skip_baidu == "1")
      s += "skip_baidu: <select name=\"skip_baidu\" ><option  value=\"1\" selected>yes</option> <option  value=\"0\">no</option>  </select>";
    else
      s += "skip_baidu: <select name=\"skip_baidu\" ><option  value=\"1\">yes</option> <option  value=\"0\" selected>no</option>  </select>";
    s += "loopsleep:<input name=\"loopsleep\" style=\"width:100px\" value='" + loopsleep + "'type=\"text\">";



    s += "<br><input type=\"submit\"></form>";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  });
  //设置写入页(后台)
  webServer.on("/setap", []() {

    report_address = new_urlDecode(webServer.arg("report_address"));
    report_url = new_urlDecode(webServer.arg("report_url"));

    baidu_key = new_urlDecode(webServer.arg("baidu_key"));
    baidu_secert = new_urlDecode(webServer.arg("baidu_secert"));

    dog_delay = new_urlDecode(webServer.arg("dog_delay"));

    volume_low = new_urlDecode(webServer.arg("volume_low"));
    volume_high = new_urlDecode(webServer.arg("volume_high"));

    volume_double = new_urlDecode(webServer.arg("volume_double"));

    define_max1 = new_urlDecode(webServer.arg("define_max1"));
    define_avg1 = new_urlDecode(webServer.arg("define_avg1"));

    define_max2 = new_urlDecode(webServer.arg("define_max2"));
    define_avg2 = new_urlDecode(webServer.arg("define_avg2"));


    pre_sound = new_urlDecode(webServer.arg("pre_sound"));
    skip_baidu = new_urlDecode(webServer.arg("skip_baidu"));
    loopsleep = new_urlDecode(webServer.arg("loopsleep"));

    wifi_ssid1 = new_urlDecode(webServer.arg("wifi_ssid1"));
    wifi_password1 = new_urlDecode(webServer.arg("wifi_password1"));

    //   Serial.print("baidu_secert: " + baidu_secert);

    //写入配置
    writeparams();
    String wifi_ssid = "";
    String wifi_password = "";

    wifi_ssid = wifi_ssid1;
    wifi_password = wifi_password1;

    String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
    s += wifi_ssid;
    s += "\" after the restart.";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    delay(3000);
    ESP.restart();
  });
  webServer.onNotFound([]() {
    String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
    webServer.send(200, "text/html", makePage("AP mode", s));
  });

  webServer.begin();
}


void setup() {

  Serial.begin(115200);

  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS init failed");
    return;
  }
  Serial.println("SPIFFS init ok");

  objmemo_historyManager = new memo_historyManager();
  objmemo_historyManager->GetCharwidth = GetCharwidth;


  preferences.begin("wifi-config");
  readparams();


  //如果进入配置模式，10分钟后看门狗会让esp32自动重启
  int wdtTimeout = dog_delay.toInt() * 60 * 1000; //设置分钟 watchdog

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000 , false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();

  tft = ttgo->tft;
  power = ttgo->power;
  Serial.println("chinese font init");
  u8f.begin(*tft);
  u8f.setFontDirection(0);            // left to right (this is default)
  //白字
  u8f.setForegroundColor(TFT_WHITE);  // apply color
  //中文字库
  //u8f.setFont(FONT);
  //u8f.setFont(u8g2_msyh36);
  u8f.setFont(u8g2_msyh18);
  u8f.setFontMode(1);                 // use u8g2 transparent mode (this is default)

  //按钮休眠
  pinMode(AXP202_INT, INPUT_PULLUP);
  attachInterrupt(AXP202_INT, [] {
    irq = true;
  }, FALLING);

  // Must be enabled first, and then clear the interrupt status,
  // otherwise abnormal
  power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ,
                   true);

  //  Clear interrupt status
  power->clearIRQ();


  //有限模式进入连接，如果30秒连接不上，返回false
  bool ret_bol = connectwifi(0);

  //wifi连接不上，进入配置模式
  if (ret_bol == false)
  {
    settingMode = true;
    setupMode();
    // Serial.println("baidu_Token:" + baidu_Token);
    return;
  }

  //I2S_BITS_PER_SAMPLE_8BIT 配置的话，下句会报错，
  //最小必须配置成I2S_BITS_PER_SAMPLE_16BIT
  I2S_Init(I2S_MODE_RX, 16000, I2S_BITS_PER_SAMPLE_16BIT);

  showtft("", "连接百度服务器");
  cloudSpeechClient = new CloudSpeechClient(pre_sound.toInt());
  //此方法必须调用成功，否则语音识别会无法进行
  while (true)
  {
    String baidu_Token = cloudSpeechClient->getToken_http_client(baidu_key, baidu_secert);
    Serial.println("baidu_Token:" + baidu_Token);
    if (baidu_Token.length() > 0 )
      break;
    //30秒再试
    delay(30000);
  }

  //NTP 时间
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  objManager_blue_to_hc08 = new Manager_blue_to_hc08();

  Serial.println("start...");
  showtft("", "启动完成！");
}



void loop() {

  //如果按下按钮，进行休眠
  if  (irq) {
    Serial.println("sleep...");
    power->clearIRQ();
    // Set screen and touch to sleep mode
    ttgo->displaySleep();


#ifdef LILYGO_WATCH_2020_V1
    ttgo->powerOff();
    // LDO2 is used to power the display, and LDO2 can be turned off if needed
    // power->setPowerOutPut(AXP202_LDO2, false);
#else
    power->setPowerOutPut(AXP202_LDO3, false);
    power->setPowerOutPut(AXP202_LDO4, false);
    power->setPowerOutPut(AXP202_LDO2, false);
    // The following power channels are not used
    power->setPowerOutPut(AXP202_EXTEN, false);
    power->setPowerOutPut(AXP202_DCDC2, false);
#endif
    // Use ext1 for external wakeup
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);

    esp_deep_sleep_start();
  }

  //如果是配置模式，不录音，识音
  if (settingMode)
  {
    //处理网页服务（必须有)
    webServer.handleClient();
    return;
  }

  //检测到wifi失联，自动连接
  connectwifi(1);

  //检测是噪音（声音识别开始)
  wait_loud();

  showtft("", "1.录音识别开始");
  //进入录音及识别模式
  begin_recordsound();
  delay(loopsleep.toInt() * 1000);
}
