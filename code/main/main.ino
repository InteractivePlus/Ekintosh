#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <stdarg.h>
#include <WiFiClient.h>
#include <math.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define SPEAKER_PIN 2     // Speaker Pin

struct WLANConfig {
  char ssid[32];
  char passwd[64];

  String s;
};

struct TextBlock {
  int x, y, width, height;
  const char* text;
};

struct FancyGrid {
  int x, y, width, height, iconWidth, iconHeight;
  const uint8_t* icon;
  const char* title;

  struct TextBlock text;
};

struct Button {
  int x, y, width, height;
  bool selected;
  
  const char* text;
};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* AP_NAME = "Ekintosh_001";
const char* page_html = "\
<!DOCTYPE html>\r\n\
<html lang='en'>\r\n\
<head>\r\n\
  <meta charset='UTF-8'>\r\n\
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n\
  <title>Document</title>\r\n\
</head>\r\n\
<body>\r\n\
  <form name='input' action='/' method='POST'>\r\n\
        wifi名称: <br>\r\n\
        <input type='text' name='ssid'><br>\r\n\
        wifi密码:<br>\r\n\
        <input type='text' name='password'><br>\r\n\
        <input type='submit' value='保存'>\r\n\
    </form>\r\n\
</body>\r\n\
</html>\r\n\
";

//CPU icon 16 * 16
static const uint8_t iconCPU[] = {
  0xff, 0xff, 0xf9, 0x9f, 0xc0, 0x03, 0xdf, 0xfb, 0xdf, 0xfb, 0x98, 0x19, 0x98, 0x19, 0xd9, 0x9b,
  0xd9, 0x9b, 0x98, 0x19, 0x98, 0x19, 0xdf, 0xfb, 0xdf, 0xfb, 0xc0, 0x03, 0xf9, 0x9f, 0xff, 0xff};

//Memory icon 14 * 10
static const uint8_t iconMemory[] = {
  0x80, 0x04, 0xbf, 0xf4, 0xb0, 0x34, 0xd7, 0xac, 0xd7, 0xac, 0xb0, 0x34, 0xbf, 0xf4, 0xbf, 0x74,
  0x80, 0x04, 0xff, 0xfc};

//Disk icon 14 * 11
static const uint8_t iconDisk[] = {
  0xff, 0xfc, 0xff, 0xf4, 0x80, 0x04, 0xbf, 0xf4, 0x9f, 0xf4, 0xbf, 0xf4, 0x9f, 0xf4, 0x9f, 0xf4, 
  0xbc, 0x34, 0x81, 0x84, 0x81, 0x84};

//Clock icon 8 * 10
static const uint8_t iconClock[] = {
  0xff, 0xff, 0xfd, 0xfb, 0xf7, 0xf7, 0xfb, 0xfd, 0xfe, 0xff};

//
static const uint8_t iconClock_16x16[] = {
  0xff, 0xff, 0xff, 0xff, 0xe4, 0x2f, 0xc3, 0xc3, 0xef, 0xf7, 0xdf, 0xfb, 0xdf, 0x7b, 0xdf, 0x7b, 
  0xbf, 0x7d, 0xdf, 0x7b, 0xdf, 0x9b, 0xdf, 0xcb, 0xef, 0xef, 0xf7, 0xcf, 0xf8, 0x1f, 0xff, 0xff};

static const uint8_t iconApple_9x9[] = {
  0xff, 0x80, 0xfb, 0x80, 0xff, 0x80, 0xc1, 0x80, 0xc1, 0x80, 0xc1, 0x80, 0xc1, 0x80, 0xc1, 0x80, 
  0xff, 0x80};

static const uint8_t iconApple_9x9_BLACK[] = {
  0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x3e, 0x00, 
  0x00, 0x00};

//Macintosh icon 16*16
static const uint8_t iconMacintosh[] = {
  0xc0, 0x03, 0xa0, 0x05, 0x8f, 0xf1, 0x8b, 0xd1, 0x8b, 0x51, 0x8f, 0x71, 0x8e, 0x71, 0x8d, 0xf1, 
  0x8e, 0x71, 0xa0, 0x05, 0xbf, 0xfd, 0xa7, 0x85, 0xbf, 0xfd, 0x80, 0x01, 0xdf, 0xfb, 0xc0, 0x03};

static const uint8_t iconMacintosh_32x32[] = {
  0x07, 0xff, 0xff, 0xe0, 0x0c, 0x00, 0x00, 0x30, 0x18, 0x00, 0x00, 0x18, 0x13, 0xff, 0xff, 0xc8, 
  0x16, 0x00, 0x00, 0x68, 0x16, 0x00, 0x00, 0x68, 0x16, 0x00, 0x00, 0x68, 0x16, 0x10, 0x08, 0x68, 
  0x16, 0x10, 0x88, 0x68, 0x16, 0x10, 0x88, 0x68, 0x16, 0x10, 0x88, 0x68, 0x16, 0x00, 0x80, 0x68, 
  0x16, 0x01, 0x80, 0x68, 0x16, 0x00, 0x00, 0x68, 0x16, 0x00, 0x00, 0x68, 0x16, 0x0c, 0x30, 0x68, 
  0x16, 0x0f, 0xf0, 0x68, 0x16, 0x00, 0x00, 0x68, 0x16, 0x00, 0x00, 0x68, 0x13, 0xff, 0xff, 0xc8, 
  0x10, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x08, 0x13, 0xc0, 0x3f, 0xc8, 
  0x10, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x08, 0x18, 0x00, 0x00, 0x18, 0x1f, 0xff, 0xff, 0xf8, 
  0x08, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x10, 0x0f, 0xff, 0xff, 0xf0};

static const uint8_t iconMacintosh_32x32_BLACK[] = {
  0xf8, 0x00, 0x00, 0x1f, 0xf3, 0xff, 0xff, 0xcf, 0xe7, 0xff, 0xff, 0xe7, 0xec, 0x00, 0x00, 0x37, 
  0xe9, 0xff, 0xff, 0x97, 0xe9, 0xff, 0xff, 0x97, 0xe9, 0xff, 0xff, 0x97, 0xe9, 0xef, 0xf7, 0x97, 
  0xe9, 0xef, 0x77, 0x97, 0xe9, 0xef, 0x77, 0x97, 0xe9, 0xef, 0x77, 0x97, 0xe9, 0xff, 0x7f, 0x97, 
  0xe9, 0xfe, 0x7f, 0x97, 0xe9, 0xff, 0xff, 0x97, 0xe9, 0xff, 0xff, 0x97, 0xe9, 0xf3, 0xcf, 0x97, 
  0xe9, 0xf0, 0x0f, 0x97, 0xe9, 0xff, 0xff, 0x97, 0xe9, 0xff, 0xff, 0x97, 0xec, 0x00, 0x00, 0x37, 
  0xef, 0xff, 0xff, 0xf7, 0xef, 0xff, 0xff, 0xf7, 0xef, 0xff, 0xff, 0xf7, 0xec, 0x3f, 0xc0, 0x37, 
  0xef, 0xff, 0xff, 0xf7, 0xef, 0xff, 0xff, 0xf7, 0xe7, 0xff, 0xff, 0xe7, 0xe0, 0x00, 0x00, 0x07, 
  0xf7, 0xff, 0xff, 0xef, 0xf7, 0xff, 0xff, 0xef, 0xf7, 0xff, 0xff, 0xef, 0xf0, 0x00, 0x00, 0x0f};

//Weather Sunny icon 16*16
static const uint8_t iconSunny[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xf7, 0xf7, 0xef, 0xfc, 0x3f, 0xfb, 0xdf, 0xfb, 0xdf, 
  0xfb, 0xdf, 0xfb, 0xdf, 0xfc, 0x3f, 0xf7, 0xef, 0xef, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static const uint8_t iconTrash_29x29[] = {
  0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xe0, 0x1f, 0xf8, 
  0xff, 0xe0, 0x1f, 0xf8, 0xff, 0xe7, 0x9f, 0xf8, 0xff, 0xe7, 0x9f, 0xf8, 0xf8, 0x00, 0x00, 0x78, 
  0xf8, 0x00, 0x00, 0x78, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xf8, 0xff, 0x73, 0x3b, 0xf8, 
  0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 
  0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 
  0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x73, 0x39, 0xf8, 0xfe, 0x7f, 0xf9, 0xf8, 
  0xfe, 0x7f, 0xf9, 0xf8, 0xfe, 0x00, 0x01, 0xf8, 0xff, 0x00, 0x03, 0xf8, 0xff, 0xff, 0xff, 0xf8, 
  0xff, 0xff, 0xff, 0xf8};

// ************************
// * START WEB CODE SPACE *
// ************************
struct WLANConfig config;
const byte DNS_PORT = 53;       //DNS端口号
bool connected = false;
IPAddress apIP(192, 168, 4, 1); //esp8266-AP-IP地址
DNSServer dnsServer;            //创建dnsServer实例
ESP8266WebServer server(80);    //创建WebServer

void handleRoot() {
  //访问主页回调函数
  server.send(200, "text/html", page_html);
}

void handleRootPost() {
  //Post回调函数
  Serial.println("handleRootPost");
  if (server.hasArg("ssid")) {//判断是否有账号参数
    Serial.print("got ssid:");
    strcpy(config.ssid, server.arg("ssid").c_str());//将账号参数拷贝到sta_ssid中
    Serial.println(config.ssid);
  } else {//没有参数
    Serial.println("error, not found ssid");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");//返回错误页面
    return;
  }
  //密码与账号同理
  if (server.hasArg("password")) {
    Serial.print("got password:");
    strcpy(config.passwd, server.arg("password").c_str());
    Serial.println(config.passwd);
  } else {
    Serial.println("error, not found password");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }

  server.send(200, "text/html", "<meta charset='UTF-8'>保存成功");//返回保存成功页面
  delay(2000);
  //连接wifi
  connectNewWifi();
}

void initSoftAP(void) {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if(WiFi.softAP(AP_NAME)){
    Serial.println("ESP8266 SoftAP is right");
  }
}

void initWebServer(void) {
  //server.on("/",handleRoot);
  //上面那行必须以下面这种格式去写否则无法强制门户
  server.on("/", HTTP_GET, handleRoot);//设置主页回调函数
  server.onNotFound(handleRoot);//设置无法响应的http请求的回调函数
  server.on("/", HTTP_POST, handleRootPost);//设置Post请求回调函数
  server.begin();//启动WebServer
  Serial.println("WebServer started!");
}

void initDNS(void) {
  if(dnsServer.start(DNS_PORT, "*", apIP)){//判断将所有地址映射到esp8266的ip上是否成功
    Serial.println("start dnsserver success.");
  }
  else Serial.println("start dnsserver failed.");
}

void connectNewWifi(void) {
  WiFi.mode(WIFI_STA);//切换为STA模式
  WiFi.setAutoConnect(true);//设置自动连接

  if(config.ssid[0] == '\0') {
    loadConfig();
  }

  if(config.ssid[0] == '\0') {
    WiFi.begin();
  }
  else {
    WiFi.begin(config.ssid, config.passwd);//连接上一次连接成功的wifi
  }

  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    count++;
    if(count > 64){//如果5秒内没有连上，就开启Web配网 可适当调整这个时间
      initSoftAP();
      initWebServer();
      initDNS();
      break;//跳出 防止无限初始化
    }
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {//如果连接上 就输出IP信息 防止未连接上break后会误输出
    Serial.println(WiFi.localIP());//打印esp8266的IP地址
    server.stop();

    saveConfig();//调用保存函数
    connected = true;
    getWiFiClock();
  }
}
// ************************
// * START WEB CODE SPACE *
// ************************



// **************************
// * START FLASH CODE SPACE *
// **************************
void saveConfig() {
 EEPROM.begin(1024);//向系统申请1024kb ROM
 //开始写入
 uint8_t *p = (uint8_t*)(&config);
  for (int i = 0; i < sizeof(config); i++)
  {
    EEPROM.write(i, *(p + i)); //在闪存内模拟写入
  }
  EEPROM.commit();//执行写入ROM
}

void loadConfig() {
  EEPROM.begin(1024);
  uint8_t *p = (uint8_t*)(&config);
  for (int i = 0; i < sizeof(config); i++)
  {
    *(p + i) = EEPROM.read(i);
  }
  EEPROM.commit();
}
// **************************
// * START FLASH CODE SPACE *
// **************************



// **************************
// * START SOUND CODE SPACE *
// **************************
void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SPEAKER_PIN, LOW);
    delayMicroseconds(tone);
  }
}
  
void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };
  
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void playStartUpTone(void) {
  int length = 1;      // the number of notes
  char notes[] = "g "; // a space represents a rest
  int beats[] = { 6 };
  int tempo = 100;

  for (int i = 0; i < length; i++) {
    if (notes[i] == ' ') {
      delay(beats[i] * tempo);
    } else {
      playNote(notes[i], beats[i] * tempo);
    }
    delay(tempo / 2);
  }
}
// ************************
// * END SOUND CODE SPACE *
// ************************



// ************************
// * START GUI CODE SPACE *
// ************************
void drawWindow(const char* title, const int titleHeight, const int borderWidth, const int controlCount, ...) {
  va_list valist;
  va_start(valist, controlCount);

  //Draw all controls
  for (int i = 0; i < controlCount; i++)
  {
    drawFancyGrid(va_arg(valist, struct FancyGrid), titleHeight, borderWidth);
  }

  int pixel = getTextPixel(title);
  int cursorX = SCREEN_WIDTH / 2 - pixel / 2;

  //Window
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 2, WHITE); //border
  display.fillRoundRect(0, 0, SCREEN_WIDTH, 9, 2, WHITE);                 //title bar
  display.drawRect(6, 1, 7, 7, BLACK);                                        //close button

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(cursorX, 1);
  display.println(title);                         //title context

  display.drawLine(1, 1, 4, 1, BLACK);
  display.drawLine(1, 3, 4, 3, BLACK);
  display.drawLine(1, 5, 4, 5, BLACK);
  display.drawLine(1, 7, 4, 7, BLACK);

  display.drawLine(14, 1, cursorX - 5, 1, BLACK);
  display.drawLine(14, 3, cursorX - 5, 3, BLACK);
  display.drawLine(14, 5, cursorX - 5, 5, BLACK);
  display.drawLine(14, 7, cursorX - 5, 7, BLACK);

  display.drawLine(cursorX + pixel + 5, 1, SCREEN_WIDTH - 2, 1, BLACK);
  display.drawLine(cursorX + pixel + 5, 3, SCREEN_WIDTH - 2, 3, BLACK);
  display.drawLine(cursorX + pixel + 5, 5, SCREEN_WIDTH - 2, 5, BLACK);
  display.drawLine(cursorX + pixel + 5, 7, SCREEN_WIDTH - 2, 7, BLACK);

  va_end(valist);
}

void drawFancyGrid(const FancyGrid control, const int titleHeight, const int borderWidth) {
  int x = control.x + borderWidth,
      y = control.y + titleHeight,
      iconWidth = control.iconWidth,
      iconHeight = control.iconHeight,
      width = control.width,
      height = control.height;

  int backgroundX = x + 2;
  int backgroundY = y + 2;

  int iconX = backgroundX + 1;
  int iconY = backgroundY + 1;

  //border
  display.drawRoundRect(x, y, width, height, 2, WHITE);

  //background UP
  display.fillRoundRect(
    iconX + iconWidth - 1, 
    backgroundY, 
    width - iconWidth - 4, 
    iconHeight + 2, 
    2, 
    WHITE); 

  //background DOWN
  display.fillRoundRect(
    backgroundX,
    iconY + iconHeight - 1,
    width - 4,
    height - iconHeight - 4,
    2,
    WHITE);

  display.drawLine(iconX - 1, iconY, iconX - 1, iconY + iconHeight, WHITE);                                   //fix round corner
  display.drawLine(iconX, iconY - 1, iconX + iconWidth, iconY - 1, WHITE);

  display.drawBitmap(iconX, iconY, control.icon, iconWidth, iconHeight, 1);                                   //icon

  drawTextBlock({
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = control.title
    }, iconX + iconWidth, iconY, iconHeight, width - 4 - iconWidth);                 //title

  drawTextBlock(control.text, iconX, iconY + iconHeight, height - 4 - iconHeight, width - 4);
}

void drawTextBlock(const TextBlock control, const int safeZoneX, const int safeZoneY, const int safeZoneHeight, const int safeZoneWidth) {
  int x = control.x + safeZoneX, 
      y = control.y + safeZoneY,
      width = control.width, 
      height = control.height;

  int aboutHeight = getTextPixel(control.text) / safeZoneWidth + 1;
  aboutHeight = aboutHeight * 8;
  int aboutY = (safeZoneHeight - aboutHeight) / 2 + y;

  display.setTextSize(1);
  display.setTextColor(BLACK);

  if(getTextPixel(control.text) < safeZoneWidth - 8) {
    display.setCursor(x + (safeZoneWidth / 2) - (getTextPixel(control.text) / 2), aboutY);
    display.println(control.text);
  } else {
    String tmp = "";
    int count = 0;
    for (int i = 0; i < strlen(control.text); i++) {
      tmp += control.text[i];
      if (getTextPixel(tmp.c_str()) > safeZoneWidth - 8) {
        i--;
        display.setCursor(x + (safeZoneWidth / 2) - (getTextPixel(tmp.c_str()) / 2), aboutY + count * 8);
        display.println(substr(tmp.c_str(), -1));
        tmp = "";
        count ++;
      }
    }
    display.setCursor(x + (safeZoneWidth / 2) - (getTextPixel(tmp.c_str()) / 2), aboutY + count * 8);
    display.println(tmp);
  }
}

void drawStartUp(const uint8_t* icon, const int iconHeight, const int iconWidth) {
  display.clearDisplay();
  display.drawBitmap(SCREEN_WIDTH / 2 - iconWidth / 2, SCREEN_HEIGHT / 2 - iconHeight / 2, icon, iconWidth, iconHeight, 1);
  display.display();
}

void drawHalfFill(const int height, const int width, const int x, const int y, const int safeZoneX, const int safeZoneY, const int safeZoneWidth, const int safeZoneHeight) {
  for (int i = 0; i <= width; i++) {
    for (int j = 0; j <= height; j++) {
      if (i % 2 == j % 2) {
        if ((x + i < safeZoneX || x + i > safeZoneX + safeZoneWidth) || (y + j < safeZoneY || y + j > safeZoneY + safeZoneHeight)) {
          display.drawPixel(x + i, y + j, WHITE);
        }
      }
    }
  }
}

void drawButton(const Button control) {
  int x = control.x,
      y = control.y,
      width = control.width,
      height = control.height;

  display.setTextSize(1);
  display.setCursor(x + 7, y);
      
  if (control.selected) {
    display.fillRect(x + 3, 0, 31, height, BLACK);
    display.setTextColor(WHITE);
  } else {
    display.setTextColor(BLACK);
  }
  
  display.println(control.text);                         //title context
}

int getTextPixel(const char* text) {
  int spaceCount = 0;
  int len = strlen(text);

  for(int i = 0; i < len; i++)
  {
    if(text[i] == 32)
      spaceCount += 1;
  }

  int pixel = len * 5 + (len - 1 - spaceCount * 2) * 1;
  return pixel;
}
// **********************
// * END GUI CODE SPACE *
// **********************


// **************************************
// * START CONTROL_PANEL_APP CODE SPACE *
// **************************************
String cpuInfo = "---%", memInfo = "--G/--G", disInfo = "--G/--G", timInfo = "00:00";

struct FancyGrid cpuMonitor = {
    .x = 2,
    .y = 2,
    .width = 54,
    .height = 32,
    .iconWidth = 16,
    .iconHeight = 16,
    .icon = iconCPU,
    .title = "CPU",
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = cpuInfo.c_str()
    }
};

struct FancyGrid memoryMonitor = {
    .x = 58,
    .y = 2,
    .width = 66,
    .height = 24,
    .iconWidth = 14,
    .iconHeight = 10,
    .icon = iconMemory,
    .title = "Memory",
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = memInfo.c_str()
    }
};

struct FancyGrid diskMonitor = {
    .x = 58,
    .y = 28,
    .width = 66,
    .height = 24,
    .iconWidth = 14,
    .iconHeight = 11,
    .icon = iconDisk,
    .title = "Disk",
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = disInfo.c_str()
    }
};

struct FancyGrid timeMonitor = {
    .x = 2,
    .y = 36,
    .width = 54,
    .height = 16,
    .iconWidth = 8,
    .iconHeight = 10,
    .icon = iconClock,
    .title = timInfo.c_str(),
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = ""
    }
};

void displayControlPanelAPP(void) {
  display.clearDisplay();
  drawWindow("Control Panel", 9, 1, 4, cpuMonitor, memoryMonitor, diskMonitor, timeMonitor);
  display.display();
}
// ************************************
// * END CONTROL_PANEL_APP CODE SPACE *
// ************************************



// ************************************
// * START SYSTEM_INFO_APP CODE SPACE *
// ************************************
struct FancyGrid systemInfoGrid = {
    .x = 2,
    .y = 2,
    .width = 122,
    .height = 50,
    .iconWidth = 16,
    .iconHeight = 16,
    .icon = iconMacintosh,
    .title = "Ekintosh 3M",
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = "C2020 Ekily Inc. all rights reserved." //
    }
};

void displaySystemInfoApp(void) {
  display.clearDisplay();
  drawWindow("System Info", 9, 1, 1, systemInfoGrid);
  display.display();
}
// **********************************
// * END SYSTEM_INFO_APP CODE SPACE *
// **********************************



// ********************************
// * START WEATHER_APP CODE SPACE *
// ********************************
struct FancyGrid weatherGrid = {
    .x = 2,
    .y = 2,
    .width = 122,
    .height = 50,
    .iconWidth = 16,
    .iconHeight = 16,
    .icon = iconSunny,
    .title = "Sunny 37C",
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = "Zhong Shan, shiqi distro."
    }
};

void displayWeatherApp(void) {
  display.clearDisplay();
  drawWindow("Weather", 9, 1, 1, weatherGrid);
  display.display();
}
// ******************************
// * END WEATHER_APP CODE SPACE *
// ******************************



// ******************************
// * START CLOCK_APP CODE SPACE *
// ******************************
String datInfo = "0000/00/00 Mon.";
String clockUrl = "http://quan.suning.com/getSysTime.do";
HTTPClient http;
WiFiClient wifiClient;

String getDay(const char* date) {
  int year = atoi(substr(date, -6).c_str()), 
      month = atoi(substr(substr(date, 4).c_str(), -3).c_str()), 
      day = atoi(substr(date, 7).c_str());

  if(month == 1 || month == 2) {
    month += 12;
    year--;
  }

  int dow = (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7;

  switch(dow) {
    case 0: return " Mon."; break;
    case 1: return " Tues."; break;
    case 2: return " Wed."; break;
    case 3: return " Thur."; break;
    case 4: return " Fri."; break;
    case 5: return " Sat."; break;
    case 6: return " Sun."; break;
  }
}

void displayClockApp(void) {
  display.clearDisplay();

  struct FancyGrid clockGrid = {
    .x = 56,
    .y = 2,
    .width = 68,
    .height = 50,
    .iconWidth = 16,
    .iconHeight = 16,
    .icon = iconClock_16x16,
    .title = timInfo.c_str(),
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = datInfo.c_str()
    }
  };

  drawWindow("Clock", 9, 1, 1, clockGrid);

  display.drawCircle(28, 36, 24, WHITE);

  int hour = atoi(substr(timInfo.c_str(), -3).c_str()),
      min = atoi(substr(timInfo.c_str(), 2).c_str());
  
  if (hour >= 12) hour -= 12;

  double angleH = 2 * 3.1415926 * hour / 12 - 3.1415926 / 2;
  double angleM = 2 * 3.1415926 * min / 60 - 3.1415926 / 2;

  display.drawLine(28, 36, 28 + 16 * cos(angleH), 36 + 16 * sin(angleH), WHITE);
  display.drawLine(28, 36, 28 + 24 * cos(angleM), 36 + 24 * sin(angleM), WHITE);

  display.display();
}

void getWiFiClock(void) {
  if (!connected) return;

  http.setTimeout(5000);
  http.begin(wifiClient, clockUrl);

  int httpCode = http.GET();
  if (httpCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        //读取响应内容
        String response = http.getString();
        String webDate = substr(substr(response.c_str(), 12).c_str(), -39);
        Serial.println(response);
        timInfo = substr(substr(response.c_str(), 23).c_str(), -33);
        datInfo = webDate + getDay(webDate.c_str());
        Serial.println(datInfo);
      }
  } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
// ****************************
// * END CLOCK_APP CODE SPACE *
// ****************************



// ****************************
// * START WELCOME CODE SPACE *
// ****************************
struct FancyGrid welcomeGrid = {
    .x = 2,
    .y = 13,
    .width = 122,
    .height = 38,
    .iconWidth = 32,
    .iconHeight = 32,
    .icon = iconMacintosh_32x32_BLACK,
    .title = "WELCOME TO EKINTOSH.",
    .text = {
      .x = 0,
      .y = 0,
      .width = 1,
      .height = 1,
      .text = ""
    }
};
// **************************
// * END WELCOME CODE SPACE *
// **************************



// ****************************
// * START DESKTOP CODE SPACE *
// ****************************
int menuIndex = 3;
const uint8_t* dstIcon;

struct Button fileButton = {
  .x = 14,
  .y = 1,
  .width = 38,
  .height = 9,
  .selected = false,
  .text = "File"
};
struct Button editButton = {
  .x = 52,
  .y = 1,
  .width = 38,
  .height = 9,
  .selected = false,
  .text = "Edit"
};
struct Button appsButton = {
  .x = 90,
  .y = 1,
  .width = 38,
  .height = 9,
  .selected = false,
  .text = "Apps"
};

void displayDesktop(void) {
  display.clearDisplay();
  display.fillRoundRect(9, 0, SCREEN_WIDTH - 9, 9, 2, WHITE);
  
  display.drawLine(0, 1, 0, 7, WHITE);

  switch(menuIndex){ 
    case 0:
      dstIcon = iconApple_9x9_BLACK;
      fileButton.selected = false;
      editButton.selected = false;
      appsButton.selected = false;
      break;
    case 1:
      dstIcon = iconApple_9x9;
      fileButton.selected = true;
      editButton.selected = false;
      appsButton.selected = false;
      break;
    case 2:
      dstIcon = iconApple_9x9;
      fileButton.selected = false;
      editButton.selected = true;
      appsButton.selected = false;
      break;
    case 3:
      dstIcon = iconApple_9x9;
      fileButton.selected = false;
      editButton.selected = false;
      appsButton.selected = true;
      break ;
  }

  display.drawBitmap(1, 0, dstIcon, 9, 9, 1);

  drawHalfFill(SCREEN_HEIGHT - 9, SCREEN_WIDTH, 0, 9, 97, 27, 32, 32);
  //display.fillRect(97, 54, 31, 9, WHITE);

  //display.drawBitmap(97, 27, iconTrash_29x29, 29, 29, 1);
  //display.setTextSize(1);
  //display.setTextColor(BLACK);
  //display.setCursor(97, 54);
  //display.println("Trash");                         //title context

  drawButton(fileButton);
  drawButton(editButton);
  drawButton(appsButton);
  
  display.display();
}
// ****************************
// * START DESKTOP CODE SPACE *
// ****************************



bool contain(const char* context, const char* flag) {
  for (int i = 0; i < strlen(context); i++) {
    if (i + strlen(flag) == strlen(context)) {
      return false;
    }

    if (context[i] == flag[0]) {
      if (strlen(flag) == 1) {
        return true;
      } else {
        for (int j = 1; j < strlen(flag); j++) {
          if (context[i + j] == flag[j]) {
            if (j == strlen(flag) - 1) {
              return true;
            }
          } else {
            break;
          }
        }
      }
    }
  }

  return false;
}

String substr(const char* context, const int start) {
  String result = "";

  if (start >= 0) {
    for (int i = start + 1; i < strlen(context); i++) {
      result += context[i];
    }
  } else {
    for (int i = 0; i < strlen(context) + start; i++) {
      result += context[i];
    }
  }

  return result;
}

void exSerialData(const String serialData) {
  if (contain(serialData.c_str(), "CPU")) {
    cpuInfo = substr(serialData.c_str(), 2);
  }
  if (contain(serialData.c_str(), "MEM")) {
    memInfo = substr(serialData.c_str(), 2);
  }
  if (contain(serialData.c_str(), "DIS")) {
    disInfo = substr(serialData.c_str(), 2);
  }
  if (contain(serialData.c_str(), "TIM")) {
    timInfo = substr(serialData.c_str(), 2);
  }
}




// **********************************
// * START INIT_HARDWARE CODE SPACE *
// **********************************
void initWiFi(void) {
  WiFi.hostname("Smart-ESP8266");

  connectNewWifi();
}

void initOLED(void) {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

  drawStartUp(iconMacintosh_32x32, 32, 32);
}

void initSoundCard(void) {
  pinMode(SPEAKER_PIN, OUTPUT);
}
// ********************************
// * END INIT_HARDWARE CODE SPACE *
// ********************************

void setup() {
  Serial.begin(115200);
  
  initOLED();
  initSoundCard();
  initWiFi();

  display.clearDisplay();
  drawFancyGrid(welcomeGrid, 0, 0);
  display.display();

  playStartUpTone();

  delay(6400);

  updateScreen();
}

int appCount = 4;
void updateScreen(void) {
  switch(appCount){ 
    case 0:
      displayControlPanelAPP();
      break;
    case 1:
      displaySystemInfoApp();
      break;
    case 2:
      displayWeatherApp();
      break;
    case 3:
      displayClockApp();
      break;
    case 4:
      displayDesktop();
      break;
    default:
      displayControlPanelAPP();
  }
}

unsigned int runningCount = 0;
void loop() {
  if (!connected) {
    server.handleClient();
    dnsServer.processNextRequest();
  }

  String serialData = "";
  while (Serial.available() > 0) {
    serialData += char(Serial.read());
    delay(2);
  }
  if (serialData.length() > 0) {
    exSerialData(serialData);
    updateScreen();
  }

  if (runningCount == 2000000) {
    runningCount = 0;
    getWiFiClock();
    updateScreen();
  } else {
    runningCount++;
  }
}