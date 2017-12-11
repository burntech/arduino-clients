/*
  Carnival_events.cpp - Carnival library
  Copyright Dec 2017 Neil Verplank.  All right reserved.

  Timed events are "messages" (equivalent to the instructions
  in a message from the server), associated with a beginning 
  time.  This allows the creation of any timed sequence of 
  events (any message that can otherwise be handled) in an
  asynchronous (non-blocking) fashion.

  As of the moment, this is generating poofer sequences, but
  it can and should handle led timing sequences, and etc.

  Currently events have a "length", though this isn't really
  used.

*/

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif


#include "Carnival_events.h"
#include "Carnival_debug.h"

extern   Carnival_debug debug;


/* 
    list of timed events.  events can apply to multiple effects, and
    can be "overlapping" (i.e. events can theoretically begin before 
    other events are finished for potentially complex behavior). 

    thus the list isn't necessarily sequential.  currently presuming
    one list at a time...

    "begin" is a time in ms.  if '0', event begins upon receipt of 
    timed sequence, >0 means begin that many ms after initial receipt.
*/

Carnival_events::Carnival_events()
{
}


/* EVENT ROUTINES  */

/*     create a new single-event list  */
event_t *Carnival_events::new_event(char *str, long begin, int length) {

    event_t *new_event;

    // allocate memory 
    if ((new_event  = (event_t *)malloc(sizeof(event_t))) == NULL) return NULL;

    // Populate data
    new_event->action       = strdup(str);          // explicity copy original into memory 
    new_event->begin        = begin;
    new_event->length       = length;
    new_event->started      = 0;
    new_event->next         = NULL;

    return new_event;
}




/* check all events in the current timed sequence */
event_t *Carnival_events::check_events(event_t *events, long seq_start, char **msg) {

    if (events == NULL) return NULL;

    long     now = millis();

    event_t *this_event, *last_event, *drop_event;

    this_event = events;
    last_event = NULL;
    drop_event = NULL;

    while (this_event!= NULL) {

        if ( now >= (seq_start + this_event->begin) && !this_event->started ) {
            // begin action (return message)
            *msg = this_event->action;
            this_event->started = 1;
            this_event = NULL;  // we skip to the end to process on this message
        } else if ( now >= (seq_start + this_event->begin + this_event->length) ) {
            // complete action
            if (last_event != NULL) {
                // drop this event from the list
                last_event->next = this_event->next;
            } else {
                // check next event
                events = this_event->next;
            }
            drop_event = this_event;
        }

        if (this_event != NULL) {
            // drop completed events from the list
            if (drop_event != NULL) {
                this_event = this_event->next;
                free_event(drop_event);
                drop_event = NULL;
            } else {
                last_event = this_event;
                this_event = this_event->next;
            }
        }
    }

    // returns an ever smaller list, eventually null
    return events;
}


/*
    appends second eventlist to first by iterating through the first.
    thus, faster if the first event list is the short one.
*/
event_t *Carnival_events::concat_events(event_t *first_event, event_t *second_event) {

    event_t *current = NULL;

    if (!first_event)  return second_event;
    if (!second_event) return first_event;
    for (current=first_event; current->next !=NULL; current=current->next);

    current->next = second_event;

    return first_event;
}



/*    frees all memory used by a linked list of events    */
void Carnival_events::free_event_list(event_t *head) {
    event_t *pos, *temp;
    pos = head;
    while(pos!=NULL) {
        temp = pos;
        pos  = pos->next;
        free_event(temp);
    }
    head = NULL;
}




/*      frees all memory for a given event   */
void Carnival_events::free_event(event_t *event) {
    free(event->action);
    free(event);
}



