/*
  Carnival_network.cpp - Carnival library
  Copyright 2016-2020 Neil Verplank.  All right reserved.

*/


#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
 #include "WConstants.h"
#endif

#ifdef ESP8266
  #include      <ESP8266WiFi.h>
#else // ESP32
  #include      <WiFi.h>
#endif

#include      <Carnival_PW.h>
#include      <Carnival_network.h>
#include      <Carnival_debug.h>
#include      <Carnival_leds.h>
#include      <Carnival_poof.h>
#include      <Carnival_events.h>

extern        Carnival_debug debug;
extern        Carnival_leds leds;
extern        Carnival_events events;
extern        Carnival_poof pooflib;

extern        int POOFING_ALLOWED;
extern        int killSwitch;

#ifdef ESP8266
  extern "C" {
    #include "gpio.h"
  }
  extern "C" {
     #include "user_interface.h"
  }
#endif



WiFiClient  client;
int         status          = WL_IDLE_STATUS;
String      WHO             = "";
boolean     DEBUG           = 1;



//------------ FLAGS, COUNTERS, VARIABLES  -- DONT CHANGE


const String  KEEPALIVE        = "KA";     // signal sent to server to maintain socket
int           looksgood        = 0;        // still connected (0 is not)
long          onlineMsg        = 60000L;   // ms to delay between "still online" flashes
long          idleTime         = 0L;       // current idle time
long          maxIdle          = 300000L;  // milliseconds network idle before keep alive signal (5 minutes)
boolean       WIFI_OVERRIDE    = false;    // don't allow poofing without server connection by default
long          last_checked     = 0L;       // last time we checked wifi connection
int           reconnections    = 0;        // how many times we've tried



// pointer list for setup strings
typedef struct _list_t_ {
    char   *setting;                    // String to send at re-hup
    struct _list_t_ *next;              // next on the list
} list_t;

list_t       *my_settings      = NULL;  // list of strings that are settings


Carnival_network::Carnival_network()
{
}



/*================= WIFI FUNCTIONS =====================*/


void Carnival_network::start(String who, bool dbug) {
    WHO    = who;
    DEBUG  = dbug;

    /* 
      It would seem this one line fixes all ESP8266 networking problems.
      Note that technically this is not the default mode (which is to sleep
      and wake up), and that in theory this could cause overheating.

      See https://github.com/esp8266/Arduino/issues/2330
    */
#ifdef ESP8266
    wifi_set_sleep_type(NONE_SLEEP_T);
#endif

    connectWifi();

    delay(2000); // wait a bit after first attempt

}


void Carnival_network::connectWifi(){

    if (WIFI_OVERRIDE) return;

    debug.MsgPart("Attempting to connect to SSID: ");
    debug.Msg(ssid);

    if ( WiFi.status() != WL_CONNECTED) {

        WiFi.mode(WIFI_STA);
        status = WiFi.begin(ssid, pass);  // connect to network
        // flash and wait 10 seconds for connection:
//        leds.flashLED(leds.bluePin(), 10, 166, false); // flash slowly (non-blocking) to show attempt
    }

    if (DEBUG && status==WL_CONNECTED) { printWifiStatus(); }
    
}



int Carnival_network::reconnect(bool output) {

    // wait for a new client:
    // Attempt a connection with base unit
   if (output) { debug.Msg("Confirming connection..."); }

    int con=0;
    // check "connected() less frequently
    if (!client || !client.connected()) {

        con = client.connect(HOST,PORT);
        if (con) {
          looksgood = 1;
          client.setNoDelay(true); // turn off Nagle's algorithm (is this the right place?)
          leds.setLED(leds.bluePin(),1);
          // (re) announce who we are to the server
          client.println(WHO); // Tell server which game
          refreshSettings();   // re-initialize any control settings, just in case 
          if (output) {
            debug.MsgPart(WHO);
            debug.Msg(": ONLINE");
          }
          reconnections = 0;
       } else {
          reconnections++;
         #ifdef ESP32
           if (reconnections > 5) {
             // massive workaround - ESP only connects 50% of time.
             // see https://github.com/espressif/arduino-esp32/issues/234
             ESP.restart();
           }
         #endif
         if (!client) {debug.MsgPart("No client - ");}
         if (!client.connected()) {debug.MsgPart("Not connected - ");}
         debug.MsgPart("couldn't connect to host:port :: ");
         debug.MsgPart(HOST);
         debug.MsgPart(":");
         debug.MsgPart(PORT);
         debug.MsgPart(" tries:");
         debug.Msg(reconnections);
       }

    } else 
        con =1;


    if (!con) {
      
        debug.Msg("Socket connection failed"); 
        leds.setLED(leds.redPin(),0);
        leds.flashLED(leds.bluePin(), 5, 25, false);   // flash 5 times to indicate failure
        delay(2000);                                   // wait a little before trying again
    } 

    return con;
    
}



void Carnival_network::confirmConnect() {

    if (!WIFI_OVERRIDE) {

        long time = millis();

        // CONFIRM CONNECTION
        looksgood   = reconnect(0);

        if (time - last_checked >= onlineMsg) {
            looksgood = reconnect(1);
            if (looksgood) {
                printWifiStatus();
            }
            last_checked = time;
        }

        keepAlive();
    }
}

// Create a new setting
list_t *new_setting(char *str) {

    list_t *new_list;

    // allocate memory for new node
    if ((new_list  = (list_t *)malloc(sizeof(list_t))) == NULL) return NULL;

    // Populate data
    new_list->setting         = strdup(str);          // explicity copy original into memory 
    new_list->next            = NULL;

    return new_list;
}



// Join two lists
list_t *concat_lists(list_t *first_list, list_t *second_list) {

    list_t *current = NULL;

    if (!first_list)  return second_list;
    if (!second_list) return first_list;
    if (first_list == second_list) return first_list;

    // get the last element on the list
    for (current=first_list; current->next !=NULL; current=current->next);

    current->next = second_list;

    return first_list;
}



// add a setting (like DNS or CC) that we want to retain and refresh
void Carnival_network::addSetting(char *str) {

    list_t *setting;
    setting = new_setting(str);
    my_settings = concat_lists(my_settings, setting);
    callServer(str);

}



// go through list and re-broadcast all our settings (if any)
void Carnival_network::refreshSettings() {

    if (!my_settings) return;

    list_t *current = NULL;
    for (current=my_settings; current !=NULL; current=current->next) 
        callServer(current->setting);

}





void Carnival_network::keepAlive() {

  long cTime = millis();
  if ((cTime - idleTime) >= maxIdle) {
      callServer(KEEPALIVE);  // Send "null" keep alive message every 5 minutes
      refreshSettings();      // Re-broadcast any settings
      idleTime = cTime;       // Restart our idle time
      debug.Msg("sending keep alive");
  }

}






boolean Carnival_network::OK() {
    if (looksgood || WIFI_OVERRIDE) { return true; }
    return false;
}




void Carnival_network::set_override(boolean wf_over_ride) {
    WIFI_OVERRIDE = wf_over_ride;
}

boolean Carnival_network::get_override() {
    return WIFI_OVERRIDE;
}



/* check wireless override (hold down button #1 while booting) */
void Carnival_network::check_override(int test){

    if (!get_override()) {
        // if it's the first button and it's pushed set override = true
        if (test == LOW) {
            // if the button is held down when booting, we're in wifi override mode.
            set_override(true);
            debug.Msg("Wireless override - engaged!");
        }
    }  

}



void Carnival_network::printWifiStatus() {

     if (!DEBUG) { return; }
     
     debug.MsgPart("\nWireless Connected to SSID: ");
     debug.Msg(WiFi.SSID());

     IPAddress ip = WiFi.localIP();
     Serial.print("IP Address: ");
     Serial.println(ip);
    
     long rssi = WiFi.RSSI();
     debug.MsgPart("signal strength (RSSI):");
     debug.MsgPart(rssi);
     debug.Msg(" dBm");
    
}




///////  MESSAGE PROCESSING  

int Carnival_network::processIncoming(msg_t **new_messages, event_t **my_events) {

    int msgs_found = 0;

    // lists of messages and events we're about to find
    msg_t   *my_msgs     = NULL;
    msg_t   *new_msgs    = NULL;
    msg_t   *new_ev_msgs = NULL;
    event_t *new_evts    = NULL;



    /*  
      readBuffer returns 1 if any new messages received, 2 if we're returning non-system
      messages inside *new_msgs.  System messages include poofing, control/broadcast settings,
      kill, and keep alive (which are all handled inside processMsg).
    */
    int new_msg    = readBuffer(&new_msgs, &new_evts);             // read any incoming wireless messages
    *my_events     = events.concat_events(*my_events,new_evts);    // create list of events
    *new_messages  = events.concat_msgs(*new_messages,new_msgs);   // create list of messages

    /*
       look to see if there are any outstandinng events (timed messages) that should be executed now.
     */
    int new_evs    = 0;
    *my_events     = events.check_events(&new_evs, *my_events, &new_ev_msgs);  // check existing events for messages
    *new_messages  = events.concat_msgs(*new_messages,new_ev_msgs); // append to message list

    if (new_msg || new_evs) 
        msgs_found = 1;
    
    checkKill();                // check kill switch / kill state


    return msgs_found;
}





/* create a new message structure and populate it */
msg_t *new_message(char *str) {

    msg_t *new_msg;

    // allocate memory for new node
    if ((new_msg  = (msg_t *)malloc(sizeof(msg_t))) == NULL) return NULL;

    // Populate data
    new_msg->msg             = strdup(str);          // explicity copy original into memory 
    new_msg->next            = NULL;

    return new_msg;
}


/* returns 1 if poofing message came in, 2 if non-system message came in */
int Carnival_network::processMsg(char *incoming, msg_t **new_msg, event_t **new_events) {

    int msgs_found = 0;

    if (strcmp(incoming, "kill") == 0) {        
      // external kill signal, set flag, stop poofing
        if (POOFING_ALLOWED) {
            pooflib.set_kill_remote(1);
            pooflib.stopPoof();
       }

    } else if (strcmp(incoming, "free") == 0) {
        if (POOFING_ALLOWED) 
            pooflib.set_kill_remote(0);
    } else if (incoming[0] == 'p' && (incoming[1] == '>' || (incoming[1] >= '0' && incoming[1] <= '9'))) {
        msgs_found = 1;

        if (POOFING_ALLOWED) 
            *new_events = pooflib.doPoof(incoming);
      
    } else {
       // else some other message.....

        // create new message struct here.  assumption is "incoming" is being processed away above,
        // or turns into a future message (new event),
        // or is a NEW non-system message, so create this:
        *new_msg    = new_message(incoming);
        msgs_found = 2;
    }

    return msgs_found;
  
} // end processMsg





/*
  If networking is on, look for a readable socket.  If so, 
  read the available content into a buffer.  Parse the buffer
  looking for messages, currently delineated by a starting '$' and 
  an ending '%'.  Strip start and end characters to extract message,
  then process each message in turn.  System messages (poofing, control
  settings, kill) will be exectued (processMsg returns 1).  And non-system
  messages that result will be passed back as a list of messages to be
  handed back to the caller in the my_msgs and my_events lists.  Events
  are basically fuuture messages.

  new_msgs are msgs I find.
  my_events may already contain events....

*/
int Carnival_network::readBuffer(msg_t **new_messages, event_t **my_events) {

    int msgs_found    = 0;    // non system messages found
    int cur_pos       = 0;    // current position in buffer
 
    // process any incoming messages

    if (looksgood && !WIFI_OVERRIDE) {

      // find out size and creaete buffer, if anything available

      int   CA        =  client.available();
      char *line      = (char*)malloc(CA+2);
      char *incoming  = (char*)malloc(CA+2);
      memset(incoming,'\0',CA+1);             // clear buffer 

      // read the whole buffer into memory
      while (cur_pos < CA) {
          char rcvd;
          rcvd = client.read();
          incoming[cur_pos] = rcvd;
          cur_pos++;
      }

      // go through buffer and find messages 

      cur_pos = 0;
      
      while (cur_pos < CA) {

          memset(line,0,CA+2); // zero out line buffer 

          // there's an incoming message - read one and loop (ignore non-phrases)
          // read incoming stream a phrase at a time.

          // read until start of phrase (clear any cruft)
          while (incoming[cur_pos] != '$' && cur_pos < CA) {
              cur_pos++;
          }

          // we strip leading '$' and trailing '%', and ignore anything
          // that doesn't fall between those characters.

          // cur_pos points to beginning of line
          if (incoming[cur_pos] == '$') { /* start of phrase */

              cur_pos++;
              int cur_begin  = cur_pos;  

              // find end of phrase or buffer
              while (incoming[cur_pos] !='%' && cur_pos <= CA) {
                  cur_pos++;
              }

              int y;
              for (y=0; y<cur_pos-cur_begin; y++)
                  line[y] = incoming[cur_begin+y];

              line[y+1]='\0';

              // output all incoming instructions
              debug.MsgPart("->");
              debug.Msg(line);
   
              msg_t   *new_msg     = NULL;
              event_t *new_evs     = NULL;

              msgs_found = processMsg(line, &new_msg, &new_evs);

              // drop any dangling system messages here, only add non-system msgs to the list
              // 1 means system msgs found/processed, 2 means client msgs avail
              if (msgs_found == 2) 
                  *new_messages = events.concat_msgs(*new_messages, new_msg);

              *my_events  = events.concat_events(*my_events,new_evs);

              cur_pos++;             // advance one position

          } // end if $ beginning of phrase
          leds.blinkBlue(2, 15, 1); // show communication, non-blocking
          idleTime = millis();  // reset idle counter
      
      } // end while CA 
      free(incoming);
      free(line);
    } // end client looks good

    return msgs_found;
  
} // end readBuffer




/* 
    Send a well-formatted message to the server.
    Looks like "WHOMAI:some message:[optional data string][:][second optional data string]".

    Here, we over override the basic function to allow the passing of different types
    types of data, and/or optional data.

*/
void Carnival_network::callServer(String message){
    callServer(WHO,message);
}

void Carnival_network::callServer(int message, int optdata){
  
    String out = "";
    out += message;
    out += ":";
    out += optdata;

    callServer(WHO,out);
}


void Carnival_network::callServer(String who, int message){
  
    String out = "";
    out += message;

    callServer(who,out);
}   

void Carnival_network::callServer(String who, int message, int optdata){
  
    String out = "";
    out += message;
    out += ":";
    out += optdata;
    
    callServer(who, out);
}   

void Carnival_network::callServer(String who, String message){
 
    if (!OK())  
        return;    // don't send message if no connection
 
    String out = who;
    out += ":";
    out += message;

    client.println(out);
    //client.flush();
    leds.blinkBlue(3, 10, 1); // show communication
    debug.MsgPart(who);
    debug.MsgPart(" called server with:");
    debug.Msg(out);
}   







/* check kill button, kill state, and any event list */
void Carnival_network::checkKill() {
        
    if (killSwitch) {  // if there's a local kill switch, check it.      
        int button_value = digitalRead(killSwitch);
        if (button_value == LOW && !pooflib.get_kill()) {         // state wrong, button *just* pressed
            pooflib.set_kill(1);
            pooflib.stopPoof();
            callServer(WHO,"kill::");
        } else if (button_value == HIGH && pooflib.get_kill()) {  // state wrong, button *just* released
            pooflib.set_kill(0);
            callServer(WHO,"free::");
        }
    }
}






// experimental sleep routines, 8266 only

#ifdef ESP8266
void callback() {
    debug.Msg("Woke up from sleep");
}

void Carnival_network::sleepNow(int wakeButton) {
  pinMode(wakeButton, INPUT_PULLUP);
  debug.Msg("going to light sleep...");
  leds.setLED(leds.bluePin(),0);
  delay(100); // short delay before going o sleep for 8266
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode
  gpio_pin_wakeup_enable(GPIO_ID_PIN(wakeButton), GPIO_PIN_INTR_LOLEVEL); //set the interrupt to look for LOW pulses on pin. 
  wifi_fpm_open();
  wifi_fpm_set_wakeup_cb(callback); // set wakeup callback function
  wifi_fpm_do_sleep(0xFFFFFFF);     // go to sleep.....
  delay(100);                       // allow sleep to settle for 8266
                                    
  connectWifi();                    // upon wake-up, resume wifi
  reconnect(1);                     // re-connect socket
}
#endif

