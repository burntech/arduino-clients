/*



    ESP Client

    Copyright 2016, 2017 - Neil Verplank (neil@capnnemosflamingcarnival.org)

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

    Rev 3.0 - Timing - Sep 17, 2016
    Rev 4.0 - Libraries, Debug messaging, Communications, LEDs

    Oct 12 - updated callServer to work correctly
    OCt 19 - beginning work non-blocking LEDs
    Dec  4 - created carnival libraries
    May 2, 2017 - added on board button=poofer
    Sep 2, 2017 - added check in startPoof to confirm wifi.  added wifi override mode.
     
    This code is represents a basic "poofer client". 

    Use the Actuator to receive "poof" commands via a wireless network
    (specifically, xc-socket-server running on a Raspberry Pi 3 configured as an AP
    with a static IP).

    In a nutshell, hook up "BAT" and ground to a 5V power supply, hookup pin
    12 (or more - see allSolenoids) to the relay (which needs it's own power!
    Huzzah can't drive the relay very well if at all).

    If all connections are good, the blue light is steady on.  When receiving a
    communication, the blue LED may "sparkle".  If poofing, the red LED will be
    steadily on. Once a minute, we check the network and socket connections, and (if DEBUG),
    we blink blue 3 times if solid, or turn off the blue LED and start blinking the red LED
    if no connection or no socket.

    As of version 3, the logic has changed - we receive one "poof on" signal, and
    subsequently a "poof off" signal, rather than the "keep alive" strategy of prior
    versions.  We have reduced the loop delay to 1 ms from 100ms.  We might be able
    to drop to 0, but various aspects of the logic would have to change.

    wifi override - device won't poof without wireless connection to server.  If it
    has an onboard button, you should be able to hold the button down when booting
    or restarting, and override the need for a server.


   INTERESTING:

   #include <ESP8266WiFiMulti.h>   // Include the Wi-Fi-Multi library

   ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

*/




/* TURN DEBUGGING, AND / OR POOFING ON AND OFF, SET SERIAL SPEED IF DEBUGGING */

const int     DEBUG            = 0;        // 1 to print useful messages to the serial port

#define       POOFS                      // comment out for buttons
#define       serialSpeed      115200      // active if debugging
#define       WHOAMI           "BIGBETTY"         // which effect am I?
int           inputButtons[]   = {5};    // What pins are buttons on? 
                                           // (NOTE pin 2 on huzzah is blue LED)
                                           // Also, we test the first pin for wifi override
int           mySolenoids[]    = {12,13};  // pins that are solenoids


// from network lib, somewhat experimental
extern boolean       wifiOverride;   // override need of wifi to use button by holding down button while booting, default = 0


/* DONT CHANGE */

#define       DEBOUNCE         50          // minimum milliseconds between state change
const int     numSolenoids     = sizeof(mySolenoids) / sizeof(int);
const int     butCount         = sizeof(inputButtons) / sizeof(int);
const int     loopDelay        = 1;        // milliseconds to delay at end of loop if nothing else occurs
boolean       onboardPush[butCount];       // physical button is currently pushed (not wireless)
long          lastButsChgd[butCount];      // most recently allowed state change for given button
int           butspushed[butCount];        // state of a given button


#include      <ESP8266WiFi.h>

#include      <Carnival_PW.h>              // your networking passwords file
#include      <Carnival_network.h>         // networking library
#include      <Carnival_leds.h>            // onboard LED library
#include      <Carnival_debug.h>           // debug messages library



/* EXTERNAL FLAGS AND LIMITS YOU CAN SET*/
#ifdef POOFS
  #include  <Carnival_poof.h>

  extern boolean POOFSTORM;     // Allow poofstorm? Default = no.
  extern int     maxPoofLimit;  // absolute maximum time to allow a poof, default = 5000 ms
  extern int     poofDelay;     // time in ms to delay after manual poofing to allow poof again, default = 2500 ms

#endif






// ----------- CREATE LIBRARIES -- DONT CHANGE

extern WiFiClient client;

Carnival_debug     debug     = Carnival_debug();
Carnival_network   network   = Carnival_network();
Carnival_leds      leds      = Carnival_leds();

#ifdef POOFS
  Carnival_poof    pooflib   = Carnival_poof(loopDelay);
#endif







void setup() {

    #ifdef POOFS
        pooflib.setSolenoids(mySolenoids, numSolenoids);   // set relay pins to OUTPUT and turn off
    #endif

    leds.startLEDS();                     // onboard LEDS
    initButtons();                        // set up onboard button if it exists
    debug.start(DEBUG, serialSpeed);
    network.start(WHOAMI, DEBUG);
    network.connectWifi();
    network.confirmConnect();       // confirm wireless connection, socket connection

    /*  
       good place to send control messages, eg:
 
         network.callServer(WHOAMI,"*:DS:1");                // set the don't_send flag (good for buttons and such)
         network.callServer(WHOAMI,"*:CC:EFFECT1,EFFECT2");  // set my broadcast collection to EFFECT1 and EFFECT2
         
    */

    // network.callServer(WHOAMI,"*:DS:1");

}


/*
   We're looping, looking for signals from the server to poof (or do something)
   or signals from the game to send to the server (buttonn pushed! high score!).

   We start poofing when we get a signal, stop when we get a stop.  We also stop
   when we reach maxPoof.
*/
void loop() {

    network.confirmConnect();       // confirm wireless connection, socket connection

    leds.checkBlue();               // check led status

    char* msg = network.readMsg();  // read any incoming messages

    if (msg) {
        processMsg(msg);            // process any incoming messages
    }

    int butPushed = checkButtons();                  // check button state, state change

#ifdef POOFS
    pooflib.checkPoofing();         // check for start, stop, or still poofing         
#endif

    network.keepAlive();            // periodically confirm connection with messaging

    delay(loopDelay);               // time sync here? nano delay?

}  // end main loop





void initButtons() {

    if (!butCount) { return; }
    
    for (int x = 0; x < butCount; x++) {
        pinMode(inputButtons[x], INPUT_PULLUP);
        digitalWrite(inputButtons[x], HIGH); // connect internal pull-up
        butspushed[x] = 0;
        if (!wifiOverride && x == 0) {
            // if it's the first button and it's pushed set override = true
            boolean test = digitalRead(inputButtons[x]);
            if (test == LOW) {
                // if the button is held down when booting, we're in wifi override mode.
                wifiOverride = true;
            }
        }
    }   
}




void processMsg(char *incoming) {

    if (incoming[0] == 'p') {
#ifdef POOFS
        // it's message about poofing, handle it
        pooflib.doPoof(incoming);
#endif
    } // else some other message.....

}






/*  do something if the inputButton has been pushed or released.  */

int checkButtons() {

    int somebutton = 0;
    
    if (butCount && network.OK()) {  // allow push if we have button, and are connected (or wifiOverride)
        for (int x = 0; x < butCount; x++) {
            int     pushed    = x+1;
            boolean shotValue = digitalRead(inputButtons[x]);

            if (shotValue == LOW) {                  // button pushed
                if (!onboardPush[x]) {               // first (?) detection of push
                    if ((millis() - lastButsChgd[x]) > DEBOUNCE) {
                        onboardPush[x] = true;
                        somebutton = pushed;
                        lastButsChgd[x] = millis();
#ifdef POOFS
                        pooflib.startPoof();         // start poofing if relevant
#endif
                        network.callServer(pushed,1);
                    }
                }
            } else {                                 // not pushed
                if (onboardPush[x]) {                // button just released
                    if ((millis() - lastButsChgd[x]) > DEBOUNCE) {
                        onboardPush[x]   = false;
                        lastButsChgd[x] = millis();
#ifdef POOFS
                        pooflib.stopPoof();           // stop poofing, if relevant
#endif
                        network.callServer(pushed,0);
                    }
                }
            }
        }
   }

   return somebutton;
}


