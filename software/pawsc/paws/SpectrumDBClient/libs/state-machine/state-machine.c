/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Own header
#include "state-machine.h"

// Project headers
#include "event_queue.h"

// Standard headers
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


static State *get_current_state(StateMachine *sm)
{
    int i = 0;
    while (1) {
        State *s = sm->state_transition_table + i;
        if (s->id == SM_STATE_INVALID)
            break;
        if (s->id == sm->current_state_id)
            return s;
        i++;
    }

    assert(0); // Current state is not present in the event transition table
    return NULL;
}


static void change_state(StateMachine *sm, int new_state_id)
{
    if (sm->current_state_id == new_state_id)
        return;

    if (sm->current_state_id != SM_STATE_INVALID)
    {
        State *current_state = get_current_state(sm);

        // Run the state-wide 'post' funcs.
        for (int i = 0; i < MAX_POST_FUNCTIONS; i++) {
            if (!current_state->post_funcs[i])
                break;

            current_state->post_funcs[i](sm->user_data);
        }
    }

    if (sm->state_changed_func)
        sm->state_changed_func(sm->user_data, sm->current_state_id, new_state_id);
        
    // Change the state.
    sm->current_state_id = new_state_id;

    // Run the state-wide 'pre' funcs.
    State *current_state = get_current_state(sm);
    for (int i = 0; i < MAX_PRE_FUNCTIONS; i++) {
        if (!current_state->pre_funcs[i])
            break;

        current_state->pre_funcs[i](sm->user_data);
    }
}


static EventTransition *get_event_transition(State *s, int event_id)
{
    for (int i = 0; i < MAX_EVENTS_PER_STATE; i++) {
        EventTransition *et = s->event_transitions + i;
        if (et->event_id == SM_EVENT_INVALID)
            break;  // The rest of this array is unused.

        if (et->event_id == event_id)
            return et;
    }

    return NULL;
}


static void process_event(StateMachine *sm, int event_id)
{
    State *current_state = get_current_state(sm);
    EventTransition *event_transition = get_event_transition(current_state, event_id);

    if (sm->event_processed_func)
        sm->event_processed_func(sm->user_data, event_id);

    if (event_transition) {
        // Run the event-specific 'post' funcs.
        for (int j = 0; j < MAX_POST_FUNCTIONS; j++) {
            if (!event_transition->post_funcs[j])
                break;
            event_transition->post_funcs[j](sm->user_data);
        }

        if (event_transition->next_state_id != SM_STATE_NO_CHANGE)
            change_state(sm, event_transition->next_state_id);
    }
}


StateMachine *sm_create(State *state_transition_table, int initial_state_id,
                        void *user_data, int run_pre_funcs_for_initial_state)
{
    StateMachine *sm = malloc(sizeof(StateMachine));
    sm->state_transition_table = state_transition_table;
    sm->event_queue = event_queue_create();
    sm->user_data = user_data;
    sm->event_processed_func = NULL;
    sm->state_changed_func = NULL;

    if (run_pre_funcs_for_initial_state) {
        sm->current_state_id = SM_STATE_INVALID;
        change_state(sm, initial_state_id);
    } else {
        sm->current_state_id = initial_state_id;
    }

    return sm;
}


void sm_register_event_processed_func(StateMachine *sm, EventProcessedFunc *event_processed_func)
{
    sm->event_processed_func = event_processed_func;
}


void sm_register_state_changed_func(StateMachine *sm, StateChangedFunc *state_changed_func)
{
    sm->state_changed_func = state_changed_func;
}


void sm_delete(StateMachine *sm)
{
    event_queue_delete(sm->event_queue);
    free(sm);
}


void sm_raise_event(StateMachine *sm, int event_id)
{
    event_queue_push(sm->event_queue, event_id);
}


int sm_process_events(StateMachine *sm)
{
    int num_events_processed = 0;
    while (!event_queue_is_empty(sm->event_queue)) {
        int event_id = event_queue_pop(sm->event_queue);
        process_event(sm, event_id);
        num_events_processed++;
    }

    return num_events_processed;
}