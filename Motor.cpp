#include "Arduino.h"
#include "Motor.h"

/**
 * Construct a Motor
 * 
 * @param directionPin The pin that control motor direction
 * @param speedPin The pin that control motor speed
 */
Motor::Motor(byte directionPin, byte speedPin)
{
  _directionPin = directionPin;
  _speedPin = speedPin;
  pinMode(_directionPin, OUTPUT);
}

/**
 * Start the motor
 * 
 * @param direction The direction; either FORWARDS or BACKWARDS
 * @param speed The speed in the range 1 to 255
 */
void Motor::start(byte direction, int speed)
{
  digitalWrite(_directionPin, direction);
  analogWrite(_speedPin, speed);
}

/**
 * Stop the motor
 */
void Motor::stop()
{
   analogWrite(_speedPin, 0);
}
