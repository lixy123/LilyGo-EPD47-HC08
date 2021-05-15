#include <WiFi.h>
#include <Preferences.h>


class smartconfigManager
{
  private:    
    //Preferences 的参数重烧固件会仍会存在！
    Preferences preferences;
  public:
    String ssid = "";
    String password = "";
      
    bool connect_cnt=0;  //判断是否成功连接成功过
    bool connect_error_cnt=0;  //多次连接一直出错次数
    smartconfigManager();
    ~smartconfigManager();

    void writeparams();
    void readparams();

    bool smartConfig(int try_num);
    bool connectwifi();
};
