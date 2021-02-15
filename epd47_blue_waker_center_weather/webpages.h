const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
</head>
<body>
  <p><h1>墨水屏消息上传</h1>  </p> 

  <form method="GET" action="/get_msg">
    input1: <input type="text" style="width:350px"  name="txt_msg">
    <input type="submit" value="Submit">    
  </form><br>
  <button onclick="rebootButton()">重启</button> <br/>
  <div id="status"> </div>
<script>


function rebootButton() {
  document.getElementById("status").innerHTML = "esp32 重启 ...";
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/reboot", true);
  xhr.send();
  window.open("/reboot","_self");
}



</script>
</body>
</html>
)rawliteral";


// reboot.html base upon https://gist.github.com/Joel-James/62d98e8cb3a1b6b05102
const char reboot_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
</head>
<body>
<h3>
  Rebooting, returning to main page in <span id="countdown">20</span> seconds
</h3>
<script type="text/javascript">
  var seconds = 20;
  function countdown() {
    seconds = seconds - 1;
    if (seconds < 0) {
      window.location = "/";
    } else {
      document.getElementById("countdown").innerHTML = seconds;
      window.setTimeout("countdown()", 1000);
    }
  }
  countdown();
</script>
</body>
</html>
)rawliteral";
