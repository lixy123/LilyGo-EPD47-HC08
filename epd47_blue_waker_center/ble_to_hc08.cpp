#include "ble_to_hc08.h"


BLEUUID serviceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");
BLEUUID    write_charUUID("0000ffe1-0000-1000-8000-00805f9b34fb");

String blue_receive_txt = "";
int blue_cmd_state = 0;    //全局共享蓝牙命令处理状态
bool blue_connected = false;

bool blue_doConnect = false;
char * blue_name = "edp47_ink";   //首次扫描时能扫描出其蓝牙地址+蓝牙名称，再次扫描蓝牙地址会扫描不出
//char * blue_name = "HC-42";   //首次扫描时能扫描出其蓝牙地址+蓝牙名称，再次扫描蓝牙地址会扫描不出

String  blue_server_address = "";  //所以将首次扫描记忆的蓝牙地址记忆下，供下次扫描时匹配用


String  search_waker_blue_server_address = "";  //这个地址仅用于连接，唤醒用

int scan_time = 60; //最长蓝牙扫描时间

BLEAdvertisedDevice* myDevice;       //需要被连接并发送文字的设备
BLEAdvertisedDevice* myDevice_waker; //需要被唤醒的设备
#ifdef blue_notify
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
#endif

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.println("blue onConnect");
    }

    void onDisconnect(BLEClient* pclient) {
      //blue_connected = false;
      Serial.println("blue onDisconnect");

      //目前蓝牙连接方法调用 pClient->connect(myDevice)有bug, 如连接失败会无限等待，程序阻塞
      //如果在调用蓝牙连接方法时出现此事件，且此变量为false
      //说明蓝牙连接在阻塞状态，无条件进行重启，防止
      if (blue_connected == false)
      {
        ets_printf("blue connect timeout, reboot\n");
        delay(100);
        //esp_restart_noos(); 旧api
        esp_restart();
      }

    }
};


/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    //打印找到的服务
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("查到蓝牙服务: ");
      Serial.println(advertisedDevice.toString().c_str());

      Serial.println(String("  ") + advertisedDevice.getAddress().toString().c_str());
      Serial.println("  " + String(advertisedDevice.getName().c_str()));

      //首次上电查询时，getName能获得正确名称，再次扫描时会扫不到名称
      //算法中记忆上次的地址，下次时直接用
      if  (search_waker_blue_server_address.length() > 0 )
      {
        if (String(advertisedDevice.getAddress().toString().c_str()) == search_waker_blue_server_address)
        {
          Serial.println("找到待唤醒的蓝牙BLE服务");
          
          myDevice_waker = new BLEAdvertisedDevice(advertisedDevice);
          blue_doConnect = true;
          BLEDevice::getScan()->stop();  //此句放在最后，否则 blue_doConnect = true; 可能来不及执行
        }
      }
      else if ( String(advertisedDevice.getName().c_str()) == blue_name ||
          ( blue_server_address.length() > 0 &&  String(advertisedDevice.getAddress().toString().c_str()) == blue_server_address)
      )
      {
        blue_server_address = advertisedDevice.getAddress().toString().c_str();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        blue_doConnect = true;        
        Serial.println("找到蓝牙BLE服务");
        BLEDevice::getScan()->stop();  //此句放在最后，否则 blue_doConnect = true; 可能来不及执行
      }

    } // onResult
}; // MyAdvertisedDeviceCallbacks


Manager_blue_to_hc08::Manager_blue_to_hc08() {
  //初始化蓝牙客户端
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  //找到服务后回调
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  //pBLEScan->start(5, false);
  Serial.println("blue_to_hc08 init");

}

Manager_blue_to_hc08::~Manager_blue_to_hc08() {
  //client.stop();
  //WiFi.disconnect();
}


//连接唤醒设备
bool Manager_blue_to_hc08::connectToServer_waker() {
  Serial.print("A.连接蓝牙地址: ");
  Serial.println(myDevice_waker->getAddress().toString().c_str());

  if (pClient_ok == false)
  {
    pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient_ok = true;
  }

  Serial.print("B.开始连接");

  //这条语句在连接不上时会一直阻塞，导致程序跑飞！！！
  //一般出现在上次连接成功，下次省去扫描步骤的情况，所以最好不要省去蓝牙扫描的步骤！
  pClient->connect(myDevice_waker);

  Serial.println(String("C.连接服务: ") + serviceUUID.toString().c_str());
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(String("D.查找特征-写入: ") + write_charUUID.toString().c_str());

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(write_charUUID );
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(write_charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  return true;
}


bool Manager_blue_to_hc08::connectToServer() {
  Serial.print("A.连接蓝牙地址: ");
  Serial.println(myDevice->getAddress().toString().c_str());

  if (pClient_ok == false)
  {
    pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient_ok = true;
  }

  Serial.print("B.开始连接");

  //这条语句在连接不上时会一直阻塞，导致程序跑飞！！！
  //一般出现在上次连接成功，下次省去扫描步骤的情况，所以最好不要省去蓝牙扫描的步骤！

  //调试发现偶尔有出现以下输出，然后阻塞的现象。。。
  //主程序有定时狗，10分钟后会自动重启跳过此问题...
  //lld_pdu_get_tx_flush_nb HCI packet count mismatch (0, 1)

  pClient->connect(myDevice);

  Serial.println(String("C.连接服务: ") + serviceUUID.toString().c_str());
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(String("D.查找特征-写入: ") + write_charUUID.toString().c_str());

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(write_charUUID );
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(write_charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

#ifdef blue_notify
  Serial.println("E.registerForNotify") ;
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);
#endif

  //blue_connected = true;
  return true;
}


//按hc-08蓝牙手册:
//  9600bps 波特率以下每个数据包不要超出 500 个字节，
//  发 20 字节间隔时间(ms) 20ms
//字串长于20字符必须特殊处理,分拆发送，否则只能发出部分数据
void Manager_blue_to_hc08::send_cmd_long(String cmd)
{
  //字节数超过250，特别处理，否则发送接收异常
  if (cmd.length() > 250)
  {
    send_cmd_long_long(cmd);
    return;
  }
  Serial.println("send_cmd_long,len=" + String(cmd.length()));
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
    delay(40);
  }
  Serial.println("send_cmd_long ok.");
}

//大于300字节情况，需要特别处理
//主要是1KB字节的天气JSON信息
void Manager_blue_to_hc08::send_cmd_long_long(String cmd)
{
  Serial.println("send_cmd_long_long,len=" + String(cmd.length()));
  //将字串拆成20个字节一组多次发出
  char cArr[cmd.length() + 1];
  cmd.toCharArray(cArr, cmd.length() + 1);
  uint8_t * sound_bodybuff;
  sound_bodybuff = (uint8_t *)cArr;
  int all_send_num = 0; //已经发送出去的字节
  int send_num = 0;     //本次准备发送出去的字节
  int loop1 = 0;
  while (all_send_num < cmd.length())
  {
    loop1 = loop1 + 1;
    Serial.println("writeValue loop1=" + String(loop1));
    if (cmd.length() - all_send_num > 20)
      send_num = 20;
    else
      send_num =  cmd.length() - all_send_num ;
    pRemoteCharacteristic->writeValue(sound_bodybuff + all_send_num, send_num);
    all_send_num = all_send_num + send_num;
    delay(40);

    //每发完5次及10次， 一次20字节休息一下！
    //1KB,分51次发送，30-40 秒发送完成， 这是发1kB数据调试的最少时间经验值，如果数据>1K，休息间隔秒数需调试
    if (loop1 % 5 == 0)
      delay(2000);
    if (loop1 % 10 == 0)
      delay(1000);
  }
  Serial.println("send_cmd_long_long ok.");
}



//蓝牙发送命令串
//delay_sec 最长等待秒数，超时
//delay_sec 最少等待秒数
bool Manager_blue_to_hc08::blue_send_cmd(String cmd, float delay_sec, int min_delay_sec)
{
  bool ret = false;
  blue_receive_txt = "";

  Serial.println(">> 发出命令 \"" + cmd + "\"");
  //pRemoteCharacteristic->writeValue((uint8_t*)cmd.c_str(), cmd.length());
  send_cmd_long(cmd);


#ifdef blue_notify
  uint32_t cmd_starttime = millis() / 1000; //蓝牙字符串命令发出时间
  blue_cmd_state = 1;   //命令发送状态

  while (true)
  {
    if (millis() / 1000 - cmd_starttime > delay_sec)
    {
      Serial.println("command [" + cmd + "] timeout! " + String(int(millis() / 1000 - cmd_starttime)) + "秒!");
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
#else
  Serial.println("sleep..." + String(min_delay_sec) + "秒!");
  delay(1000 * min_delay_sec);
#endif

  return true;
}




//注意算法,尽量不重新创建对象
bool Manager_blue_to_hc08::blue_connect_sendmsg(String txt, bool quick)
{
  uint32_t sendmsg_time = millis() / 1000;

  //唤醒蓝牙服务器址清空，防止混淆
  search_waker_blue_server_address = "";

  //不允许在非退出状态插入新任务
  if (blue_doConnect_step > 0) return false;

  blue_doConnect = false;
  blue_connected = false;
  blue_datasend = false;

  //1.扫描
  //注：如果扫描时已被其它蓝牙客户端连接，或者蓝牙服务器已关停， 扫描会找不到蓝牙服务端，这样能阻止下一步连接服务器造成错误
  //代价是每次扫描会占用一些时间。
  blue_doConnect_step = 1;
  Serial.println("1.开始蓝牙扫描目标服务器");

  //上次找到过蓝牙地址，跳过蓝牙扫描，省略会带来阻塞问题，最好不要省去此步！
  if (quick)
  {
    if (blue_server_address.length() > 0)
    {
      blue_doConnect = true;
      Serial.println("skip 蓝牙扫描");
    }
    else
    {
      pBLEScan->start(scan_time, false);  //阻塞式查找蓝牙设备
    }
  }
  else
    pBLEScan->start(scan_time, false);  //阻塞式查找蓝牙设备


  //Serial.println("blue_doConnect="+String(blue_doConnect));

  blue_doConnect_step = 2;
  //2.连接目标蓝牙服务器
  //有可能在连接时阻塞，建议用定时狗，如异常就重启！！！
  if (blue_doConnect == true) {
    Serial.println("2.连接目标蓝牙服务器");
    blue_connected = connectToServer();

  }

  blue_doConnect_step = 3;
  //3.蓝牙服务器连接上
  if (blue_connected == true)
  {
    Serial.println("3.向蓝牙服务器发送数据");
    //3.1发出唤醒命令，远程唤醒蓝牙服务端,最少要3秒
    bool ret = blue_send_cmd("waker", 3, 3);
    delay(500);
    //理论应不返回数据
    Serial.println(">>>" + blue_receive_txt);

    //3.2发送文本串数据
    //普通数据，给5秒时间就够
    //大于300字节，需要60秒
    if (txt.length() > 300)
      ret = blue_send_cmd(txt + ">>>", 60, 1);
    else
      ret = blue_send_cmd(txt + ">>>", 5, 1);

    Serial.println(">>>" + blue_receive_txt);

    //全部任务执行完毕，可以开始休眠
    blue_datasend = true;
  }

  blue_doConnect_step = 4;
  //4.关闭连接(如果连接上服务器)
  if (blue_connected == true)
  {
    Serial.println("4.蓝牙服务器断开");
    pClient->disconnect();
  }

  //非工作状态
  blue_doConnect_step = 0;

  //经验值：5-6秒(接收通知)
  //      3-4秒(不接收通知)
  Serial.println("blue_connect_sendmsg 总用时:" + String(millis() / 1000 - sendmsg_time) + "秒！");

  return blue_datasend;
}

//远程连接地址，最长蓝牙扫描时间，连接上蓝牙后停留时间
bool Manager_blue_to_hc08::waker_remote_blue(String remote_blue_address, int find_seconds, int delay_seconds)
{
  uint32_t sendmsg_time = millis() / 1000;
  blue_doConnect = false;
  blue_connected = false;
  search_waker_blue_server_address = remote_blue_address;  //仅连接本地址，其它的不连接

  //1.阻塞式查找蓝牙设备
  Serial.println("1 pBLEScan");
  pBLEScan->start(find_seconds, false);


  //2.如果找到指定地址的设备，进行连接
  if (blue_doConnect)
  {
    Serial.println("2 connectToServer_waker");
    blue_connected = connectToServer_waker();
  }

  //3.连接上，停顿n秒,并断开连接
  if (blue_connected)
  {
    Serial.println("3.1 sleep");
    delay(delay_seconds * 1000);
    Serial.println("3.2 蓝牙服务器断开");

    pClient->disconnect();
  }
  Serial.println("waker_remote_blue 总用时:" + String(millis() / 1000 - sendmsg_time) + "秒！");
  search_waker_blue_server_address = "";
  return blue_connected;
}
