// OSC部はArduinoOSCなのでESPと同じです
#include <M5StickCPlus.h>
#include <ArduinoOSC.h>

// --------------------
// ここを書き換えてください
// --------------------
#define DEVICE_IDENTITY 1

#define WIDTH = 240
#define HEIGHT = 135

//WiFiルータ Settings
const char* ssid = "HUMAX-BD2EB";  // ネットワーク名
const char* pwd = "MjdjMmNxMEgaX";  // パスワード
IPAddress ip(192, 168, 0, 100+DEVICE_IDENTITY);  // 自分のIPを決定する（pingコマンドなどで事前にIPが空いているか確認するのが望ましい） 
const char* host = "192.168.0.12";  // 送信先のIPを決定する（pingコマンドなどで事前にIPが空いているか確認するのが望ましい） 
const IPAddress gateway(192, 168, 1, 1);  // ゲートウェイ = ネットワークのベース
const IPAddress subnet(255, 255, 255, 0);  // サブネット = だいたいこの値
const int portIncomming = 10001;  // サーバ（受信）ポート
const int portOutgoing = 10000;
bool isConnected = false;

#define SAMPLE_PERIOD 10    // サンプリング間隔(ミリ秒)
float ax, ay, az;  // 加速度データを読み出す変数
float threshold = 3;
unsigned long interval = 200;
unsigned long t = 0;
unsigned long lastTime = 0;
unsigned long wait_until = 0;

char ipStr[16];
int batteryPercent = 100;

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setRotation(3);

  M5.Lcd.drawLine(0, 100, 240, 100, WHITE);

  connectWiFi();
  
  delay(1000);
}

void loop(){
  M5.update();
  OscWiFi.update();
  t = millis();

  if(isConnected) {  

    if(t > wait_until) {
      float lastMag = sqrt(ax*ax + sqrt(ay*ay + az*az));
      M5.IMU.getAccelData(&ax, &ay, &az);  // MPU6886から加速度を取得
      float mag = sqrt(ax*ax + sqrt(ay*ay + az*az));
      M5.Lcd.fillRect(10 + 220 * mag / 10 - 2, 50, 220 * abs(lastMag-mag) / 10 + 4, 31, BLACK);  // 前回と今回の差分(+-2px)のみ塗りつぶす
      M5.Lcd.drawRect(10, 55, 220, 20, WHITE);
      for(int i = 0; i < 10; i++) M5.Lcd.drawLine(10 + ((float)i / 10) * 220, 70, 10 + ((float)i / 10) * 220, 74, WHITE);  // 目盛
      M5.Lcd.fillRect(10, 55, 220 * mag / 10, 20, WHITE);
      if(mag > threshold) {
        char* address = getOscAddress("shake");
        sendOsc(address, 1); 
        delete[] address;
        wait_until = t + interval;
      }
      M5.Lcd.drawLine(10 + 220 * threshold / 10, 50, 10 + 220 * threshold / 10, 80, RED);
    }

    // 500msに一回
    if(lastTime % 500 > t % 500) setStatus();
  } else {

  }

  if(M5.BtnB.wasPressed()) connectWiFi();
  
  lastTime = t;
  delay(SAMPLE_PERIOD);
}

void connectWiFi() {
  isConnected = false;
  M5.Lcd.fillRect(0, 101, 240, 34, BLACK);
  WiFi.disconnect(true, true);
  delay(1000);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, pwd);
  WiFi.config(ip, gateway, subnet);
  int cnt = 0;
  delay(1000);

  M5.Lcd.setCursor(10, 110);
  M5.Lcd.print("INFO : Wi-Fi Start Connect.");

  // WiFiがつながるまでwhileを回す
  while (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.print(".");
    delay(500);
    if (cnt == 5) {
      M5.Lcd.setCursor(10, 120);
      M5.Lcd.println("INFO : Wi-Fi Connect Failed.");
      return;
    }
    cnt += 1;
  }
  isConnected = WiFi.status() == WL_CONNECTED;

  M5.Lcd.setCursor(10, 120);
  M5.Lcd.println("INFO : Wi-Fi Connected.");
  onWiFiInitialized();
}

void onWiFiInitialized() {
  IPAddress localIp = WiFi.localIP();
  snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", localIp[0], localIp[1], localIp[2], localIp[3]);
  M5.Lcd.fillRect(10, 20, 230, 20, BLACK);
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print(String(getDeviceNumber()) + " ");
  M5.Lcd.println(ipStr);
  M5.Lcd.setTextSize(1);
  
  // 受信のリスナー設定
  OscWiFi.subscribe(portIncomming, "/threshold", onOscReceivedThreshold);
  OscWiFi.subscribe(portIncomming, "/interval", onOscReceivedInterval);

  // 最初に一回同期させるステータス
  char* ipAdd = getOscAddress("ip");
  sendOsc(ipAdd, ipStr);
  delete[] ipAdd;
}

void onOscReceivedThreshold(OscMessage& m) {
  float v = m.arg<float>(0);
  threshold = v;
  M5.Lcd.fillRect(10, 50, 220, 31, BLACK);
}

void onOscReceivedInterval(OscMessage& m) {
  unsigned long v = m.arg<unsigned long>(0);
  interval = v;
}

void sendOsc(const char* address, float val){
  OscWiFi.send(host, portOutgoing, address, val);
}

void sendOsc(const char* address, const char* val){
  OscWiFi.send(host, portOutgoing, address, val);
}

void setStatus() {
  char* statusAdd = getOscAddress("status");
  sendOsc(statusAdd, 1); 
  delete[] statusAdd;
  float vbat = M5.Axp.GetBatVoltage();
  int percent = (int)((vbat-3.0)/1.2*100);
  char* batteryAdd = getOscAddress("battery");
  sendOsc(batteryAdd, percent); 
  delete[] batteryAdd;
  if(percent != batteryPercent) {
    // display
    M5.Lcd.fillRect(0, 0, 240, 5, BLACK);
    M5.Lcd.fillRect(0, 0, 240 * percent / 100, 5, GREEN);
  }
  batteryPercent = percent;
}

char* getOscAddress(const char* key) {
  String address = "/device/" + String(getDeviceNumber()) + "/" + key;
  char *cstr = new char[address.length() + 1];
  strcpy(cstr, address.c_str());
  return cstr;
}

int getDeviceNumber() {
  int deviceNumber = DEVICE_IDENTITY % 4;
  return deviceNumber == 0 ? 4 : deviceNumber;
}
