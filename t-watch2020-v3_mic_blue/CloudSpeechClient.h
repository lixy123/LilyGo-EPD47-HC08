#define SHOW_DEBUG
#include "Wav.h"
#include "SPIFFS.h"
#include <base64.h>
#include <ArduinoJson.h>
//https://blog.csdn.net/lgj123xj/article/details/78626160
#include "urlencode.h"
#include <WiFi.h>
#include "HTTPClient.h"

class CloudSpeechClient {
    WiFiClient client;
    HTTPClient http_client;  //这个层次更高，用起来更方便
    String Findkey(String line);
    const char * token_url = "https://openapi.baidu.com/oauth/2.0/token";
    //const char * getvoice_url  = "http://tsn.baidu.com/text2audio";
    const char * upvoice_url = "http://vop.baidu.com/server_api";
    const char * cu_id = "cu_id";
    char *   textfile = "/text.wav";

    String baidu_Token = "";
    uint32_t PrintHttpBody2();
    String Find_baidutext(String line);

  public:
    uint8_t buff[1024];
    uint8_t buff_base64[2048];
    CloudSpeechClient(int sec);
    ~CloudSpeechClient();

    const char *   recordfile = "record";

    const int headerSize = 44;
    //这个psram版本就是提前分配了一个内存区,专用于存放声音数据,调用百度声音识别时不需要借助于SD卡做为交换.
    //wav文件头的44个字节, 为base64 处理需要, 必须是6的倍数,  多加点数据
    byte * wav_head;

    //2秒就够了
    //定义保存pre_sec秒音频的空间，用于检测非静音阶段，会用于百度识别
    //检测是否有动静阶段用 ,
    int pre_sec = 2;
    uint32_t pre_maxnum_sound_buff;
    byte* pre_sound_buff;
    uint32_t pre_sound_buf_p;
    //数据存入缓存
    void pre_push_sound_buff(byte * src_buff, uint32_t len);

    //清空下2秒缓存
    void zero_pre_push_sound_buff();


    //定义保存20秒音频的空间，用于录音阶段存储声音， 会用于百度识别
    //约 640kB 音频信号就不用在TF卡上交换了
    const uint32_t maxnum_bodysound__buff = 32000 * 12;
    byte* sound_bodybuff;
    uint32_t sound_bodybuff_p;
    //数据存入缓存
    void push_bodybuff_buff(byte * src_buff, uint32_t len);

    //返回口令字符串
    String getToken(String api_key, String api_secert);

    //返回口令字符串
    //http_client的版本，更易用
    String getToken_http_client(String api_key, String api_secert);

    //语音转文本
    String getVoiceText();

    //本地录音文件上传(检测结果用！)
    bool uploadfile(String ftp_host, int ftp_port, String fn);

    String posturl(String host, int port, String url);

};
