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

#define       DEBUG         1       // 1 to print useful messages to the serial port
#define       serialSpeed   115200  // As appropriate

#include      <Carnival_debug.h>
#include      <Carnival_network.h>
#include      <Carnival_leds.h>


//------------ WHICH GAME, WHAT MODE, HOW HOOKED UP

#define       WHOAMI                 "B"  // which device am I?
#define       butCount                3   // number of buttons connected
int           allButtons[butCount]    = {4,5,12};  // What pins are connected to buttons
int           butspushed[butCount];
#define       ON                      LOW



//------------ DELAY BEHAVIOR 

/* 
  we define the loop delay, then set maxPoofLimit and poofDelay as
  integer loop counters based on the actual loop delay.  The default
  is to poof for no more than 5 seconds, then not respond for 2.5 seconds.  
  note that we specify time limits as a function of the loop delay, in case
  it's more than 1 ms.
*/

const int           loopDelay     = 1;                      // milliseconds to delay at end of loop
const int           poofDelay     = int(2500/loopDelay);    // ms to delay after poofing as loop count
const int           onlineMsg     = int(60000/loopDelay);   // ms to delay between "still online" flashes


int                 stillOnline   = 0;      // Check for connection timeout every 5 minutes


Carnival_debug   debug   = Carnival_debug();
Carnival_leds    leds    = Carnival_leds();
Carnival_network network = Carnival_network();


void setup() {
    

    for (int x = 0; x < butCount; x++) {
        pinMode(allButtons[x], INPUT_PULLUP);
        butspushed[x] = 0;
    }

    leds.startLEDS();

    debug.start(DEBUG,serialSpeed);
    network.start(WHOAMI,DEBUG);
    network.connectWifi();

}


void loop() {
 
  // CONFIRM CONNECTION
    int looksgood = network.reconnect(0);
    

    stillOnline++;
    if(stillOnline >= onlineMsg){
        if(looksgood) {
          leds.blinkBlue(3, 30, 1); // connection maintained (non-blocking)
          network.printWifiStatus(); 
          stillOnline = 0;
        } else {
          looksgood = network.reconnect(1);
        }
    } 

    leds.checkBlue();   

  // REVIEW ALL BUTTONS, STORE AND PUBLISH STATE CHANGE

    for (int x = 0; x < butCount; x++) {

        int pushed = x+1;
        if(digitalRead(allButtons[x])==ON) {
            if (!butspushed[x]) {
                butspushed[x] = 1;
                if (looksgood) { network.callServer(pushed,1); }
            }
        } else {
            if (butspushed[x]) {
                butspushed[x] = 0;
                if (looksgood) { network.callServer(pushed,0); }
            }          
        }
    }

   
  // wait a tiny little bit before looping... because reasons
    
    delay(loopDelay);


} // end main loop






