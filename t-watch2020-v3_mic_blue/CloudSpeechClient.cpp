#include "CloudSpeechClient.h"

CloudSpeechClient::CloudSpeechClient(int sec) {
  pre_sec = sec;
  pre_maxnum_sound_buff = pre_sec * 32000;
  sound_bodybuff = (byte*)ps_malloc(maxnum_bodysound__buff);
  sound_bodybuff_p = 0;

  pre_sound_buff = (byte*)ps_malloc(pre_maxnum_sound_buff);
  zero_pre_push_sound_buff();

  //wav head
  wav_head = (byte*)ps_malloc(headerSize + 4);
  CreateWavHeader(wav_head, headerSize);
}

CloudSpeechClient::~CloudSpeechClient() {
  //client.stop();
  //WiFi.disconnect();
}



//上传文件
bool CloudSpeechClient::uploadfile(String ftp_host, int ftp_port, String fn)
{
  unsigned long starttime = 0;
  unsigned long stoptime = 0;
  starttime = millis() / 1000;

  Serial.println("uploadfile begin");
  if (!client.connect(ftp_host.c_str(), ftp_port)) {
    Serial.println("ftp connect fail");
    return (false);
  }

  uint32_t filesize =  sound_bodybuff_p + pre_maxnum_sound_buff + headerSize;
  uint32_t fileSizeMinus8 =  sound_bodybuff_p + pre_maxnum_sound_buff + headerSize - 8;
  wav_head[4] = (byte)(fileSizeMinus8 & 0xFF);
  wav_head[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  wav_head[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  wav_head[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);

  fileSizeMinus8 =   sound_bodybuff_p + pre_maxnum_sound_buff;
  wav_head[40] = (byte)(fileSizeMinus8 & 0xFF);
  wav_head[41] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  wav_head[42] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  wav_head[43] = (byte)((fileSizeMinus8 >> 24) & 0xFF);

  String tmpfn = fn;
  tmpfn.replace("\\", "");
  Serial.println( String("begin to send|") + tmpfn );
  client.print(String("begin to send|") + tmpfn);
  delay(500);
  uint32_t filesend = 0;

  uint32_t writenum = 0;
  uint32_t  active_write = 0;
  uint32_t now_write = 0;
  uint32_t seg_len = 2048;
  starttime = millis() / 1000;
  //1.发送wav head
  //反复发送，确保都发送出去
  //注意：此处有坑！！！
  while (active_write < headerSize)
  {
    stoptime = millis() / 1000;
    if (stoptime - starttime >= 10)
    {
#ifdef SHOW_DEBUG
      Serial.println("headerSize response time out >10s");
#endif
      return (false);
    }


    writenum = 1024;
    if (headerSize - active_write < writenum)
      writenum = headerSize - active_write;
    now_write = client.write(wav_head + active_write, writenum);
    if (now_write != writenum)
    {
      Serial.println("fail 待发送：" + String(writenum) + " 实际发送:" + String(now_write));
      delay(10);
    }
    active_write = active_write + now_write;
    filesend = filesend + now_write;
  }

  //2.发送缓存的3秒声音
  active_write = pre_sound_buf_p;
  //反复发送，确保都发送出去
  //注意：此处有坑！！！
  starttime = millis() / 1000;
  while (active_write < pre_maxnum_sound_buff)
  {
    stoptime = millis() / 1000;
    if (stoptime - starttime >= 10)
    {
#ifdef SHOW_DEBUG
      Serial.println("pre_maxnum_sound_buff response time out >10s");
#endif
      return (false);
    }

    //2k缓存，上传文件平均3秒
    writenum = 4096;
    if (pre_maxnum_sound_buff - active_write < writenum)
      writenum = pre_maxnum_sound_buff - active_write;
    now_write = client.write(pre_sound_buff + active_write, writenum);
    if (now_write != writenum)
    {
      Serial.println("fail 待发送：" + String(writenum) + " 实际发送:" + String(now_write));
      delay(10);
    }
    active_write = active_write + now_write;
    filesend = filesend + now_write;
  }

  starttime = millis() / 1000;
  if (pre_sound_buff > 0)
  {
    active_write = 0;
    //反复发送，确保都发送出去
    //注意：此处有坑！！！
    while (active_write < pre_sound_buf_p)
    {

      stoptime = millis() / 1000;
      if (stoptime - starttime >= 10)
      {
#ifdef SHOW_DEBUG
        Serial.println("pre_sound_buff response time out >10s");
#endif
        return (false);
      }


      //2k缓存，上传文件平均3秒
      writenum = 4096;
      if (pre_sound_buf_p - active_write < writenum)
        writenum = pre_sound_buf_p - active_write;
      now_write = client.write(pre_sound_buff + active_write, writenum);
      if (now_write != writenum)
      {
        Serial.println("fail 待发送：" + String(writenum) + " 实际发送:" + String(now_write));
        delay(10);
      }
      active_write = active_write + now_write;
      filesend = filesend + now_write;
    }
  }
  //3.发送声音文件主要内容
  active_write = 0;
  //反复发送，确保都发送出去
  //注意：此处有坑！！！
  starttime = millis() / 1000;
  while (active_write < sound_bodybuff_p)
  {
    stoptime = millis() / 1000;
    if (stoptime - starttime >= 10)
    {
#ifdef SHOW_DEBUG
      Serial.println("sound_bodybuff response time out >10s");
#endif
      return (false);
    }

    //2k缓存，上传文件平均3秒
    writenum = 4096;
    if (sound_bodybuff_p - active_write < writenum)
      writenum = sound_bodybuff_p - active_write;
    now_write = client.write(sound_bodybuff + active_write, writenum);
    if (now_write != writenum)
    {
      Serial.println("fail 待发送：" + String(writenum) + " 实际发送:" + String(now_write));
      delay(10);
    }
    active_write = active_write + now_write;
    filesend = filesend + now_write;
  }

  client.stop();
  stoptime = millis() / 1000;
#ifdef SHOW_DEBUG
  Serial.println(String("上传声音用时:") +  String(stoptime - starttime) + "秒,文件大小:" + String(filesize) + "字节" +
                 ",本次共发送:" + String(  filesend) + "字节");
#endif
  return (true);
}



void CloudSpeechClient::zero_pre_push_sound_buff()
{
  pre_sound_buf_p = 0;
  for (unsigned long loop1 = 0; loop1 < pre_maxnum_sound_buff; loop1++)
    pre_sound_buff[loop1] = 0;
}

void CloudSpeechClient::pre_push_sound_buff(byte * src_buff, uint32_t len)
{
  memcpy(pre_sound_buff + pre_sound_buf_p, src_buff, len);
  pre_sound_buf_p = pre_sound_buf_p + len;
  if (pre_sound_buf_p >= pre_maxnum_sound_buff)
    pre_sound_buf_p = 0;
}

void CloudSpeechClient::push_bodybuff_buff(byte * src_buff, uint32_t len)
{
  memcpy(sound_bodybuff + sound_bodybuff_p, src_buff, len);
  sound_bodybuff_p = sound_bodybuff_p + len;
  if (sound_bodybuff_p >= maxnum_bodysound__buff)
    sound_bodybuff_p = 0;
}


String CloudSpeechClient::Findkey(String line)
{
  //Serial.println("Findkey:" + line);
  //字符长1049
  StaticJsonBuffer<2048> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(line);
  String gatheredStr = root["access_token"].as<String>();
  return gatheredStr;
}


String CloudSpeechClient::getToken_http_client(String api_key, String api_secert)
{
  String req_url = "https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=" + api_key + "&client_secret=" + api_secert;
  http_client.begin(req_url);
  int http_code = http_client.GET();
  String rsp;
  String Token = "";
  Serial.println(http_code);

  //Serial.printf("HTTP get code: %d\n", http_code);
  if (http_code == HTTP_CODE_OK)
  {
    rsp = http_client.getString();
    if (rsp.length() > 0)
      Token = Findkey(rsp);
  }
  //Serial.println(Token);
  baidu_Token = Token;
  return Token;
}

String CloudSpeechClient::getToken(String api_key, String api_secert)
{
  while (!client.connect("tsn.baidu.com", 80)) {
    Serial.println("connection failed");
  }

  //以下信息通过 Fiddler 工具分析协议，Raw部分获得
  String url = "https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=" + api_key + "&client_secret=" + api_secert;
  String  HttpHeader = String("GET ") + String(url) +
                       String(" HTTP/1.1\r\nHost: openapi.baidu.com\r\nConnection: keep-alive") + String("\r\n\r\n");
#ifdef SHOW_DEBUG
  Serial.println(HttpHeader);
#endif
  client.print(HttpHeader);
  String retstr = "";
  String line = "";
  uint32_t starttime = 0;
  uint32_t stoptime = 0;
  starttime = millis() / 1000;
  bool head_ok = false;
  bool http_ok = false;
  int resultsize = 0;
  String tmpstr;
  bool chunked = false;
  bool head_ok_2 = false;
  bool head_ok_3 = false;
  bool data_ok = false;
  while (!client.available())
  {
    stoptime = millis() / 1000;
    if (stoptime - starttime >= 5)
    {
#ifdef SHOW_DEBUG
      Serial.println("timeout >5s");
#endif
      return "";
    }
  }
  Serial.println("处理返回数据 ...");

  while (client.available())
  {

    if (head_ok == true && chunked && line.length() > 1 )
    {
      //等于1说明是空串（读到的1个字符是回车！）
      Serial.println("-----------------");
      //如果超过65535可能会有问题!
      if (resultsize == 0)
      {
        //解析的数字是410,实际读到的字串长度是1050 ，两者关系？
        Serial.println("line=" + line + ",len=" + line.length());
        resultsize = line.toInt();
      }
      else
      {
        Serial.println("line=" + line + ",len=" + line.length());
        if (line == "0\r")
        {
          data_ok = true;
          Serial.println("data_ok");
        }
        else if (line.length() > 10)
          retstr = Findkey(line);

      }

    }

    //返回值不含\n
    line = client.readStringUntil('\n');

    if (line.startsWith("HTTP/1.1"))
    {
      Serial.println(">" + line);
      if (line.startsWith("HTTP/1.1 200 OK"))
        http_ok = true;

    }
    //    if (line.startsWith("Content-Length: "))
    //    {
    //      tmpstr = line.substring(String("Content-Length: ").length(), line.length());
    //
    //      Serial.println(">Content-Length: " + tmpstr);
    //
    //      //如果超过65535可能会有问题!
    //      resultsize = tmpstr.toInt();
    //    }
    if (line.length() == 1 && line.startsWith("\r"))
    {
      if (head_ok == false)
      {
        head_ok = true;
        Serial.println(">end 1");
      }
      else if (head_ok_2 == false)
      {
        head_ok_2 = true;
        Serial.println(">end 2");
      }
      else
      {
        head_ok_3 = true;
        Serial.println(">end 3");
        if (data_ok)
          Serial.println("read http end...");  //此时，数据读取该结束了...
      }

    }
    if (line.startsWith("Transfer-Encoding: chunked"))
    {
      chunked = true;
      Serial.println(">chunked");
    }

    Serial.println(line);
    delay(10);
    yield();
  }

  delay(10);
  client.stop();
  baidu_Token = retstr;
  //Serial.println("baidu_Token=" + baidu_Token);
  return (retstr);
}


//输入来源：sound_bodybuff, pre_sound_buff两秒缓冲环
uint32_t CloudSpeechClient::PrintHttpBody2()
{
  //当前已处理字节数
  uint32_t now_read_num = 0;
  //下一批次计划读取字节数
  uint32_t readnum = 0;

  String enc = "";
  uint32_t writenum = 0;
  //发送字节总计
  uint32_t writenum_enc = 0;
  //循环次数，用于输出进度用
  uint32_t   cnt = 0;

  //当前实际发送字节数
  uint32_t   active_write = 0;
  uint32_t  now_write = 0;

  //多出的4个字节是为了 base64 凑倍数用的
  uint32_t fileSizeMinus8 =  sound_bodybuff_p  + pre_maxnum_sound_buff + 4 + 44 - 8;
  wav_head[4] = (byte)(fileSizeMinus8 & 0xFF);
  wav_head[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  wav_head[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  wav_head[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);

  fileSizeMinus8 =   sound_bodybuff_p + pre_maxnum_sound_buff + 4 ;
  wav_head[40] = (byte)(fileSizeMinus8 & 0xFF);
  wav_head[41] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  wav_head[42] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  wav_head[43] = (byte)((fileSizeMinus8 >> 24) & 0xFF);


  //1.发送wav head
  enc = base64::encode((byte*)wav_head, headerSize + 4);
  enc.replace("\n", "");// delete last "\n"

  //注意:此处需要循环发送,确保数据正确发送完成
  enc.toCharArray((char *)buff_base64, enc.length() + 1);
  active_write = 0;
  //反复发送，确保都发送出去
  //注意：此处有坑！！！
  while (active_write < enc.length())
  {
    now_write = client.write(buff_base64 + active_write, enc.length() - active_write);
    if (now_write != (enc.length() - active_write))
    {
      //Serial.println("待发送：" + String( enc.length() - active_write) + " 实际发送:" + String(now_write));
      delay(10);
    }
    active_write = active_write + now_write;
    writenum_enc = writenum_enc + now_write;
  }
  writenum = headerSize + 4;

  //2.发送缓存环的2秒音频数据
  //2.1缓存环的后半段
  now_read_num = pre_sound_buf_p;
  //注意：  size must be multiple of 3 for Base64 encoding. Additional byte size must be even because wave data is 16bit.
  while (now_read_num < pre_maxnum_sound_buff)
  {
    //数值不宜过大，否则容易网络中断
    //用1020是为了必须是3的倍数，同时为2的倍数
    readnum = 1020;
    if (pre_maxnum_sound_buff - now_read_num >= readnum)
    {
      memcpy(buff , pre_sound_buff + now_read_num, readnum);
    }
    else
    {
      readnum = pre_maxnum_sound_buff - now_read_num;

      //不是6的整数倍,余数给去除
      int tmpint = readnum % 6;
      if (tmpint > 0)
      {
        readnum = readnum - tmpint;
        now_read_num = now_read_num + tmpint;
        writenum = writenum + tmpint;
        //Serial.println("pass:" + String(tmpint) );

        //Serial.println(String(">loss: ") + String(tmpint));

      }
      memcpy(buff , pre_sound_buff + now_read_num, readnum);
    }


    if (readnum > 0)
    {
      enc = base64::encode((byte*)buff, readnum);
      enc.replace("\n", "");// delete last "\n"

      //注意:此处需要循环发送,确保数据正确发送完成
      enc.toCharArray((char *)buff_base64, enc.length() + 1);
      active_write = 0;
      //反复发送，确保都发送出去
      //注意：此处有坑！！！
      while (active_write < enc.length())
      {
        now_write = client.write(buff_base64 + active_write, enc.length() - active_write);
        if (now_write != (enc.length() - active_write))
        {
          //Serial.println("Content-Length已发送字节:" + String(writenum_enc) + " 待发送：" + String(enc.length() - active_write) + " 实际发送:" + String(now_write));
          delay(10);
        }
        active_write = active_write + now_write;
        writenum_enc = writenum_enc + now_write;
      }

      if (cnt % 10 == 0)
      {
        Serial.print(">");
        //Serial.println("Content-Length已发送字节:" + String(writenum_enc));
      }

      cnt = cnt + 1;
    }
    now_read_num = now_read_num + readnum;
    writenum = writenum + readnum;
  }


  //2.2 缓存环的前半段
  if (pre_sound_buf_p > 0)
  {
    now_read_num = 0;
    //注意：  size must be multiple of 3 for Base64 encoding. Additional byte size must be even because wave data is 16bit.
    while (now_read_num < pre_sound_buf_p)
    {
      //数值不宜过大，否则容易网络中断
      //用1020是为了必须是3的倍数，同时为2的倍数
      readnum = 1020;
      if (pre_sound_buf_p - now_read_num >= readnum)
      {
        memcpy(buff , pre_sound_buff + now_read_num, readnum);
      }
      else
      {
        readnum = pre_sound_buf_p - now_read_num;
        //不是6的整数倍,余数给去除
        int tmpint = readnum % 6;
        if (tmpint > 0)
        {
          readnum = readnum - tmpint;
          now_read_num = now_read_num + tmpint;
          writenum = writenum + tmpint;
          //Serial.println("pass:" + String(tmpint) );

          //Serial.println(String(">loss: ") + String(tmpint));

        }
        memcpy(buff , pre_sound_buff + now_read_num, readnum);
      }


      if (readnum > 0)
      {
        enc = base64::encode((byte*)buff, readnum);
        enc.replace("\n", "");// delete last "\n"

        //注意:此处需要循环发送,确保数据正确发送完成
        enc.toCharArray((char *)buff_base64, enc.length() + 1);
        active_write = 0;
        //反复发送，确保都发送出去
        //注意：此处有坑！！！
        while (active_write < enc.length())
        {
          now_write = client.write(buff_base64 + active_write, enc.length() - active_write);
          if (now_write != (enc.length() - active_write))
            delay(10);
          active_write = active_write + now_write;
          writenum_enc = writenum_enc + now_write;
        }

        if (cnt % 10 == 0)
        {
          Serial.print(">");
          //Serial.println("Content-Length已发送字节:" + String(writenum_enc));
        }

        cnt = cnt + 1;
      }  //end if (readnum > 0)
      now_read_num = now_read_num + readnum;
      writenum = writenum + readnum;
    }
  }

  //3.发送主要的录音数据
  now_read_num = 0;
  //注意：  size must be multiple of 3 for Base64 encoding. Additional byte size must be even because wave data is 16bit.

  //Serial.println("待处理字节:" + String(sound_bodybuff_p) + " 预期发送字节:" + String(sound_bodybuff_p * 4 / 3));

  while (now_read_num < sound_bodybuff_p)
  {
    //Serial.println("0 now_read_num:" + String(now_read_num) + " sound_bodybuff_p=" + String(sound_bodybuff_p));

    //用1020是为了必须是3的倍数，同时为2的倍数
    readnum = 1020;
    if (sound_bodybuff_p - now_read_num >= readnum)
    {
      memcpy(buff , sound_bodybuff + now_read_num, readnum);
    }
    else
    {
      readnum = sound_bodybuff_p - now_read_num;
      //不是6的整数倍,余数给去除
      int tmpint = readnum % 6;
      if (tmpint > 0)
      {
        readnum = readnum - tmpint;
        now_read_num = now_read_num + tmpint;
        writenum = writenum + tmpint;
        //Serial.println("pass:" + String(tmpint) );

        // Serial.println(String(">loss: ") + String(tmpint));

      }
      memcpy(buff , sound_bodybuff + now_read_num, readnum);
    }

    //Serial.println("1 now_read_num:" + String(now_read_num) + " sound_bodybuff_p=" + String(sound_bodybuff_p));
    //Serial.println("readnum:" + String(readnum) );

    if (readnum > 0)
    {
      enc = base64::encode((byte*)buff, readnum);
      enc.replace("\n", "");// delete last "\n"
      //注意:此处需要循环发送,确保数据正确发送完成
      enc.toCharArray((char *)buff_base64, enc.length() + 1);
      active_write = 0;
      //反复发送，确保buff_base64 的字节数enc.length() 都发送出去
      //注意：此处有坑！！！
      while (active_write < enc.length())
      {
        now_write = client.write(buff_base64 + active_write, enc.length() - active_write);
        if (now_write != (enc.length() - active_write))
        {
          //Serial.println("Content-Length已发送字节:" + String(writenum_enc) + " 待发送：" + String(enc.length() - active_write) + " 实际发送:" + String(now_write));
          delay(10);
        }
        active_write = active_write + now_write;
        writenum_enc = writenum_enc + now_write;
      }

      if (cnt % 10 == 0)
      {
        Serial.print(">");
        //Serial.println("Content-Length已发送字节:" + String(writenum_enc));
      }

      cnt = cnt + 1;
    }
    now_read_num = now_read_num + readnum;
    writenum = writenum + readnum;

  }

  //Serial.println("writenum:" + String(writenum) + " now_read_num= " + String(now_read_num) + " writenum_enc=" + String(writenum_enc));
  return (writenum_enc);
}


//examples:
//{"err_msg":"json read error.","err_no":3300,"sn":""}
//{"corpus_no":"6641105603733134892","err_msg":"success.","err_no":0,"result":["今天天气不错。"],"sn":"747471463051546252892"}
String CloudSpeechClient::Find_baidutext(String line)
{
  const char* p_result;
  const char* p_err_msg;
  String err_msg = "";
  String result_msg = "";
#ifdef SHOW_DEBUG
  Serial.println(">" + line);
#endif
  if (line.indexOf("err_msg") > 0)
  {
    StaticJsonBuffer<512> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(line);
    p_err_msg =  root["err_msg"];
    err_msg = String(p_err_msg);
    if (err_msg.startsWith("success."))
    {
      p_result = root["result"][0];
      result_msg = String(p_result );
    }
    if (err_msg.startsWith("speech quality error."))
    {
      result_msg = "speech quality error";
    }

  }
  return (result_msg);
}

//语音转文字
String CloudSpeechClient::getVoiceText()
{
  if (baidu_Token.length() == 0)
    return ("");

  uint32_t wav_filesize =  headerSize + 4 + sound_bodybuff_p + pre_maxnum_sound_buff ;
  //Serial.println(String("FileName:") + fn + " FileSize=" + String(wav_filesize));
  //保证字节数是6的倍数，否则base64转换会异常
  //wav 除不尽, 损失几个字节无关紧要

  //声音文件，丢掉一些数据
  uint32_t tmpint = sound_bodybuff_p % 6;
  if (tmpint > 0)
  {

    //Serial.println(String(">loss: ") + String(tmpint));

    wav_filesize = wav_filesize - tmpint;
  }

  //缓存后半段丢掉一些数据
  tmpint = (pre_maxnum_sound_buff - pre_sound_buf_p) % 6;

  if (tmpint > 0)
  {

    //Serial.println(String(">loss: ") + String(tmpint));

    wav_filesize = wav_filesize - tmpint;
  }
  // 缓存前半段丢掉一些数据
  tmpint = pre_sound_buf_p % 6;

  if (tmpint > 0)
  {

   // Serial.println(String(">loss: ") + String(tmpint));

    wav_filesize = wav_filesize - tmpint;
  }


  //Serial.println(wav_filesize);
  if (!client.connect("vop.baidu.com", 80)) {

    Serial.println("connection failed");

    return ("");
  }

  uint32_t httpBody2Length = wav_filesize * 4 / 3; //  4/3 is from base64 encoding

  //Serial.println(String("httpBody2Length:") + String(httpBody2Length));

  //20210131增加 dev_pid参数
  String HttpBody1 = String("") + "{\"format\":\"wav\", \"channel\":1, \"cuid\":\"python_test\"," +
                     "\"token\":\"" + baidu_Token + "\"," +   "\"dev_pid\":1537," +
                     " \"rate\":16000, \"len\":" + String(wav_filesize) + ",\"speech\":\"";
  String HttpBody3 = "\"}";
  String ContentLength = String(HttpBody1.length() + httpBody2Length + HttpBody3.length());

  // Serial.println("ContentLength:" + String(ContentLength));

  String HttpHeader = String("POST ") + String(upvoice_url) + " HTTP/1.1\r\n" +
                      "Content-Type: application/json\r\n" +
                      "Host: vop.baidu.com\r\n" +
                      "Content-Length: " + String(ContentLength) + "\r\n" +
                      "Connection: keep-alive\r\n\r\n";


  //Serial.println(String(HttpHeader) + String(HttpBody1));


  client.print(HttpHeader);

  //Serial.println("send HttpBody");

  client.print(HttpBody1);

  uint32_t writenum_enc = PrintHttpBody2();


  //Serial.println(String("bodylen:") + String(HttpBody1.length() + writenum_enc + HttpBody3.length()));
  //Serial.println(HttpBody3);


  client.print(HttpBody3);


  //Serial.println("wait response");

  String line = "";
  bool http_ok = false;
  bool head_ok = false;
  uint32_t resultsize = 0;
  String findresult = "";
  String tmpstr = "";

  uint32_t starttime = 0;
  uint32_t stoptime = 0;
  starttime = millis() / 1000;

  while (!client.available())
  {
    stoptime = millis() / 1000;
    if (stoptime - starttime >= 10)
    {

      //Serial.println("response time out >10s");

      return "";
    }
    delay(200);
  }

  //HTTP/1.1 500 Internal Server Error
  //{"corpus_no":"6641105603733134892","err_msg":"success.","err_no":0,"result":["今天天气不错。"],"sn":"747471463051546252892"}
  while (client.available())
  {
    if (head_ok == true)
    {
      //Serial.println("head_ok quit");
      if (http_ok  && resultsize > 0)
      {
        if (resultsize <= 1024)
        {
          // Serial.println(resultsize);
          client.read(buff, resultsize);
          char tmparray[resultsize + 1] ;
          memcpy(tmparray, buff, resultsize);

          tmparray[resultsize] = 0;
          // Serial.println(resultsize);
          findresult = Find_baidutext( String(tmparray));
        }
      }
      break;
    }
    line = client.readStringUntil('\n');

    if (line.startsWith("HTTP/1.1"))
    {

      // Serial.println(">" + line);

      if (line.startsWith("HTTP/1.1 200 OK"))
        http_ok = true;
      //line不含'\n'
      //Serial.println(String("HTTP/1.1 200 OK").length());
      // Serial.println(line.length());
    }
    if (line.startsWith("Content-Length: "))
    {
      tmpstr = line.substring(String("Content-Length: ").length(), line.length());

      // Serial.println(">Content-Length: " + tmpstr);

      //如果超过65535可能会有问题!
      resultsize = tmpstr.toInt();
    }
    if (line.length() == 1 && line.startsWith("\r"))
    {
      head_ok = true;
      //Serial.println(">end");
    }

    //Serial.println(line);

  }
  delay(10);
  client.stop();
  return (findresult);
}



//有些路由器对自建服务器会支持不好，原因不明
//自制 LED类控制，connect 这一步会失败，原因不明！
String CloudSpeechClient::posturl(String host, int port, String url)
{

  // Serial.println(String("host:") + host + " url:" + url);

  //可能有啥技巧不了解
  if (!client.connect(host.c_str(), port))
  {
#ifdef SHOW_DEBUG
    Serial.println("posturl connection failed");
#endif
    return ("posturl connection failed");
  }
  String  HttpHeader = String("GET ") + url + " HTTP/1.1\r\n" +
                       "Host: " + host + ":" + String(port) + "\r\n" +
                       "Connection: keep-alive\r\n\r\n" ;

#ifdef SHOW_DEBUG
  Serial.println(HttpHeader);
#endif
  client.print(HttpHeader);

  String retstr = "";
  String line = "";

  bool http_ok = false;
  bool head_ok = false;
  uint32_t resultsize = 0;

  String tmpstr = "";
  uint32_t starttime = 0;
  uint32_t stoptime = 0;
  starttime = millis() / 1000;
  while (!client.available())
  {
    stoptime = millis() / 1000;
    if (stoptime - starttime >= 5)
    {
#ifdef SHOW_DEBUG
      Serial.println("timeout >5s");
#endif
      return "";
    }
  }
  //Content-Type: text/plain
  while (client.available())
  {
    if (head_ok == true)
    {
      if (http_ok && resultsize > 0)
      {
        //Serial.println(resultsize);
        if (resultsize < 1024)
        {
          client.read(buff, resultsize);
          char tmparray[resultsize + 1] ;
          memcpy(tmparray, buff, resultsize);
          tmparray[resultsize] = 0;
          retstr =  String(tmparray);
        }
        break;
      }
    }
    line = client.readStringUntil('\n');

    if (line.startsWith("HTTP/1.1 200 OK"))
    {
      http_ok = true;
#ifdef SHOW_DEBUG
      Serial.println(">HTTP/1.1 200 OK");
#endif
      //line不含'\n'
      //Serial.println(String("HTTP/1.1 200 OK").length());
      // Serial.println(line.length());
    }
    if (line.startsWith("Content-Length: "))
    {
      tmpstr = line.substring(String("Content-Length: ").length(), line.length());
#ifdef SHOW_DEBUG
      Serial.println(">Content-Length: " + tmpstr );
#endif
      //Serial.println();
      //如果超过65535可能会有问题!
      resultsize = tmpstr.toInt();
    }

    if (line.length() == 1 && line.startsWith("\r"))
    {
      head_ok = true;
      //Serial.println(">end");
    }
    //Serial.println(line);
  }
  delay(10);
  client.stop();
#ifdef SHOW_DEBUG
  Serial.println("用时:" + String( millis() / 1000 - starttime) + "秒");
#endif
  return (retstr);
}
