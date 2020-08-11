#include "Arduino.h"
#include "LED.h"

/**
 * Construct a LED
 * 
 * @param pin The pin the LED is connected to
 * @param initialState The inital state of the LED
 */
LED::LED(byte pin, byte initialState)
{
  _pin = pin;
  _state = OFF;
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, initialState);
}

/**
 * Turn the LED on
 */
void LED::on()
{
  digitalWrite(_pin, HIGH);
  _state = ON;
  _isOff = false;
  if (_opposite)
    _opposite->off();
}

/**
 * Turn the LED off
 */
void LED::off()
{
  digitalWrite(_pin, LOW);
  _state = OFF;
  _isOff = true;
  if (_opposite)
    _opposite->on();
}

/**
 * Blink the LED on and off until turned on or off
 * 
 * @param duration The duration of the on and off in milliseconds
 */
void LED::blink(int duration)
{
  _duration = duration;
  _isOff = true;
  _timeLastChange = millis();
  this->off();
  _state = BLINK;
}

/**
 * Make the LED blink every second
 */
void LED::slowBlink()
{
  this->blink(1000);
}

/**
 * Make the LED blink every 250 milliseconds
 */
void LED::fastBlink()
{
  this->blink(250);
}

/**
 * Set the LED whose state will be set to the opposite of this one
 */
void LED::setOpposite(LED * opposite)
{
  _opposite = opposite;
  if (_isOff)
    _opposite->on();
  else
    _opposite->off();
}

/**
 * Remove the LED whose state will be set to the opposite of this one
 */
void LED::removeOpposite()
{
  _opposite = 0;
}

/**
 * Interact the the LED via the pin depending on the blink state
 */
void LED::interact()
{
  if (_state == BLINK)
  {
    unsigned long now = millis();
    if (now - _timeLastChange >= _duration)
    {
      if (_isOff)
      {
        digitalWrite(_pin, HIGH);
        if (_opposite)
          _opposite->off();
      }
      else
      {
        digitalWrite(_pin, LOW);
        if (_opposite)
          _opposite->on();
      }
      _timeLastChange = now;
      _isOff = ! _isOff;
    }
  }
}
