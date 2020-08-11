#ifndef LED_h
#define LED_h

#include "Arduino.h"

class LED
{
  public:
    const byte ON = 1;
    const byte OFF = 0;
    const byte BLINK = 2; 
    
    LED(byte pin, byte initialState);
    void on();
    void off();
    void interact();
    void blink(int duration);
    void slowBlink();
    void fastBlink();
    void setOpposite(LED * opposite);
    void removeOpposite();

  private:
    byte _pin;
    byte _state;
    int _duration;
    byte _isOff;
    unsigned long _timeLastChange;
    LED * _opposite;
};


#endif
