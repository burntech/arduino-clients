/*
  Carnival_analog.cpp - Carnival library
  Copyright 2020 Neil Verplank.  All right reserved.
*/

// include main library descriptions here as needed.
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif

#include <Carnival_debug.h>
#include <Carnival_analog.h>
#include <Carnival_network.h>

#define       DEBOUNCE         35          // minimum milliseconds between button state change


extern   Carnival_debug   debug;
extern   Carnival_network network;
extern   int killSwitch; 
extern   int allAnalog[];
extern   int inputButtons[];
extern   int button_state[];
extern   int adc_reading[];
extern   int lastButsChgd[];


int adc_count; 
int button_count; 

Carnival_analog::Carnival_analog()
{

}


/*================= ANALOG FUNCTIONS =====================*/


void Carnival_analog::initButtons() {
    initButtons(0, 0);
}

void Carnival_analog::initButtons(int count) {
    initButtons(count, 0);
}



/* initialize any button pins and button states */
void Carnival_analog::initButtons(int count, boolean wireless_override) {

  #ifdef killSwitch
    if (killSwitch) {
        pinMode(killSwitch, INPUT_PULLUP);  // connect internal pull-up
        digitalWrite(killSwitch, HIGH); 
    }
  #endif

    button_count = count;
 
    for (int x = 0; x < button_count; x++) {
        pinMode(inputButtons[x], INPUT_PULLUP);
        button_state[x] = false;       
        if (x==0)
            if (wireless_override)
                network.check_override(LOW);
            else
                network.check_override(digitalRead(inputButtons[0]));
        // now turn off button state
        digitalWrite(inputButtons[x], HIGH); 
    
    }  
}



/* Initialize any Analog pins and set current readings */
void Carnival_analog::initAnalog(int count) {

#ifdef ESP32
// not sure if we need to do this, or if using the Arduino IDE, 
// this is set by default?
    analogReadResolution(12);        // 12 bits of precision
    analogSetAttenuation(ADC_11db);  // Use 3.3 V For all pins
#endif
    adc_count = count;

    for (int x = 0; x < adc_count; x++) {
        pinMode(allAnalog[x], INPUT);
        adc_reading[x] = 0;
    }  
}




/*  
 *   update state of onboard button(s), send state to server, do something (like poof).  
 *   returns non-zero if any button just pushed.
*/
int Carnival_analog::checkButtons() {

    boolean button_value;
    long now        = millis();
    int  somebutton = 0;
    
    // allow push if we have button, and are connected (or WIFI_OVERRIDE)
    if (button_count && network.OK()) {  
        for (int x = 0; x < button_count; x++) {
            int     button_number    = x+1;
            button_value = digitalRead(inputButtons[x]);

            // if button *just* pushed (first detection since last state change)
            if (button_value == LOW) {
                if (!button_state[x] && ((now - lastButsChgd[x]) > DEBOUNCE)) {
                    button_state[x] = true;
                    somebutton      = button_number;
                    lastButsChgd[x] = now;
                  #ifdef POOFS
                    if (x==0)
                    pooflib.startPoof();
                  #endif
                    network.callServer(button_number,1);
                }
            } else {
                // if button *just* released...
                if (button_state[x] && ((now - lastButsChgd[x]) > DEBOUNCE)) {
                    button_state[x] = false;
                    lastButsChgd[x] = now;
                  #ifdef POOFS
                    if (x==0)
                      pooflib.stopPoof();
                  #endif
                    network.callServer(button_number,0);
                }
            }
        }
    }

    return somebutton;
}



/* get current readings for anything connected to an Analog pin.  */
void Carnival_analog::readAnalog() { 
    for (int x = 0; x < adc_count; x++) {
        adc_reading[x] = analogRead(allAnalog[x]);
    }
}




char *int2str (int num) {

    int NUMLEN = 2; // at least one digit plus null
    if (num)
        NUMLEN = (int)((ceil(log10(num))+1)*sizeof(char));
    char* out        = (char*)malloc(NUMLEN);
   
    itoa(num, out, 10);
    
    return out;
}

