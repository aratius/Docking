// OSC部はArduinoOSCなのでESPと同じです
#include <M5StickCPlus.h>
#include <ArduinoOSC.h>

// --------------------
// ここを書き換えてください
// --------------------
#define DEVICE_NUMBER 1

//WiFiルータ Settings
const char* ssid = "aratamatsumoto";  // ネットワーク名
const char* pwd = "aratamatsumoto";  // パスワード
IPAddress ip(172, 20, 10, 10+DEVICE_NUMBER);  // 自分のIPを決定する（pingコマンドなどで事前にIPが空いているか確認するのが望ましい） 
const char* host = "172.20.10.2";  // 送信先のIPを決定する（pingコマンドなどで事前にIPが空いているか確認するのが望ましい） 
const IPAddress gateway(172, 20, 10, 1);  // ゲートウェイ = ネットワークのベース
const IPAddress subnet(255, 255, 255, 240);  // サブネット = だいたいこの値
const int portIncomming = 10001;  // サーバ（受信）ポート
const int portOutgoing = 10000;
bool isConnected = false;
char ipStr[16];

#define SAMPLE_PERIOD 10    // サンプリング間隔(ミリ秒)
float ax, ay, az;  // 加速度データを読み出す変数
float threshold = 3;
unsigned long interval = 200;
unsigned long t = 0;
unsigned long lastTime = 0;
unsigned long wait_until = 0;

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setRotation(3);

  connectWiFi();
  
  delay(1000);
}

void loop(){
  M5.update();
  OscWiFi.update();
  t = millis();

  if(isConnected) {  
    M5.IMU.getAccelData(&ax, &ay, &az);  // MPU6886から加速度を取得

    if(t > wait_until) {
      float mag = sqrt(ax*ax + sqrt(ay*ay + az*az));
      if(mag > threshold) {
        char* address = getOscAddress("shake");
        sendOsc(address, 1); 
        delete[] address;
        wait_until = t + interval;
      }
    }

    if(M5.BtnA.wasPressed()) incrementIp();

    // 500msに一回
    if(lastTime % 500 > t % 500) setStatus();
  } else {

  }

  if(M5.BtnB.wasPressed()) connectWiFi();
  
  lastTime = t;
  delay(SAMPLE_PERIOD);
}

void onWiFiInitialized() {
  IPAddress localIp = WiFi.localIP();
  snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", localIp[0], localIp[1], localIp[2], localIp[3]);
  M5.Lcd.println(ipStr);
  
  // 受信のリスナー設定
  OscWiFi.subscribe(portIncomming, "/threshold", onOscReceivedThreshold);
  OscWiFi.subscribe(portIncomming, "/interval", onOscReceivedInterval);

  // 最初に一回同期させるステータス
  char* thresholdAdd = getOscAddress("threshold");
  sendOsc(thresholdAdd, threshold); 
  delete[] thresholdAdd;
  char* intervalAdd = getOscAddress("interval");
  sendOsc(intervalAdd, interval); 
  delete[] intervalAdd;
  char* ipAdd = getOscAddress("ip");
  sendOsc(ipAdd, ipStr);
  delete[] ipAdd;
}

void connectWiFi() {
  isConnected = false;
  M5.Lcd.println("INFO : Wi-Fi Disconnect.");  
  WiFi.disconnect(true, true);
  delay(1000);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, pwd);
  WiFi.config(ip, gateway, subnet);
  int cnt = 0;
  delay(1000);

  M5.Lcd.print("INFO : Wi-Fi Start Connect.");

  // WiFiがつながるまでwhileを回す
  while (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.print(".");
    delay(500);
    if (cnt == 5) {
      M5.Lcd.println("");
      M5.Lcd.println("INFO : Wi-Fi Connect Failed.");
      return;
    }
    cnt += 1;
  }
  isConnected = WiFi.status() == WL_CONNECTED;

  M5.Lcd.println("");
  M5.Lcd.println("INFO : Wi-Fi Connected.");
  onWiFiInitialized();
}

void incrementIp() {

}

void onOscReceivedThreshold(OscMessage& m) {
  float v = m.arg<float>(0);
  threshold = v;
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
}

char* getOscAddress(const char* key) {
  String address = "/device/" + String(DEVICE_NUMBER) + "/" + key;
  char *cstr = new char[address.length() + 1];
  strcpy(cstr, address.c_str());
  return cstr;
}
