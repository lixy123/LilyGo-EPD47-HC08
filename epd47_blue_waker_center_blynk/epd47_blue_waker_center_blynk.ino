#include <BlynkSimpleEsp32.h>
#include "ble_to_hc08.h"
#include "smartconfigManager.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

bool Blynk_run = false;
//注意: 这个key需要用手机APP安装blynk后创建项目获得，每个唯一硬件需要唯一的key
//说明见:http://m.elecfans.com/article/1003098.html
char auth[] = ""; 

/*
1.程序功能：
  ESP32连接blynk服务器，接收到手机发来的文字信息后通过蓝牙转发给墨水屏供显示
  墨水屏休眠状态，连接有低能耗BLE蓝牙设备hc08，通过蓝牙外置硬件完成被唤醒，接收文字信息功能
  hc08：一级节能模式电流约 6μA ~2.6mA （待机） /1.6mA（工作）

2.硬件：
   ESP32模块*1
   
3.软件及编译：
  arduino 1.8.13
  arduino-esp32 版本 1.0.6
  开发板：ESP32 DEV Module
  编译分区：HUGE APP
  编译后固件大小: 1.5M

4.用法：
  A.手机APP安装blynk, 
  B.新建项目,硬件:esp32，连接:wifi,添加如下2个小部件
     V4: Text Input, 字长限制改成200
     V6：Labeled Value, PUSH
  C.烧录完如上程序的esp32上电
  D.首次运行时ESP32需要配置配置wifi连接，代码中用到了 ESP32 SmartConfig 配网技术, 
    参考:https://www.zhangtaidong.cn/archives/124/ 微信扫描配置wifi网络.否则会每120秒重启,无法进入主功能.
  E.以上如果均正常，手机APP上启动新建的项目，看到esp32连接上线状态，在输和部件输入文字并回车，约5秒左右，文字会远程显示到墨水屏
  注： 手机APP连接的网不需要与esp32在同一个局域网内。  
     
4.其它
  blynk服务器在国外，偶尔会连接不上，用起来不可靠，可考虑自建国内的blynk云服务器，稳定性和速度会更好
  需要长连wifi，没法节能，电流约:60-80ma左右 
  
*/


hw_timer_t *timer = NULL;

uint32_t loop_num = 0; //dog用

smartconfigManager* objsmartconfigManager;

Manager_blue_to_hc08* objManager_blue_to_hc08;

String g_ink_showtxt = "";


void rebootESP() {
  Serial.print("Rebooting ESP32: ");
  delay(100);
  //ESP.restart();  左边的方法重启后连接不上esp32
  esp_restart();
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
  int wdtTimeout = 10 * 60 * 1000; //设置10分钟 watchdog

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000 , false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  //3.连接wifi
  objsmartconfigManager = new smartconfigManager();
  bool connect_ok = objsmartconfigManager->connectwifi();

  //只有setup连接成功，才会启用Blynk
  if (connect_ok == true)
  {
    Blynk_run = true;
    Serial.println("Blynk connect");
    Blynk.begin(auth, objsmartconfigManager->ssid.c_str(), objsmartconfigManager->password.c_str());
    Serial.println("Blynk success");
  }
  else
  {
    //连接不上wifi, 20秒后复位esp32
    delay(20*1000);
    rebootESP();
  }
  
  //打开蓝牙
  objManager_blue_to_hc08 = new Manager_blue_to_hc08();
  Serial.println("Blynk monit start...");
}


BLYNK_WRITE(V4)
{
  Serial.println(param.asString());
  Blynk.virtualWrite(V6, String("收到:") + param.asString());
  g_ink_showtxt=param.asString();  
}


void loop()
{
  delay(1000); // Delay a second between loops.

  //每30秒 feed dog 一次, 
  //蓝牙连接时如果阻塞，让esp32有机会重启
  loop_num = loop_num + 1;
  if (loop_num > 30)
  {
    // Serial.println("feed watchdog...");
    loop_num = 0;
    timerWrite(timer, 0); //reset timer (feed watchdog)
  }


  //此代码内部有自动连接机制
  //能让blynk自动wifi连接，并连接至blynk服务器(已测）
  Blynk.run();

  //如果有待发送数据，进行发送
  if (g_ink_showtxt.length() > 0)
  {
    //正常300字节内5秒发送完成
    bool ret_bool = objManager_blue_to_hc08->blue_connect_sendmsg(g_ink_showtxt, false);
    if (ret_bool)
      g_ink_showtxt = "";
    delay(5000);

    //调试用
    //delay(10000);
  }

}
