#include "Arduino.h"
#include "Buzzer.h"

/**
 * Create a buzzer that uses pin 4 to control the sound
 */
Buzzer::Buzzer()
{
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
}

/**
 * Sound the buzzer
 * 
 * @param duration How many millisseconds to buzz for
 * @param count The number of buzzes to do
 */
void Buzzer::buzz(long duration, byte count)
{
  _intervalStart = millis();
  _duration = duration;
  _buzzes = count;
  _silences = count - 1;
  _silent = false;
   digitalWrite(4, HIGH);
}

/**
 * Interact with the pin depending on the state of the buzzing
 */
void Buzzer::interact()
{
  if (_duration == 0)
    return;
  
  if (_buzzes > 0 && millis() - _intervalStart > _duration && ! _silent)
  {
    _buzzes--;
    digitalWrite(4, LOW);
    if (_buzzes == 0)
      _duration = 0;
    else
    {
      _intervalStart = millis();
      _silent = true;
    }
  }

  if (_silences > 0 && millis() - _intervalStart > _duration && _silent)
  {
    _silences--;
    digitalWrite(4, HIGH);
    _intervalStart = millis();
    _silent = false;
  }

  
}

/**
 * Stop the buzzing and prevent any more buzzes that might be left from occurring
 */
void Buzzer::stop()
{
  digitalWrite(4, LOW);
  _duration = 0;
}

 
