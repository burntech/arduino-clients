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

#ifdef ESP8266
int         REDLED        = 0;
int         BLUELED       = 2;
const       boolean ON    = LOW;     // LED ON
const       boolean OFF   = HIGH;    // LED OFF
#endif

#ifdef ESP32
int         REDLED        = 13;       // adafruit huzzah esp32
int         BLUELED       = -1;       // no actual blue pin
const       boolean ON    = HIGH;     // LED ON  // seems odd the 32 would be opposite
const       boolean OFF   = LOW;      // LED OFF
#endif

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
    startLEDS(REDLED,BLUELED);
}

void Carnival_leds::startLEDS(int red, int blue) {
    REDLED  = red;
    BLUELED = blue;
    pinMode(REDLED, OUTPUT);           // set up LED
    digitalWrite(REDLED, OFF);         // and turn it off
#ifdef ESP32
#else
    pinMode(BLUELED, OUTPUT);          // set up Blue LED
    digitalWrite(BLUELED, OFF);        // and turn it off
#endif
}

int Carnival_leds::redPin() {
    return REDLED;
}

int Carnival_leds::bluePin() {
    return BLUELED;
}

// we pass in 1 for light on, 0 for light off,
// but digital pins are on when low....
void Carnival_leds::setLED(int ledpin, bool state) {
#ifdef ESP32
    return;
#endif
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
#ifdef ESP32
    return;
#endif
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
#ifdef ESP32
    return;
#endif
  
    bool endstate = OFF;
    if (finish==1) { endstate = ON; }

    blue_counter = times+1;
    blue_wait    = wait;
    blue_finish  = endstate;
    checkBlue();
    
}
  
void Carnival_leds::checkBlue() {
#ifdef ESP32
    return;
#endif
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
