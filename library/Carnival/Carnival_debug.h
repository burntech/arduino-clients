/*
  Carnival_debug.h - Carnival library 
  Copyright 2016 Neil Verplank.  All right reserved.
*/

// ensure this library description is only included once
#ifndef Carnival_debug_h
#define Carnival_debug_h

// include types & constants of other libraries here:
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "WString.h"


// library interface description
class Carnival_debug
{
  // user-accessible "public" interface
  public:
    Carnival_debug();
    void start(int,int);
    void Msg(int message);
    void Msg(unsigned int message);
    void Msg(long message);
    void Msg(unsigned long message);
    void Msg(String message);
    void MsgPart(int message);
    void MsgPart(unsigned int message);
    void MsgPart(long message);
    void MsgPart(unsigned long message);
    void MsgPart(String message);

  // library-accessible "private" interface
  private:
    void MsgOut(void);
};

#endif

