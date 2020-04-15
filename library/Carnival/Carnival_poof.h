/*
  Carnival_poof.h - Carnival library 
  Copyright 2016-2020 Neil Verplank.  All right reserved.
*/

// ensure this library description is only included once
#ifndef Carnival_poof_h
#define Carnival_poof_h

// include types & constants of other libraries here:
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "WString.h"


#include "Carnival_events.h"

// library interface description
class Carnival_poof
{
  // user-accessible "public" interface
  public:
    Carnival_poof();
    Carnival_poof(long poof_limit, long poof_delay);
    void setSolenoids(int aS[], int size);
    void set_kill(int state);
    int  get_kill();
    void set_kill_remote(int state);
    int  get_kill_remote();

    // non-blocking
    void startPoof();
    void startPoof(int sols[], int size);
    void stopPoof();
    void stopPoof(int sols[], int size);
    void checkPoofing();
    void checkPoofing(int sols[], int size);
    event_t *doPoof(char *incoming);
    void poofAll(boolean state);
    void poofAll(boolean state, int sols[], int size);
    boolean isPoofing(int whichSolenoid);

    // timed sequences
    long int poofStorm(event_t *new_events);  
    long int poof_across(boolean state, int rate, long int st_time, int dir, event_t *new_events);  
    long int gunIt(long int st_time, event_t *new_events); 
    long int poof_alt(int rate, long int st_time, int dir, event_t *new_event);
    long int poofChooChoo(int start, int decrement, int rounds, long int st_time, event_t *new_event); 

  private:
    int get_relays(char *str, int *relays, int *binary, int *relays_off, int *num_relays_off);
};

#endif
