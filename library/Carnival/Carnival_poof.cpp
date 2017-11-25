/*
  Carnival_poof.cpp - Carnival library
  Copyright 2016 Neil Verplank.  All right reserved.
*/

// include main library descriptions here as needed.
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif

#include "Carnival_poof.h"
#include "Carnival_debug.h"
#include "Carnival_network.h"
#include "Carnival_leds.h"


const    boolean RELAY_ON   = 0;       // opto-isolated arrays are active LOW
const    boolean RELAY_OFF  = 1;


int      POOFING            = 0;       // whether we allow poofing for this sketch
int      allSolenoids[]     = {};      // relay for all the pins that are solenoids
int      solenoidCount      = 0;       // will be the size of the array
int      maxPoof            = 0;       // counter while poofing, for timing out
int      pausePoofing       = 0;       // hit max poof limit, or pause between poofs
boolean  poofing            = 0;       // poofing state (0 is off)
int      loopDelay          = 1;       // ms to delay each loop, default is 1 ms

boolean  POOFSTORM           = 0;      // Allow poofstorm?
int      maxPoofLimit        = 5000;   // milliseconds to limit poof, converted to loop count
int      poofDelay           = 2500;   // ms to delay after poofLimit expires to allow poofing again


extern   Carnival_debug debug;
extern   Carnival_network network;
extern   Carnival_leds leds;



// from networking
extern   boolean       wifiOverride;   // override need of wifi to use button by holding down button while booting.
extern   int           looksgood;      // still connected (0 is not)
extern   long          idleTime;       // current idle time



/*================= CREATION AND SETUP =================*/

Carnival_poof::Carnival_poof(int LD)
{
    // initialize this instance's variables
    POOFING=1;
    if (LD) {
        loopDelay = LD;
    }
}


void Carnival_poof::setSolenoids(int aS[], int size) {

    solenoidCount = size;

    for (int i=0; i<size; i++) {
        allSolenoids[i] = aS[i];
        pinMode(allSolenoids[i], OUTPUT);
        digitalWrite(allSolenoids[i], RELAY_OFF);
    }
}




/*================= POOF FUNCTIONS =====================*/

/* set state to on for all solenoids */
void Carnival_poof::startPoof() {

  if (pausePoofing == 0) {
    if (!poofing && maxPoof <= maxPoofLimit) { // if we're not poofing nor at the limit
      poofing = 1;
      poofAll(true);
      debug.MsgPart("Poofing started:");
      debug.Msg(millis());
      leds.setRedLED(1);
    }
  }
}


/* set state to off for all solenoids */
void Carnival_poof::stopPoof() {
    poofing = 0;
    poofAll(false);
    maxPoof = 0;
    debug.MsgPart("Poofing stopped:");
    debug.Msg(millis());
    leds.setRedLED(0);
 
}



/* check state on every loop   */
void Carnival_poof::checkPoofing() {

    if (pausePoofing > 0) {                  // poofing paused, decrement pause counter
        pausePoofing = pausePoofing - loopDelay;
    } else if (poofing == 1) {               // poofing, increment max timer
        maxPoof = maxPoof + loopDelay;
    }

    if (!looksgood && poofing) {              // stop poofing if no network connection
        stopPoof();
    } else if (maxPoof > maxPoofLimit) {      // pause poofing for 'poofDelay' if we hit max pooflimit
        pausePoofing = poofDelay;
        stopPoof();
    }
}


/* act on poofing message */
void Carnival_poof::doPoof(char *incoming) {

    if (strcmp(incoming, "p1") == 0) {        // start / keep poofing
        startPoof();
    } else if (strcmp(incoming, "p0") == 0) { // stop poofing
        stopPoof();
    } else if (strcmp(incoming, "p2") == 0) { // poofstorm if allowed and still poofing
        if (pausePoofing == 0) {
            if (POOFSTORM) {
                poofStorm();
            }
        }
    } 
}



/* set state for all poofers  */
void Carnival_poof::poofAll(boolean state) {
    for (int y = 0; y < solenoidCount; y++) {
        if(state == true){
            digitalWrite(allSolenoids[y], RELAY_ON);
        } else {
            digitalWrite(allSolenoids[y], RELAY_OFF);
        }
    }
}











/*

    The remaining routines are all blocking, and are not currently being used,
    though there are hooks for POOFSTORM.

*/


void Carnival_poof::poofStorm(){  // crazy poofer display
  debug.Msg("POOFSTORM");
  poofAll(true);
  delay(300);
  poofAll(false); 
  poofSingleLeft(200);
  delay(50);
  poofSingleRight(200);
  delay(50);
  poofLeft(true);
  poofAll(false);
  delay(200);
  poofRight(true);
  poofAll(false);
  delay(200);
  poofAll(true);
  delay(600);
  poofAll(false);
}








void Carnival_poof::poofRight(boolean state) {
   poofRight(state,200);
}

void Carnival_poof::poofRight(boolean state, int rate) {  // overloaded function offering speed setting
   if(!rate){ rate = 200;}
   for (int y = 0; y < solenoidCount; y++) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(rate);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(rate);
      }
  }
}


void Carnival_poof::poofLeft(boolean state) {
   poofLeft(state,200);
}

void Carnival_poof::poofLeft(boolean state, int rate) {  // overloaded function offering speed setting
   if(!rate){ rate = 200;}
   for (int y = solenoidCount; y == 0;  y--) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(rate);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(rate);
      }
  }
}

void Carnival_poof::poofSingleRight(int rate) { // briefly turn on individual poofers
   if(!rate){ rate = 200;}
   for (int y = 0; y < solenoidCount+1; y++) {
    digitalWrite(allSolenoids[y], RELAY_ON);
    if(y > 0){
      digitalWrite(allSolenoids[y-1], RELAY_OFF);
    }
    delay(rate);
  }
}

void Carnival_poof::poofSingleLeft(int rate) { // briefly turn on all poofers to the left
  int arraySize = solenoidCount+1;
   if(!rate){ rate = 200;}
   for (int y = arraySize; y == 0;  y--) {
    digitalWrite(allSolenoids[y], RELAY_ON);
    if(y!= arraySize){
      digitalWrite(allSolenoids[y+1], RELAY_OFF);
    }
    delay(rate);
  }
}

void Carnival_poof::gunIt(){ // rev the poofer before big blast
  for(int i = 0; i < 4; i++){
    poofAll(true);
    delay(100);
    poofAll(false);
    delay(50);
  }
  poofAll(true);
  delay(500);  
}

void Carnival_poof::poofEven(int rate){  // toggle even numbered poofers
  for (int y = 0; y < solenoidCount; y++) {
    if(y != 0 && y%2 != 0 ){
      digitalWrite(allSolenoids[y], RELAY_ON);
      delay(rate);
      digitalWrite(allSolenoids[y], RELAY_OFF);
      delay(rate);
    }
  }
}

void Carnival_poof::poofOdd(int rate){ // toggle odd numbered poofers
  for (int y = 0; y < solenoidCount; y++) {
    if(y == 0 || y%2 == 0 ){
      digitalWrite(allSolenoids[y], RELAY_ON);
      delay(rate);
      digitalWrite(allSolenoids[y], RELAY_OFF);
      delay(rate);
    }
  }
}

void Carnival_poof::lokiChooChoo(int start, int decrement, int rounds){ // initial speed, speed increase, times
  for(int i = 0; i <= rounds; i++){
    poofOdd(start);
    start -= decrement;
    poofEven(start);
    start -= decrement;
  }
  delay(start);
  poofAll(true);
  delay(start);
  poofAll(false);
}


// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

//void Carnival::doSomethingSecret(void) { }
