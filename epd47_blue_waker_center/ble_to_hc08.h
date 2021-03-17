#include "BLEDevice.h"
#include <String.h>
#include <Arduino.h>

//定义是否接收蓝牙通知
//实际测试可以不接收蓝牙反馈字节，能节省1-2秒时间， 调试时可加上
//#define blue_notify 

class Manager_blue_to_hc08
{
  private:
    bool pClient_ok = false;
    int blue_doConnect_step = 0;

    bool blue_datasend = false;


    BLEScan* pBLEScan;
    BLEClient*  pClient ;

    BLERemoteCharacteristic* pRemoteCharacteristic;


    bool connectToServer() ;
    void send_cmd_long(String cmd);  //300字节内调用
    void send_cmd_long_long(String cmd); //大于300字节调用, 如用前一函数，会丢失或数据混乱
    bool blue_send_cmd(String cmd, float delay_sec, int min_delay_sec);

    bool connectToServer_waker(); //连接待唤醒的蓝牙服务器，仅供 waker_remote_blue调用
  public:
  
    Manager_blue_to_hc08();
    ~Manager_blue_to_hc08();

    //连接到此蓝牙地址，并发送文字信息
    //  quick 跳过蓝牙扫描这一步，直接连接, 实测没有减少连接时间，都是6秒左右.
    bool blue_connect_sendmsg(String txt, bool quick);
    
    //远程连接地址，最长蓝牙扫描时间，连接上蓝牙后停留时间
    bool waker_remote_blue(String remote_blue_address,int find_seconds, int delay_seconds); 
};
