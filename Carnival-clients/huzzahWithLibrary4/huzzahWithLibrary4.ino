/*

    ESP XC-Client

    Copyright 2016, 2017, 2018 - Neil Verplank (neil@capnnemosflamingcarnival.org)

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

#define       WHOAMI           "BIGBETTY"  // which effect am I? (B=The Button, ORGAN, LULU, CAMERA - can be any string)

const int     DEBUG            = 1;        // 1 to turn serial on and print useful (ha!) messages

#define       ESP8266                      // comment out if using an ESP32 of any kind

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
      int       killSwitch       = 0;              // should be 4 - kill switch
      int       inputButtons[]   = {5};            // should be 5 - poof button(s) 
      int       mySolenoids[]    = {12,13};  // 12,13,14,16 - poofer relay(s)
      int       allAnalog[]      = {};             // A0 - don't set if nothing on the pin
#else
  #ifdef ESP32
    #ifdef SPARKFUN
      int       killSwitch       = 15;
      int       inputButtons[]   = {4};
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





//////////////// IN THEORY MARGE, YOU CAN IGNORE THE REST OF THIS CODE FOR NOW....

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
#define       DEBOUNCE         35          // minimum milliseconds between button state change


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
#ifdef POOFS
  #include    <Carnival_poof.h>            // poofer library
#endif

Carnival_debug     debug     = Carnival_debug();
Carnival_network   network   = Carnival_network();
Carnival_leds      leds      = Carnival_leds();     // display leds (red is poofing. on 8266, blue is networking)
Carnival_events    events    = Carnival_events();
#ifdef POOFS
  Carnival_poof    pooflib   = Carnival_poof();
#endif


event_t        *my_events = NULL;           // struct for holding the current timed sequence
long           seq_start  = 0L;             // start time for timed sequence





void setup() {

    debug.start(DEBUG, SERIAL_SPEED);

    #ifdef POOFS
        pooflib.setSolenoids(mySolenoids, numSolenoids);
    #endif
    #ifdef SPARKFUN
        leds.startLEDS(5,-1);
    #else
        leds.startLEDS();
    #endif
    initButtons();
    initAnalog();

    
    network.start(WHOAMI, DEBUG);
    network.connectWifi();
    network.confirmConnect();
    
    /*  
       good place to send control messages, eg:
 
         network.callServer(WHOAMI,"*:DS:1");                // set the don't_send flag (good for buttons and such)
         network.callServer(WHOAMI,"*:CC:EFFECT1,EFFECT2");  // set my broadcast collection to EFFECT1 and EFFECT2         
    */

#ifdef DNS
     // turn off incoming messages
     network.callServer(WHOAMI,"*:DS:1");
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
    checkButtons();                   // check/update button(s) state(s)
    readAnalog();                     // check readings on analog pins

    char* msg = network.readMsg();    // read any incoming wireless messages

    if (msg==NULL || strlen(msg)==0)
        // if no wireless messages, check for any timed events
        my_events = events.check_events(my_events, seq_start, &msg);
     
    if (msg && strlen(msg)>0) {
        // process any incoming messages or timed events (free event list if message is "kill")
        my_events = processMsg(msg, my_events);
        debug.Msg(msg);
    }

    my_events = checkKill(my_events); // check kill switch / kill state, clean events if needed

    #ifdef POOFS
      pooflib.checkPoofing();         // check/update poofing state(s)
    #endif

    yield();                          // give wireless time to do its thing (~100uSec on avg?)

}  // end main loop



void initButtons() {
    initButtons(0);
}



/* initialize any button pins and button states */
void initButtons(boolean wireless_override) {

    if (killSwitch) {
        pinMode(killSwitch, INPUT_PULLUP);  // connect internal pull-up
        digitalWrite(killSwitch, HIGH); 
    }
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
void initAnalog() {

#ifdef ESP32
// not sure if we need to do this, or if using the Arduino IDE, 
// this is set by default?
    analogReadResolution(12);        // 12 bits of precision
    analogSetAttenuation(ADC_11db);  // Use 3.3 V For all pins
#endif
    for (int x = 0; x < adc_count; x++) {
        pinMode(allAnalog[x], INPUT);
        adc_reading[x] = 0;
    }  
}



/* do something with an incoming message */
event_t *processMsg(char *incoming, event_t *my_events) {

    event_t *new_events;
    new_events = NULL;
    
    if (strcmp(incoming, "kill") == 0) {        
      // external kill signal, set flag, stop poofing
      #ifdef POOFS
        pooflib.set_kill_remote(1);
        pooflib.stopPoof();
      #endif

      // nullify any timed events, since we're killed
      events.free_event_list(my_events);
      my_events = NULL;

    } else if (strcmp(incoming, "free") == 0) {
      #ifdef POOFS
        pooflib.set_kill_remote(0);
      #endif
    } else if (incoming[0] == 'p') {
      #ifdef POOFS
        new_events = pooflib.doPoof(incoming);
      #endif
    } else {
      // else some other message.....
    }

    if (my_events || new_events)
        my_events = events.concat_events(my_events,new_events);

    return my_events;
}




/*  
 *   update state of onboard button(s), send state to server, do something (like poof).  
 *   returns non-zero if any button just pushed.
*/
int checkButtons() {

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




/* check kill button and kill state */
event_t *checkKill(event_t *my_events) {
        
    if (killSwitch) {  // if there's a local kill switch, check it.      
        int button_value = digitalRead(killSwitch);
        if (button_value == LOW && !KILL_SWITCH) {         // state wrong, button *just* pressed
            KILL_SWITCH = 1;
            network.callServer(WHOAMI,"kill::");
        } else if (button_value == HIGH && KILL_SWITCH) {  // state wrong, button *just* released
            KILL_SWITCH = 0;
            network.callServer(WHOAMI,"free::");
        }
    }

  #ifdef POOFS
    int k_state = pooflib.get_kill();
    if (k_state != KILL_SWITCH) {
        if (KILL_SWITCH) {  // just killed, locally
            pooflib.stopPoof();
            pooflib.set_kill(1);
        } else {
            pooflib.set_kill(0);
        }
    }
  #endif

    if (KILL_SWITCH && my_events != NULL) {
        events.free_event_list(my_events);
        my_events = NULL;
    }

    return my_events;
}


/* get current readings for anything connected to an Analog pin.  */
void readAnalog() { 

    int reading;
    for (int x = 0; x < adc_count; x++) {
        reading = analogRead(allAnalog[x]);
        if (reading != adc_reading[x]) {
           debug.MsgPart("Threshold reading: ");
           debug.MsgPart(reading);
           debug.MsgPart("  ");
           debug.Msg(allAnalog[x]);           
           adc_reading[x] = reading;
        }
    }
}





/* required, at least on some installations, to eliminate "impure_ptr" compilation error for ESP 32 DEV board */
#ifdef ESP32

void *operator new(size_t size)       {  return malloc(size);  }
void *operator new[](size_t size)     {  return malloc(size);  }
void  operator delete(void * ptr)     {  free(ptr);  }
void  operator delete[](void * ptr)   {  free(ptr);  }
extern "C" void __cxa_pure_virtual    (void) __attribute__ ((__noreturn__));
extern "C" void __cxa_deleted_virtual (void) __attribute__ ((__noreturn__));
void __cxa_pure_virtual(void)         {  abort(); }
void __cxa_deleted_virtual(void)      {  abort(); }

#endif
