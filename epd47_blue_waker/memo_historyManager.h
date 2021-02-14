#include <String.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include "epd_driver.h"
//https://github.com/tomstewart89/Vector
//bug 不能用！
//#include <Vector.h>


#include <ArduinoJson.h>

//https://github.com/ivanseidel/LinkedList
#include <LinkedList.h>  //列表对象

#define FILESYSTEM SPIFFS
#define TXTDATA_FILENAME "/config.data"
#define TXT_LIST_NUM 6   //历史文本列表最多6行， 显示屏高度显示6行字


class memo_historyManager {
  private:
    String buff_command[TXT_LIST_NUM];
    String buff_command2[TXT_LIST_NUM];
    String  readData(char * myFileName);
    String Do_MultiLineString(String content);
    void multi_append_txt_list_2(String txt);
    void append_txt_list(String txt);

  public:
    int (* GetCharwidth)(String ch);  //声明函数指针
    LinkedList<String> memolist=LinkedList<String>();;  //历史文本列表


    void splitString(String message, String dot, String outmsg[], int len);

    memo_historyManager();
    ~memo_historyManager();

    void multi_append_txt_list(String txt);
    void save_list();
    int load_list();
};
