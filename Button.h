/*
  Button.h - Library for button handling with debounce.

*/
#ifndef Button_h
#define Button_h

#include "Arduino.h"

class Button
{
  public:
    /**
    * Construct a Button
    * 
    * @param normallyClosed If true this a normally closed button otherwise normally open    
    * @param pin The pin number the button is connected to
    * @param buttonPressedFunction An optional function that is called when the button is pressed 
    */
    Button(bool normallyClosed, byte pin, void (*buttonPressedFunction)());

    /** 
    * Determine if the button is pressed
    * @return true if it is
    */
    boolean isPressed();

    /**
    * Read the button state from the pin and after handling bouncing if the state has changed 
    * set the state variable and call the button pressed function if it is defined.
    */
    void interact();
    
  private:
    byte _pin;
    bool _normallyClosed;
    void (*_buttonPressedFunction)();
    byte _state;
    byte _previousState = 77;
    unsigned long _lastDebounceTime = 0;  // the last time the output pin was toggled
    unsigned long _debounceDelay = 50;    // the debounce time; increase if the output flickers
};

#endif
