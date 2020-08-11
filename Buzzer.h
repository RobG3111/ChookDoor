#ifndef Buzzer_h
#define Buzzer_h

#include "Arduino.h"


class Buzzer
{
    public:


      /**
       * Create a buzzer that uses pin 4 to control the sound
       */
      Buzzer();
      
      /**
       * Sound the buzzer
       * 
       * @param duration How many millisseconds to buzz for
       * @param count The number of buzzes to do
       */
      void buzz(long duration, byte count);

      /**
       * Interact with the pin depending on the state of the buzzing
       */
      void interact();

      /**
       * Stop the buzzing and prevent any more buzzes that might be left from occurring
       */
      void stop();
 
    private:
      unsigned long _intervalStart;
      long _duration; 
      byte _buzzes; 
      byte _silences;
      bool _silent; 
};


#endif
