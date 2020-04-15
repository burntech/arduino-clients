/*
  Carnival_network.h - Carnival library 
  Copyright 2016-2020 Neil Verplank.  All right reserved.
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

#include "Carnival_events.h"

// library interface description
class Carnival_network
{
  // user-accessible "public" interface
  public:
    Carnival_network();
    void      start(String who, bool dbug);
    int       reconnect(bool output);
    void      checkSocket();
    void      addSetting(char *str);
    void      refreshSettings();
    void      keepAlive();
    void      confirmConnect();
    boolean   OK();
    void      printWifiStatus();
    void      connectWifi();
    void      set_override(boolean wf_over_ride);
    void      check_override(int test);
    boolean   get_override();
    int       processIncoming(msg_t **new_messages, event_t **my_events);
    int       processMsg(char *incoming, msg_t **new_msg, event_t **new_events);
    int       readBuffer(msg_t **new_messages, event_t **my_events);
    void      callServer(String who, String message);
    void      callServer(String message);
    void      callServer(int message,int optdata);
    void      callServer(String who,int message);
    void      callServer(String who,int message,int optdata);
    void      sleepNow(int wakeButton);
    void      checkKill();

};

#endif
