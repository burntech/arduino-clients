/*
  Carnival_network.cpp - Carnival library
  Copyright 2016 Neil Verplank.  All right reserved.
*/


// include main library descriptions here as needed.
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
boolean       WIFI_OVERRIDE     = false;    // don't allow poofing without server connection by default
long          last_checked     = 0L;       // last time we checked wifi connection

extern Carnival_debug debug;
extern Carnival_leds leds;

Carnival_network::Carnival_network()
{
}



/*================= WIFI FUNCTIONS =====================*/


void Carnival_network::start(String who, bool dbug) {
    WHO    = who;
    DEBUG  = dbug;

    /* 
      It would seem this one line fixes all ESP8266 networking problems.
    */
#ifdef ESP8266
    wifi_set_sleep_type(NONE_SLEEP_T);
#endif
}

void Carnival_network::connectWifi(){

  debug.MsgPart("Attempting to connect to SSID: ");
  debug.Msg(ssid);

  if ( WiFi.status() != WL_CONNECTED) {

        WiFi.mode(WIFI_STA);
        status = WiFi.begin(ssid, pass);  // connect to network
        // flash and wait 10 seconds for connection:
        leds.flashLED(leds.bluePin(), 10, 166, false); // flash slowly (non-blocking) to show attempt
    }
    if (DEBUG) {
       if (status==WL_CONNECTED) { printWifiStatus(); }
    }
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
          client.setNoDelay(1); // turn off Nagle's algorithm (is this the right place?)
          leds.setLED(leds.bluePin(),1);
          // (re) announce who we are to the server
          client.println(WHO); // Tell server which game
          if (output) {
            debug.MsgPart(WHO);
            debug.Msg(": ONLINE");
          }
       } else {
         if (!client) {debug.MsgPart("No client - ");}
         if (!client.connected()) {debug.MsgPart("Not connected - ");}
         debug.MsgPart("couldn't connect to host:port :: ");
         debug.MsgPart(HOST);
         debug.MsgPart(":");
         debug.Msg(PORT);
       }
    } else { con =1; }
    if (!con) {
      
        debug.Msg("Socket connection failed"); 
        leds.setLED(leds.redPin(),0);
        leds.flashLED(leds.bluePin(), 5, 25, false);   // flash 5 times to indicate failure
        delay(1000);            // wait a little before trying again
        return 0;
        
    } else {
        return 1;
    }
}



void Carnival_network::confirmConnect() {

    if (!WIFI_OVERRIDE) {

        int time = millis();

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




void Carnival_network::keepAlive() {
  long cTime = millis();
  if ((cTime - idleTime) >= maxIdle) {
      callServer(KEEPALIVE);
      idleTime = cTime;
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




char* Carnival_network::readMsg() {

    
    // process any incoming messages
    int CA = 0;
    char incoming[1024];
    memset(incoming, NULL, 1024);

    if (looksgood && !WIFI_OVERRIDE) {

        CA =  client.available();
        if (CA) {
      
          // there's an incoming message - read one and loop (ignore non-phrases)
          // read incoming stream a phrase at a time.
          int x = 0;
          int cpst;
          char rcvd = client.read();

          // read until start of phrase (clear any cruft)
          while (rcvd != '$' && x < CA) {
              rcvd = client.read();
              x++;
          }

          if (rcvd == '$') { /* start of phrase */
              int n = 0;

              while (rcvd = client.read()) {
                  if (rcvd == '%') { /* end of phrase - do something */

                      debug.Msg(incoming);
                      break; // phrase read, end loop

                  } else { // not the end of the phrase
                      incoming[n] = rcvd;
                      n++;
                  }
              } // end while client.read

              leds.blinkBlue(2, 15, 1); // show communication, non-blocking

          } // end if $ beginning of phrase

          idleTime = millis();  // reset idle counter
      
      } // end if  client available
  
    } // end client looks good

    return incoming;
  
}




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
    out += ":";

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
 
    if (!OK()) { return; }   // don't send message if no connection
 
    String out = who;
    out += ":";
    out += message;

    client.println(out);
    leds.blinkBlue(3, 10, 1); // show communication
    debug.MsgPart(who);
    debug.MsgPart(" called server with:");
    debug.Msg(out);
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
  delay(100);
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode
  gpio_pin_wakeup_enable(GPIO_ID_PIN(wakeButton), GPIO_PIN_INTR_LOLEVEL); //set the interrupt to look for LOW pulses on pin. 
  wifi_fpm_open();
  wifi_fpm_set_wakeup_cb(callback); // set wakeup callback function
  wifi_fpm_do_sleep(0xFFFFFFF);     // go to sleep.....
  delay(100);                       // allow sleep to settle
                                    
  connectWifi();                    // upon wake-up, resume wifi
  reconnect(1);                     // re-connect socket
}
#endif

