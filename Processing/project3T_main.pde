import hypermedia.net.*;

import netP5.*;
import oscP5.*;

OscP5 oscP5;

// テスト関係
// 範囲
int from = -25;
int to = 25;
int testData = from;

// 何ms間隔でデータを送るか
int interval = 100;

int testArduinoIdx = 0;
int[] cnt = {0, 1, 2, 3, 4, 5, 6, 7};

// UDP受信 Arduinoから受け取る
UDP udp;

// OSC送信 Maxに送信
NetAddress myRemoteLocation;

// ネットワーク設定
String broadCastIP = "";
int broadCastPort = 0;
int arduinoRcvPort = 0;
String maxSendIP = "127.0.0.1";
int maxSendPort = 0;
int maxRcvPort = 0;

JSONObject macJson;
JSONObject netJson;
// macアドレス格納
String[] macAddrs;

void setup() {
  println("=== RECONNECT TEST === \n\n\n\n\n\n");
  // configファイルを読み込み
  macJson = loadJSONObject("macAddresses.json");
  netJson = loadJSONObject("networkSetting.json");
  
  // configに定義された各データを格納
  broadCastIP = netJson.getString("broadCastIP");
  broadCastPort = netJson.getInt("broadCastPort");
  arduinoRcvPort = netJson.getInt("arduinoRcvPort");
  maxSendIP = netJson.getString("maxSendIP");
  maxSendPort = netJson.getInt("maxSendPort");
  maxRcvPort = netJson.getInt("maxRcvPort");
  
  macAddrs = new String[macJson.size()];
  
  // エフェクト番号0から対応するmacアドレスを取得
  for (int i = 0; i < macJson.size(); i++) {
    String idx = str(i);
    macAddrs[i] = macJson.getString(idx);
  }
  println("macAddrs: " + macAddrs);
  
  // Arduinoから受信するための初期設定
  udp = new UDP(this, arduinoRcvPort);
  udp.listen(true);

  // Maxに送信するための初期設定
  oscP5 = new OscP5(this, maxRcvPort);
  myRemoteLocation = new NetAddress(maxSendIP, maxSendPort);
  println(broadCastIP + " " + broadCastPort + " " + arduinoRcvPort + " " + maxSendIP + " " + maxSendPort + " " + maxRcvPort);
}

void draw() {
  // 以下をTrueにすれば、仮データを送信
  // Arduino無しでも、Processing-Max間の通信を確認
  // test();
  
  //multiTest();
}

/**
 * ArduinoからUDP通信を受信すると実行される
 */
void receive(byte [] rawData, String ip, int arduinoPort) {
  // 受け取ったデータを文字列に変換
  String transformedData = new String(rawData);
  String[] parts = transformedData.split(",");

  // 識別子を取得
  int effectNum = getEffectNum(parts[0]);

  // コンソール表示
  /*
  if (effectNum == 0) {
    println("Receive: " + transformedData);// + " from Arduino: " + ip + "processing port: " + arduinoPort);
  }
  */
  
  if (int(parts[1]) == -1000) {
    println("ERROR: ", effectNum + " OverFlow " + parts[2]);
  }

  // OSC通信のデータ形式に変換
  // [スラッシュパラメータ名 具体的な値
  OscMessage oscData = new OscMessage("/data");
  oscData.add(effectNum);
  if (parts.length > 0) {
    for (int i = 1; i < parts.length; i++) {
      oscData.add(int(parts[i])); 
    }
  }

  // どのArduinoから受信したかによって
  // Maxの対応する送り先へ送信
  // switch文だとエラー起きた
  /*
  if (effectNum == 0) {
    println("GET Arduino " + effectNum + " SEND TO " + maxSendPort);
  }
  */
  
  println("Arduino " + effectNum + "Receive: " + transformedData);

  oscP5.send(oscData, myRemoteLocation);
}

int getEffectNum(String macAddr) {
  int ret = -1;
  for (int i = 0; i < macAddrs.length; i++) {
    if (macAddr.equals(macAddrs[i])) {
      ret = i;
      break;
    }
  }
  
  return ret;
}

/**
 * MaxからOSCを受信 振動トリガー
 */
void oscEvent(OscMessage msg) {
  int receivedValue = msg.get(0).intValue(); // 0番目の引数を整数として取得
  //println("Received int: " + receivedValue + " " + broadCastIP + " " + broadCastPort);

  // 全てのArduinoにブロードキャストで送信
  // 実装をシンプルにするために、全ての立体が音源に反応する
  udp.send(str(receivedValue), broadCastIP, broadCastPort);
}

/**
 * 仮データ送信
 */
void test() {
  int testEffectNo = 0;
  // OSC通信のデータ形式に変換
  OscMessage oscData = new OscMessage("/data");
  // 仮のデータを付加
  // 本番はArduinoから受け取ったデータを送信
  // エフェクト番号,数値
  oscData.add(testEffectNo);
  oscData.add(testData);
  delay(interval);
  testData++;

  if (testData > to) {
    testData = from;
  }

  // Maxに送信
  oscP5.send(oscData, myRemoteLocation);
}

/**
 * Arduino1-3号機のデータをMaxの対応するポートに送る
 */
void multiTest() {
  OscMessage oscData = new OscMessage("/data");

  if (testArduinoIdx == 8) {
    testArduinoIdx = 0;
  }

  oscData.add(testArduinoIdx);
  int[][] datas = {
                    {1, 2, 3, 4, 5, 6}, 
                    {2, 3, 4, 5, 6, 7},
                    {3, 4, 5, 6, 7, 8}, 
                    {4, 5, 6, 7, 8, 9},
                    {5, 6, 7, 8, 9, 10},
                    {6, 7, 8, 9, 10, 11},
                    {7, 8, 9, 10, 11, 12},
                    {8, 9, 10, 11, 12, 13}
                  };
                  
  for (int i = 0; i < 6; i++) {
      oscData.add(datas[cnt[testArduinoIdx]][i]);
  }
  
  cnt[testArduinoIdx]++;
  cnt[testArduinoIdx] %= 8;

  // どのArduinoから受信したかによって
  // Maxの対応する送り先へ送信
  // switch文だとエラー起きた
  println("TEST GET Arduino " + testArduinoIdx + " SEND TO " + maxSendPort);
  oscP5.send(oscData, myRemoteLocation);
  
  testArduinoIdx++;

  //delay(100);
}
