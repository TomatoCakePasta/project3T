#ifndef WiFiManager_h
#define WiFiManager_h

// WiFi関連
typedef struct {
  const char* ssid;
  const char* pass;
  // ご自身のPCのIPアドレス
  // ターミナルで「ifconfig」と入力
  // en0の項目のinet以降があなたのIPアドレスです
  // Processing送信先のIPアドレスとポート
  // Processingを起動しているPCのIPアドレスとポートを入力
  const char* targetIP;
  // ArduinoIDに対応したProcessingの待ち受けポートを指定
  const int targetPort;

  // Processing受信用のポート
  const int processingRcvPort;
  String macAddr;
  long previousWiFiMillis;
  const long wifiCheckInterval;
} WiFiConfig;

#endif