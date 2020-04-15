/*
  Carnival_events.cpp - Carnival library
  Copyright Dec 2017-2020 Neil Verplank.  All right reserved.

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
#include "Carnival_network.h"

extern   Carnival_debug   debug;
extern   Carnival_network network;

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
event_t *Carnival_events::new_event(char *str, long begin, int length, long seq_start) {

    event_t *new_event;

    // allocate memory 
    if ((new_event  = (event_t *)malloc(sizeof(event_t))) == NULL) return NULL;

    // Populate data
    new_event->action       = strdup(str);          // explicity copy original into memory 
    new_event->begin        = begin + seq_start;    // time event to begin relative to current sequence
    new_event->length       = length;
    new_event->started      = 0;
    new_event->next         = NULL;

    return new_event;
}




/* check all events in the current timed sequence */
int Carnival_events::check_events(event_t *events, msg_t **new_msgs) {

    int msgs_found = 0;

    if (events == NULL) return msgs_found;

    long     now = millis();

    event_t *this_event, *last_event, *drop_event;

    this_event = events;
    last_event = NULL;
    drop_event = NULL;

    while (this_event!= NULL) {

        if ( now >= (this_event->begin) && !this_event->started ) {

            // process the timed event
            msg_t   *new_msg    = NULL;
            event_t *new_events = NULL;

            // check for system messages
            msgs_found = network.processMsg(this_event->action,&new_msg,&new_events);

            // check for non system messags
            if (msgs_found) 
                *new_msgs = concat_msgs(*new_msgs, new_msg);

            // start event 
            this_event->started = 1;

            // if it's a one-off, drop from event queue
            if (!this_event->length)
                drop_event = this_event;

        } else if ( now >= (this_event->begin + this_event->length) ) {
            // complete action

// JUST GOING TO NOTE WE ACTUALLY DO NOTHING AT COMPLETION OF LENGTH 
// (and instead seem to just set a start and a finish action at different timmes....

            // event done, drop from queue
            drop_event = this_event;

        }

        if (drop_event != NULL) {  // dropping event we're pointing to

            // cut completed events from the list if in the middle of the list
            if (last_event != NULL) {
                // drop this event from the list
                last_event->next = this_event->next;
            } 

            // move current pointer
            this_event = this_event->next;

            // and free up memory
            free_event(drop_event);

        } else {                   // moving to next event, this one's fine.
            last_event = this_event;
            this_event = this_event->next;
        }
    }

    // returns an ever smaller list, eventually null
    return msgs_found;
}


/*
    appends second eventlist to first by iterating through the first.
    thus, faster if the first event list is the short one.
*/
event_t *Carnival_events::concat_events(event_t *first_event, event_t *second_event) {

    event_t *current = NULL;
    if (!first_event)  return second_event;
    if (!second_event) return first_event;
    if (first_event == second_event) return first_event;

    // get the last element on the list
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
}




/*      frees all memory for a given event   */
void Carnival_events::free_event(event_t *event) {
    free(event->action);
    free(event);
}


msg_t *Carnival_events::concat_msgs(msg_t *first_msg, msg_t *second_msg) {

    msg_t *current = NULL;

    if (!first_msg)  return second_msg;
    if (!second_msg) return first_msg;
    if (first_msg == second_msg) return first_msg;

    // get the last element on the list
    for (current=first_msg; current->next !=NULL; current=current->next);

    current->next = second_msg;

    return first_msg;
}


/*    frees all memory used by a linked list of events    */
void Carnival_events::free_msg_list(msg_t *head) {
    msg_t *pos, *temp;
    pos = head;
    while(pos!=NULL) {
        temp = pos;
        pos  = pos->next;
        free_msg(temp);
    }
}




/*      frees all memory for a given event   */
void Carnival_events::free_msg(msg_t *msg) {
    free(msg->msg);
    free(msg);
}




