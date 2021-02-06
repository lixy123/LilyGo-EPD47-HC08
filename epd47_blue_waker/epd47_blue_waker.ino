#include <HardwareSerial.h>
#include "epd_driver.h"
#include "firasans.h"

HardwareSerial MySerial(1);
uint32_t wakeup_time = 0;
//墨水屏缓存区
uint8_t *framebuffer;


/*
  功能：
    随时唤醒休眠中的lilygo-epd47墨水屏显示文字信息

  数据指标:
    1.利用有限的电池电量最大化节能，休眠时整体电流<1ma，18650电池平均能用1-3个月
    2.唤醒时间<1秒

  硬件说明
  1. lilygo-epd47墨水屏
  2. hc-08 BLE4.0蓝牙模块 (购买时要询问是告诉卖方要双晶振的，否则不支持一级节能模式!)
     hc-08设置成客户模式，一级节能模式,其蓝牙名称最好用官方工具修改下,防止被别的设备误连，
     建议名称：edp47_ink
     注：官方数据，一级节能模式电流约 6μA ~2.6mA （待机） /1.6mA（工作）
        相对于全速模式 8.5mA（待机）/9mA（待机） 节能效果明显
        hc-08模块每日大部分时间应处于在 6μA ~2.6mA （待机）模式,理论电流消耗极低
  3.接线
     lilygo-epd47  hc-08
       VCC         VCC
       12          TX
       13          RX
       GND         GND
  注：墨水屏进入节能休眠模式后，顶端12，13引脚处的VCC的电压输出会关闭，不能在此处取电，要从dh2.0或18650处取电

  电流实测:
  1.休眠： <1ma （客户端蓝牙发完信息要断开，否则墨水屏的蓝牙模块不能进入休眠，电流约9ma)
  2.唤醒后: 50-60ma
  蓝牙模块官方数据提到待机电流约6μA ~2.6mA来看，墨水屏待机电流约0.1ma(口头询问，官方数据未找到)，估算平均电流0.5ma
  1200ma的电池约能用 1200*0.8/24/0.4=80天,满足预期
  注：不能由USB供电，内部电路会导致休眠时也要耗电80ma, 达不到节能目的,必须由dh2.0或18650电池供电


  编译后固件大小:
     2.6M, 分区类型必须用Huge APP选项

*/

void showtxt(String txt)
{
  epd_poweron();
  volatile uint32_t t1 = millis();
  epd_clear();
  volatile uint32_t t2 = millis();
  printf("EPD clear took %dms.\n", t2 - t1);

  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  int cursor_x = 200;
  int cursor_y = 300;

  cursor_x = 200;
  cursor_y += 50;
  writeln((GFXfont *)&FiraSans, (char *)txt.c_str(), &cursor_x, &cursor_y, NULL);
  epd_poweroff();
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  // 可用  RX,TX

  epd_init();

  //framebuffer = (uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    Serial.println("alloc memory failed !!!");
    delay(1000);
    while (1);
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  //                               RX, TX
  MySerial.begin(9600, SERIAL_8N1, 12, 13);


  //蓝牙串口唤醒的字串不能直接用，前面几个字节的数据会乱码，需要丢掉
  //需要约定，先随意发一串数据，过>200ms后再正式发送信息
  clear_uart(200);
  wakeup_time = millis() / 1000;
  Serial.println("start");
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
    while (MySerial.available())
    {
      ch = (char) MySerial.read();
      Serial.print(ch);
    }
    yield();
    delay(5);
  }
}

void loop() {

  if (millis() / 1000 < wakeup_time)
    wakeup_time = millis() / 1000;

  //唤醒n秒后， 如果无工作，进入休眠模式，直到uart有信号再重新唤醒
  if (millis() / 1000 - wakeup_time >=15)
  {
    Serial.println("sleep...");
    delay(100);
    //不调用此句无法节能, 此句必须在深度休眠起作用，轻度休眠无效！
    epd_poweroff_all();

    //必须蓝牙断开，否则休眠，马上唤醒！最好加判断！
    
    //唤醒后n秒进入休眠
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 0);
    //第一个参数：RTCIO的编号
    //第二个参数：是高电平唤醒还是低电平唤
    //esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0);

    esp_deep_sleep_start();

    //esp_light_sleep_start();
    return;
  }
  if (millis() / 1000 < wakeup_time)
    wakeup_time = millis() / 1000 ;

  while (MySerial.available())
  {
    String tmpstr = "";
    char ch;
    while (MySerial.available())
    {
      ch = (char) MySerial.read();
      tmpstr = tmpstr + ch;
      //Serial.print(ch);
    }
    Serial.println(">>>" + tmpstr);
    wakeup_time = millis() / 1000; //工作状态，休眠时间重新计时

    MySerial.print("ok:" + tmpstr);
    showtxt(tmpstr);
  }

  delay(2000);
  Serial.print(".");
}
