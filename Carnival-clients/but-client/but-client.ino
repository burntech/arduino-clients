/*
       
    The Button
    
    Copyright Nov 26, 2016 - Neil Verplank (neil@capnnemosflamingcarnival.org)
        
    This file is part of The Carnival.

    The Carnival is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Carnival is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The Carnival.  If not, see <http://www.gnu.org/licenses/>.
    
    ##########
    
    
    This is a variation on the The Carnival's Huzzah client software, 
    specific to The Button.  It allows for many buttons to be read 
    and reported to the server with very low latency.

    All buttons listed in the array (allButtons) send a signal to the server, 
    which decides what to do.  A signal is sent when the button is pressed, and 
    when released, along with the corresponding number of the button in 
    the array (note that the first element, the ZEROth element in programming 
    parlance is button ONE, and etc).

    Note that the first button pin listed is "THE BUTTON" - it sends the
    poof signal out when pushed, then the reset signal when released.
    
    The second button listed is the "round robin" button.
    
    The third button listed sends the "poofstorm" signal.

    Note that the logic of how these buttons "behave" is in the
    xc-socket-server.

    Any additional buttons listed and connected will send a pushed and
    released signal, along with the corresponding button number.
    
*/

//------------ WHICH GAME, WHAT MODE, HOW HOOKED UP

#define        DEBUG                   1           // 1 to print useful messages to the serial port
#define        serialSpeed             115200      // As appropriate
#define        WHOAMI                  "B"         // which device am I?
int            allButtons[]            = {4,5};    // What pins are connected to button(s)
      int      sleep                   = 0;        // 1 - go to sleep if unused for a time, 0, always on
const int      shake                   = 0;        // detect shaking / has shake sensor / send shake message



#define        ON                      LOW
const int      butCount                = sizeof(allButtons) / sizeof(int);
int            butspushed[butCount];

extern "C" {
#include "gpio.h"
}

#include      <Carnival_debug.h>
#include      <Carnival_network.h>
#include      <Carnival_leds.h>




const int      knockSensor             = A0;
const int      threshold               = 20;
unsigned long  lastShake               = 0L;
int            sensorReading           = 0;
int            secondSensor            = 0;
int            lastSensor              = 0;



long           lastPush                = 0;
long           curTime                 = 0;

//------------ DELAY BEHAVIOR 

/* 
  we define the loop delay, then define counters based on the actual loop delay 
  in case loop Delay is more than 1 ms.
*/

const int           loopDelay     = 1;                      // milliseconds to delay at end of loop



Carnival_debug   debug   = Carnival_debug();
Carnival_leds    leds    = Carnival_leds();
Carnival_network network = Carnival_network();


void setup() {
 
    gpio_init();     // Initialise GPIO pins
    initButtons();  // set up onboard buttons

    // turn off sleep mode if it's on and the button is pressed when booting
    // may not be working....
    if(digitalRead(allButtons[0])==LOW && sleep) {
        sleep = 0;
        debug.Msg("sleep mode disabled");
    }
 
    leds.startLEDS();                // set up onboard leds
    debug.start(DEBUG,serialSpeed); 
    network.start(WHOAMI,DEBUG);
    network.connectWifi();
    curTime = lastPush = millis();

}



boolean pushed = false;
boolean shaken = false;


void loop() {
 
    network.confirmConnect();
 
    curTime = millis();
    
    leds.checkBlue();

    pushed = checkButtons();

    shaken = checkSensor();

    if (!pushed && !shaken) {
        checkSleep();
    }

    network.keepAlive();
    
    delay(loopDelay); // wait a tiny little bit before looping... because reasons


} // end main loop












void initButtons() {
    for (int x = 0; x < butCount; x++) {
        pinMode(allButtons[x], INPUT_PULLUP);
        digitalWrite(allButtons[x], HIGH); // connect internal pull-up

        butspushed[x] = 0;
    }
}



// see if any button has been pushed, or released, and send that status
boolean checkButtons() {

    boolean  someButtonPushed = false;
    if (!butCount) { return false; }
    
    for (int x = 0; x < butCount; x++) {
        int pushed = x+1;
        if(digitalRead(allButtons[x])==LOW) {
            if (!butspushed[x]) {
                butspushed[x] = 1;
                someButtonPushed = true;
                network.callServer(pushed,1);
            }
        } else {
            if (butspushed[x]) {
                butspushed[x] = 0;
                network.callServer(pushed,0);
            }          
        }
    }
    if (someButtonPushed) {
        lastPush = curTime;
        debug.Msg("button pushed");
    }
    return someButtonPushed;
}



// this is non-blocking, and we look for several high readings in a row
boolean checkSensor(void) {


  if (!shake) { return false; }

/*
  boolean movement = false;

  sensorReading = analogRead(knockSensor);
 
  if (lastSensor>= threshold && secondSensor>=threshold && sensorReading>=threshold) {
      movement = true;
      network.callServer("SHAKE",sensorReading);
      debug.MsgPart("Knock:");
      debug.Msg(sensorReading);
  }


  secondSensor = sensorReading;
  lastSensor = secondSensor;

  if (movement) {
      lastShake = curTime;    
  }

  return movement;

  */
}



// go to sleep if in sleep mode, and it's been "wakeTime" seconds since button(s) pushed
void checkSleep() {

/*    if (!sleep) {return;}
    
    if ((curTime - lastPush) > wakeTime) {
        network.sleepNow(wakeButton);
        debug.Msg("I'm awake!!!");
        lastPush = millis();
        stillOnline = 0;
    }  
*/

}

