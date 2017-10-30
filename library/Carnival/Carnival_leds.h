/*
  Carnival_leds.h - Carnival library 
  Copyright 2016 Neil Verplank.  All right reserved.
*/

// ensure this library description is only included once
#ifndef Carnival_leds_h
#define Carnival_leds_h

// include types & constants of other libraries here:
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "WString.h"


// library interface description
class Carnival_leds
{
  // user-accessible "public" interface
  public:
    Carnival_leds();
    void startLEDS();
    void startLEDS(int red, int blue);
    int  redPin();
    int  bluePin();
    void setLED(int ledpin, bool state);
    void setRedLED(bool state);
    void flashLED(int whichLED, int times, int rate, bool finish);
    void blinkBlue(int times, int wait, bool finish);
    void checkBlue();

};

#endif
