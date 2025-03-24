#include <MPU6050.h>

#include <WiFi.h>
#include <WiFiUdp.h>

#include <WiFiS3.h>
#include "Arduino_LED_Matrix.h"

// ジャイロセンサー I2C制御用ライブラリ
#include <Wire.h>

// 構造体でデータを一括管理
#include "IMUManager.h"
#include "WiFiManager.h"
#include "LEDManager.h"
#include "ShakeManager.h"

#include "config.h"

// LED関連 ピン番号
#define LED_CANDLE_PIN 9

// グローバル変数は構造体にしてまとめて管理すると良い
// AcX, Y, Z, 
// Tmp, 
// GyX, Y, Z, 
// gyroX_offset, Y, Z, 
// previousGyroMillis, gyroInterval
// resetMillis
// currentCalibrate
// preAcX, preAcY, preAcZ, preGyX, preGyY, preGyZ
// retAcX, retAcY, retAcZ, retGyX, retGyY, retGyZ

// FIFOOverflowを防ぐために200msまたは10msが良いかも
// 500msとかだと詰まる
GyroData gyro = {
                  0, 0, 0, 
                  0, 
                  0, 0, 0, 
                  0, 0, 0,
                  0, 200,
                  50,
                  0, 0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0
                };

// MPU-6050のデフォルトアドレス
// 使用するセンサの種類によって変わります
const int MPU_addr = 0x68;

// ssid, pass, 
// targetIP, targetPort, 
// processingRcvPort, 
// macAcdr
// previousWiFiMillis, wifiCheckInterval
WiFiConfig wifi = {
                    "", "", 
                    "192.168.50.112", 31002, 
                    31001, 
                    "",
                    0, 1000
                  };

// previousLEDMillis, interval, 
// noiseValue, noiseIncremenet
// led_width, isLightOn
LEDConfig led = {
                  0, 10,
                  0.0, 0.05,
                  127, true
                };

// UDP通信の設定
WiFiUDP Udp;

// 識別子によってProcessingの送り先を変える
// シンプルに送信ポートは1つで識別子を元にProcessing側で振り分けるのもあり。

// 振動
// SHAKE_SOCKET, previousShakeMillis, shakeInterval
ShakeManager shake = {11, 0, 2000};

ArduinoLEDMatrix matrix;

uint8_t g_frame[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 現在時刻
unsigned long g_currentMillis = 0;

MPU6050 mpu;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  // matrixLED
  matrix.begin();

  // LED関連
  pinMode(LED_CANDLE_PIN, OUTPUT);

  // 振動
  pinMode(shake.SHAKE_SOCKET, OUTPUT);
  
  // ジャイロセンサ関連
  // Wireライブラリ初期化
  Wire.begin();
  // MPU6050初期化
  // 指定したI2Cデバイスとの送信を開始
  Wire.beginTransmission(MPU_addr);
  // データ書き込み
  // PWR_MGMT_1 register
  // レジスタアドレス
  // これ以降の16進数のコードは使用するセンサによって変わります
  Wire.write(0x6B);
  // 1を書き込んでいるかも 詳細不明
  // 内部クロック8MHzを使用
  Wire.write(0x01); 
  //送信を終了
  Wire.endTransmission(true);
  delay(100);  // 安定するまで待機

  // DLPF設定（5Hzフィルタ）
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1A);  // CONFIG register
  Wire.write(0x06);  // 5Hzローパスフィルタ
  Wire.endTransmission(true);

  // ジャイロキャリブレーション 補正
  calibrateGyro();

  // WiFi接続
  WiFiConnect();
}

// メイン関数はスッキリさせる!
// 人は表面的な部分を最初に見て概要を掴む
// 気になったら自然と深く見ていく,そこは任意
// loopはセンサ,LED, 振動で3つの関数だけにした方が良い
// Flagよりその関数コメントアウトするだけでテストできたりする
// loopの全体処理がわかるからシンプルにする。
void loop() {
  // 現在時刻の取得
  g_currentMillis = millis();

  matrix.renderBitmap(g_frame, 8, 12);

  // WiFi接続チェック
  // wifiCheckInterval(ms)周期で確認
  WiFiCheck();

  // ジャイロ処理
  // gyroInterval周期でジャイロデータを送信
  GyroController();

  // 振動ON/OFF処理
  ShakeController();

  // LEDグラデーション処理
  LEDController();

  // UDP受信待機
  ReceiveUDP();

  // センサリセット
  CheckFIFOOverFlow();

}

/**
 * WiFi接続
 */
void WiFiConnect() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    Serial.println("\n\n ==== System exit ==== \n\n");
    // don't continue
    exit(0);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  int errorCnt = 20;
  // attempt to connect to WiFi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(wifi.ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:

    if (strlen(wifi.pass) == 0) {
      // Free WiFiパスワード不要の場合
      WiFi.begin(wifi.ssid);
    }
    else {
      WiFi.begin(wifi.ssid, wifi.pass);
    }

    --errorCnt;
    if (errorCnt < 0) {
      Serial.println("Error: Failed to connect WiFi 20 times");
      exit(0);
    }

    // wait 3 seconds for connection:
    delay(3000);
  }

  Serial.println("Connected to WiFi");

  DisplayWiFiConnected();
  
  printWifiStatus();

  Serial.println("\nStarting connection to server...");

  UDPConnect();
}

void UDPConnect() {
  Udp.begin(wifi.processingRcvPort);
}

/**
 * WiFi設定表示
 */
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("ProcessingRcvPort: ");
  Serial.println(wifi.processingRcvPort);

  // MACアドレス取得
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // リセット
  wifi.macAddr = "";

  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) {
      wifi.macAddr += "0";
    }

    wifi.macAddr += String(mac[i], HEX);
    if (i < 5) {
      wifi.macAddr += ":";
    }
  }

  Serial.print("MAC Address: ");
  Serial.println(wifi.macAddr);
}

/**
 * WiFi接続確認
 */
void WiFiCheck() {
  if (g_currentMillis - wifi.previousWiFiMillis >= wifi.wifiCheckInterval) {
    // 未接続の時
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("PAUSE Reconnect to SSID: ");
      Serial.println(wifi.ssid);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:

      DisplayWiFiDisconnected();
      matrix.renderBitmap(g_frame, 8, 12);

      // 再接続
      WiFiConnect();

      // 再接続メッセージを送信
      String data = "\n \n ====== Success Reconnect ====== \n\n";
      SendUDP(data);
      Serial.println("Success to reconnect");
    }

    wifi.previousWiFiMillis = g_currentMillis;
  }
}

/**
 * ジャイロセンサの初期補正関数
 */
void calibrateGyro() {
  int numSamples = 1000;

  for (int i = 0; i < numSamples; i++) {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 6, true);
    gyro.gyroX_offset += (Wire.read() << 8 | Wire.read());
    gyro.gyroY_offset += (Wire.read() << 8 | Wire.read());
    gyro.gyroZ_offset += (Wire.read() << 8 | Wire.read());
    delay(3);
  }

  gyro.gyroX_offset /= numSamples;
  gyro.gyroY_offset /= numSamples;
  gyro.gyroZ_offset /= numSamples;
}

int cnt = 0;
/**
 * ジャイロ処理
 */
void GyroController() {
  //今回は100行程度なのでこの関数内に全て書いても良い
  if (g_currentMillis - gyro.previousGyroMillis > gyro.gyroInterval) {
    gyro.previousGyroMillis = g_currentMillis;

    Wire.beginTransmission(MPU_addr);
    // [15:8]は15bit目から8bitつまりMSB上位バイト
    // [7:0]は7bit目から0bitまで, つまりLSB下位バイト
    Wire.write(0x3B);
    Wire.endTransmission(false);
    // 14bytesのデータを要求
    Wire.requestFrom(MPU_addr, 14, true);
    
    // センサからデータ取得
    // 8ビットシフト
    // MPU6050は2bytesでデータ送信する
    // read()は1byteずつしか読み取れないためシフトして加算
    gyro.AcX = Wire.read() << 8 | Wire.read();
    gyro.AcY = Wire.read() << 8 | Wire.read();
    gyro.AcZ = Wire.read() << 8 | Wire.read();
    gyro.Tmp = Wire.read() << 8 | Wire.read();
    
    gyro.GyX = Wire.read() << 8 | Wire.read();
    gyro.GyY = Wire.read() << 8 | Wire.read();
    gyro.GyZ = Wire.read() << 8 | Wire.read();

    // オフセット補正
    gyro.GyX -= gyro.gyroX_offset;
    gyro.GyY -= gyro.gyroY_offset;
    gyro.GyZ -= gyro.gyroZ_offset;

    // 生データ
    /*
    Serial.print("AcX = "); Serial.print(gyro.AcX);
    Serial.print(" | AcY = "); Serial.print(gyro.AcY);
    Serial.print(" | AcZ = "); Serial.print(gyro.AcZ);
    // Serial.print(" | Tmp = "); Serial.print(Tmp / 340.00 + 36.53);
    Serial.print(" | GyX = "); Serial.print(gyro.GyX);
    Serial.print(" | GyY = "); Serial.print(gyro.GyY);
    Serial.print(" | GyZ = "); Serial.println(gyro.GyZ);
    */

    int range = 100;

    // 活用しやすい範囲に値を変更
    int mapAcX = map(gyro.AcX, -16500, 16500, 0, range);
    int mapAcY = map(gyro.AcY, -16500, 16500, 0, range); 
    int mapAcZ = map(gyro.AcZ, -16500, 16500, 0, range);
    int mapGyX = map(gyro.GyX, -16500, 16500, 0, range); //-20, 20);
    int mapGyY = map(gyro.GyY, -16500, 16500, 0, range); //-20, 20);
    int mapGyZ = map(gyro.GyZ, -16500, 16500, 0, range); //-20, 20);

    // コンソール表示用
    /*
    Serial.print("mapAcX = "); Serial.print(mapAcX);
    Serial.print(" | mapAcY = "); Serial.print(mapAcY);
    Serial.print(" | mapAcZ = "); Serial.print(mapAcZ);
    Serial.print(" | mapGyX = "); Serial.print(mapGyX);
    Serial.print(" | mapGyY = "); Serial.print(mapGyY);
    Serial.print(" | mapGyZ = "); Serial.println(mapGyZ);
    */

    int min = 0, max = 0;

    // 加速度
    // 前フレームより10以上差があれば傾きを反映させる
    gyro.retAcX += retPlusMinus(mapAcX, gyro.preAcX);
    gyro.retAcY += retPlusMinus(mapAcY, gyro.preAcY);
    gyro.retAcZ += retPlusMinus(mapAcZ, gyro.preAcZ);
    // gyro.retGyX += retPlusMinus(mapGyX, gyro.preGyX);
    // gyro.retGyY += retPlusMinus(mapGyY, gyro.preGyY);
    // gyro.retGyZ += retPlusMinus(mapGyZ, gyro.preGyZ);

    // 加速度を指定範囲に収める
    gyro.retAcX = constrain(gyro.retAcX, 0, 100);
    gyro.retAcY = constrain(gyro.retAcY, 0, 100);
    gyro.retAcZ = constrain(gyro.retAcZ, 0, 100);

    // ジャイロ円滑化
    SmoothGyro(gyro.retGyX, mapGyX);
    SmoothGyro(gyro.retGyY, mapGyY);
    SmoothGyro(gyro.retGyZ, mapGyZ);

    // 送信データ作成
    String data = wifi.macAddr + "," + gyro.retAcX + "," + gyro.retAcX + "," + gyro.retAcZ + "," + gyro.retGyX + "," + gyro.retGyY + "," + gyro.retGyZ;

    Serial.print(gyro.retAcX);Serial.print(" ");
    Serial.print(gyro.retAcY);Serial.print(" ");
    Serial.print(gyro.retAcZ);Serial.print(" ");
    Serial.print(gyro.retGyX);Serial.print(" ");
    Serial.print(gyro.retGyY);Serial.print(" ");
    Serial.print(gyro.retGyZ);Serial.println("");

    // 前回の値として記録
    gyro.preAcX = mapAcX;
    gyro.preAcY = mapAcY;
    gyro.preAcZ = mapAcZ;
    // gyro.preGyX = mapGyX;
    // gyro.preGyY = mapGyY;
    // gyro.preGyZ = mapGyZ;

    // テストデータ用
    /*
    String datas[] = {"1,2,3,4,5,6", "2,3,4,5,6,7", "3,4,5,6,7,8"};
    
    data = wifi.macAddr + "," + datas[cnt];
    cnt++;
    cnt %= 3;
    */

    // UDPデータ送信
    SendUDP(data);

    // TouchDesignerデータ送信
    SendTD(data);
  }
}

int retPlusMinus(float current, float base) {
  int ret = 0;
  int range = 1;
  int diff = current - base;

  if (diff > 1) {
    ret = constrain(diff, range, 100);
  }
  else if (diff < -1) {
    ret = constrain(diff, -100, range);
  }

  return ret;
}

/**
 * ジャイロの円滑化
 */
void SmoothGyro(int16_t &gyroValue, float newValue) {
  int diff = 50 - gyroValue;
  // ジャイロを円滑化
  // とりあえずgyYで調査
  if (-5 <= diff && diff <= 5) {
    // 値を更新
    gyroValue = constrain(newValue, 0, 100);
  }
  else {
    int ret = 0;
    // 50になるまで加減算
    // 基準より大きい場合
    if (diff < 0) {
      // 下げる
      ret = -5;
    }
    // 基準より小さい場合
    else {
      // 上げる
      ret = 5;
    }

    gyroValue += ret;
  }
}

/**
 * 振動OFF処理
 */
void ShakeController() {
  // 振動ON/OFF
  if (g_currentMillis - shake.previousShakeMillis >= shake.shakeInterval) {
    // digitalWrite(shake.SHAKE_SOCKET, LOW);
    // tone(shake.SHAKE_SOCKET, 80, shake.shakeInterval);
  }
}

void ShakeOn() {
  // digitalWrite(shake.SHAKE_SOCKET, HIGH);
  Serial.print("Shake: ");
  Serial.print(shake.SHAKE_SOCKET);
  Serial.print(" ");
  Serial.println(shake.shakeInterval);
  tone(shake.SHAKE_SOCKET, 80, shake.shakeInterval);
}

/**
 * LEDグラデーション制御
 */
void LEDController() {
  if ((g_currentMillis - led.previousLEDMillis >= led.interval)) { //} && led.isLightOn) {
    led.previousLEDMillis = g_currentMillis;

    // 徐々に変化するノイズを生成
    led.noiseValue += led.noiseIncrement;
    
    // 振幅を小さくする
    int newValue = 80 * sin(led.noiseValue) + 127;

    // 平滑化(急激な変化を抑える)
    led.led_width = (led.led_width * 0.9) + (newValue * 0.1);

    analogWrite(LED_CANDLE_PIN, led.led_width);

    // Serial.println(led.led_width);
  }

  if (!led.isLightOn) {
    analogWrite(LED_CANDLE_PIN, 1);
  }
}

/**
 * UDP送信
 */
void SendUDP(String data) {
  Udp.beginPacket(wifi.targetIP, wifi.targetPort);
  Udp.write(data.c_str());
  Udp.endPacket();
}

/**
 * TD送信
 */
void SendTD(String data) {
  // とりあえずProcessingと重ならないようにテキトウにポートを12000にしました
  // ここにTDを起動するPCのIPを直接指定
  Udp.beginPacket("192.168.48.112", 12000);
  Udp.write(data.c_str());
  Udp.endPacket();
}

/**
 * UDP受信待機
 */
void ReceiveUDP() {
  // ProcessingからUDP受信した際のデータ形式変換用
  char packetBuffer[256]; //buffer to hold incoming packet
  char  ReplyBuffer[] = "acknowledged\n";       // a string to send back

  int parseSize = Udp.parsePacket();
  if (parseSize) {
    Serial.print("GET OSC");
    int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';  // 文字列終端を追加

      // 文字列を整数に変換
      int receivedInt = atoi(packetBuffer);

      // Serial.print("Received: ");
      // Serial.println(packetBuffer);
      Serial.print("Parsed Int: ");
      Serial.println(receivedInt);

      switch (receivedInt) {
        case 0:
          break;

        case 1:
          // 加速度を一斉に補正
          Serial.print("\n\n Calibrate ");
          Serial.println(gyro.currentCalibrate);
          gyro.retAcX = gyro.currentCalibrate;
          gyro.retAcY = gyro.currentCalibrate;
          gyro.retAcZ = gyro.currentCalibrate;

          break;

        case 2:
          // 点灯
          led.isLightOn = true;
          analogWrite(LED_CANDLE_PIN, 100);
          break;

        case 3:
          // 消灯
          led.isLightOn = false;
          analogWrite(LED_CANDLE_PIN, 1);
          break;

        default:
          break;
      }

      ShakeOn();
    }
  }
}

/**
 * 接続中LED表示
 */
void DisplayWiFiConnected() {
  // 表示パターンを定義
  uint8_t connect_led[8][12] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  };

  // 表示を更新
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 12; j++) {
      g_frame[i][j] = connect_led[i][j];
    }
  }
}

/**
 * 切断時LED表示
 */
void DisplayWiFiDisconnected() {
  // 表示パターンを定義
  uint8_t disconnect_led[8][12] = {
    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  };

  // 表示を更新
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 12; j++) {
      g_frame[i][j] = disconnect_led[i][j];
    }
  }
}

/**
 * オーバーフローチェック
 */
void CheckFIFOOverFlow() {
  if (g_currentMillis - gyro.resetMillis >= 1000) {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3A);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 1, true);

    uint8_t status = Wire.read();
    // 0001 0000で 指定したbitに入っているオーバーフローのステータスを取得
    if ((status & 0x10) != 0) {
      resetMPU6050();
    } 

    gyro.resetMillis = g_currentMillis;

    // Serial.println("\n\n\n ==== Gyro Sensor Check === \n\n\n");
  }
}

/**
 * ジャイロセンサリセット
 */
void resetMPU6050() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6A);
  Wire.write(0x04); // FIFOリセット
  Wire.endTransmission(true);

  delay(100);

  // 電源関係
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  // センサの再起動
  // Wire.write(0x00);
  Wire.write(0x01);
  Wire.endTransmission(true);

  delay(100);

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1A);
  Wire.write(0x06);
  Wire.endTransmission(true);
  
  Serial.println("=== Gyro Sensor Reset === \n\n\n");

  String data = wifi.macAddr + "," + -1000 + ",Gyro Reset";
  SendUDP(data);

  // Arduinoをリセットする
  // Arduinoの再起動でありジャイロはリセットされない
  // NVIC_SystemReset();

  delay(3000);
}