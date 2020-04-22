/*
  Carnival_events.h - Carnival library 
  Copyright Dec 2017-2020 Neil Verplank.  All right reserved.
*/

// ensure this library description is only included once
#ifndef Carnival_events_h
#define Carnival_events_h

// include types & constants of other libraries here:
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "WString.h"

// list of events
typedef struct _event_t_ {
    char   *action;                     // type of event (e.g. "poof")
    long    begin;                      // relative time from 0 to begin
    long    length;                     // length of event in ms
    int     started;                    // 1 if begun
    struct _event_t_ *next;             // next event on a list
} event_t;

// list of messages
typedef struct _msg_t_ {
    char   *msg;                      
    struct _msg_t_ *next;            
} msg_t;


// library interface description
class Carnival_events
{
  // user-accessible "public" interface
  public:
    Carnival_events();

    event_t  *new_event(char *str, long begin, long length, long seq_start);
    event_t  *check_events(int *msgs_found, event_t *events, msg_t **new_msgs);
    event_t  *concat_events(event_t *first_event, event_t *second_event);
    event_t  *parse_events(char *str);
    void      free_event_list(event_t *head);
    void      free_event(event_t *event);
    msg_t    *concat_msgs(msg_t *first_msg, msg_t *second_msg);
    void      free_msg_list(msg_t *head);
    void      free_msg(msg_t *msg);

//  private:
};

#endif

