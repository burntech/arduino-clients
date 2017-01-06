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

// include this library's description file
#include "Carnival_poof.h"
#include "Carnival_debug.h"

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

const boolean RELAY_ON      = 0;       // opto-isolated arrays are active LOW
const boolean RELAY_OFF     = 1;


int POOFING=0;
int allSolenoids[] = {};
int solenoidCount  = 0;

extern Carnival_debug debug;


Carnival_poof::Carnival_poof()
{
  // initialize this instance's variables
  POOFING=1;
}

// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

//void Carnival::doSomething(void)
//{
  // even though this function is public, it can access
  // and modify this library's private variables

  // it can also call private functions of this library
  //doSomethingSecret();
//}


void Carnival_poof::setSolenoids(int aS[], int size) {

    solenoidCount = size;

    for (int i=0; i<size; i++) {
        allSolenoids[i] = aS[i];
        pinMode(allSolenoids[i], OUTPUT);
        digitalWrite(allSolenoids[i], RELAY_OFF);
    }
}


/*================= POOF FUNCTIONS =====================*/

void Carnival_poof::poofAll(boolean state) {
  // turn on all poofers
  for (int y = 0; y < solenoidCount; y++) {
    if(state == true){
      digitalWrite(allSolenoids[y], RELAY_ON);
    }else{
      digitalWrite(allSolenoids[y], RELAY_OFF);
    }
  }
}

void Carnival_poof::poofRight(boolean state) {
   for (int y = 0; y < solenoidCount; y++) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(200);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(200);
      }
  }
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
   for (int y = solenoidCount; y == 0;  y--) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(200);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(200);
      }
  }
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

void Carnival_poof::puffRight(boolean state, int rate) {
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

void Carnival_poof::puffSingleRight(int rate) { // briefly turn on individual poofers
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

void Carnival_poof::poofStorm(){  // crazy poofer display
  debug.Msg("POOFSTORM");
  poofAll(true);
  delay(300);
  poofAll(false); 
  poofSingleLeft(200);
  delay(50);
  puffSingleRight(200);
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
