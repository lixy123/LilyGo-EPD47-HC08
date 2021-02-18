#include <String.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include "epd_driver.h"
//https://arduinojson.org/v6/doc/deserialization/?utm_source=github&utm_medium=readme
#include <ArduinoJson.h>

//天气图标
#include "w0.h"
#include "w1.h"
#include "w2.h"
#include "w3.h"
#include "w4.h"
#include "w5.h"
#include "w6.h"
#include "w7.h"
#include "w8.h"
#include "w9.h"
#include "w10.h"
#include "w11.h"
#include "w12.h"
#include "w13.h"
#include "w14.h"
#include "w15.h"
#include "w16.h"
#include "w17.h"
#include "w18.h"
#include "w19.h"
#include "w20.h"
#include "w21.h"
#include "w22.h"
#include "w23.h"
#include "w24.h"
#include "w25.h"
#include "w26.h"
#include "w27.h"
#include "w28.h"
#include "w29.h"
#include "w30.h"
#include "w31.h"
#include "w32.h"
#include "w39.h"
#include "w49.h"
#include "w53.h"
#include "w54.h"
#include "w55.h"
#include "w56.h"
#include "w57.h"
#include "w58.h"
#include "w99.h"
#include "w301.h"
#include "w302.h"

class weatherManager {
  private:
    //墨水屏缓存区
    uint8_t *framebuffer;
    //String weather_info;

    //在函数中一次申请过长缓存会重启
    uint8_t * readbuff;
  public:
    weatherManager();
    ~weatherManager();
    void draw_weather(String weather_result);

    void  (* ShowStr)(String mystring, int x0, int y0, int fontsize, uint8_t * framebuffer); //声明函数指针

    void ShowRec(int x0, int y0, int x1, int y1);

    void ShowLine(int x0, int y0, int x1, int y1);
    String  readData(char * myFileName);

    uint8_t *  get_icon(String no);
};
