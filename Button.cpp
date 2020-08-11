#include "Arduino.h"
#include "Button.h"

/**
 * Construct a Button
 * 
 * @param pin The pin number the button is connected to
 * @param buttonPressedFunction An optional function that is called when a state change is detected 
 */
Button::Button(bool normallyClosed, byte pin, void (*buttonPressedFunction)())
{
  _normallyClosed = normallyClosed;
  _pin = pin;
  _buttonPressedFunction = buttonPressedFunction;
  pinMode(_pin, INPUT_PULLUP);
  _state = -99;
}

/** 
 * Determine if the button is pressed
 * @return true if it is
 */
boolean Button::isPressed()
{
  if (_normallyClosed)
    return _state == HIGH;
    
  return _state == LOW;
}

/**
 * Read the button state from the pin and after handling bouncing if the state has changed set the state variable and call the state change function if it is defined.
 */
void Button::interact()
{
  int stateRead = digitalRead(_pin);
  if (stateRead != _previousState) 
  {
    _lastDebounceTime = millis();
  }
  
  if ((millis() - _lastDebounceTime) > _debounceDelay) 
  {
    if (_state != stateRead)
    {
      _state = stateRead;
      if (isPressed())
      {
        //Serial.print("Button on pin ");
        //Serial.print(_pin);
        //Serial.println(" pressed");
      }
      if (_buttonPressedFunction && isPressed())
      {
        (*_buttonPressedFunction)();
      }

    }
  }
  _previousState = stateRead;
}
