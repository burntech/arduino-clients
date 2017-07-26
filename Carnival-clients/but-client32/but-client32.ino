/*
  testing with sparkfunesp32, not yet working
       
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


#define       DEBUG         1       // 1 to print useful messages to the serial port
#define       serialSpeed   115200  // As appropriate

extern "C" {
//#include "gpio.h"
}

#include      <Carnival_debug.h>
#include      <Carnival_network.h>
#include      <Carnival_leds.h>


//------------ WHICH GAME, WHAT MODE, HOW HOOKED UP


#define        WHOAMI                 "B"           // which device am I?
#define        butCount                1            // number of buttons connected
int            allButtons[butCount]    = {0};       // What pins are connected to button(s)
int            butspushed[butCount];
#define        ON                      LOW
int            looksgood               = 0;


      int      sleep                   = 1;         // 1 - go to sleep if unused for a time, 0, always on
const int      shake                   = 1;         // detect shaking / has shake sensor / send shake message




const int      knockSensor             = A0;
const int      threshold               = 20;

unsigned long  curTime                 = 0L;

const int      wakeTime                = 10000;     // milliseconds to stay awake after movement stops
const int      wakeButton              = 0;         // must assign a particular button to wake up.  "0" on the nodeMCU mini is GPIO 0 = D3


unsigned long  lastPush                = 0L;
unsigned long  lastShake               = 0L;

int            sensorReading           = 0;
int            secondSensor            = 0;
int            lastSensor              = 0;



//------------ DELAY BEHAVIOR 

/* 
  we define the loop delay, then define counters based on the actual loop delay 
  in case loop Delay is more than 1 ms.
*/

const int           loopDelay     = 1;                      // milliseconds to delay at end of loop
const int           onlineMsg     = int(60000/loopDelay);   // ms to delay between "still online" flashes (5 min)
int                 stillOnline   = 0;



Carnival_debug   debug   = Carnival_debug();
Carnival_leds    leds    = Carnival_leds();
Carnival_network network = Carnival_network();


void setup() {
 
    gpio_init(); // Initilise GPIO pins

    startButtons();

    // turn off sleep mode if it's on and the button is pressed when booting
    // may not be working....
    if(digitalRead(allButtons[0])==LOW && sleep) {
        sleep = 0;
        debug.Msg("sleep mode disabled");
    }
 
    leds.startLEDS();

    debug.start(DEBUG,serialSpeed);
    network.start(WHOAMI,DEBUG);
    network.connectWifi();

    curTime = lastPush = millis();

}





void loop() {
 
    checkConnection();
 
    curTime = millis();
    
    leds.checkBlue();
    boolean pushed = checkButtons();
    boolean shaken = checkSensor();

//#if ESP
/*
    if (!pushed && !shaken) {
        checkSleep();
    }
*/    
    delay(loopDelay); // wait a tiny little bit before looping... because reasons


} // end main loop












void startButtons() {
    for (int x = 0; x < butCount; x++) {
        pinMode(allButtons[x], INPUT_PULLUP);
        digitalWrite(allButtons[x], HIGH); // connect internal pull-up

        butspushed[x] = 0;
    }
}



// see if any button has been pushed, or released, and send that status
boolean checkButtons() {

    boolean  someButtonPushed = false;
    
    for (int x = 0; x < butCount; x++) {
        int pushed = x+1;
        if(digitalRead(allButtons[x])==LOW) {
            if (!butspushed[x]) {
                butspushed[x] = 1;
                someButtonPushed = true;

                if (looksgood) { network.callServer(pushed,1); }
            }
        } else {
            if (butspushed[x]) {
                butspushed[x] = 0;
                if (looksgood) { network.callServer(pushed,0); }
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

  boolean movement = false;

  sensorReading = analogRead(knockSensor);
 
  if (lastSensor>= threshold && secondSensor>=threshold && sensorReading>=threshold) {
      movement = true;
// causes a segfault - because it "reads" as a large integer button instead of a message. ("B:shake:")
//      network.callServer("shake");
      debug.MsgPart("Knock:");
      debug.Msg(sensorReading);
  }


  secondSensor = sensorReading;
  lastSensor = secondSensor;

  if (movement) {
      lastShake = curTime;    
  }

  return movement;
}





// confirm we have a connected port and wifi
void checkConnection() {
    looksgood = network.reconnect(0);

    stillOnline++;
    if(stillOnline >= onlineMsg){
        if(looksgood) {
          leds.blinkBlue(3, 30, 1); // connection maintained (non-blocking)
          network.printWifiStatus(); 
          stillOnline = 0;
        } else {
          looksgood = network.reconnect(1);
          if (looksgood) {
            leds.blinkBlue(3, 30, 1); // connection maintained (non-blocking)
          }
        }
    } 
}

//if ESP
/*
// go to sleep if in sleep mode, and it's been "wakeTime" seconds since button(s) pushed
void checkSleep() {

    if (!sleep) {return;}
    
    if ((curTime - lastPush) > wakeTime) {
        network.sleepNow(wakeButton);
        debug.Msg("I'm awake!!!");
        lastPush = millis();
        stillOnline = 0;
    }  
}
*/
