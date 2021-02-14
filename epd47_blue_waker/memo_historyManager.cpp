#include "memo_historyManager.h"

memo_historyManager::memo_historyManager()
{
  //LinkedList<String> memolist= LinkedList<String>();
}


memo_historyManager::~memo_historyManager()
{
  memolist.clear();
}

//自动换行算法
void memo_historyManager::multi_append_txt_list(String txt)
{
  int i;
  //最多拆6行,多了也显示不下
  splitString(txt, "\n", buff_command, TXT_LIST_NUM);
  for (i = 0; i < TXT_LIST_NUM; i++)
  {
    if (buff_command[i].length() > 0)
    {
      //append_txt_list(buff_command[i]);
      //二次文字自动换行处理
      multi_append_txt_list_2(buff_command[i]);
    }
  }
}

//2次检查
void memo_historyManager::multi_append_txt_list_2(String txt)
{
  int i;

  Serial.println("字串总宽度=" + String(GetCharwidth(txt)));
  String new_string = Do_MultiLineString(txt);

  Serial.println("new_string="+new_string);
  
  splitString(new_string, "\n", buff_command2, TXT_LIST_NUM);
  //最多拆6行,多了也显示不下
  for (i = 0; i < TXT_LIST_NUM; i++)
  {
    if (buff_command2[i].length() > 0)
    {
      append_txt_list(buff_command2[i]);
    }
  }
}


void memo_historyManager::append_txt_list(String txt)
{
  if (memolist.size() == TXT_LIST_NUM)
    memolist.remove(0);
  memolist.add(txt);

}



int memo_historyManager::load_list()
{
  int i;
  if (!FILESYSTEM.exists(TXTDATA_FILENAME)) {
    Serial.println("load configure fail");
    return -1;
  }
  StaticJsonBuffer<1024> jsonBuffer;
  String tmp = readData(TXTDATA_FILENAME);
  JsonArray& array =  jsonBuffer.parseArray(tmp);
  char * str1;

  for ( i = 0; i < array.size(); i++)
  {
    str1 = (char *)array.get<char*>(i);
    //Serial.print("i=" + String(i) + ",");
    //Serial.println(str1);

    if (strlen(str1) > 0)
    {
      //先进先出
      if (memolist.size() == TXT_LIST_NUM)
        memolist.remove(0);
      //Serial.println("add:" + String(str1) );
      //追加上
      memolist.add( String(str1));
    }

    //Serial.println("txt_list[" + String(i) + "]=" + txt_list[i]);
  }
  //txt_list_index =  TXT_LIST_NUM - 1;

  //最后一个有效索引+1,并回转
  //txt_list_index = (last_index + 1) % TXT_LIST_NUM;
  Serial.println("load_list,memolist.size=" + String(memolist.size()));

  return memolist.size();
}


void memo_historyManager::save_list()
{
  int i;
  File file = FILESYSTEM.open(TXTDATA_FILENAME, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }
  StaticJsonBuffer<1024> jsonBuffer;
  JsonArray& array = jsonBuffer.createArray();
  String now_string;

  for ( i = 0; i < memolist.size(); i++)
  {
    now_string = memolist[i];
    if (now_string.length() > 0)
    {
      array.add(now_string);
    }
  }
  array.printTo(now_string);
  Serial.println("save_list:" + now_string);
  if (array.printTo(file) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }
  file.close();
  Serial.println("save_list rows=" + String(memolist.size()) );
}


String memo_historyManager::readData(char * myFileName) {
  uint8_t readbuff[1024];
  long readnum = 0;
  File file = SPIFFS.open(myFileName, FILE_READ);
  readnum = file.read(readbuff, 512);
  file.close();
  readbuff[readnum] = '\0';

  Serial.println("readData=" +  String((char *)readbuff));
  return String((char *)readbuff);
}



//UTF8字符串拆解出字母，汉字
String memo_historyManager::Do_MultiLineString(String content)
{
  LinkedList<String> contentArray = LinkedList<String>();
  LinkedList<int> contentArray_width = LinkedList<int>();
  char tmp_cArr[10];
  char cArr[content.length() + 1];
  content.toCharArray(cArr, content.length() + 1);

  //将字串拆成字
  for (size_t i = 0, len = 0; i != strlen(cArr); i += len)
  {
    unsigned char byte = (unsigned)cArr[i];
    if (byte >= 0xFC)
      len = 6;
    else if (byte >= 0xF8)
      len = 5;
    else if (byte >= 0xF0)
      len = 4;
    else if (byte >= 0xE0)
      len = 3;
    else if (byte >= 0xC0)
      len = 2;
    else
      len = 1;
    //substring 对中英字混合不好用！
    //ch = content.substring(i, len);
    memcpy(tmp_cArr, cArr + i, len);
    tmp_cArr[len] = '\0';
    //Serial.println("tmp_cArr=" + String(tmp_cArr) + ",len=" + String(len));

    //字追加到队列
    contentArray.add(String(tmp_cArr));
    //计算字的显示宽度
    contentArray_width.add(GetCharwidth(tmp_cArr));

  }
  String ret_content = "";
  String ret_content_now = "";
  int now_width = 0;
  int all_width = 0;
  //将字符拆成每contentAreaWidthWithMargin长度一段
  int line_num = 0;
  if (contentArray.size() > 0)
    line_num = 1;

  //每个字宽度相加不等于总宽度？
  for (size_t j = 0; j < contentArray.size(); j++)
  {
    //判断是否需要换行, 屏幕总宽-45, 不占满，留一些边角
    if ((now_width + contentArray_width.get(j)) < EPD_WIDTH - 80)
    {
      ret_content_now = ret_content_now + contentArray.get(j);
      //计算当前串的长度
      now_width = GetCharwidth(ret_content_now);
      ret_content = ret_content + contentArray.get(j);
    }
    else
    {
      all_width = all_width + now_width;
      ret_content_now = contentArray.get(j);
      ret_content = ret_content + "\n" + contentArray.get(j);
      now_width = contentArray_width.get(j);
      //Serial.println("换行!");
      line_num = line_num + 1;
    }


    // Serial.println("j=" + String(j) + ",ch=" + contentArray.get(j) + ",all_width=" + String(all_width + now_width) + ",width=" + String(contentArray_width.get(j)));


  }

  // Serial.println("char_size=" + String(contentArray.size()) + ",line_num=" + String(line_num) + ",all_width=" + String(all_width + now_width) + ",ret_content=" + ret_content);
  contentArray.clear();
  contentArray_width.clear();

  return ret_content;
}



void memo_historyManager::splitString(String message, String dot, String outmsg[], int len)
{
  int commaPosition, outindex = 0;
  for (int loop1 = 0; loop1 < len; loop1++)
    outmsg[loop1] = "";
  do {
    commaPosition = message.indexOf(dot);
    if (commaPosition != -1)
    {
      outmsg[outindex] = message.substring(0, commaPosition);
      outindex = outindex + 1;
      message = message.substring(commaPosition + 1, message.length());
    }
    if (outindex >= len) break;
  }
  while (commaPosition >= 0);

  if (outindex < len)
    outmsg[outindex] = message;
}
