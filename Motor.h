#ifndef Motor_h
#define Motor_h

#include "Arduino.h"

const byte FORWARDS = HIGH;
const byte BACKWARDS = LOW;

class Motor
{
    public:


      Motor(byte directionPin, byte speedPin);
      void start(byte direction, int speed);
      void stop();

    private:
      byte _directionPin;
      byte _speedPin;
    
};


#endif
