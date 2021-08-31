#ifndef Door_h
#define Door_h

#include "Arduino.h"
#include "LED.h"
#include "Motor.h"
#include "Button.h"

const byte UNKNOWN = 0;
const byte OPEN = 1;
const byte OPENING = 2;
const byte CLOSED = 3;
const byte CLOSING = 4;
const byte HALTED = 5;

class Door
{
  public:
    Door(byte closedButtonPin, byte openButtonPin, byte motorDirectionPin, byte motorSpeedPin, LED &redLED, LED &greenLED, void (*halt)());
    void interact();
    void closeDoor(bool manual);
    void openDoor(bool manual);
    void stop();
    bool isOpenOrOpening();
    bool isClosedOrClosing();
    void changeLEDState();
    void autoOn();
    
    
  private:
    void checkForJam();
    Button * _closedButton;
    Button * _openButton;
    Motor * _motor;
    LED * _redLED;
    LED * _greenLED;
    volatile int _state;
    unsigned long _motorStartTime;
    void (*_halt)();
    bool _manualOverride = false;

    
};

#endif
