#ifndef LEDManager_h
#define LEDManager_h

typedef struct {
  unsigned long previousLEDMillis;
  // mS
  const long interval;
  float noiseValue;
  float noiseIncrement; // 変化のスピードを抑える
  int led_width; // 初期値を中間輝度に
  boolean isLightOn;
} LEDConfig;

#endif