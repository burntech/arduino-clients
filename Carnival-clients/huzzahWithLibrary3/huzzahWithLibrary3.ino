/*

    Capn Nemo's Flaming Carnival
    
    Adafruit Huzzah / Esp8266 controller
    
    Rev 3.0 - Timing

    Sep 17, 2016 (c) neil verplank
    
    Use the Actuator to receive "poof" commands via a wireless network
    (specifically, xc-socket-server running on a Raspberry Pi 3 configured as an AP
    with a static IP).
    
    In a nutshell, hook up "BAT" and ground to a 5V power supply, hookup pin
    12 (or more - see allSolenoids) to the relay (which needs it's own power! 
    Huzzah can't drive the relay very well if at all).  IF you want to send high score
    or some other success to the server, connect pin 16 to ground (via a button,
    or have the game Arduino use one of its relays to short inputButton to ground
    momentarily).
    
    If all connectinos are good, the blue light is steady on.  When receiving a 
    communication, the blue LED should "sparkle".  If poofing, the red LED will be 
    steadily on. Once a minute, we check the network and socket connections, and blink
    blue 3 times if solid, or turn off the blue LED and start blinking the red LED 
    if no connection or no socket.

    
    As of version 3, the logic has changed - we receive one "poof on" signal, and
    subsequently a "poof off" signal, rather than the "keep alive" strategy of prior
    versions.  We have reduced the loop delay to 1 ms from 100ms.  We might be able
    to drop to 0, but various aspects of the logic would have to change.

    

    Just noticed this interesting feature:

    ESP.deepSleep(6000000, WAKE_RF_DEFAULT); // Sleep for 6 seconds, then wait for sensor change
    
*/

#include      <ESP8266WiFi.h>

#define       DEBUG         0       // 1 to print useful messages to the serial port

//------------ WHICH GAME, WHAT MODE, HOW HOOKED UP

#define       WHOAMI         "ENTRY"  // which game am I?
#define       REPORT         "pushed"   // what happened? (e.g. "high score!:725")
const boolean HASBUTTON      = 0;     // Does this unit have a button on
const int     inputButton    = 5;     // What pin is that button on?
int           allSolenoids[] = {12,13};  // What pins are connected to solenoids (Default is 12[,13,14,..])
const boolean POOFSTORM      = 1;     // Allow poofstorm?


//------------ NETWORK VARIABLES

WiFiClient  client;
const char    ssid[]         =  "CapnNemosCarnival";
const char    pass[]         =  "c6pNp3ss!";
#define       HOST           "192.168.100.1"
#define       PORT           5061


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




//------------ HARDWARE SPECIFICS (currently Adafruit Huzzah ESP8266)

#define       REDLED          0        // what pin?
#define       BLUELED         2        // what pin?
const boolean RELAY_ON      = 0;       // opto-isolated arrays are active LOW
const boolean RELAY_OFF     = 1;
const boolean ON            = LOW;     // LED ON
const boolean OFF           = HIGH;    // LED OFF


//------------ FLAGS, COUNTERS, VARIABLES

int         stillOnline      = 0;      // Check for connection timeout every 5 minutes
int         maxPoof          = 0;      // poofer timeout
int         stopPoofing      = 0;      // hit max poof limit
int         looksgood        = 0;      // still connected (0 is not)
boolean     poofing          = 0;      // poofing state (0 is off)
boolean     notified         = true;   // notify once the poofers are off
int         status           = WL_IDLE_STATUS;
int         solenoidCount    = sizeof(allSolenoids)/sizeof(int);


void setup() {

    // set all relay pins to OUTPUT mode and turn off
    for (int x = 0; x < solenoidCount; x++) {
        pinMode(allSolenoids[x], OUTPUT);
        digitalWrite(allSolenoids[x], RELAY_OFF);
    }
  
    pinMode(REDLED, OUTPUT);           // set up LED
    pinMode(BLUELED, OUTPUT);          // set up Blue LED
    digitalWrite(REDLED, OFF);         // and turn it off
    digitalWrite(BLUELED, OFF);        // and turn it off

    if (HASBUTTON) { pinMode(inputButton, INPUT_PULLUP);   }


    

    if (DEBUG) { Serial.begin(115200); }
    connectWifi();
    // you're connected now, so print out the status:
    if (DEBUG) { printWifiStatus(); }
}


/*
   We're looping, looking for signals from the server to poof (or do something)
   or signals from the game to send to the server (high score!).
   
   We start poofing when we get a signal, stop when we get a stop.  We also stop
   when we reach maxPoof.
*/
void loop() {
 
    // CONFIRM CONNECTION
    int CA = 0;
    
    if (!client) {
       debugMsg("Attempting connection...");
       looksgood = reconnect();
    }
    

    // process any incoming messages
    
    if (client && looksgood) {        
        
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
                 
                 if (DEBUG) {
                   // adds a delay.  at any rate, only in debug mode...
                   flashBlue(1, 15, true); // show communication
                 }
 
                 while (rcvd=client.read()) {
                   if(rcvd=='%') { /* end of phrase - do something */
                     if (strcmp(incoming, "p1")==0) {
                       /* strings are equal, poof it up */
                       // if we've previously hit maxPoofLimit, we ignore incoming
                       // attempts to poof for poofDelay loops
                       if (stopPoofing==0) {
                         if(!poofing && maxPoof <= maxPoofLimit) { // if we're not poofing nor at the limit
                           poofing = 1;
                           digitalWrite(REDLED,ON);  // turn light on
                           notified = false;     // let us know when the poofer stops
                           poofAll(true);
                         } 
                       }
                     } else if (strcmp(incoming, "p2")==0) {
                         if (stopPoofing==0) {
                           debugMsg(incoming);
                           if (POOFSTORM) { poofStorm(); }
                         }
                     } else if (strcmp(incoming, "p0")==0) {
                         notified = true;     // let us know when the poofer stops
                         debugMsg(incoming);
                         poofing=0;
                     } else {
                         // it's some other phrase
                         poofing = 0;
                         debugMsg(incoming);
                         debugMsg("other phrase, poofing 0");
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
        digitalWrite(REDLED,OFF); // done poofing, turn off lights
        // turn off all poofers
        poofAll(false);
        maxPoof=0;
    }

    if (!client.available()) { // Is the server asking for something?
        if(HASBUTTON) { checkScore(); }
          
        /* 
          if not poofing, decrement our paus poofing counter.
          if poofing, increment our timeout counter.
        */
        
        if (stopPoofing>0) { stopPoofing--; }  
        else if (poofing==1) { maxPoof++; }
        
        /* wait a tiny little bit... because reasons */                 
        delay(loopDelay);

        
    }
   
    stillOnline++;
    if(stillOnline >= onlineMsg){
      debugMsg("Wifi and socket status...");
      if (DEBUG) { 
          printWifiStatus(); 
          
          /* 
             I THINK THIS MIGHT MESS WITH TIMING, as it introduces a delay
             in the loop, so we keep it inside a DEBUG.  Also, we now know that
             it starts flashing red if a connection is lost...
          */
          flashBlue(3, 30, true); // connection maintained
      }
      checkSocket();
      stillOnline = 0;
   }
}
/*================= END MAIN LOOP== =====================*/


/*  
    do something if the inputButton has been pushed.
    we may want to use serial communications with the main game
    board, so we can get alphanumeric info...
*/
 
int checkScore(){
    boolean shotValue=digitalRead(inputButton);
    if(shotValue==ON) {
        // button being pushed
        debugMsg("Button pushed!");
        callServer(REPORT);
    }
}

/*================= WIFI FUNCTIONS =====================*/

int reconnect() {
    // wait for a new client:
    // Attempt a connection with base unit
    if (!client.connect(HOST, PORT)) {
        debugMsg("connection failed");
        digitalWrite(2,OFF);   //turn off blue connection light
        flashRed(5,25,false);   // flash 5 times to indicate failure
        delay(1000);            // wait a little before trying agagin
        return 0;
    } else {
      digitalWrite(2,ON); //turn on blue connection light
      // re-announce we we are to the server
      // clean out the input buffer:
      client.flush();
      debugPiece(WHOAMI);
      debugMsg(": ONLINE");
      client.println(WHOAMI); // Tell server which game

      return 1;
    }
}

void checkSocket() {
   if (!client.connected()) {
       debugMsg("socket down - attempting to reconnect...");
       reconnect();
   }
}

void printWifiStatus() {
 
    // print the SSID of the network you're attached to:
    if (DEBUG){
      Serial.print("SSID: ");
      Serial.println(WiFi.SSID());
    }
    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    if (DEBUG) {
      Serial.print("IP Address: ");
      Serial.println(ip);
    }
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    if (DEBUG) {
      Serial.print("signal strength (RSSI):");
      Serial.print(rssi);
      Serial.println(" dBm");
    }
}

void connectWifi(){
  while ( status != WL_CONNECTED) {
        debugPiece("Attempting to connect to SSID: ");
        debugMsg(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // flash and wait 10 seconds for connection:
        flashBlue(15, 166, false); // flash 15 times slowly to indicate connecting
    }
}

void callServer(String message){
    String out = WHOAMI;
    out += ":";
    out += message;
    out += ":";
    client.flush();
    client.println(out);
    if (DEBUG) flashBlue(5, 30, true); // show communication
}

/*================= DEBUG FUNCTIONS =====================*/

void flashRed(int times, int rate, bool finish){
  // flash `times` at a `rate` and `finish` in which state
  for(int i = 0;i <= times ;i++){
    digitalWrite(REDLED, finish);
    delay(rate);
    digitalWrite(REDLED, !finish);
    delay(rate);
  }
}

void flashBlue(int times, int rate, bool finish){
  // flash `times` at a `rate` and `finish` in which state
  for(int i = 0;i <= times ;i++){
    digitalWrite(BLUELED, finish);
    delay(rate);
    digitalWrite(BLUELED, !finish);
    delay(rate);
  }
}

void debugMsg(String message){
  if (DEBUG) { Serial.println(message); }
}

void debugPiece(String message){
  if (DEBUG) { Serial.print(message); }
}

/*================= POOF FUNCTIONS =====================*/

void poofAll(boolean state) {
  // turn on all poofers
  for (int y = 0; y < solenoidCount; y++) {
    if(state == true){
      digitalWrite(allSolenoids[y], RELAY_ON);
    }else{
      digitalWrite(allSolenoids[y], RELAY_OFF);
    }
  }
  if(state == true){ // optional messaging
    debugMsg("Poofing started");
  }else{
    if(!notified){
      debugMsg("Poofing stopped");
    }
  }
}

void poofRight(boolean state) {
   for (int y = 0; y < solenoidCount; y++) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(200);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(200);
      }
  }
}

void poofRight(boolean state, int rate) {  // overloaded function offering speed setting
   if(!rate){ rate = 200;}
   for (int y = 0; y < solenoidCount; y++) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(rate);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(rate);
      }
  }
}

void poofLeft(boolean state) {
   for (int y = solenoidCount; y == 0;  y--) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(200);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(200);
      }
  }
}

void poofLeft(boolean state, int rate) {  // overloaded function offering speed setting
   if(!rate){ rate = 200;}
   for (int y = solenoidCount; y == 0;  y--) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(rate);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(rate);
      }
  }
}

void puffRight(boolean state, int rate) {
   if(!rate){ rate = 200;}
   for (int y = 0; y < solenoidCount; y++) {
      if(state == true){
        digitalWrite(allSolenoids[y], RELAY_ON);
        delay(rate);
      }else{
        digitalWrite(allSolenoids[y], RELAY_OFF);
        delay(rate);
      }
  }
}

void puffSingleRight(int rate) { // briefly turn on individual poofers
   if(!rate){ rate = 200;}
   for (int y = 0; y < solenoidCount+1; y++) {
    digitalWrite(allSolenoids[y], RELAY_ON);
    if(y > 0){
      digitalWrite(allSolenoids[y-1], RELAY_OFF);
    }
    delay(rate);
  }
}

void poofSingleLeft(int rate) { // briefly turn on all poofers to the left
  int arraySize = solenoidCount+1;
   if(!rate){ rate = 200;}
   for (int y = arraySize; y == 0;  y--) {
    digitalWrite(allSolenoids[y], RELAY_ON);
    if(y!= arraySize){
      digitalWrite(allSolenoids[y+1], RELAY_OFF);
    }
    delay(rate);
  }
}

void poofStorm(){  // crazy poofer display
  debugMsg("POOFSTORM");
  poofAll(true);
  delay(300);
  poofAll(false); 
  poofSingleLeft(200);
  delay(50);
  puffSingleRight(200);
  delay(50);
  poofLeft(true);
  poofAll(false);
  delay(200);
  poofRight(true);
  poofAll(false);
  delay(200);
  poofAll(true);
  delay(600);
  poofAll(false);
}

void gunIt(){ // rev the poofer before big blast
  for(int i = 0; i < 4; i++){
    poofAll(true);
    delay(100);
    poofAll(false);
    delay(50);
  }
  poofAll(true);
  delay(500);  
}

void poofEven(int rate){  // toggle even numbered poofers
  for (int y = 0; y < solenoidCount; y++) {
    if(y != 0 && y%2 != 0 ){
      digitalWrite(allSolenoids[y], RELAY_ON);
      delay(rate);
      digitalWrite(allSolenoids[y], RELAY_OFF);
      delay(rate);
    }
  }
}

void poofOdd(int rate){ // toggle odd numbered poofers
  for (int y = 0; y < solenoidCount; y++) {
    if(y == 0 || y%2 == 0 ){
      digitalWrite(allSolenoids[y], RELAY_ON);
      delay(rate);
      digitalWrite(allSolenoids[y], RELAY_OFF);
      delay(rate);
    }
  }
}

void lokiChooChoo(int start, int decrement, int rounds){ // initial speed, speed increase, times
  for(int i = 0; i <= rounds; i++){
    poofOdd(start);
    start -= decrement;
    poofEven(start);
    start -= decrement;
  }
  delay(start);
  poofAll(true);
  delay(start);
  poofAll(false);
}
