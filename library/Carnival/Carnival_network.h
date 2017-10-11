/*
  Carnival_network.h - Carnival library 
  Copyright 2016 Neil Verplank.  All right reserved.
*/

// ensure this library description is only included once
#ifndef Carnival_network_h
#define Carnival_network_h

// include types & constants of other libraries here:
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "WString.h"


// library interface description
class Carnival_network
{
  // user-accessible "public" interface
  public:
    Carnival_network();
    void    start(String who, bool dbug);
    int     reconnect(bool output);
    void    checkSocket();
    void    keepAlive();
    void    confirmConnect();
    boolean OK();
    void    printWifiStatus();
    void    connectWifi();
    char*   readMsg();
    void    callServer(String who, String message);
    void    callServer(String message);
    void    callServer(int message,int optdata);
    void    callServer(String who,int message);
    void    callServer(String who,int message,int optdata);
    void    sleepNow(int wakeButton);

};

#endif
