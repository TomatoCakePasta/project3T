#ifndef IMUManager_h
#define IMUManager_h

#include <Arduino.h>

// ジャイロデータ保存構造体
typedef struct {
  int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
  int16_t gyroX_offset, gyroY_offset, gyroZ_offset;
  // delayを使わずにジャイロの送信周期を設定
  long previousGyroMillis;
  const long gyroInterval;
  long resetMillis;
  int currentCalibrate;
  int16_t preAcX, preAcY, preAcZ, preGyX, preGyY, preGyZ;
  int16_t retAcX, retAcY, retAcZ, retGyX, retGyY, retGyZ;
} GyroData;

#endif