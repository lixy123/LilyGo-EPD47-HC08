#include <WiFi.h>
#include <Preferences.h>


class smartconfigManager
{
  private:
    String ssid = "";
    String password = "";
    //Preferences 的参数重烧固件会仍会存在！
    Preferences preferences;
  public:
    smartconfigManager();
    ~smartconfigManager();

    void writeparams();
    void readparams();

    bool smartConfig(int try_num);
    void connectwifi();
};
