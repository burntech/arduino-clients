/*
  Carnival_analog.h - Carnival library 
  Copyright 2020 Neil Verplank.  All right reserved.
*/

// ensure this library description is only included once
#ifndef Carnival_analog_h
#define Carnival_analog_h

// include types & constants of other libraries here:
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "WString.h"


// library interface description
class Carnival_analog
{
  // user-accessible "public" interface
  public:
    Carnival_analog();
    void initButtons();
    void initButtons(int size);
    void initButtons(int size, boolean override);
    void initAnalog(int size);
    int  checkButtons();
    void readAnalog();

};

#endif
