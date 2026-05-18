// Подавление дребезга контактов кнопки
// Алгоритм из интернета, реализация - egeht
// (с) 2026

#ifndef SimpleButton_h
#define SimpleButton_h

#include "Arduino.h"

class SimpleButton {
  private:
    uint8_t btn;
    uint16_t state;
    
  public:
    void begin(uint8_t button); // инициализация
    bool debounce();    // чтение текущего состояния кнопки, если только что нажата - true
    bool onRelease();   // если только что отпущена - true
    bool isIdle();      // если не нажата - true     
    bool isHeld();      // если нажата - true
};

#endif
