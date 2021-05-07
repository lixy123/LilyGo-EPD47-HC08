#include "ble_to_hc08.h"

//编译后固件大小 1.2M
uint32_t send_txt_cnt = 0;
uint32_t last_send_time = 0;
bool first_loop = true;
uint32_t loop_work_time = 0;

hw_timer_t *timer = NULL;
Manager_blue_to_hc08* objManager_blue_to_hc08;


void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

void setup() {
  Serial.begin(115200);
  //WiFi.mode(WIFI_OFF)
  send_txt_cnt = 0;

  Serial.println("setup");

  //为防意外，n秒后强制复位重启，一般用不到。。。
  //n秒如果任务处理不完，看门狗会让esp32自动重启,防止程序跑死...
  //esp32蓝牙库有bug,当蓝牙信息不稳定时，在蓝牙连接过程中有一定机率堵塞，需要借助此处的dog进行复位
  int wdtTimeout = 10 * 60 * 1000; //设置n分钟 watchdog

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000 , false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt
  //timerAlarmDisable(timer);

  objManager_blue_to_hc08 = new Manager_blue_to_hc08();
  loop_work_time = millis() / 1000;
  delay(2000);
  Serial.println("start");
}



void loop()
{
  if ( millis() / 1000 < loop_work_time)
    loop_work_time = millis() / 1000;

  delay(1000);

  //每n秒蓝牙发送一次
  if (millis() / 1000 - loop_work_time > 60 || first_loop)
  {
    send_txt_cnt = send_txt_cnt + 1;
    first_loop = false;
    //蓝牙连接,发送信息有一定机率阻塞，
    //设置定时狗，当设定时间没完成任务说明阻塞，重启。
    //如果用wifi，尽量避免重启，因为wifi重启后有可能重连连接不上。

    //喂狗，防止在蓝牙交互中程序阻塞，
    //如阻塞，esp32有机会复位
    timerWrite(timer, 0);
    timerAlarmEnable(timer);

    // objManager_blue_to_hc08->blue_connect_sendmsg(String(send_txt_cnt),false);
    //快速连接模式，不蓝牙扫描，本开发库在连接时容易阻塞
    //假定条件：蓝牙服务端从不关闭，只被此设备独占连接条件下用快速连接模式，节省扫描时间和电池能量
    objManager_blue_to_hc08->blue_connect_sendmsg(String(send_txt_cnt), false);

    timerAlarmDisable(timer);
    loop_work_time = millis() / 1000;
  }

}
