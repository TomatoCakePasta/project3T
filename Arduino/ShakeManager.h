#ifndef ShakeManager_h
#define ShakeManager_h

#include <Arduino.h>

// 振動データ
typedef struct {
  const int SHAKE_SOCKET;
  // タイマーインターバル
  // 振動中に再度ブロードキャストが来たら
  long previousShakeMillis;
  // mS
  const long shakeInterval;
} ShakeManager;

#endif