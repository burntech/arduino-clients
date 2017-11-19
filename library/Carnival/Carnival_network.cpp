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

#include      <ESP8266WiFi.h>
#include      <Carnival_PW.h>
#include      <Carnival_network.h>
#include      <Carnival_debug.h>
#include      <Carnival_leds.h>


extern "C" {
#include "gpio.h"
}
extern "C" {
#include "user_interface.h"
}

const boolean ON            = LOW;     // LED ON
const boolean OFF           = HIGH;    // LED OFF


WiFiClient  client;
int         status          = WL_IDLE_STATUS;
String      WHO             = "";
boolean     DEBUG           = 1;



//------------ FLAGS, COUNTERS, VARIABLES  -- DONT CHANGE


const String  KEEPALIVE        = "KA";     // signal sent to server to maintain socket
int           looksgood        = 0;        // still connected (0 is not)
long          stillOnline      = 0L;        // Check for connection timeout every 5 minutes
long          onlineMsg        = 60000L;    // ms to delay between "still online" flashes
long          idleTime         = 0L;       // current idle time
long          maxIdle          = 300000L;  // milliseconds network idle before keep alive signal (5 minutes)
boolean       wifiOverride     = false;    // don't allow poofing without server connection by default


extern Carnival_debug debug;
extern Carnival_leds leds;

Carnival_network::Carnival_network()
{
}



/*================= WIFI FUNCTIONS =====================*/


void Carnival_network::start(String who, bool dbug) {
    WHO    = who;
    DEBUG  = dbug;
    wifi_set_sleep_type(NONE_SLEEP_T);
}

void Carnival_network::connectWifi(){

  debug.MsgPart("Attempting to connect to SSID: ");
  debug.Msg(ssid);

  if ( WiFi.status() != WL_CONNECTED) {

// This may be required for lower power consumption.  unconfirmed.
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
         debug.Msg("couldn't connect to host:port");
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
  if (!wifiOverride) {
    // CONFIRM CONNECTION
    looksgood = reconnect(0);
    stillOnline++;
    if (stillOnline >= onlineMsg) {
      looksgood = reconnect(1);
      if (looksgood) {
        printWifiStatus();
        stillOnline = 0;
      }
    }
  }
}




void Carnival_network::keepAlive() {
  long cTime = millis();
  if ((cTime - idleTime) > maxIdle) {
      callServer(KEEPALIVE);
      idleTime = cTime;
      debug.Msg("sending keep alive");
  }
}




boolean Carnival_network::OK() {
    if ((looksgood && stillOnline) || wifiOverride) { return true; }
    return false;
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

    if (looksgood && !wifiOverride) {

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




// send a well-formatted message to the server
// looks like "WHOMAI:some message:[optional data]"

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
 
    if (!OK()) { return; }
 
    String out = who;
    out += ":";
    out += message;

    client.println(out);
    leds.blinkBlue(3, 10, 1); // show communication
    debug.MsgPart(who);
    debug.MsgPart(" called server with:");
    debug.Msg(out);
}   



// experimental sleep routines


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
  wifi_fpm_set_wakeup_cb(callback); //wakeup callback
  wifi_fpm_do_sleep(0xFFFFFFF); 
  delay(100); 
  connectWifi();
  reconnect(1);
}

