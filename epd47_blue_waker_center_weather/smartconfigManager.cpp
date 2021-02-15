#include "smartconfigManager.h"

smartconfigManager::smartconfigManager()
{
  //初始化配置类
  preferences.begin("wifi-config");
  readparams();
}

smartconfigManager::~smartconfigManager()
{
  //preferences.close();
}

void smartconfigManager::writeparams()
{
  Serial.println("Writing params to EEPROM... >" + ssid + " >" + password);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
}

void smartconfigManager::readparams()
{
  ssid = preferences.getString("ssid");
  password = preferences.getString("password");
  Serial.println("Read params >" + ssid + " >" + password);
}

//配网函数 限时try_num秒
//为防止无限等待，超时返回false
bool smartconfigManager::smartConfig(int try_num)
{
  int trytime = 0;
  Serial.println("\r\nWait for Smartconfig...");//串口打印
  WiFi.beginSmartConfig();//等待手机端发出的名称与密码
  //死循环，等待获取到wifi名称和密码
  while (1)
  {
    //等待,每秒计时一次
    Serial.print(".");
    delay(1000);
    trytime = trytime + 1;
    //超过计时，不再尝试
    if (trytime > try_num)
    {
      return false;
    }
    if (WiFi.smartConfigDone())//获取到之后退出等待
    {
      Serial.println("SmartConfig Success");
      //打印获取到的wifi名称和密码
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      //写入配置
      ssid = WiFi.SSID();
      password = WiFi.psk();
      writeparams();


      delay(1000);
      break;
    }
  }
  return true;
}



void smartconfigManager::connectwifi()
{
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.disconnect();
  delay(200);

  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WIFI");

  bool connect_ok = false;
  if (ssid.length() > 0)
  {
    int lasttime = millis() / 1000;
    WiFi.begin(ssid.c_str(), password.c_str());

    while ((!(WiFi.status() == WL_CONNECTED))) {
      delay(1000);
      Serial.print(".");

      //10秒连接不上，自动重启
      if ( abs(millis() / 1000 - lasttime ) > 10 )
      {
        Serial.println("WiFi连接失败，请用手机进行配网");
        connect_ok = smartConfig(120); //smartConfig技术配网,限时120秒
        break;
      }
    }
    connect_ok = true;
  }
  else
  {
    Serial.println("WiFi未配置，请用手机进行配网");
    connect_ok = smartConfig(120); //smartConfig技术配网,限时120秒
  }

  if (connect_ok)
  {
    Serial.println("Connected");
    Serial.println("My Local IP is : ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("连接失败，重启！");
    delay(100);
    esp_restart();
  }

}
