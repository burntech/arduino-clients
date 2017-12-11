/*

    Poofer Client

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
        
    Rev 3.0 - Timing - Sep 17, 2016
    Rev 4.0 - Libraries, Debug messaging, Communications, LEDs

    Oct 12 - updated callServer to work correctly
    OCt 19 - beginning work non-blocking LEDs
    Dec  4 - created carnival libraries

    This code is represents a basic "poofer client".  The 
    
    Use the Actuator to receive "poof" commands via a wireless network
    (specifically, xc-socket-server running on a Raspberry Pi 3 configured as an AP
    with a static IP).
    
    In a nutshell, hook up "BAT" and ground to a 5V power supply, hookup pin
    12 (or more - see allSolenoids) to the relay (which needs it's own power! 
    Huzzah can't drive the relay very well if at all).  

    If you want to send high score or some other success to the server, 
    set HASTBUTTON=1, then connect pin inputButton (5?) to ground to send REPORT 
    (via a button, or have the game Arduino use one of its relays to 
    short inputButton to ground momentarily).
    
    If all connections are good, the blue light is steady on.  When receiving a 
    communication, the blue LED may "sparkle".  If poofing, the red LED will be 
    steadily on. Once a minute, we check the network and socket connections, and (if DEBUG),
    we blink blue 3 times if solid, or turn off the blue LED and start blinking the red LED 
    if no connection or no socket.
    
    As of version 3, the logic has changed - we receive one "poof on" signal, and
    subsequently a "poof off" signal, rather than the "keep alive" strategy of prior
    versions.  We have reduced the loop delay to 1 ms from 100ms.  We might be able
    to drop to 0, but various aspects of the logic would have to change.

    

    Just noticed this interesting feature somewhere:

    // Sleep for 6 seconds, then wait for sensor change
    ESP.deepSleep(6000000, WAKE_RF_DEFAULT); 
    
*/





#define       DEBUG         1         // 1 to print useful messages to the serial port
#define       POOFS         1         // whether or not we're hooked up to a poofer (and need the library)
#define       serialSpeed   115200


#include      <ESP8266WiFi.h>
#include      <Carnival_PW.h>
#include      <Carnival_network.h>
#include      <Carnival_leds.h>
#include      <Carnival_neopx.h>
#include      <FastLED.h>
#include      <Carnival_debug.h>
#ifdef POOFS
    #include  <Carnival_poof.h>
#endif



//------------ WHICH GAME, WHAT MODE, HOW HOOKED UP

#define       WHOAMI         "NEO"    // which game am I?
#define       REPORT         "pushed"  // what to report when the button is pushed, if relevant
const boolean HASBUTTON      = 0;      // Does this unit have a button on
const int     inputButton    = 5;      // What pin is that button on?


const boolean POOFSTORM      = 1;      // Allow poofstorm?
int           mySolenoids[]  = {12,13,14};  // pins that are poofer solenoids
const int     numSolenoids   = sizeof(mySolenoids)/sizeof(int);

extern WiFiClient client;



//------------ DELAY BEHAVIOR 

/* 
  we define the loop delay, then set maxPoofLimit and poofDelay as
  integer loop counters based on the actual loop delay.  The default
  is to poof for no more than 5 seconds, then not respond for 2.5 seconds.  
  note that we specify time limits as a function of the loop delay, in case
  it's more than 1 ms.
*/

const int           loopDelay     = 1;                      // milliseconds to delay at end of loop
const int           maxPoofLimit  = int(5000/loopDelay);    // milliseconds to limit poof, converted to loop count
const int           poofDelay     = int(2500/loopDelay);    // ms to delay after poofing as loop count
const int           onlineMsg     = int(60000/loopDelay);   // ms to delay between "still online" flashes


//------------ FLAGS, COUNTERS, VARIABLES

int         maxPoof          = 0;      // poofer timeout
int         stopPoofing      = 0;      // hit max poof limit
boolean     poofing          = 0;      // poofing state (0 is off)



int         stillOnline      = 0;      // Check for connection timeout every 5 minutes
int         looksgood        = 0;      // still connected (0 is not)




Carnival_debug     debug     = Carnival_debug();
Carnival_network   network   = Carnival_network();
Carnival_leds      leds      = Carnival_leds();
Carnival_neopx     neopx     = Carnival_neopx();


#ifdef POOFS
    Carnival_poof  pooflib   = Carnival_poof();
#endif


//---NEOP

/*
 * Current Possible FastLED Chipsets:
 * APA102, APA104, DOTSTAR, GW6205, GW6205_400, LPD8806, NEOPIXEL, P9813, SM16716, TM1803, TM1804, TM1809, WS2801, WS2811, WS2812, WS2812B, WS2811_400, UCS1903, UCS1903B
*/

const  String    CHIPSET          = "WS2812";
const  int       NUM_LEDS         = 100;       // How many leds are in the strip?
const  int       LED_DATA_PIN     = 2;         // 7 is D7/GPIO 13 on nodemcu (serial data, often 2 / yellow on arduino)
const  int       CLOCK_PIN        = 3;         // 5 is D5/GPIO 14 on nodemcu (clock, often 3 / green on arduino)
CRGB             THE_LEDS[NUM_LEDS];           // Max array out
extern int       BRIGHTNESS  = 50;
extern int       rgb;


void setup() {

#ifdef POOFS
    pooflib.setSolenoids(mySolenoids, numSolenoids);   // set relay pins to OUTPUT and turn off
#endif
    leds.startLEDS();
    neopx.startNEOS(DEBUG);

    if (HASBUTTON) { pinMode(inputButton, INPUT_PULLUP);   }    

    debug.start(DEBUG,serialSpeed);
    network.start(WHOAMI,DEBUG);
    network.connectWifi();

}


/*
   We're looping, looking for signals from the server to poof (or do something)
   or signals from the game to send to the server (high score!).
   
   We start poofing when we get a signal, stop when we get a stop.  We also stop
   when we reach maxPoof.
*/
void loop() {
 
  // CONFIRM CONNECTION
    looksgood = network.reconnect(0);
    

    stillOnline++;
    if(stillOnline >= onlineMsg){
        looksgood = network.reconnect(1);
        if(looksgood) {
          leds.blinkBlue(3, 30, 1); // connection maintained (non-blocking)
          network.printWifiStatus(); 
          stillOnline = 0;
        }
    } 

    leds.checkBlue();

  // process any incoming messages
    int CA = 0;

    if (looksgood) {        
        
        // there's an incoming message - read one and loop (ignore non-phrases)
        
        CA =  client.available();
        if (CA) {

            // read incoming stream a phrase at a time.
              int x=0;
              int cpst;
              char incoming[64];
              memset(incoming,NULL,64);
              char rcvd = client.read();

              // read until start of phrase (clear any cruft)
              while (rcvd != '$' && x< CA) { rcvd = client.read(); x++;}
              
              if (rcvd=='$') { /* start of phrase */
                 int n=0;
                 
                 leds.blinkBlue(4, 15, 1); // show communication, non-blocking
 
                 while (rcvd=client.read()) {
                   if(rcvd=='%') { /* end of phrase - do something */
                     if (strcmp(incoming, "p1")==0) {
                       /* strings are equal, poof it up */
                       // if we've previously hit maxPoofLimit, we ignore incoming
                       // attempts to poof for poofDelay loops
                       if (stopPoofing==0) {
                         if(!poofing && maxPoof <= maxPoofLimit) { // if we're not poofing nor at the limit
                           poofing = 1;
                           debug.Msg("Poofing started");
                           leds.setRedLED(1);
                           pooflib.poofAll(true);
                         } 
                       }
                     } else if (strcmp(incoming, "p2")==0) {
                         if (stopPoofing==0) {
                           debug.Msg(incoming);
                           if (POOFSTORM) { pooflib.poofStorm(); }
                         }
                     } else if (strcmp(incoming, "p0")==0) {
                         debug.Msg("Poofing stopped");
                         poofing=0;
                     } else {
                         // it's some other phrase
                         poofing = 0;
                         debug.Msg(incoming);
                         debug.Msg("other phrase, poofing 0");
                     } 
                     break; // phrase read, end loop
                    
                   } else { // not the end of the phrase
                       incoming[n]= rcvd;
                       n++;
                   }
                 } // end while client.read
              } // end if $ beginning of phrase
        } // end if  client available
    } // end client looksgood
    else { // no connection, stop poofing
      poofing = 0;
    }
    
    if (maxPoof > maxPoofLimit){
        /* hit our poofing limit, we force a stop for a minimum period of time */
        poofing = 0;
        stopPoofing = poofDelay;
        maxPoof = 0;
    }
    
    if (poofing == 0) {
        leds.setRedLED(0);
        // turn off all poofers
        pooflib.poofAll(false);
        maxPoof=0;
    }

    if (!client.available()) { // Is the server asking for something?
        if(HASBUTTON) { checkButton(); }
          
        /* 
          if not poofing, decrement our paus poofing counter.
          if poofing, increment our timeout counter.
        */
        
        if (stopPoofing>0) { stopPoofing--; }  
        else if (poofing==1) { maxPoof++; }
        
        /* wait a tiny little bit... because reasons */                 
        delay(loopDelay);
   
    }   
   
}


/*  
    do something if the inputButton has been pushed.
    we may want to use serial communications with the main game
    board, so we can get alphanumeric info...
*/
 
int checkButton(){
  
    boolean shotValue=digitalRead(inputButton);
    
    if(shotValue==LOW) {
        // button being pushed
        debug.Msg("Button pushed!");
        network.callServer(REPORT);
        leds.blinkBlue(5, 30, 1); // show communication
    }

}


