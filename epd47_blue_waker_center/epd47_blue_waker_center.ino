#include "BLEDevice.h"

//1.2M

char * blue_name = "edp47_ink";
//String buff_command[10];  //注意：此处是多定义几个备用，解析日期需要的数量较多

BLEClient*  pClient ;

uint32_t send_txt_cnt = 0;
uint32_t last_send_time = 0;

//hc08
static BLEUUID serviceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");
static BLEUUID    write_charUUID("0000ffe1-0000-1000-8000-00805f9b34fb");
//static BLEUUID    read_charUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

static BLERemoteCharacteristic* pRemoteCharacteristic;
//static BLERemoteCharacteristic* pRemoteCharacteristic_read;
static BLEAdvertisedDevice* myDevice;

boolean blue_doConnect = false;
int blue_doConnect_step = 0;
boolean blue_connected = false;
boolean blue_doScan = false;


int blue_cmd_state = 0;  //全局共享蓝牙命令处理状态
String blue_receive_txt = "";
uint32_t all_start_time = 0;

//会分好几次收到。。。
//blue_cmd_state=2只表示至少收到1次
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic_tmp,
  uint8_t* pData,
  size_t length,
  bool isNotify)
{
  Serial.print(">>>> 收到返回 <");
  char tmp_cArr[length + 1] = {0};
  memcpy((uint8_t *)tmp_cArr, pData, length);
  tmp_cArr[length] = '\0';
  Serial.print(pBLERemoteCharacteristic_tmp->getUUID().toString().c_str());
  Serial.print("> 长度=");
  Serial.println(length);
  Serial.println((char*)tmp_cArr);
  //blue_ask_work = false;
  blue_receive_txt = blue_receive_txt + String(tmp_cArr);

  //收到>>>结束标志，设置标志位
  if (blue_receive_txt.endsWith(">>>"))
  {
    blue_receive_txt.replace(">>>", "");
    blue_cmd_state = 3;
  }

  if (blue_cmd_state == 1)
    blue_cmd_state = 2;
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }

    void onDisconnect(BLEClient* pclient) {
      blue_connected = false;
      Serial.println("blue onDisconnect");
    }
};



bool connectToServer(bool first) {
  Serial.print("连接蓝牙地址: ");
  Serial.println(myDevice->getAddress().toString().c_str());

  if (first)
  {
    pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
  }

  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

  Serial.println(String("连接服务: ") + serviceUUID.toString().c_str());
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(String("查找特征-写入: ") + write_charUUID.toString().c_str());

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(write_charUUID );
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(write_charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  blue_connected = true;
  return true;
}

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    //打印找到的服务
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("查到蓝牙服务: ");
      Serial.println(advertisedDevice.toString().c_str());

      Serial.println(String("  ") + advertisedDevice.getAddress().toString().c_str());
      //  Serial.println(advertisedDevice.getServiceDataUUID().toString().c_str());   返回:NULL
      // Serial.println(advertisedDevice.getServiceUUID().toString().c_str());  打印异常！
      Serial.println("  " + String(advertisedDevice.getName().c_str()));
      if ( String(advertisedDevice.getName().c_str()).indexOf(blue_name) > -1)
      {
        Serial.println("找到蓝牙BLE服务");
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        blue_doConnect = true;
        blue_doScan = true;

      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks





void setup() {
  Serial.begin(115200);
  //WiFi.mode(WIFI_OFF);
  all_start_time = millis() / 1000;
  last_send_time =  millis() / 1000;

  Serial.println("Starting Arduino BLE Client application...");

  blue_doConnect_step = 0;

  //初始化蓝牙客户端
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  //找到服务后回调
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  Serial.println("setup end...");

}

//按hc-08蓝牙手册:
//  9600bps 波特率以下每个数据包不要超出 500 个字节，
//  发 20 字节间隔时间(ms) 20ms
//字串长于20字符必须特殊处理,分拆发送，否则只能发出部分数据
void send_cmd_long(String cmd)
{
  //将字串拆成20个字节一组多次发出
  char cArr[cmd.length() + 1];
  cmd.toCharArray(cArr, cmd.length() + 1);
  uint8_t * sound_bodybuff;
  sound_bodybuff = (uint8_t *)cArr;
  int all_send_num = 0; //已经发送出去的字节
  int send_num = 0;     //本次准备发送出去的字节
  while (all_send_num < cmd.length())
  {
    if (cmd.length() - all_send_num > 20)
      send_num = 20;
    else
      send_num =  cmd.length() - all_send_num ;
    pRemoteCharacteristic->writeValue(sound_bodybuff + all_send_num, send_num);
    all_send_num = all_send_num + send_num;
    delay(20);
  }
  Serial.println(">> 发出长度 " + String(cmd.length() ));
  Serial.println(">> 发出长度 all_send_num=" + String(all_send_num));
}


//蓝牙发送命令串
//delay_sec 最长等待秒数，超时
bool blue_send_cmd(String cmd, float delay_sec)
{
  bool ret = false;
  blue_receive_txt = "";

  Serial.println(">> 发出命令 \"" + cmd + "\"");
  //pRemoteCharacteristic->writeValue((uint8_t*)cmd.c_str(), cmd.length());
  send_cmd_long(cmd);

  uint32_t cmd_starttime = millis() / 1000; //蓝牙字符串命令发出时间
  blue_cmd_state = 1;   //命令发送状态

  while (true)
  {
    if (millis() / 1000 - cmd_starttime > delay_sec)
    {
      Serial.println("command " + cmd + " timeout! " + String(int(millis() / 1000 - cmd_starttime)) + "秒!");
      break;
    }
    if (millis() / 1000 < cmd_starttime)
      cmd_starttime = millis() / 1000;

    //检查此状态值是否修改成返回状态
    if (blue_cmd_state == 2)
    {
      ret = true;
      //保底信息，不跳出，要等到时间用完！
      //break;
    }
    else if (blue_cmd_state == 3)
    {
      ret = true;
      //跳出
      break;
    }

    yield();
    delay(10);
  }
  Serial.println("用时:" + String(millis() / 1000 - cmd_starttime) + "秒！");
  return ret;
}




void loop()
{
  if (millis() / 1000 < all_start_time)
    all_start_time = millis() ;
  if (millis() / 1000 < last_send_time)
    last_send_time = millis() ;


  //离上次执行超过60秒
  if (blue_doConnect_step == 11)
  {
    if (millis() / 1000 - last_send_time > 30)
    {
      bool tmpret = false;
      //10秒1次，反复尝试
      while (true)
      {
        Serial.println("重新连接 ble服务端");
        tmpret = connectToServer(false);
        if (tmpret) break;
        else
          delay(10 * 1000);
      }
      blue_doConnect_step = 1;
      last_send_time = millis() / 1000;
      all_start_time = millis() / 1000;
    }
    return;
  }

  //正常情况下从唤醒，蓝牙连接，到显示墨水屏信息,2-10秒足够了
  //如果30秒还未完成任务，直接进入唤醒状态，下次再来, 同时也能避免重启
  //  if ((millis() / 1000 - all_start_time) > 30)
  //  {
  //    Serial.println("task time out!");
  //    blue_doConnect_step = 10;
  //  }


  //0.蓝牙服务器连接中状态
  if (blue_doConnect_step == 0)
  {
    //仅连接一次，查询一次服务列表
    if (blue_doConnect == true) {
      if (connectToServer(true)) {
        Serial.println("检索 BLE Server 服务成功！");
      } else {
        Serial.println("检索 BLE Server 服务失败！");
      }
      blue_doConnect = false;
    }

    if (blue_connected)
    {
      blue_doConnect_step = 1;
    }
    else if (blue_doScan)
    {
      //触发注册特征
      BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
    }
  }
  //1.蓝牙连接上状态
  else if (blue_doConnect_step == 1 )
  {
    //发出唤醒命令，远程唤醒蓝牙服务端
    //时间要略长一点，太快有可能蓝牙端重启没收到信息,最少要2秒，否则服务端收不到数据
    bool ret = blue_send_cmd("waker", 2);
    //delay(500);
    Serial.println(">>>" + blue_receive_txt);

    send_txt_cnt = send_txt_cnt + 1;
    //ret = blue_send_cmd("center:" + String(send_txt_cnt), 5);
    //ret = blue_send_cmd("192.168.1.115 2021-02-06 19:12 N=" + String(send_txt_cnt) + ">>>", 5);
    ret = blue_send_cmd("树莓派Pico的开发环境是基于树莓派3B/4B来设计的,国内已经有爱好者将其适配到了Ubuntu>>>", 5);
    Serial.println(">>>" + blue_receive_txt);

    //全部任务执行完毕，可以开始休眠
    blue_doConnect_step = 10;

  }

  //10. 所有步骤处理完，进入休眠
  else if (blue_doConnect_step == 10)
  {
    Serial.println("关闭 ble连接");
    delay(1000);
    pClient->disconnect();  //断开蓝牙连接
    last_send_time = millis() / 1000;

    Serial.println("命令用时:" + String(millis() / 1000 - all_start_time) + "秒！");
    blue_doConnect_step = 11;
  }
  delay(10); // Delay a second between loops.
}
