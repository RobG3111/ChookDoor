#include "Door.h"
#include <Arduino.h>

/**  
 * Construct a Door  
 * 
 * @param closedButtonPin The button the close limit switch is on
 * @param openButtonPin The button the open limit switch is on
 * @param motorDirectionPin The motor direction pin
 * @param motorSpeedPin The motor speed pin
 * @param redLED A pointer to the red LED
 * @param greenLED A pointer to the green LED
 * @param halt A pointer to the halt function; called if the door doesn't close or open within a time limit
 */
Door::Door(byte closedButtonPin, byte openButtonPin, byte motorDirectionPin, byte motorSpeedPin, LED &redLED, LED &greenLED, void (*halt)())
{
  _closedButton = new Button(true, closedButtonPin, NULL);
  _openButton = new Button(true, openButtonPin, NULL);
  _motor = new Motor(motorDirectionPin, motorSpeedPin);
  _redLED = &redLED;
  _greenLED = &greenLED;
  _halt = halt;
  _state = UNKNOWN;
  _closedButton->interact();
  if (_closedButton->isPressed())
    _state = CLOSED;

  _openButton->interact();
  if (_openButton->isPressed())
    _state = OPEN;
}

/**
 * Interact with the close and open limit swirches and then control the motor based on the door state
 */
void Door::interact()
{
    // //Serial.println("Door::interact");
  _closedButton->interact();
  _openButton->interact();

  switch(_state)
  {
    case OPENING:
      ////Serial.println("Door::interact - opening");
      if (_openButton->isPressed())
      {
         //Serial.println("Door::interact - open pressed");
        _state = OPEN;
        _motor->stop();
        changeLEDState();
      }
      else
      {
        checkForJam();
      }
      break;

    case CLOSING:
      ////Serial.println("Door::interact - closing");
      if (_closedButton->isPressed())
      {
        //Serial.println("Door::interact - closed pressed");
        _state = CLOSED;
        _motor->stop();
        changeLEDState();
      }
      else
      {
        checkForJam();        
      }
      break;
  
  }
      // //Serial.println("Door::interact - exit");
}

/**
 * Set the manual override flag off and change the LED state based on where the door is
 */
void Door::autoCloseOn()
{
  _manualOverride = false;
  changeLEDState();
}

/**
 * Change the LED state basd=ed on the door state. 
 * 
 * If the door is open then 
 *    if in auto mode then
 *        turn the green LED on
 *    else
 *        make the green LED blink fast
 *        
 * If the door is closed then  
 *    if in auto mode then
 *        turn the red LED on
 *    else
 *        make the red LED blink fast
 */
void Door::changeLEDState()
{
  if (_state == OPEN)
  {
    if (_manualOverride)
      _greenLED->fastBlink();
    else
      _greenLED->on();
  }
  else if (_state == CLOSED)
  {
    if (_manualOverride)
      _redLED->fastBlink();
    else
      _redLED->on();
  }
}

/**
 * Close the door.
 * 
 * @param manual If true then closing due to manual override
 */
void Door::closeDoor(bool manual)
{
 
  if (_state != CLOSED && _state != CLOSING)
  {
    //Serial.println("Door::closeDoor set state CLOSING");
    _state = CLOSING;
    _motor->start(BACKWARDS, 255);
    _motorStartTime = millis();
    _greenLED->off();
    _redLED->slowBlink(); 
    _manualOverride = manual;    
  }
  
}

/**
 * Open the door.
 * 
 * @param manual If true then opening due to manual override
 */
void Door::openDoor(bool manual)
{
  if (_state != OPEN && _state != OPENING)
  {
     //Serial.println("Door::openDoor set state OPENING");
    _state = OPENING;
    _motor->start(FORWARDS, 255);
    _motorStartTime = millis();
    _redLED->off();
    _greenLED->slowBlink();
    _manualOverride = manual;
  }
  
}

/**
 * Check for a door jam. If more than 2 minutes has elapsed since the motor started then stop the motor, set the state to HALTED
 * and call the halt() function.
 */
void Door::checkForJam()
{
  ////Serial.println("Door::checkForJam");
  if (millis() - _motorStartTime > 120000)
  {
     //Serial.println("Door::checkForJam - call halt");
    _motor->stop();
    _state = HALTED;
    (*_halt)();
  }
  ////Serial.println("Door::checkForJam - exit");
}

/**
 * Stop the door and turn the LEDs off
 */
void Door::stop()
{
  //Serial.println("Door::stop");
  _motor->stop();
  _redLED->off();
  _greenLED->off();
}

/**
 * Determine if the door is open or opening
 * 
 * @return true if the door is open or opening
 */
bool Door::isOpenOrOpening()
{
  return _state == OPEN || _state == OPENING;
}

/**
 * Determine if the door is closed or closing
 * 
 * @return true if the door is closed or closing
 */
bool Door::isClosedOrClosing()
{
  return _state == CLOSED || _state == CLOSING;
}
