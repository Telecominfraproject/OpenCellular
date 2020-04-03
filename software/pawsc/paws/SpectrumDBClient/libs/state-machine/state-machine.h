/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


#pragma once

/*
This module provides functionality to make it easy to implement state machines.
The main principle is that you define a static state transition table. You pass
that table into sm_create() and get back a StateMachine object. Here's an
example of such a table:

State state_transition_table[] = {
    //                                               State wide
    // State            State wide 'pre' funcs       'post' funcs     Event                 Next state          Event specific 'post' funcs
    WAIT_FOR_FAP,       { init_states,
                          remove_iot_from_loggers,
                          kill_master_sm,
                          kill_slave_sms },           { NULL },       { FAP_DEFINED,        WAIT_FOR_GPS,       { add_iot_to_loggers, check_gps } },
    WAIT_FOR_GPS,       { NULL },                     { NULL },       { FAP_NOT_DEFINED,    WAIT_FOR_FAP,       { NULL },
                                                                        GPS_READ,           WAIT_FOR_SLAVES,    { check_slave_info } },
    WAIT_FOR_SLAVES,    { kill_slave_sms },           { NULL },       { FAP_NOT_DEFINED,    WAIT_FOR_FAP,       { NULL },
                                                                        SLAVE_DEFINED,      HANDLE_SPECTRUMS,   { master_set_slave_info } },

    // Last state must be this end marker.
    SM_STATE_INVALID,   { NULL },                     { NULL },       { SM_EVENT_INVALID,   SM_STATE_INVALID,   { NULL } }
};

For this to work, a few rules have had to be imposed:

1. You must set the MAX_XXX #defines below this comment to state the maximum
   sizes of various arrays.
2. The state id and event id types are int. The value of zero is reserved and
   is used to mean INVALID or end of list. This was done so you don't have to
   specify all the elements of the arrays in the table. Instead you can rely
   on the compiler to zero pad incompletely specified arrays, and for the
   functions in this library to skip over the zeroed elements. It is
   recommended that you define your state and event ids with enums, where the
   first value is set to SM_EVENT_START_OF_USER_IDS or 
   SM_STATE_START_OF_USER_IDS as appropriate, eg:

typedef enum
{
    HANDLE_SPECTRUMS = SM_STATE_START_OF_USER_IDS,
    WAIT_FOR_FAP,
    WAIT_FOR_GPS,
    WAIT_FOR_SLAVES
} MyStateIds;

typedef enum
{
    FAP_DEFINED = SM_EVENT_START_OF_USER_IDS,
    FAP_NOT_DEFINED,
    GPS_READ,
    SLAVE_DEFINED
} MyEventIds;
*/

#define MAX_PRE_FUNCTIONS  4
#define MAX_POST_FUNCTIONS 3
#define MAX_EVENTS_PER_STATE 14


typedef enum
{
    SM_EVENT_INVALID = 0,
    SM_EVENT_START_OF_USER_IDS
} SM_EVENT_ID;


typedef enum 
{
    SM_STATE_INVALID = 0,
    SM_STATE_NO_CHANGE,
    SM_STATE_START_OF_USER_IDS
} SM_STATE_ID;


// Prototype for 'pre' and 'post' functions.
typedef void StateFunc(void *user_data);


// Prototypes for 'event processed' and 'state changed' callbacks. 
// These are only generally registered by an application that wants to 
// print debug messages when these things happen.
typedef void EventProcessedFunc(void *user_data, int event_id);
typedef void StateChangedFunc(void *user_data, int old_state, int new_state);


typedef struct _EventQueue EventQueue;


typedef struct
{
    int event_id;       // Event that kicks-off this state transition
    int next_state_id;
    StateFunc *post_funcs[MAX_POST_FUNCTIONS];
} EventTransition;


typedef struct
{
    int id;
    StateFunc *pre_funcs[MAX_PRE_FUNCTIONS]; // Functions to be called on entry to this state.
    StateFunc *post_funcs[MAX_POST_FUNCTIONS]; // Functions to be called on exit from this state.
    EventTransition event_transitions[MAX_EVENTS_PER_STATE];
} State;


typedef struct
{
    int current_state_id;
    State *state_transition_table;
    EventQueue *event_queue;
    void *user_data;
    EventProcessedFunc *event_processed_func;
    StateChangedFunc *state_changed_func;
} StateMachine;


// Create a state machine. Returns NULL on failure.
// Params:
//   state_transition_table - A table that represents all the states and state
//      transitions in the state machine. See comments at top of file for an
//      example.
//
//   initial_state_id - The state to start in.
//
//   user_data - This is a pointer to user_data. We don't care what it is. You
//      pass it in here and we pass it back to you whenever we call one of the
//      function handles you registered (ie a StateFunc). Can be NULL.
//
//   run_pre_funcs_for_initial_state - Must be 0 or 1. Typically you pass 1 if
//      this is a new state machine and 0 if you are resuming an existing state
//      machine session.
StateMachine *sm_create(State *state_transition_table, int initial_state_id,
                        void *user_data, int run_pre_funcs_for_initial_state);

// Register "event processed" and "state changed" handlers. These are only generally 
// registered by an application that wants to print debug messages when these
// things happen.
// Passing either handler as NULL prevents that handler from being called.
void sm_register_event_processed_func(StateMachine *sm, EventProcessedFunc *event_processed_func);
void sm_register_state_changed_func(StateMachine *sm, StateChangedFunc *state_changed_func);

void sm_delete(StateMachine *sm);

// Push an event into the event queue. Won't be acted on until sm_process_events
// is called. It is perfectly safe to call this function from inside a StateFunc
// callback.
void sm_raise_event(StateMachine *sm, int event_id);

// Processes all the events in the the event queue. Returns the number of events
// that were processed.
int sm_process_events(StateMachine *sm);
