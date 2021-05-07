# LilyGo-EPD47-HC08
LilyGo-EPD47 利用hc08蓝牙硬件实现平时休眠节能，随时按需唤醒显示文字的demo

<b>一.功能：</b><br/>
1.记事留言提醒是人类的最基本需求，现今互联网，云服务时代，信息无处不在，但可惜，绝大部分信息都是对商家有用，个人无益的无用信息，并不是 专为个人准备的。信息需要有序，按需供应，该出现时就出现，不需要时就关闭。<br/>
2.lilygo-epd47结合了墨水屏和 ESP32的低功耗特点. 都具有休眠唤醒节能特性，适合省电场景下的记事留言。其它硬件方案无法提供此功能的关键点是供电，摆放位置周围必须有插座，限制了显示屏留言提醒的使用场景。虽然手机本身很容易做成提醒留言，但手机信息干扰太多,且打开手机APP会增加操作步骤,不太适合做提醒留言，因为增加了几步的点击和人的懒惰,减少了使用顺畅度。<br/>
3.hc08是一个低功耗的蓝牙设备，一级节能模式的待机电流约0.2-1ma,可以在几秒内通过蓝牙扫描到并连接及激活宿主机器。特别适合与以上设备协作工作。<br/>
4.所以,最终场景设计为 lilygo-epd47电池供电状态放置在桌子上,大门后,冰箱上做为能无线接收文字信息并显示提醒留言,天气等信息的智能显示屏，除了简单的天气预报,行事历同步,甚至可以结合树莓派拖管微信,MQTT服务端转发来发送更高级的文字信息应用, 例如出门携带物品提醒，记事安排提醒等. <br/>
5.lilygo-epd47墨水屏平时不工作时处于休眠状态，最大化节省电池电量。当需要显示信息时及时唤醒并显示文字<br/>
  数据指标:<br/>
    5.1 休眠时整体电流<1ma <br/>
    5.2 唤醒时间<1秒 <br/>
    5.3 lilygo-epd47墨水屏每天唤醒并显示3次天气信息，平时偶尔有零星文字提醒记事显示， 普通的18650电池能支持墨水屏约1-3个月<br/>
6.关于唤醒硬件选择,尝试过lora, nb-iot, 其中nb-iot模块关闭节能约1ma, 相对hc08蓝牙闲时电流(时间均滩<0.3ma)显得过大,不适合直接连接到墨水屏,但可做到发送器上.<br/>

<b>二.硬件需求：</b><br/>
1.lilygo-epd47 电池供电, HC08蓝牙设备, 随时接收文字信息并显示的屏<br/>
     hc-08 BLE4.0蓝牙模块 (购买时要询问,告诉卖方要双晶振版本，否则不支持一级节能模式)<br/>
     hc-08设置成客户模式，一级节能模式,蓝牙名称最好用AT指令修改下,防止被别的设备误连，建议名称：edp47_ink<br/>
     lilygo 公司已开始出货 hc-08在lilygo-epd47 墨水屏上的专用集成模块成品,使用触摸屏长供电的vcc,数据引脚,集成度更好. <br/>
     <br/>
     注：官方数据，一级节能模式电流约 6μA ~2.6mA （待机） /1.6mA（工作）<br/>
        相对于全速模式 8.5mA（待机）/9mA（待机） 节能效果明显<br/>
        hc-08模块每日大部分时间应处于在 6μA ~2.6mA （待机）模式,理论电流消耗极低<br/>
     引脚连接:<br/>
     lilygo-epd47  hc-08<br/>
       VCC         VCC<br/>
       14          TX<br/>
       15          RX<br/>
       GND         GND<br/>
    注：墨水屏进入节能休眠模式后，墨水屏顶端VCC,14，15,GND 插槽处的VCC的3.3V电压输出会中断，不能在此插槽处取电，要从ph2.0或18650处取电<br/>   
    hc08 AT命令预处理:<br/>
    AT+MODE=1        //设置成一级节能模式(必须)<br/>
    AT+NAME=INK_047  //修改蓝牙名称，用于客户端查找蓝牙用<br/>
    AT+LED=0          //关闭led灯，省电<br/>
    注: 也可以通过连接到lilygo-epd47后,自编程序用lilygo-epd47虚拟串口传入AT命令
    
    
2.ESP32  用于给前一设备信息推送<br/>
  3.1 ESP32开发板芯片(建议带psram) <br/>
  3.2 如果后期想搞语音转文字高级功能，需要ESP32带PSRAM的版本, 最好集成MIC, t-watch系列本质上是将ESP32加上了一些外设进行了高度集成。<br/>  
3.树莓派 (待扩充) <br/>

<b>三.代码说明:</b> <br/>
  <b> 1.epd47_blue_waker  (蓝牙从机-外设) </b> <br/>
   <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/ink_epd47_1.jpg?raw=true' /> <br/>
   <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/ink_epd47_2.jpg?raw=true' /><br/>
   <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/ink_weather.jpg?raw=true' /><br/>
   <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/ink_chixi.jpg?raw=true' /><br/>
     烧录到LilyGo-EPD47墨水屏， 实现墨水屏电池供电情境下, 平时休眠,按需显示。<br/>
     软件: arduino 1.8.13 <br/>
     库文件: <br/>
https://github.com/espressif/arduino-esp32 版本:1.0.6 <br/>
https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library 最新版本, 仅为了用到它定义的开发板 <br/>
https://github.com/Xinyuan-LilyGO/LilyGo-EPD47 最新版本 <br/>
https://github.com/bblanchon/ArduinoJson  版本: 6 <br/>
https://github.com/ivanseidel/LinkedList 最新版本 <br/>
     开发板选择：TTGO-T-WATCH <br/>
     电池供电. <br/>
     
  <b> 2.epd47_blue_waker_center (蓝牙主机-中心)</b>  <br/>
     烧录到普通便宜的的ESP32开发板上，每1分钟发送1段文字到墨水屏显示，仅为了演示蓝牙发送效果。 <br/>
     通过插线板供电,如电池供电支持不了1天<br/>
   注:与lilygo-epd47墨水屏配合使用,一个发,一个收.<br/>
   
  <b> 3.epd47_blue_waker_center_weather (蓝牙主机-中心)</b>  <br/>
   <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/esp32_center.jpg?raw=true' /> <br/>
     <b>3.1 硬件组成：</b> <br/>
     A.ESP32开发板(建议带psram) <br/>
     B.DS3231时钟模块 <br/>
     <b>3.2 技术说明:</b> <br/>
     每天设置2次在设定时间wifi获取天气, 将天气预报通过esp32内置蓝牙功能推送至墨水屏并显示<br/>
     通过wifi除了获取天气信息，还可获取万年历，日期节日，记事提醒等文字，待发挥。<br/>
     config.h 需要配置心知天气key,极速天气key ,注册方式见config.h的说明<br/>
     心知天气用的免费版本，不限次，只适合发送一串文字信息，混在提醒记事文本串中，显示较简陋。<br/>
     极速天气可展示多天天气，表格状，界面华丽，使用其API需要给天气供应商付费 <br/>
     <b>3.3 已实现功能:</b> <br/>
     目前实现有如下三种信息推送至墨水屏:<br/>
     A. 2行文字信息的天气信息<br/>
     B. 华丽表格版本的天气信息，共显示5天天气。<br/>
     C. 将连接有HC08的机器唤醒。原理是待连接的HC08与继电器相连触发远程设备开机引脚(例如树莓派)或供电，以达到节电等目的。<br/>
     <br/>
     计划实现: <br/>
     1.使用Blynk技术, 借助 Blynk客户端手机APP软件将文字信息推给Blynk服务器,并转发到发送器,再由发送器转发给墨水屏显示. 初步测试过,可行.<br/>
     
     设计成插线板供电,不适合电池供电, 代码没有节能优化,用电大约50ma电流,普通18650电池支持不了几天,  估算公式 2000ma/24小时/50ma*0.8=1.3 <br/>
     
   注:<br/>
   1.与lilygo-epd47墨水屏配合使用,一个发,一个收.<br/>
   2.esp32需要连接路由器,首次使用时需要配置wifi, 用了 ESP32 SmartConfig 配网技术, 首次用时参考:https://www.zhangtaidong.cn/archives/124/ 微信扫描配置wifi网络.<br/>
     否则会每120秒重启,不能使用.<br/>
   

  <b> 4.epd47_blue_waker_center_nb_iot (蓝牙主机-中心-NB-IOT版本)</b>    
    <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/sim7020-2.jpg?raw=true' /> <br/>
    <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/sim7020-1.jpg?raw=true' /> <br/>
      epd47_blue_waker_center_weather的 NB-IOT版本, 用nb-iot网代替wifi网<br/>
     <b> 4.1 硬件组成：</b><br/>
      A.ESP32开发板(建议带psram)<br/>
      B.DS3231时钟模块 <br/>
      C.Sim7020c模块 <br/>
      把NB-IOT当路由器使用，适合于仓库，家里或车上没有wifi网络，或公司里虽然有wifi,但各种上网验证，不适合单片机连接上网的场合。<br/>
     <b> 4.2 引脚连接:</b>  <br/>
  ESP32 Sim7020c <br/> 
  5V    5v <br/>
  GND   GND <br/>
  12    TX <br/>
  13    RX <br/>
  15    RESET  (注：仅用于sim7020复位，不用能用开，关sim7020) <br/>
  <b> 4.3 技术说明: </b><br/>
      通过插线板供电,如电池供电支持不了1天<br/>
      注： 如果未来需求仅仅是在固定时间点获取天气并显示到墨水屏，ESP32不适合找到插线板供电(例如车内)，有如下2改进方案：<br/>
      1.本套硬件的代码增加休眠节能, ESP32不用时休眠，sim7020不用时关闭，最多电流可优化到7ma左右, 可初步做到电池供电<br/>
      2.将本代码与epd47_blue_waker代码硬件软件合一，省去本套硬件，ds3231,sim7020c连接到墨水屏上，因为用电设备多，勉强能0.5-1个月充电一次 <br/>
      都是一种不错的选择 <br/>
  <b>4.4 商务前景:</b><br/>
  <一>特点:<br/>
  1.整体低耗能电池供电: <br/>
     解决随处摆放,低频次,随时得到信息的痛点<br/>
  2.低配版NB-IOT路由器:  <br/>
     解决一般的wifi连接路由器配置繁琐,甚至无网可用,有网不能连接的痛点,(例如车上，家里无宽带独居老人)<br/>
  3.特别是独居老人,车内场景<br/>
     给其建立了用最低成本的发出信息,收到信息的机会通道.<br/>
  <二>定位<br/>
   1.考虑到成本最低约300左右,适合商务礼品.<br/>
   2.也适合技客玩家<br/>
  <三>现存问题: <br/>
   1.外形不够简约,大气,需重构.<br/>
   2.nb-iot网可能仅能在某些城市可用, 可能不适合全国.<br/>
   3.代码待磨合.<br/>

  <b> 5.epd47_blue_waker_center_nb_iot_twatch2019 (蓝牙主机-中心-NB-IOT-Twatch2019版本)</b>    <br/>
  是前一程序的另一版本，只不是将esp32+DS3231+sim7020c模块组合换成了内置esp32的twatch2019+sim7020c，<br/>
  借用了现成的集成模块，显得更简洁，<br/>
  引脚连接见代码注释.<br/>
  效果如图，等待twatch2010的sim7020c扩展板推出.
   <img src= 'https://github.com/lixy123/LilyGo-EPD47-HC08/blob/main/watch2019-ink.jpg?raw=true' /> <br/> 
  实测发现蓝牙信号略比露天摆放的esp32稍弱，摆放时离墨水屏尽量近一些，不要有遮挡
 <br/>
 
  <b> 6.t-watch2020-v3_mic_blue (t-watch2020 v3带麦克风版本) </b>  <br/>
     lilygo 电话手表通过调用百度服务将语音转文字，通过蓝牙发送文字到墨水屏显示<br/>
 <b>1 配置: </b> <br/>
ESP32首次运行时会自动初始化内置参数,自动进入路由器模式,创建一个ESP32SETUP的路由器，电脑连接此路由输入http://192.168.4.1 进行配置<br/>
A.esp32连接的路由器和密码<br/>
B.百度语音的账号校验码<br/>
baidu_key: 一个账号字串 (必须注册获得)<br/>
baidu_secert: 一个账号校验码 (必须注册获得)<br/>
这两个参数需要注册百度语音服务,在如下网址获取 https://ai.baidu.com/tech/speech<br/>
开通中文普通话短语音识别，单次语音最长识别60秒。新用户可免费用一段时间，再用必须开通收费，1000次3.4元左右价位，如果不限制使用，最多有可能每天调用5000-10000次，所以增加了休眠功能控制调用次数,防止欠费<br/>
C.其它音量监测参数: 默认是在家里安静环境下,如果周围较吵,需要将值调高<br/>
define_max1 每0.5秒声音峰值（感应人声判断）<br/>
define_avg1 每0.5秒声音均值（感应人声判断）<br/>
define_max2 每0.5秒声音峰值（录音中静音判断）<br/>
define_avg3 每0.5秒声音均值（录音中静音判断）<br/>
 
 <b>2 运行： </b> <br/>
A.手表开机<br/>
B.说话<br/>
C.手表会显示语音识别的进度,识别到文字后通过蓝牙发送给lilygo-epd47墨水屏<br/>
D.按下手表上的电源按钮进入休眠<br/>
  
 


演示视频地址:<br/>
   https://raw.githubusercontent.com/lixy123/LilyGo-EPD47-HC08/main/vid_20210215.avi
<br/>    
<b>四.电流实测:</b><br/>
  1.休眠： <1ma （客户端蓝牙发完信息要断开，否则墨水屏的蓝牙模块不能进入休眠，电流约9ma)<br/>
  2.唤醒后: 50-60ma<br/>
  蓝牙模块官方数据提到待机电流约6μA ~2.6mA，墨水屏待机电流约0.17ma，合计电流约<0.4ma<br/>
  注：<br/>
  不能由USB供电，休眠仍要耗电80ma, 达不到节能目的,必须由ph2.0或18650电池供电<br/>
  触屏节能设计不太好,休眠时仍要耗电2ma, 如要节能,最好拔下硬件接头或弃用触屏.<br/>
  3.1200ma的电池约能用 1200*0.8/24/0.4=80天,满足预期<br/>
  

  
  
  
