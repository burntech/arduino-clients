/*

  Carnival_poof.cpp - Carnival library
  Copyright 2016-2020 Neil Verplank.  All right reserved.

  Used to hold 1 or more pin+solenoid associations, and maintain state
  for each of those relays (which we are assuming is hooked up to a 
  flame effect).  One key element is a time limit (default is 8 seconds), 
  after which a given poofer will be turned off.  This is a safety 
  mechanism, to insure a poofer always goes off, even if the off signal
  didn't come through.

  In addition to "poofing all" the relays (the general default), 
  each relay is accesible individually.  We assume incoming requests
  will be for an ordinal number (e.g. from 1 to 12), which we internally
  map to a particular pin.

  Various timed sequences are included as options.

  NOTE that only poofAll should turn a solenoid on or off, and be
  called with a particular solenoid if/as needed, so that all ON/OFF
  signals are routed through poofAll (which also checks for a local
  or remote kill signal).
  
*/

// include main library descriptions here as needed.
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif

#include "Carnival_poof.h"
#include "Carnival_debug.h"
#include "Carnival_network.h"
#include "Carnival_leds.h"

#define  MAX_SOLS             20      // Maximum number of solenoids, arbitrary, but probably pin-limited.


const    boolean RELAY_ON   = 0;       // opto-isolated arrays are active LOW
const    boolean RELAY_OFF  = 1;

int      KILL_LOCAL         = 0;       // kill switch set?
int      KILL_REMOTE        = 0;       // remote kill sent?
int      POOFING_ALLOWED    = 0;       // whether we allow poofing for this sketch

int      solenoidCount      = 0;       // will be the size of the array
long     lastChecked        = 0L;

int      allSolenoids[MAX_SOLS];       // relay for all the pins that are solenoids
int      allSolArray[MAX_SOLS];        // map solenoid pins to their positions
int      maxPoof[MAX_SOLS];            // counter while poofing, for timing out
int      pausePoofing[MAX_SOLS];       // hit max poof limit, or pause between poofs
boolean  poofing[MAX_SOLS];            // poofing state (0 is off)

int      maxPoofLimit        = 6000;   // milliseconds to limit poof, converted to loop count (can pass in new limit at creation)
int      poofDelay           = 2500;   // ms to delay after poofLimit expires to allow poofing again

extern   Carnival_debug debug;
extern   Carnival_network network;
extern   Carnival_leds leds;
extern   Carnival_events events;




/*================= CREATION AND SETUP =================*/

Carnival_poof::Carnival_poof()
{
    // initialize this instance's variables
    POOFING_ALLOWED=1;
}

Carnival_poof::Carnival_poof(long poof_limit, long poof_delay)
{
    // initialize this instance's variables
    POOFING_ALLOWED=1;
    maxPoofLimit = poof_limit;
    poofDelay    = poof_delay;
}

/* initalize our solenoid array with pins, set definite size */
void Carnival_poof::setSolenoids(int aS[], int size) {

    // for sanity's sake.  should never be true, or handled differntly
    if (size > MAX_SOLS)
        size = MAX_SOLS;

    solenoidCount = size;

    int i;
    for (i=0; i<size; i++) {
        allSolArray[i]  = i+1;    // we create a "map", 1..size corresponding to "all" solenoids
        allSolenoids[i] = aS[i];
        maxPoof[i]      = 0;
        poofing[i]      = 0;
        pausePoofing[i] = 0;
        pinMode(allSolenoids[i], OUTPUT);
        digitalWrite(allSolenoids[i], RELAY_OFF);
    }
}



void Carnival_poof::set_kill(int state) {
    KILL_LOCAL = state;
}


int Carnival_poof::get_kill() {
    return KILL_LOCAL;
}


void Carnival_poof::set_kill_remote(int state) {
    KILL_REMOTE = state;
}


int Carnival_poof::get_kill_remote() {
    return KILL_REMOTE;
}



/*================= POOF FUNCTIONS =====================*/


/* return state for a given solenoids */
boolean Carnival_poof::isPoofing(int whichSolenoid) {
    return poofing[whichSolenoid];
}


/* set state to on for all solenoids */
void Carnival_poof::startPoof() {
    startPoof(allSolArray,solenoidCount);
}




/* set state to on for 1 or more solenoids.
   note that the incoming array is NOT the pins, but the "addresses" of the solenoids.  IOW, if there are three
   solenoids in the allSolenoids array, they would be 1, 2, and 3 respectively.  So an array [1,3] would have a 
   size 2, and would turn on the 1st and 3rd solenoids in the allSolenoids array.

   We check each solenoid to make sure it hasn't exceeded pooflimit.
 */
void Carnival_poof::startPoof(int sols[], int size) {

    int new_poof = 0;
    int new_sols[size];

    // go through all incoming solenoid "on" requests, see if on
    // already on, if timer hasn't elapsed, then turn on
    for (int i=0; i<size; i++) {
        if (pausePoofing[sols[i]-1] == 0) {
            if (!poofing[sols[i]-1] && maxPoof[sols[i]-1] <= maxPoofLimit) { // if we're not poofing nor at the limit
                poofing[sols[i]-1] = 1;  // poofing started on this solenoid
                maxPoof[sols[i]-1] = 0;  // start counter for this solenoid
                new_sols[new_poof] = sols[i];  // actually turning on this solenoid #
                new_poof++; // at least one of requested poofers actually turning on
            }
        }
    }

    if (new_poof) {
        poofAll(true, new_sols, new_poof);
        debug.MsgPart("Poofing started:");
        debug.Msg(millis());
        leds.setRedLED(1);
    }
}




/* set state to off for all solenoids */
void Carnival_poof::stopPoof() {
    stopPoof(allSolArray,solenoidCount);
}




/* set state to off for 1 or more solenoids. This array works like the one in startPoof above, see comments. 
   same array rules apply as startPoof above.
*/
void Carnival_poof::stopPoof(int sols[], int size) {

    int new_stops = 0;
    int new_sols[size];

    for (int i=0; i<size; i++) {
        if (poofing[sols[i]-1]) {
            poofing[sols[i]-1] = 0;
            new_sols[new_stops] = sols[i];
            new_stops++;
            maxPoof[sols[i]-1] = 0;
        }
    }

    if (new_stops) {
        poofAll(false, new_sols, new_stops);
        debug.MsgPart("Poofing stopped:");
        debug.Msg(millis());
        leds.setRedLED(0);
    }
}




/* check state on every loop   */
void Carnival_poof::checkPoofing() {

    int new_stops = 0;
    int new_sols[solenoidCount];

    int time = millis() - lastChecked;

    int i;
    for (i=0; i<solenoidCount; i++) {
        if (pausePoofing[i] > 0) {                          // poofing paused, decrement pause counter
            pausePoofing[i] = pausePoofing[i] - time;
            if (pausePoofing[i] < 0) pausePoofing[i] = 0;   // we test for return to zero, so must not be negative.
        } else if (poofing[i] == 1) {                       // poofing, increment max timer
            maxPoof[i] = maxPoof[i] + time;
        }

        if (poofing[i] && !network.OK()) {                  // stop poofing if no network connection (not if WIFI override)
            new_sols[new_stops] = i+1;
            new_stops++;
        } else if (maxPoof[i] > maxPoofLimit) {             // pause poofing for 'poofDelay' if we hit max pooflimit
            pausePoofing[i] = poofDelay;
            new_sols[new_stops] = i+1;
            new_stops++;
        }
    }
    if (new_stops) {
        stopPoof(new_sols,new_stops);
    }

    lastChecked = millis();
}




/* act on poofing message */
event_t *Carnival_poof::doPoof(char *incoming) {

    /* 
      p0 - stop poof all
      p1 - poof all
      p2 - start poofstorm
      p1>3,5,7 = start poof on relay number 3, 5, and 7

      p>>nnn where 'nnn' is a number from 0 to 65,536
        In this case, it's (up to ) a 16-bit sequence, where the low-order
        bit is the first relay, and the nth bit would the nth relay,
        and that bit being set means poof-on.

        I.e., 100110001110 means 12th relay on, 11, 10 off, 9-8 on...
        relay 1 off.  Which is 2446 decimal.


        NOTE - making relays 24, most that could be in two incoming binary bytes....
    */
    int num_relays = 0;
    int num_relays_off = 0;
    int tot_chars  = strlen(incoming);
    int relays[24], relays_off[24];
    int binary;

    if (tot_chars > 2 and incoming[2]=='>') {
        num_relays = get_relays(incoming, relays, &binary, relays_off, &num_relays_off);
    }

    event_t *new_events;
    new_events = NULL;

    if (incoming[1] == '1') {          // p1, start / keep poofing ALL

        if (num_relays) 
            startPoof(relays, num_relays);
        else 
            startPoof(); 

    } else if (incoming[1] == '0') {   // p0, stop poofing ALL

        if (num_relays)
            stopPoof(relays, num_relays);
        else 
            stopPoof();

    } else if (incoming[1] == '2') {   // p2, poofstorm if allowed and still poofing

        if (pausePoofing[0] == 0) 
            long int storm_length = poofStorm(new_events);

    } else if (binary == 1) {

        if (num_relays) 
            startPoof(relays, num_relays);

        if (num_relays_off) 
            stopPoof(relays_off, num_relays_off);
    } 

    return new_events;
}




/* set state for an array of poofers  */
void Carnival_poof::poofAll(boolean state) {
    poofAll(state, allSolArray, solenoidCount);
}




/* Actually do the poofing, turning solenoids on / off unless we've
   been locally (with a switch) or remotely (via wireless) been killed.  */
void Carnival_poof::poofAll(boolean state, int sols[], int size) {

    for (int y = 0; y < size; y++) {
        if(state == true){
            // keep KILL logic simple, as this is the only place we actually
            // turn on a relay....
            if (!KILL_LOCAL && !KILL_REMOTE) {
                digitalWrite(allSolenoids[sols[y]-1], RELAY_ON);
            } else {
                debug.Msg("poofing is currently killed....");
            }
        } else {
            digitalWrite(allSolenoids[sols[y]-1], RELAY_OFF);
        }
    }
}


/* find an array of relays if specified in string */
int Carnival_poof::get_relays(char *incoming, int relays[], int *binary, int relays_off[], int *num_relays_off) {
    /*
       string looks like: "p1>1,2,6,7"
       so we skip 3 characters, then find #'s between commas,
       fill an array via a pointer, and return its size.

       Some error-checking would likely be wise.

       p>>nnnn
    */


    int num_relays = 0;
    int NUMLEN     = strlen(incoming);
    int cur_let    = 3;

    if (incoming[1] == '>') {
        *binary = 1;
    }

    while (cur_let < NUMLEN) {

        int cur_num     = 0; // number of "on" relays.  could be a comma separated list
        int cur_off_num = 0;
        int accum       = 0;

        while (incoming[cur_let+cur_num] != ',' && cur_let+cur_num < NUMLEN) {
            accum = 10*accum;
            accum += (incoming[cur_num+cur_let]-0x30);
            cur_num++;
        }

        if (*binary == 1) {
            cur_num = 0;    
            int count = solenoidCount;
            while (count > 0) {

                int byte = pow(2,count-1);
                if (accum >= byte) {
                    relays[cur_num] = count;
                    accum           = accum - byte;
                    cur_num++;
                } else {
                    relays_off[cur_off_num] = count;
                    cur_off_num++;
                }
                count--;
            }

            *num_relays_off = cur_off_num;
            num_relays      = cur_num;
            cur_let = NUMLEN;

        } else {

            // should range from 1..solenoidCount, no more
            if (accum && accum<=solenoidCount) {
                relays[num_relays] = accum;
                num_relays++;
            }
            cur_let += cur_num+1;
        }
    }

    return num_relays;
}




//////////  TIMED SEQUENCES //////////

/* 
   various routines for generating timed sequences.  note that
   most of them expect a pointer to the current time, and modify
   that time internally (so that the end time is available upon
   return).
*/

/* Crazy Poofer display */
long int Carnival_poof::poofStorm(event_t *poofstorm) {

    event_t *new_event;

    long curtime     = millis()+1;
    long time_offset = 0L;
    int  short_delay = 50;
    int  delay_time  = 200;
    char *str;

    debug.Msg("POOFSTORM");

    // all on
    str = "p1";
    poofstorm    = events.new_event(str,time_offset,0,curtime);
    time_offset += delay_time;

    // all off
    str = "p0";
    new_event = events.new_event(str,time_offset,0,curtime);
    poofstorm = events.concat_events(poofstorm,new_event);
    time_offset += delay_time;
  
    // all on, left to right
    long length = poof_across(true, delay_time, curtime+time_offset, 1, new_event);
    poofstorm = events.concat_events(poofstorm,new_event);
    time_offset += length;
    time_offset += delay_time;

    // all off
    str = "p0";
    new_event = events.new_event(str,time_offset,0,curtime);
    poofstorm = events.concat_events(poofstorm,new_event);
    time_offset += delay_time;
  
    // all on, right to left
    length = poof_across(true, delay_time, curtime+time_offset, -1, new_event);
    poofstorm = events.concat_events(poofstorm,new_event);
    time_offset += length;
    time_offset += delay_time;

    // all off
    str = "p0";
    new_event = events.new_event(str,time_offset,0,curtime);
    poofstorm = events.concat_events(poofstorm,new_event);
    time_offset += delay_time;
  

    // all on 4 x delay, all off
    str = "p1";
    new_event = events.new_event(str,time_offset,0,curtime);
    poofstorm = events.concat_events(poofstorm,new_event);
    time_offset += 4*delay_time;
    str = "p0";
    new_event = events.new_event(str,time_offset,0,curtime);
    poofstorm = events.concat_events(poofstorm,new_event);

    return time_offset;
}



/* poof right to left (or left to right) */
long int Carnival_poof::poof_across(boolean state, int rate, long int curtime, int dir, event_t *poof_right) {

    event_t *new_event, *cur_ev;
    cur_ev     = NULL;
    new_event  = NULL;
    long int time_offset = 0;

    int st   = 1;
    int y    = 1;
    int inc  = 1;

    debug.Msg("POOF ACROSS");

    if (dir == -1) {
        y  = solenoidCount;
        inc = -1;
    } 

    char  out[5] = {'p','0','>',0,0};
    if (state == true)
        out[1] = '1';
  
    while (st <= solenoidCount) {
        if (y<10) {
            out[3] = (char)(y+0x30);
            out[4] = NULL;
        } else {
            int tens = y / 10;
            int ones = y%10;
            out[3] = (char)(tens+0x30);
            out[4] = (char)(ones+0x30);
        }
        out[5] = NULL;
        new_event = events.new_event(out,time_offset,0,curtime);
        time_offset += rate;

        if (!poof_right) {
            poof_right = new_event;
            cur_ev = new_event;
        } else {
            cur_ev->next = new_event;
            cur_ev = new_event;
        }
        st++;
        y += inc;
    }
    return time_offset;
}




/*  several short blasts and a big poof */
long int Carnival_poof::gunIt(long int curtime, event_t *gunit){

    event_t  *new_event, *cur_ev;
    new_event  = NULL;
    cur_ev     = NULL;
    long int time_offset = 0;

    int  loops       = 4;   // the last poof is long
    int  short_delay = 50;
    int  delay_time  = 100;
    int  long_delay  = 1000;
    char *str;

    debug.Msg("GUN IT!");

    for(int i = 0; i <= loops; i++){
        // all on
        str = "p1";
        new_event = events.new_event(str,time_offset,0,curtime);
        if (!cur_ev) {
            cur_ev = new_event;
            gunit  = new_event;
        } else {
            cur_ev->next = new_event;
            cur_ev       = new_event;
        }
        if (i==loops) 
            time_offset += long_delay;
        else
            time_offset += delay_time;

        // all off
        str = "p0";
        new_event = events.new_event(str,time_offset,0,curtime);
        cur_ev->next = new_event;
        cur_ev       = new_event;
        time_offset += short_delay;
    }

    return time_offset;
}




/* poof even (or odd) in sequential order */
long int Carnival_poof::poof_alt(int rate, long int curtime, int dir, event_t *poofs) { 

    event_t *new_ev, *cur_ev;
    new_ev  = NULL;
    cur_ev  = NULL;
    long int time_offset = 0;
  
    char *str;
    char  out[5] = {'p','0','>',0,0};

    for (int y = 1; y <= solenoidCount; y++) {
        int dopoof = 0;
        if (dir == -1) {
            if(y != 0 && y%2 != 0 )  dopoof = 1; // odds
        } else { 
            if (y == 0 || y%2 == 0 ) dopoof = 1; // evens
        }

        if (dopoof) {
            out[1] = '1';
            if (y<10) {
                out[3] = (char)(y+0x30);
                out[4] = NULL;
            } else {
                int tens = y / 10;
                int ones = y%10;
                out[3] = (char)(tens+0x30);
                out[4] = (char)(ones+0x30);
            }
            out[5] = NULL;
            new_ev = events.new_event(out,time_offset,0,curtime);
            if (!cur_ev) {
                cur_ev = new_ev;
                poofs  = new_ev;
            } else {
                cur_ev->next = new_ev;
                cur_ev       = new_ev;
            }
            time_offset+= rate;
   
            out[1] = '0';
            new_ev = events.new_event(out,time_offset,0,curtime);
            cur_ev->next = new_ev;
            cur_ev       = new_ev;
        }
    }

    return time_offset;
}




/* start slow, go faster and faster */
long int Carnival_poof::poofChooChoo(int start, int decrement, int rounds, long int curtime, event_t *poofs){ 

// HEY!! START CAN GO TO 0 OR BELOW.......

    event_t *new_event;
    new_event = NULL;
    char *str;
    int init = start;

    long int time_offset = 0;

    debug.Msg("Poof Choo Choo!");

    for (int i = 0; i <= rounds; i++) {
        time_offset += Carnival_poof::poof_alt(start, curtime+time_offset, -1,new_event);
        poofs  = events.concat_events(poofs,new_event);
        if (start > decrement) // sanity check
            start -= decrement;
        time_offset += Carnival_poof::poof_alt(start, curtime+time_offset, 1,new_event);
        poofs  = events.concat_events(poofs,new_event);
        if (start > decrement)
           start -= decrement;
    }
    time_offset += start;
    str = "p1";
    new_event = events.new_event(str,time_offset,0, curtime);
    poofs  = events.concat_events(poofs,new_event);
    time_offset += init;
    str = "p0";
    new_event = events.new_event(str,time_offset,0, curtime);
    poofs  = events.concat_events(poofs,new_event);

    return time_offset;
}

