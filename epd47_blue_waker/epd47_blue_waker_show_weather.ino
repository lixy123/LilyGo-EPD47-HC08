#include <HardwareSerial.h>
//memo缓存管理
#include "memo_historyManager.h"
#include "hz3500_36.h"
#include "hz3500_16.h"

#include "showweather.h"


#define BUTTON_BOOT_PRESS  39

bool pin_link = false;
esp_sleep_wakeup_cause_t wakeup_reason;

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
     3.6M,
     开发板选择：TTGO-T-WATCH

  ArduinoJson使用版本6

*/
HardwareSerial MySerial(1);
uint32_t wakeup_time = 0;
//墨水屏缓存区
uint8_t *framebuffer;
memo_historyManager* objmemo_historyManager;
weatherManager* obweatherManager;

bool init_hard_ok = false;

hw_timer_t *timer = NULL;

String blue_receive_txt = "";
uint32_t blue_receive_txt_time = 0;
String buff_command[5];

bool check_pin()
{
  pinMode(BUTTON_BOOT_PRESS, INPUT);

  delay(500);
  int pin_link = digitalRead(BUTTON_BOOT_PRESS) ;

  if (pin_link == 0)
    return true;
  else
    return false;
}

//https://hackaday.io/project/168668-paperdink/log/173710-release-v01
//https://github.com/rgujju/paperdink/blob/master/Software/paperd.ink/GUI.cpp
//days_of_month 本月总天数
//day_offset 第一天是星期几
void display_calender( int mmonth, int mday, int days_of_month, int day_offset)
{
  // display calender
  Serial.println(String(mmonth) + "," + String(mday) + "," + String(days_of_month) + "," + String(day_offset));
  int  x1, y1;
  int w, h;
  int now_x, now_y;
  int16_t calender_base_x = 120;
  int16_t calender_base_y = 70;

  uint8_t num_offset;
  uint8_t print_valid = 0;
  uint8_t day_now = 1;  //待显示的天

  String outmsg = "";

  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  //竖线
  epd_fill_rect(calender_base_x - 5, 0, 5, EPD_HEIGHT, 0, framebuffer);

  //横线
  epd_fill_rect(0, calender_base_y + 15, EPD_WIDTH, 5, 0, framebuffer);

  now_x = calender_base_x;  //字的左下坐标
  now_y = calender_base_y;  //字的左下坐标
  char * title = "一  二  三  四  五  六  日  ";
  write_string( (GFXfont *)&msyh36, title, &now_x, &now_y, framebuffer);

  FontProperties properties;
  get_text_bounds((GFXfont *)&msyh36, title, &now_x, &now_y, &x1, &y1,  &w, &h, &properties);
  Serial.println(String("x1=") + String(x1) + ",y1=" + String(y1) + ",w=" + String(w) + ",h=" + String(h));
  // x1 = 909;  ?
  // y1 = 170;  ?
  // w = 778; //宽
  // h = 70;  //高
  w = w + 10; //修正宽度
  num_offset = 0;
  int x_offset = 20;
  int y_offset = 15;
  for (uint8_t j = 0; j <= 5; j++) {
    for (uint8_t i = 1; i <= 7 && day_now <= days_of_month; i++) {
      if (j == 0 && i == day_offset) {
        // start from the offset in the month,
        //ie which day does 1st of the month lie on
        print_valid = 1;
      }
      if (print_valid)
      {
        now_x = calender_base_x + ((i - 1) * (w / 7)) - num_offset + x_offset;
        now_y = calender_base_y + ((j + 1) * (h + 4)) + y_offset;
        write_string( (GFXfont *)&msyh36, (char *) String(day_now).c_str(), &now_x, &now_y, framebuffer);
        if (day_now == mday)
        {
          now_x = calender_base_x + ((i - 1) * (w / 7)) - num_offset + x_offset;
          now_y = calender_base_y + ((j + 1) * (h + 4)) + y_offset;

          //epd_fill_rect(now_x, now_y - h , h + 30, h+5, 0, framebuffer);
          //write_mode函数有问题，不能显示出黑底白字效果
          //write_mode( (GFXfont *)&msyh36, (char *) String(day_now).c_str(), &now_x, &now_y, framebuffer, BLACK_ON_WHITE, NULL);
          //write_string( (GFXfont *)&msyh36, (char *) String(day_now).c_str(), &now_x, &now_y, framebuffer);
          epd_draw_rect(now_x - 2,   now_y - h + 3,   h + 30,   h + 5,   0, framebuffer);
          epd_draw_rect(now_x - 2 + 1, now_y - h + 3 + 1 , h + 30 - 2, h + 5 - 2, 0, framebuffer);
          epd_draw_rect(now_x - 2 + 2, now_y - h + 3 + 2 , h + 30 - 4, h + 5 - 4, 0, framebuffer);
          epd_draw_rect(now_x - 2 + 3, now_y - h + 3 + 3 , h + 30 - 6, h + 5 - 6, 0, framebuffer);
          epd_draw_rect(now_x - 2 + 4, now_y - h + 3 + 4 , h + 30 - 8, h + 5 - 8, 0, framebuffer);
        }

        // once the offset is reached, start incrementing
        //display->println(day);
        day_now += 1;
      }
    }
  }

  char buf[128];

  // display month
  now_x = 5;
  now_y = 220;
  snprintf(buf, 128, "%02d",  mmonth);
  write_string( (GFXfont *)&msyh36, buf, &now_x, &now_y, framebuffer);

  // display day
  now_x = 5;
  now_y = 320;

  snprintf(buf, 128, "%02d",  mday);


  write_string( (GFXfont *)&msyh36, buf, &now_x, &now_y, framebuffer);

  epd_poweron();
  //uint32_t t1 = millis();
  //全局刷
  epd_clear();

  //前面不要用writeln,有一定机率阻塞,无法休眠
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);

  //delay(500);
  epd_poweroff();
}

void ShowStr( String mystring, int x0, int y0, int font_size, uint8_t * framebuffer )
{

  y0 = y0 + 60;


  if (font_size >= 100)
    write_string( (GFXfont *)&msyh36, (char *) mystring.c_str(), &x0, &y0, framebuffer);
  else
    write_string( (GFXfont *)&msyh16, (char *) mystring.c_str(), &x0, &y0, framebuffer);
}

//初始化硬件及变量
//如果非首次上电,且蓝牙没获取到新信息，则跳过此步，提升效率
void init_hard(bool load_history)
{


  //初始化SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS init failed");
    Serial.println("SPIFFS format ...");
    if (SPIFFS.format())
    {
      Serial.println("SPIFFS format ok");
      Serial.println("SPIFFS re_init");
      if (SPIFFS.begin(true))
      {
      }
      else
      {
        Serial.println("SPIFFS re_init error");
        ets_printf("reboot\n");
        esp_restart();
        return;
      }
    }
    else
    {
      Serial.println("SPIFFS format failed");
      ets_printf("reboot\n");
      esp_restart();
      return;
    }
  }
  Serial.println("SPIFFS init ok");

  if (pin_link)
  {
    if (SPIFFS.exists(TXTDATA_FILENAME)) {
      SPIFFS.remove(TXTDATA_FILENAME);
      Serial.println(String(TXTDATA_FILENAME) + " remove");
    }
  }
  else
  {
    if (load_history)
      objmemo_historyManager->load_list();
  }
  Serial.println("init_hard ok");
}


//GetCharwidth函数本来应放在 memo_historyManager类内部
//但因为引用了 msyh24海量字库变量，会造成编译失败,所以使用了一些技巧
//将函数当指针供类memo_historyManager 使用
//计算墨水屏显示的单个字的长度
int GetCharwidth(String ch)
{
  //修正，空格计算的的宽度为0, 强制36 字体不一样可能需要修改！
  if (ch == " ") return 28;

  char buf[50];
  int x1 = 0, y1 = 0, w = 0, h = 0;
  int tmp_cur_x = 0;
  int tmp_cur_y = 0;
  FontProperties properties;
  get_text_bounds((GFXfont *)&msyh36, (char *) ch.c_str(), &tmp_cur_x, &tmp_cur_y, &x1, &y1, &w, &h, &properties);
  //sprintf(buf, "x1=%d,y1=%d,w=%d,h=%d", x1, y1, w, h);
  //Serial.println("ch="+ ch + ","+ buf);

  //负数说明没找到这个字,会不显示出来
  if (w <= 0)
    w = 0;
  return (w);
}

//文字显示
void Show_hz(String rec_text, bool loadbutton)
{
  //最长限制160字节，40汉字
  //6个字串，最长约在 960字节，小于1024, json字串最大不超过1024
  rec_text = rec_text.substring(0, 160);
  Serial.println("begin Showhz:" + rec_text);

  //更正汉字符号显示的bug
  rec_text.replace("，", ",");
  rec_text.replace("。", ".");
  rec_text.replace("？", "?");

  epd_poweron();
  //uint32_t t1 = millis();
  //全局刷
  epd_clear();
  //局刷,一样闪屏
  //epd_clear_area(screen_area);
  //epd_full_screen()

  //此句不要缺少，否则显示会乱码
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  //uint32_t t2 = millis();
  //printf("EPD clear took %dms.\n", t2 - t1);
  int cursor_x = 10;
  int cursor_y = 80;

  //多行文本换行显示算法。
  if (!loadbutton)
    objmemo_historyManager->multi_append_txt_list(rec_text);

  String now_string = "";
  int i;
  //write_string 能根据手工加的 "\n"换行，但不能自由控制行距，此处我自行控制了.
  for ( i = 0; i < objmemo_historyManager->memolist.size() ; i++)
  {
    now_string = objmemo_historyManager->memolist.get(i);
    //Serial.println("Show_hz line:" + String((now_index + i) % TXT_LIST_NUM) + " " + now_string);

    if (now_string.length() > 0)
    {
      //加">"字符，规避epd47的bug,当所有字库不在字库时，esp32会异常重启
      // “Guru Meditation Error: Core 1 panic'ed (LoadProhibited). Exception was unhandled."
      now_string = ">" + now_string;
      //墨水屏writeln不支持自动换行
      //delay(200);
      //一定要用framebuffer参数，否则当最后一行数据过长时，会导致代码在此句阻塞，无法休眠，原因不明！

      writeln((GFXfont *)&msyh36, (char *)now_string.c_str(), &cursor_x, &cursor_y, framebuffer);
      //writeln调用后，cursor_x会改变，需要重新赋值
      cursor_x = 10;
      cursor_y = cursor_y + 85;
    }
  }

  //前面不要用writeln，有一定机率阻塞，无法休眠
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);

  //delay(500);
  epd_poweroff();

  Serial.println("end Showhz:" + rec_text );
}




void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  //esp_restart_noos(); 旧api
  esp_restart();
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

void goto_sleep()
{
  Serial.println("sleep...");
  delay(100);

  //不调用此句无法节能, 此句必须配合esp32深度休眠,轻度休眠无效！
  epd_poweroff_all();

  //第一个参数：RTCIO的编号
  //第二个参数：是高电平唤醒还是低电平唤
  //只能使用RTC功能的GPIO：0，2，4，12-15，25-27，32-39
  //唤醒后n秒进入休眠
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0);

  esp_deep_sleep_start();
}



void setup() {
  Serial.begin(115200);  // 可用  RX,TX
  wakeup_reason = esp_sleep_get_wakeup_cause();

  pin_link = check_pin();

  wakeup_time = millis() / 1000;
  blue_receive_txt_time = 0;
  blue_receive_txt = "";
  //为防意外，n秒后强制复位重启，一般用不到。。。
  //n秒如果任务处理不完，看门狗会让esp32自动重启,防止程序跑死...
  //如果串口有新数据，时间会重新计算
  int wdtTimeout = 5 * 60 * 1000; //设置5分钟 watchdog

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000 , false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt


  init_hard_ok = false;
  objmemo_historyManager = new memo_historyManager();
  objmemo_historyManager->GetCharwidth = GetCharwidth;

  /*
    Serial.println("init_hard");
    init_hard(false);
    init_hard_ok = true;
  */

  obweatherManager = new weatherManager();
  obweatherManager->ShowStr = ShowStr; //函数指针
  /*
    String weather_info = obweatherManager->readData("/weather.data");
    delay(1000);
    obweatherManager->draw_weather(weather_info);

    display_calender(2, 18, 1);
  */

  //如果启动后不调用此函数，有可能电流一直保持在在60ma，起不到节能效果
  //此步骤不适合在唤醒后没有显示需求时优化掉
  epd_init();
  // framebuffer = (uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    Serial.println("alloc memory failed !!!");
    delay(1000);
    while (1);
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);


  //清屏
  if (pin_link)
  {
    Serial.println("init_hard");
    init_hard(false);
    init_hard_ok = true;
    Show_hz("", true);
  }

  //                                 RX, TX
  //MySerial.begin(9600, SERIAL_8N1, 12, 13);
  MySerial.begin(9600, SERIAL_8N1, 14, 15);


//Serial.println("AT 预处理hc08");
//初始化hc08, hc08仅运行一次即可
//delay(2000);
//MySerial.write("AT+LED=0");
//clear_uart(5000);
//MySerial.write("AT");
//clear_uart(5000);
//MySerial.write("AT+MODE=?");
//clear_uart(5000);
//MySerial.write("AT+MODE=1");
//clear_uart(5000);
//MySerial.write("AT+NAME=INK_047");
//clear_uart(5000);


  //蓝牙串口唤醒的字串不能直接用，前面几个字节的数据会乱码，需要丢掉
  //需要约定，先随意发一串数据，过>300ms后再正式发送信息
  clear_uart(100);

  Serial.println("start");

  //  delay(2000);
  //  MySerial.write("AT+LED=0");
  //  clear_uart(1000);

  //首次上电或按RetSet,进入深度休眠， 局开
  //方便客户端使用，1.唤醒。 2.发送数据
  //if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0)
  Serial.println("wakeup_reason=" + String(wakeup_reason));

  //hc08引脚触发的唤醒后要接收数据，需要工作状态，其它状态均直接休眠
  if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0)
  {
    delay(100);
    goto_sleep();
  }
}


void loop() {

  if (millis() / 1000 < wakeup_time)
    wakeup_time = millis() / 1000;

  //唤醒n秒后， 如果无工作，进入休眠模式，直到蓝牙uart有信号唤醒
  //正常启动后5-7秒可完成文字接收显示，文字多的话需要时间略长
  //如果出现意外情况，15秒还未完，强制进入休眠
  //天气JSON,约1KB,需要30-40秒
  if (millis() / 1000 - wakeup_time > 100)
  {
    Serial.println("boot timeout >100s");
    goto_sleep();
    return;
  }


  while (MySerial.available())
  {
    String tmpstr = "";
    char ch;

    //串口数据加延时能减少一个字串会拆成好几次收到的机率
    //现在通过结束符>>> 确保一次收到完整的字串
    while (MySerial.available())
    {
      ch = (char) MySerial.read();
      tmpstr = tmpstr + ch;
      //Serial.print(ch);
      delay(30);
    }

    //记录首次收到部分字符的时间
    if (blue_receive_txt.length() == 0)
      blue_receive_txt_time = millis() / 1000 ;
    Serial.println(">>" + tmpstr);
    blue_receive_txt = blue_receive_txt + tmpstr;
  }

  //超时N秒，或收到结束符标志
  //天气JSON,约1KB,需要36左右处理， 此处60秒是为了兼容天气JSON
  if (blue_receive_txt.length() > 0 && blue_receive_txt_time != 0 &&
      (  millis() / 1000 - blue_receive_txt_time > 60 ||  blue_receive_txt.endsWith(">>>") )
     )
  {

    blue_receive_txt.replace(">>>", "");
    Serial.println(">>>" + blue_receive_txt);
    //wakeup_time = millis() / 1000; //工作状态，休眠时间重新计时

    timerWrite(timer, 0); //reset timer (feed watchdog)



    //唤醒专用命令字符，不显示
    // 因蓝牙空中传播，发出的字串可能会分多次收到， 用>>> 表示字符结束，
    Serial.println("blue_receive_txt len=" + String(blue_receive_txt.length()));

    //是天气json信息
    if ( blue_receive_txt.indexOf("\"status\"") > -1 && blue_receive_txt.indexOf("\"result\"") > -1 && blue_receive_txt.indexOf("{") > -1 &&  blue_receive_txt.indexOf("}") > -1)
    {
      //初始化硬件驱动，收到首条文字后再做，节省资源
      if (init_hard_ok == false)
      {
        Serial.println("init_hard");
        init_hard(false);
        init_hard_ok = true;
      }

      Serial.println("draw_weather");
      obweatherManager->draw_weather(blue_receive_txt);
    }
    else if (blue_receive_txt.indexOf("###") == 0)
    {
      //初始化硬件驱动，收到首条文字后再做，节省资源
      if (init_hard_ok == false)
      {
        Serial.println("init_hard");
        init_hard(false);
        init_hard_ok = true;
      }
      Serial.println("draw_weather");
      blue_receive_txt.replace("###", "");
      objmemo_historyManager->splitString(blue_receive_txt, ",", buff_command, 4);
      int mmonth, mday, days_of_month, mday_offset;
      mmonth = buff_command[0].toInt();
      mday = buff_command[1].toInt();
      days_of_month = buff_command[2].toInt();
      mday_offset = buff_command[3].toInt();
      display_calender(mmonth, mday,  days_of_month, mday_offset);
    }
    //是一般的留言信息
    else
    {
      //初始化硬件驱动，收到首条文字后再做，节省资源
      if (init_hard_ok == false)
      {
        Serial.println("init_hard");
        init_hard(true);
        init_hard_ok = true;
      }
      Show_hz(blue_receive_txt, false);
      objmemo_historyManager->save_list();
      MySerial.print(String("ok: len=") + String(blue_receive_txt.length()) + ">>>");
    }

    //一般约10-30汉字4-5秒左右
    //json 天气信息约30-40秒左右
    Serial.println("处理文字用时:" + String(millis() / 1000 - wakeup_time) + "秒！");
    blue_receive_txt = "";
    //收到一条文字信息后直接休眠
    goto_sleep();
  }

  delay(10);
}
