/*

    ESP XC-Client

    Copyright 2016, 2017, 2018, 2019 - Neil Verplank (neil@capnnemosflamingcarnival.org)

    This file is part of The Carnival.  See README.md for more detail.

    NOTE that you will need to copy the file library/Carnival/Carnvial_PW.h.default to 
    Carnival_PW.h, and change the wifi name and password to reflect your server's values.

    http://github.com/burntech/

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

    Depressing the poofer button overrides the wireless server requirement.

*/

/* determine size of an allocated array */
#define NUM_ELS(x)  (sizeof(x) / sizeof(x[0]))



/* 

  At a minimum, you should change the name of the effect for each ESP
  you use.  Comment out "#define ESP8266" to use the ESP32 (more details
  below).  And edit the appropriate pin matrix below depending on your
  physical connections.

*/

#define       WHOAMI           "LOKI"   // which effect am I? (B=The Button, ORGAN, LULU, CAMERA - can be any string)

const int     DEBUG            = 1;        // 1 to turn serial on and print useful (ha!) messages

//#define       ESP8266                      // comment out if using an ESP32 of any kind

#ifndef ESP8266
  #define       ESP32                      // comment out 8266 above to use Adafruit-style ESP32
  #define       SPARKFUN                   // specifically if using sparkfun esp32 thing (comment out if not the sparkfun)
#endif

#define       POOFS                        // COMMENT OUT FOR BUTTONS AND ETC&
//#define     DNS                          // UN-COMMENT 'Do Not Signal' to turn off incoming signals (ie. if you're a button)


/* 
 * Fill in the appropriate pin or pins, if any, for each of the following arrays
 * (for the processor and board you're using). 
 * 
 * Note that the array order is important - for the button, the first pin listed
 * would be "THE" button.  For solenoids, they are "named" internally in an ordinal
 * fashion - e.g., {43,12,3} means pin 43 = array element '0', but poofer # '1',
 * 12 would be array element 1, and poofer # '2', etc. etc.  This matters when 
 * you're doing timed sequences and want to address a particular poofer on a 
 * particular effect (e.g. poofers 1 through 3, or number 11), and don't want to be
 * hard-coding pin numbers in your timing sequences.
 * 
 * Typically, you would eliminate pin definitions below if there's nothing on that  
 * pin.  It won't "hurt" to leave them definied, but a) it slows things down a bit,
 * and b) certain functions (e.g. poofing around an array of solenoids) don't make 
 * a lot of sense if there's nothing actually hooked up.  Analog pins with nothing
 * on them just return noise, button states should just be 0 if there aren't any
 * hooked up.
 * 
 * However, it's also helpful to see all the pins available and laid out - in many
 * cases, which pin you choose for what *matters* - only some are analog, only some
 * can be both input and output, there are only some with pull up/down resistors, and
 * some have LEDS already, which we use (red shows when poofing (or blue on the esp32)
 * and blue is solid when networking is active on the ESP8266.
 */
 

#ifdef ESP8266 // Adafruit Huzzah ESP8266 and similar, possibly NodeMCU
//      int       killSwitch       = 0;            // could be 0 or 4 on the 8266 - define if there is a kill switch
      int       inputButtons[]   = {};             // 5 on the 8266 Feather, = D1 on the Entryway.  First one is poof button 
      int       mySolenoids[]    = {12};        // 12,13, 14 on the 8266 = D6, D7 on the Wemos.  These are poofer relay(s)
      int       allAnalog[]      = {};             // A0 - don't set if nothing on the pin
#else
  #ifdef ESP32
    #ifdef SPARKFUN
      int       killSwitch       = 15;
      int       inputButtons[]   = {}; // 4?
      int       mySolenoids[]    = {32,33,25,26,27,14,17,16,18,23,19,22};
      int       allAnalog[]      = {};             // 36...39 - don't set if nothing on the pin
    #else // Adafruit Huzzah ESP32, possibly others
      int       killSwitch       = 33;             // 33
      int       inputButtons[]   = {32};           // 32
      int       mySolenoids[]    = {12,13,14,15};  // 12...15
      int       allAnalog[]      = {};             // A0...A3 - don't set if nothing on the pin
    #endif
  #endif
#endif





//////////////// IN THEORY, YOU CAN IGNORE THE REST OF THIS CODE FOR NOW....

/* 
 *  If you're creating a wireless poofer, using (say) a NodeMCU or an 
 *  Adafruit Huzzah 8266 or what have you, and you've wired it according
 *  to the pin definitions above, and you've set up your variables right,
 *  and your server is up and running correctly, your ESP client will
 *  "just work" (and turn a poofing relay on and off, and blink its 
 *  lights correctly).
 *  
 *  
 */


/* Probably don't need to change */

#define       SERIAL_SPEED     115200      // active if debugging


/* Constants, State-arrays, Flags */

const int     numSolenoids     = NUM_ELS(mySolenoids);
const int     button_count     = NUM_ELS(inputButtons);
const int     adc_count        = NUM_ELS(allAnalog);
long          lastButsChgd[button_count];  // most recently allowed state change for given button
boolean       button_state[button_count];  // physical button is currently pushed (not wireless)
int           adc_reading[adc_count];      // reading on current adc pins
int           KILL_SWITCH           = 0;   // local kill-state



// ----------- CARNIVAL LIBRARY DEFINITIONS AND CREATION


#ifdef ESP8266
  #include      <ESP8266WiFi.h>            // ESP8266
#else
  #include      <WiFi.h>                   // ESP32
#endif


#include      <Carnival_PW.h>              // your networking passwords file
#include      <Carnival_network.h>         // networking library
#include      <Carnival_leds.h>            // onboard LED library
#include      <Carnival_debug.h>           // debug messages library
#include      <Carnival_events.h>          // timed sequences
#include      <Carnival_analog.h>          // timed sequences
#ifdef POOFS
  #include    <Carnival_poof.h>            // poofer library
#endif

Carnival_debug     debug     = Carnival_debug();
Carnival_network   network   = Carnival_network();
Carnival_leds      leds      = Carnival_leds();     // display leds (red is poofing. on 8266, blue is networking)
Carnival_events    events    = Carnival_events();
Carnival_analog    analog    = Carnival_analog();
#ifdef POOFS
  Carnival_poof    pooflib   = Carnival_poof();
#endif


event_t           *my_events             = NULL;         // struct for holding all pending timed sequences


void setup() {

    // turn on debugging and output if DEBUG==1 above
    debug.start(DEBUG, SERIAL_SPEED);

    // set up solenoids if any
    #ifdef POOFS
        pooflib.setSolenoids(mySolenoids, numSolenoids);
    #endif

    // set up onboard leds (red is poof, blue is network, board dependent)
    #ifdef SPARKFUN
        leds.startLEDS(5,-1);
    #else
        leds.startLEDS();
    #endif

    // set up buttons, if any.  First button is poof button, and 
    // when re-starting while button held, go into network override mode.
    // force that here by initButtons(button_count,1)
    analog.initButtons(button_count);
    analog.initAnalog(adc_count);

    // initialize and start network connection
    network.start(WHOAMI, DEBUG);
    
    /*  
       Control messages (sent immediately, then re-broadcast with keep alive and reconnect)
 
         network.addSetting("*:DS:1");                // set the don't_send flag (good for buttons and such)
         network.addSetting("*:CC:EFFECT1,EFFECT2");  // set my broadcast collection to EFFECT1 and EFFECT2         
    */

#ifdef DNS
     // turns off incoming messages
     network.addSetting("*:DS:1");
#endif

}


/*
   We're looping, looking for signals from the server to poof (or do something)
   or signals from the game to send to the server (button pushed! high score!).

   We start poofing when we get a signal, stop when we get a stop.  We also stop
   when we reach maxPoof, or if the kill switch is on, or we've received an
   external kill signal.
*/
void loop() {

    network.confirmConnect();         // confirm wireless connection, socket connection
    leds.checkBlue();                 // check/update led status (non-blocking)
    analog.checkButtons();            // check/update button(s) state(s)
    analog.readAnalog();              // check readings on analog pins

    


    /*
     * Read and process any incoming wireless messages, as well as any queued 
     * events ready to begin.  Process all messages for system messages
     * (eg poof, kill, keep alive, control settings).  Ultimately returns
     * a linked list of non-system messages that came in or came current.
     */
    msg_t  *new_msgs   = NULL;
    if(network.processIncoming(&new_msgs, &my_events)) {    
        doMsgs(new_msgs);                // process any new non-system messages
    }

    
    #ifdef POOFS
      pooflib.checkPoofing();         // check/update poofing state(s)
    #endif

    yield();                          // give wireless time to do its thing (~100uSec on avg?)

}  // end main loop




/*
 * Iterate through all non system messages, free any memory allocated to messages
 */
void doMsgs(msg_t *my_msgs) {

    msg_t  *new_msg;

    for (new_msg = my_msgs; new_msg != NULL; new_msg = new_msg->next) {

        // This is where you would do something meaningful with an incoming line
        // new_msg->msg contains an incoming string

        //debug.MsgPart("DoMsg:");
        //debug.Msg(new_msg->msg);

    }

    if (my_msgs)
        events.free_msg_list(my_msgs);  // free up memory from msg list

}










