/*
  Carnival_leds.cpp - Carnival library
  Copyright 2016 Neil Verplank.  All right reserved.
*/

// include main library descriptions here as needed.
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif

#include <Carnival_leds.h>

#define     REDLED        0          // what pin?
#define     BLUELED       2          // what pin?
const       boolean ON    = LOW;     // LED ON
const       boolean OFF   = HIGH;    // LED OFF
int         blue_counter  = 0;
int         blue_wait     = 0;
boolean     blue_finish   = OFF;
boolean     blue_state    = OFF;

long        current_time  = 0L;
long        state_change_time = 0L;

Carnival_leds::Carnival_leds()
{

}


/*================= LED FUNCTIONS =====================*/


void Carnival_leds::startLEDS() {
    pinMode(REDLED, OUTPUT);           // set up LED
    pinMode(BLUELED, OUTPUT);          // set up Blue LED
    digitalWrite(REDLED, OFF);         // and turn it off
    digitalWrite(BLUELED, OFF);        // and turn it off
}


// we pass in 1 for light on, 0 for light off,
// but digital pins are on when low....
void Carnival_leds::setLED(int ledpin, bool state) {
    if (state==1) { state = ON; }
    else {state = OFF;}
    digitalWrite(ledpin,state);
    if (ledpin == BLUELED) {
        blue_state=blue_finish=state;
    }
}


void Carnival_leds::setRedLED(bool state) {
    if (state==1) { state = ON; }
    else {state = OFF;}
    digitalWrite(REDLED,state);
}

// blocking
void Carnival_leds::flashLED(int whichLED, int times, int rate, bool finish){
  // flash `times` at a `rate` and `finish` in which state
  for(int i = 0;i <= times ;i++){
    digitalWrite(whichLED, finish);
    delay(rate);
    digitalWrite(whichLED, !finish);
    delay(rate);
  }
}





// NOT YET WORKING CORRECTLY.....
// non-blocking blinking of LED
// needs significant modification to handle both leds....
void Carnival_leds::blinkBlue(int times, int wait, bool finish) {
  
    bool endstate = OFF;
    if (finish==1) { endstate = ON; }

    blue_counter = times+1;
    blue_wait    = wait;
    blue_finish  = endstate;
    checkBlue();
    
}
  
void Carnival_leds::checkBlue() {
    if (blue_counter > 0 || blue_state != blue_finish) {
      
        current_time = millis();
  
        if ((current_time - state_change_time) > (long)blue_wait) {
            state_change_time = current_time;
            blue_state        = !blue_state;
            digitalWrite(BLUELED,blue_state);
            if (blue_state) blue_counter--;  // every other time
        }       
    }
}
