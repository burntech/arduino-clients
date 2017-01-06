/*
  Carnival_debug.cpp - Carnival library
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
#include "Carnival_debug.h"

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

int MY_DEBUG=0;
int MY_SPEED=0;
String piece_out = "";


Carnival_debug::Carnival_debug()
{
}

// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

//void Carnival_Debug::doSomething(void)
//{
  // even though this function is public, it can access
  // and modify this library's private variables

  // it can also call private functions of this library
  //doSomethingSecret();
//}

void Carnival_debug::start(int debugValue, int sSpeed) {
  MY_DEBUG = debugValue;
  MY_SPEED = sSpeed;
  if (MY_DEBUG && MY_SPEED) { 
      Serial.begin(MY_SPEED); 
      Msg("Debugging on!");
  }
}

void Carnival_debug::Msg(int message){
  if (MY_DEBUG) { piece_out += message; Carnival_debug::MsgOut(); }
}
void Carnival_debug::Msg(unsigned int message){
  if (MY_DEBUG) { piece_out += message; Carnival_debug::MsgOut(); }
}
void Carnival_debug::Msg(long message){
  if (MY_DEBUG) { piece_out += message; Carnival_debug::MsgOut(); }
}
void Carnival_debug::Msg(unsigned long message){
  if (MY_DEBUG) { piece_out += message; Carnival_debug::MsgOut(); }
}
void Carnival_debug::Msg(String message){
  if (MY_DEBUG) { piece_out += message; Carnival_debug::MsgOut(); }
}


void Carnival_debug::MsgPart(int message){
  if (MY_DEBUG) { piece_out += message; }
}
void Carnival_debug::MsgPart(unsigned int message){
  if (MY_DEBUG) { piece_out += message; }
}
void Carnival_debug::MsgPart(long message){
  if (MY_DEBUG) { piece_out += message; }
}
void Carnival_debug::MsgPart(unsigned long message){
  if (MY_DEBUG) { piece_out += message; }
}
void Carnival_debug::MsgPart(String message){
  if (MY_DEBUG) { piece_out += message; }
}



// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

//void Carnival_debug::doSomethingSecret(void) { }

void Carnival_debug::MsgOut() {
    Serial.println(piece_out);
    piece_out = "";
}
