/**
 * A chook door automator. Opens and closes the door based on the time of day. 
 * 
 * The opening and closing times vary depending on the month of the year.
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
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "Button.h"
#include "LED.h"
#include "Motor.h"
#include "Buzzer.h"
#include "Door.h"

// close 10 minutes after sunset
#define CLOSE_MINUTES_AFTER_SUNSET 10

const byte AUTO = 0;
const byte MANUAL_OPEN = 1;
const byte MANUAL_CLOSE = 2;
const byte STOPPED = 4;

/*
 * Calculate the time as minutes since midnight
 * 
 * @param h The hour
 * @param m The minute
 * @return The time as minutes since midnight
 */
static int asMins(int h, int m)
{
  return (h * 60) + m;
}

//                               Sunrise and sunset without daylight saving adjustments (as minutes since midnight)
//                                    Jan             Feb             Mar             Apr             May             Jun             Jul             Aug             Sep             Oct             Nov             Dec
const int sunriseTimes[] = { asMins(4, 41), asMins( 5, 16), asMins( 5, 51), asMins( 6, 26), asMins( 6, 58), asMins( 7, 27), asMins( 7, 38), asMins( 7, 20), asMins( 6, 37), asMins( 5, 46), asMins( 4, 58), asMins( 4, 32) } ;
const int sunsetTimes[] =  { asMins(7, 48), asMins( 7, 32), asMins( 6, 55), asMins( 6, 04), asMins( 5, 18), asMins( 4, 50), asMins( 4, 51), asMins( 5, 15), asMins( 5, 46), asMins( 6, 16), asMins( 6, 52), asMins( 7, 28) } ;

Button _closeButton = Button(false, A0, &closePressed);
Button _openButton = Button(false, A3, &openPressed);
Button _autoButton = Button(false, A4, &autoPressed);
LED _redLED = LED(A2, LOW);
LED _greenLED = LED(A1, LOW);
Buzzer _buzzer = Buzzer();
Door _innerDoor = Door(3, 2, 12, 10, _redLED, _greenLED, &halt);
byte _state = AUTO;
RTClib RTC;
DS3231 Clock;
DateTime now;
unsigned long _lastAutoCheck = 0;
bool messages = false;
byte dayStart;
byte monthStart;
byte yearStart;
byte hourStart;
byte minuteStart;
byte secondStart; 

/**
 * Calculate the number of days in a month
 * 
 * @param month The month
 * @param year The year
 * @return the number of days
 */
double daysInMonth(int month, int year)
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
 * Calculate the sun rise or sun set time for the given date
 * 
 * @param day The day (1 - 31)
 * @param month The month (1 - 12)
 * @param year The year (2019 - )
 * @param times The sunrise or sunset times
 * 
 * @return the time the sun will rise or set which is minutes since midnight
 */
int riseOrSetTime(int day, int month, int year, const int times[])
{
  int t1 = times[month-1];
  if (day == 1)
    return t1;

  int t2 = times[month == 12 ? 0 : month];
  double diff = t2 - t1;
  return t1 + (int)((diff * (day - 1) / daysInMonth(month, year)));
}

/**
 * Calculate the time of sun rise
 * 
 * @param day The day (1 - 31)
 * @param month The month (1 - 12)
 * @param year The year (2019 - )
 * @return The time as an int which is minutes since midnight
 */
int sunRise(int day, int month, int year)
{
  return riseOrSetTime(day, month, year, sunriseTimes);
}
                        
/**
 * Calculate the time of sun set
 * 
 * @param day The day (1 - 31)
 * @param month The month (1 - 12)
 * @param year The year (2019 - )
 * @return The time as an int which is minutes since midnight
 */
int sunSet(int day, int month, int year)
{
  return riseOrSetTime(day, month, year, sunsetTimes) + 720;
}

/**
 * Setup 
 */
void setup() 
{
  Wire.begin();
  Serial.begin(9600);
  wdt_enable(WDTO_1S); // enable watchdog; times out after 1 second
  DateTime dt = RTC.now();
  yearStart = dt.year();
  monthStart = dt.month();
  dayStart = dt.day();
  hourStart = dt.hour();
  minuteStart = dt.minute();
  secondStart = dt.second();
}

/**
 * Loop around forever
 */
void loop() 
{
  wdt_reset();  // watchdog reset
  _openButton.interact();
  _closeButton.interact();
  _autoButton.interact();
  _redLED.interact(); 
  _greenLED.interact();
  _buzzer.interact();
  _innerDoor.interact();
 
  unsigned long now = millis();
  if (_state == AUTO && now - _lastAutoCheck > 1000)
  {
    _automateDoor();
    _lastAutoCheck = now;
  }
  
  if (Serial.available() > 0)
    userInput();   
}

/**
 * The open button was pressed. Sound one buzz and cpen the door
 */
void openPressed()
{
  _state = MANUAL_OPEN;
  _buzzer.buzz(100, 1);
  _redLED.removeOpposite();
  _innerDoor.openDoor(true);
}

/**
 * The close button was pressed. Sound two buzzes and close the door.
 */
void closePressed()
{
  _state = MANUAL_CLOSE;
  _buzzer.buzz(100, 2);
  _redLED.removeOpposite();
  _innerDoor.closeDoor(true);
}

/**
 * The auto button was pressed. Sound three buzzes and put the state into auto mode
 */
void autoPressed()
{
  _buzzer.buzz(100, 3);
  _redLED.removeOpposite();
  _lastAutoCheck = 0;
  _state = AUTO;
  _innerDoor.autoOn();
}

/**
 * Check if it is time to open or close the door
 */
void _automateDoor() 
{
  if (messages) Serial.println("automate door start");
  
  DateTime dt = RTC.now();
  int year = dt.year();
  int month = dt.month();
  int day = dt.day();
  int hour = dt.hour();
  int minute = dt.minute();
  int now = asMins(hour, minute);
  int rise = sunRise(day, month, year);
  int set = sunSet(day, month, year) + CLOSE_MINUTES_AFTER_SUNSET;  

  if (messages)
  {
    Serial.print("month=");
    Serial.println(month);
    Serial.print(" day=");
    Serial.println(day);
    Serial.print(" hour=");
    Serial.println(hour);
    Serial.print("minute=");
    Serial.println(minute);
    Serial.print("now=");
    Serial.println(now);
    Serial.print("rise=");
    Serial.println(rise);
    Serial.print("set=");
    Serial.println(set);
  }
  

  if (now >= rise && now < set)
  {
    if ( ! _innerDoor.isOpenOrOpening())
    {
      if (messages)
        Serial.println("open door");
        
      _innerDoor.openDoor(false);
    }
  }
  else
  {
    if ( ! _innerDoor.isClosedOrClosing())
    {
      if (messages)
        Serial.println("close door");
        
      _innerDoor.closeDoor(false);
    }
  }
}

/**
 * Halt the program by putting into the stopped state. Blink the red and green LEDs alternatively.
 */
void halt()
{
  Serial.println("Halted!");
  _state = STOPPED;
  _buzzer.stop();
  _innerDoor.stop();
  _redLED.setOpposite(&_greenLED);
  _redLED.slowBlink();
}

/**
 * Read user input and respond
 */
void userInput()
{
  wdt_disable();
  switch (Serial.read())
  {
    case 'd':
    case 'D':
      changeDate();
      break;
      
    case 'm':
    case 'M':
      messages = ! messages;
      Serial.print("Messages ");
      if (messages)
        Serial.println("on");
      else
        Serial.println("off");
      break;
    
    case 's':
    case 'S':
      printStatus();
      break;

    case 't':
    case 'T':
      changeTime();
      break;
  }
  wdt_enable(WDTO_1S); // enable watchdog; times out after 1 second
}

/**
 * Print the time and date on Serial
 */
void printDateTime(int hour, int minute, int second, int day, int month, int year)
{
  Serial.print(hour);
  Serial.print(":");
  printZeroed(minute);
  Serial.print(":");
  printZeroed(second);
  Serial.print(" ");
  Serial.print(day);
  Serial.print("/");
  printZeroed(month);
  Serial.print("/");
  Serial.println(year);

}

/**
 * Print the given minutes since midnight as h:mm AM/PM
 */
void printMinsFromMidnight(int minsFromMid)
{
  int hour = minsFromMid / 60;
  int minute = minsFromMid % 60;
  bool am = true;
  if (hour > 11)
    am = false;

  if (hour > 12)
    hour -= 12;

  Serial.print(hour);
  Serial.print(':');
  printZeroed(minute);
  Serial.print(' ');
  if (am)
    Serial.print("AM");
  else
    Serial.print("PM");
    
}


/**
 * Print the status of DoorMan
 */
void printStatus()
{
  Serial.println();
  Serial.print("Door state ");
  switch (_state)
  {
    case AUTO:
      Serial.println("AUTO");
      break;
      
    case MANUAL_OPEN:
      Serial.println("MANUAL_OPEN");
      break;
      
    case MANUAL_CLOSE:
      Serial.println("MANUAL_CLOSE");
      break;

    case STOPPED:
      Serial.println("STOPPED");
      break;

    default:
      Serial.print("unknown = ");
      Serial.println(_state);
  }
  
  DateTime dt = RTC.now();
  Serial.print("Now ");
  printDateTime(dt.hour(), dt.minute(), dt.second(), dt.day(), dt.month(), dt.year());
  
  Serial.print("Mins since midnight ");
  Serial.println(asMins(dt.hour(), dt.minute()));
   
  Serial.print("Sun rise @ ");
  printMinsFromMidnight(sunRise(dt.day(), dt.month(), dt.year()));
  Serial.println();
  
  Serial.print("Sun set @ ");
  printMinsFromMidnight(sunSet(dt.day(), dt.month(), dt.year()) + 45);
  Serial.println();

  Serial.print("Booted at ");
  printDateTime(hourStart, minuteStart, minuteStart, dayStart, monthStart, yearStart);
  Serial.println();
  Serial.println();
  Serial.println("S = Status, M = toggle messages, D = change date, T = change time");
}

int readValue(const char* valueName, int minimum, int maximum)
{
  int v = 0;
  while (true)
  {
    Serial.print("Enter value (; or , to end input) for ");
    Serial.print(valueName);
    Serial.print(' ');
    v = readInt();
    if (v < minimum || v > maximum)
      Serial.println("Error - value out of range");
    else
      break;
  }
  Serial.println();
  return v;
}

/**
 * Read an int from Serial
 */
int readInt()
{
  while (Serial.read() == '\n') {};
  while (Serial.read() == '\r') {};

    //Serial.println("readInt begin");
  int value = 0;
  while (true)
  {
    if (Serial.available())
    {
      int ch = Serial.read();
      //Serial.print("ch=");
      Serial.print(ch);
      if (ch == ';' || ch == ',')
        break;

      if (ch >= '0' && ch <= '9')
        value = (value * 10) + ch - '0';
    }
  }
  //Serial.print("readInt end - return ");
  //Serial.println(value);
  return value;
}

/**
 * Print the value with a leading zero (if below 10)
 */
void printZeroed(int value)
{
  if (value < 10)
    Serial.print("0");
  Serial.print(value);
}

/**
 * Change the time
 */
void changeTime()
{
  Serial.print("Enter time without daylight saving\n\n");
  
  int hour = readValue("hour", 0, 23);
  int minute = readValue("minute", 0, 59);
    
  Clock.setClockMode(false);  // set to 24h   
  Clock.setHour(hour);
  Clock.setMinute(minute);

  Serial.print("Time is ");
  Serial.print(hour);
  Serial.print(":");
  printZeroed(minute);
  Serial.println();
  Serial.println();
}

/**
 * Change the date
 */
void changeDate()
{
  int day = readValue("day", 0, 31);
  int month = readValue("month", 1, 12);
  int year = readValue("year", 19, 99);
  
  Clock.setDate(day);
  Clock.setMonth(month);
  Clock.setYear(2000 + year);

  Serial.print("date is ");
  Serial.print(day);
  Serial.print("/");
  printZeroed(month);
  Serial.print("/");
  Serial.print(year);
  Serial.println();
  Serial.println();
}
