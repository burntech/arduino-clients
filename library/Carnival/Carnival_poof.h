/*
  Carnival_poof.h - Carnival library 
  Copyright 2016 Neil Verplank.  All right reserved.
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


// library interface description
class Carnival_poof
{
  // user-accessible "public" interface
  public:
    Carnival_poof(int loopDelay);
    void setSolenoids(int aS[], int size);

    void startPoof();
    void stopPoof();
    void checkPoofing();
    void doPoof(char *incoming);

    void poofAll(boolean state);
    void poofRight(boolean state);
    void poofRight(boolean state, int rate);  
    void poofLeft(boolean state);
    void poofLeft(boolean state, int rate);  
    void puffRight(boolean state, int rate);
    void puffSingleRight(int rate); 
    void poofSingleLeft(int rate); 
    void poofStorm();  
    void gunIt(); 
    void poofEven(int rate);  
    void poofOdd(int rate); 
    void lokiChooChoo(int start, int decrement, int rounds); 

  // library-accessible "private" interface
//  private:
//    int value;
//    void doSomethingSecret(void);
};

#endif


/*================= POOF FUNCTIONS =====================*/


// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

//void Carnival::doSomethingSecret(void) { }
