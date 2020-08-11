/**
 * A chook door automator. Opens and closes the door based on the time of day. The opening and closing times vary
 * depending on the month of the year.
 * 
 * Also supports manual opening and closing of the door.
 * 
 * 
      Pins used

        2 = Inner door open limit switch
        3 = Inner door close limit switch
        4 = Buzzer

        12 = Inner door direction
        13 = Inner door speed
        
        A0 = Close button
        A3 = Open button
        A4 = Auto button 
*/

#include <DS3231.h>
#include <Wire.h>
#include <avr/wdt.h>

#include "Button.h"
#include "LED.h"
#include "Motor.h"
#include "Buzzer.h"
#include "Door.h"

const byte AUTO = 0;
const byte MANUAL_OPEN = 1;
const byte MANUAL_CLOSE = 2;
const byte STOPPED = 4;

//                               Sunrise and sunset without daylight saving adjustments
//                               Jan   Feb   Mar   Apr   May   Jun   Jul   Aug   Sep   Oct   Nov   Dec
static double sunriseTimes[] = { 4.41, 5.16, 5.51, 6.26, 6.58, 7.27, 7.38, 7.20, 6.37, 5.46, 4.58, 4.32 } ;
static double sunsetTimes[] =  { 7.48, 7.32, 6.55, 6.04, 5.18, 4.50, 4.51, 5.15, 5.46, 6.16, 6.52, 7.28 } ;


// Times are specified as floats in the format hh.mm
const float OPEN_TIME = 7.15;
const float CLOSE_TIME = 18.45;

Button _closeButton = Button(false, A0, &closePressed);
Button _openButton = Button(false, A3, &openPressed);
Button _autoButton = Button(false, A4, &autoPressed);
LED _redLED = LED(A2, LOW);
LED _greenLED = LED(A1, LOW);
Buzzer _buzzer = Buzzer();
Door _innerDoor = Door(3, 2, 12, 10, _redLED, _greenLED, &halt);
byte _state = AUTO;
RTClib RTC;
DateTime now;
unsigned long _lastClock = 0;

/**
 * Calculate the number of days in a month
 * 
 * @param month The month
 * @param year The year
 * @return the number of days
 */
int days(int month, int year)
{
  switch (month)
  {
    case 2:   // Feb
      return (year % 4 == 0) ? 29 : 28;

    case 4:   // Apr
    case 6:   // Jun
    case 9:   // Sep
    case 11:  // Nov
      return 30;

    case 1:   // Jan
    case 3:   // Mar
    case 5:   // May
    case 7:   // Jul
    case 8:   // Aug
    case 10:  // Oct
    case 12:  // Dec
      return 31;
    default:
      break;
  }
  return 0;
}

/**
 * Calculate the sun rise or sun set time for today
 * 
 * @param day The day
 * @param month The month
 * @param year The year
 * @param times The sunrise or sunset times
 * 
 * @return the time the sun will rise or set
 */
double riseOrSetTime(int day, int month, int year, double times[])
{
  double t1 = times[month-1];
  if (day == 1)
    return t1;

  //  5.45 - 6.27 : diff = -0.42 , days 30, -0.0.14 per day
  double t2 = times[month == 12 ? 0 : month];
  double diff = t2 - t1;
  return t1 + (diff * (day - 1) / days(month, year));
}

/**
 * Calculate the time of sun rise
 * 
 * @param day The day
 * @param month The month
 * @param year The year
 */
double sunRise(int day, int month, int year)
{
  return riseOrSetTime(day, month, year, sunriseTimes);
}
                        
/**
 * Calculate the time of sun set
 * 
 * @param day The day
 * @param month The month
 * @param year The year
 */
double sunSet(int day, int month, int year)
{
  return riseOrSetTime(day, month, year, sunsetTimes) + 12.0;
}

/**
 * Add a value to a decimal time. Decimal time is hour.min where min goes from 0 to .59
 * 
 * E.g
 *    5.45 + 0.45 = 6.30
 *    6.17 + 0.22 = 6.39
 *    8.35 + 1.50 = 10.25
 * 
 * @param to The decimal time to add to
 * @param add The value to add. This can be a decimal time too 
 * 
 * @return The result as a decimal time  
 */
double addTime(double to, double add)
{
  int whole = (int)to;
  double frac = to - whole;
  if (frac + add >= 0.6)
  { 
    // 5.45 + 0.45 = 6.30
    double mins = frac + add;
    double sum = whole;
    while (mins >= 1.0)
    {
      mins -= 1.0;
      sum += 1.0;
    }
    if (mins >= 0.6)
    {
      mins -= 0.6;
      sum += 1;
    }
    return sum + mins;
  }
  return to + add;
}

/**
 * Setup 
 */
void setup() 
{
  Wire.begin();
  Serial.begin(9600);
  wdt_enable(WDTO_1S);
  //Serial.println("start");
}

/**
 * Loop around forever
 */
void loop() 
{
  wdt_reset();
  _openButton.interact();
  _closeButton.interact();
  _autoButton.interact();
  _redLED.interact(); 
  _greenLED.interact();
  _buzzer.interact();
  _innerDoor.interact();
 
  unsigned long now = millis();
  if (_state == AUTO && now - _lastClock > 1000)
  {
    _automateDoor();
    _lastClock = now;
  }
}

/**
 * The open button was pressed. Sound one buzz and cpen the door
 */
void openPressed()
{
  _state = MANUAL_OPEN;
  _buzzer.buzz(100, 1);
  _innerDoor.openDoor(true);
}

/**
 * The close button was pressed. Sound two buzzes and close the door.
 */
void closePressed()
{
  _state = MANUAL_CLOSE;
  _buzzer.buzz(100, 2);
  _innerDoor.closeDoor(true);
}

/**
 * The auto button was pressed. Sound three buzzes and put the state into auto mode
 */
void autoPressed()
{
  _buzzer.buzz(100, 3);
  _lastClock = 0;
  _state = AUTO;
  _innerDoor.autoCloseOn();
}

/**
 * Check if it is time to open or close the door
 */
void _automateDoor() 
{
  //Serial.println("automate door start");
  DateTime dt = RTC.now();
  int year = dt.year();
  int month = dt.month();
  ////Serial.print("automate door month");
  ////Serial.println(month);
  int day = dt.day();
  //Serial.print("automate door day=");
  //Serial.println(day);

  int hour = dt.hour();
  //Serial.print("automate door hour=");
  //Serial.println(hour);
  int minute = dt.minute();
  //Serial.print("automate door minute=");
  //Serial.println(minute);
  
  float hhmm = hour + ((float)minute / 100);

  //Serial.println("automate door check time");

  double rise = addTime(sunRise(day, month, year), 0.5);
  double set = addTime(sunSet(day, month, year), 0.45);

  if (hhmm >= rise && hhmm < set)
  {
    if ( ! _innerDoor.isOpenOrOpening())
    {
      //Serial.println("automate door open");
      _innerDoor.openDoor(false);
    }
  }
  else
  {
    if ( ! _innerDoor.isClosedOrClosing())
    {
      //Serial.println("automate door close");
      _innerDoor.closeDoor(false);
    }
  }


  
  //Serial.print(hhmm); 
  //Serial.print('\n');
  //Serial.print(minute);
  //Serial.print('\n');
  //Serial.println("automate door end");

}

/**
 * Halt the program by putting into the stopped state. Blink the red and green LEDs alternatively.
 */
void halt()
{
  //Serial.println("Halt called");
  _state = STOPPED;
  _buzzer.stop();
  _innerDoor.stop();
  _redLED.setOpposite(&_greenLED);
  _redLED.slowBlink();
}
