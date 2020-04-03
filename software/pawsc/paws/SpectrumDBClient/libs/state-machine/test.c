/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


// Project headers
#include "state-machine.h"
#include "timers/timer.h"

// Standard headers
#include <stdlib.h>
#include <stdio.h>


#ifdef _MSC_VER
# include <windows.h>
# define sleep(x) Sleep(x * 1000)
#else
# include <unistd.h>
#endif


void add_iot_to_loggers             (void *data) { puts(__FUNCTION__); }
void check_gps                      (void *data) { puts(__FUNCTION__); }
void check_slave_info               (void *data) { puts(__FUNCTION__); }
void check_spec_states              (void *data) { puts(__FUNCTION__); }
void create_master_sm               (void *data) { puts(__FUNCTION__); }
void create_slave_sms               (void *data) { puts(__FUNCTION__); }
void fap_disable                    (void *data) { puts(__FUNCTION__); }
void init_states                    (void *data) { puts(__FUNCTION__); }
void kill_master_sm                 (void *data) { puts(__FUNCTION__); }
void kill_slave_sms                 (void *data) { puts(__FUNCTION__); }
void master_set_slave_info          (void *data) { puts(__FUNCTION__); }
void raise_master_dlul_spec_avail   (void *data) { puts(__FUNCTION__); }
void raise_slave_dlul_spec_avail    (void *data) { puts(__FUNCTION__); }
void remove_iot_from_loggers        (void *data) { puts(__FUNCTION__); }
void select_dlul_spectrum           (void *data) { puts(__FUNCTION__); }
void set_dl_spec_state_avail        (void *data) { puts(__FUNCTION__); }
void set_dl_spec_state_notif_success(void *data) { puts(__FUNCTION__); }
void set_ul_spec_state_avail        (void *data) { puts(__FUNCTION__); }
void set_ul_spec_state_notif_success(void *data) { puts(__FUNCTION__); }


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


State state_transition_table[] = {
    //                  State-wide                  State-wide                                                  Event specific
    // State            'pre' funcs                 'post' funcs    Event                   Next state          'post' funcs
    { WAIT_FOR_FAP,     { init_states,
                          remove_iot_from_loggers,
                          kill_master_sm,
                          kill_slave_sms },         { NULL },       { { FAP_DEFINED,        WAIT_FOR_GPS,       { add_iot_to_loggers, check_gps } } } },
    { WAIT_FOR_GPS,     { NULL },                   { NULL },       { { FAP_NOT_DEFINED,    WAIT_FOR_FAP,       { NULL } },
                                                                      { GPS_READ,           WAIT_FOR_SLAVES,    { check_slave_info } } } },
    { WAIT_FOR_SLAVES,  { kill_slave_sms },         { NULL },       { { FAP_NOT_DEFINED,    WAIT_FOR_FAP,       { NULL } },
                                                                      { SLAVE_DEFINED,      HANDLE_SPECTRUMS,   { master_set_slave_info } } } },
    { SM_EVENT_INVALID, { NULL },                   { NULL },       { { SM_EVENT_INVALID,   SM_EVENT_INVALID,   { NULL } } } }
};


StateMachine *sm = NULL;


void timer_event_handler(unsigned id, void *user_data)
{
    (void)user_data;
    printf("Timer event handler called with event id of %d\n", id);
    printf("Raising state machine event %d\n", id);
    sm_raise_event(sm, id);
}


static const char *get_state_name(int id)
{
    switch (id) {
    case HANDLE_SPECTRUMS: return "handle spectrums";
    case WAIT_FOR_FAP: return "wait for fap";
    case WAIT_FOR_GPS: return "wait for slaves";
    default: return "unknown";
    }
}


void state_changed_handler(void *user_data, int old_state_id, int new_state_id)
{
    printf("State changed from '%s' to '%s'\n", get_state_name(old_state_id), get_state_name(new_state_id));
}


static const char *get_event_name(int id)
{
    switch (id)
    {
    case FAP_DEFINED: return "fap defined";
    case FAP_NOT_DEFINED: return "fap not defined";
    case GPS_READ: return "gps read";
    case SLAVE_DEFINED: return "slave defined";
    default: return "unknown";
    }
}


void event_processed_handler(void *user_data, int id)
{
    printf("Event '%s' processed\n", get_event_name(id));
}


int main()
{
    sm = sm_create(state_transition_table, WAIT_FOR_FAP, NULL, 1);
    sm_register_event_processed_func(sm, event_processed_handler);
    sm_register_state_changed_func(sm, state_changed_handler);
    TimerManager *tman = timer_manager_create(timer_event_handler, NULL);

    // FAP_DEFINED
    timer_manager_create_timer(tman, FAP_DEFINED, 1);
    timer_manager_start_timer(tman, FAP_DEFINED);
    sleep(2);
    timer_manager_do_tick(tman);
    sm_process_events(sm);

    // FAP_NOT_DEFINED
    timer_manager_create_timer(tman, FAP_NOT_DEFINED, 1);
    timer_manager_start_timer(tman, FAP_NOT_DEFINED);
    sleep(2);
    timer_manager_do_tick(tman);
    sm_process_events(sm);

    // Raise an event that has no EventTransition in the current state - should
    // be safely ignored.
    sm_raise_event(sm, GPS_READ);
    sm_process_events(sm);

    timer_manager_delete(tman);
    sm_delete(sm);

    return 0;
}
