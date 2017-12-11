/*
  Carnival_events.h - Carnival library 
  Copyright Dec 2017 Neil Verplank.  All right reserved.
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

typedef struct _event_t_ {
    char   *action;                     // type of event (e.g. "poof")
    long    begin;                      // relative time from 0 to begin
    long    length;                     // length of event in ms
    int     started;                    // 1 if begun
    struct _event_t_ *next;             // next event on a list
} event_t;


// library interface description
class Carnival_events
{
  // user-accessible "public" interface
  public:
    Carnival_events();

    event_t *new_event(char *str, long begin, int length);
    event_t *check_events(event_t *events, long seq_start, char **msg);
    event_t *concat_events(event_t *first_event, event_t *second_event);
    event_t *parse_events(char *str);
    void     free_event_list(event_t *head);
    void     free_event(event_t *event);

//  private:
};

#endif

