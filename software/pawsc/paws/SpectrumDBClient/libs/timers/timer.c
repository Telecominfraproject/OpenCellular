/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Own header
#include "timer.h"

// Project headers
#include "string-buffer/string_buffer.h"

// Standard headers
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


typedef struct
{
    unsigned id;
    unsigned duration_in_seconds;
    time_t end_time;    // Zero if timer is inactive
} TimerItem;


// ****************************************************************************
// A singly linked list to store timer items
// ****************************************************************************

typedef struct ListItem
{
    TimerItem timer;
    struct ListItem *next;
} ListItem;


static ListItem *list_push_front(ListItem *head, TimerItem timer)
{
    ListItem *new_node = malloc(sizeof(ListItem));
    if (!new_node)
        return NULL;

    new_node->timer = timer;
    new_node->next = head;
    return new_node;
}


static ListItem *list_find(ListItem *head, unsigned id)
{
    for (ListItem *curr = head; curr; curr = curr->next)
        if (curr->timer.id == id)
            return curr;
    
    return NULL;
}


// head MUST NOT be NULL. If id is not present in the list, the list will not be altered.
// Returns the new head of the list (which is the existing head unless that was just removed).
static ListItem *list_remove(ListItem *head, unsigned id)
{
    if (id == head->timer.id) {
        ListItem *new_head = head->next;
        free(head);
        return new_head;
    }

    ListItem *prev = NULL;
    for (ListItem *curr = head; curr; curr = curr->next) {
        if (curr->timer.id == id) {
            prev->next = curr->next;
            free(curr);
            break;
        }

        prev = curr;
    }

    return head;
}


// ****************************************************************************
// timer_manager
// ****************************************************************************

struct _TimerManager
{
    ListItem *timers;
    TimerEventHandler handler_func;
    void *user_data;
};


TimerManager *timer_manager_create(TimerEventHandler handler_func, void *user_data)
{
    TimerManager *tman = calloc(1, sizeof(TimerManager));
    if (!tman)
        return NULL;

    tman->handler_func = handler_func;
    tman->user_data = user_data;

    return tman;
}


void timer_manager_delete(TimerManager *tman)
{
    while (tman->timers)
        tman->timers = list_remove(tman->timers, tman->timers->timer.id);
    free(tman);
}


void timer_manager_do_tick(TimerManager *tman)
{
    time_t now = time(NULL);
    for (ListItem *node = tman->timers; node; node = node->next) {
        if (node->timer.end_time != 0 && now >= node->timer.end_time) {
            // Timer has fired
            node->timer.end_time = 0;
            tman->handler_func(node->timer.id, tman->user_data);
        }
    }
}


char *timer_manager_save_state(TimerManager *tman)
{
    StrBuf *strbuf = strbuf_alloc();

    for (ListItem *node = tman->timers; node; node = node->next) {
        if (node->timer.end_time != 0) {
            TimerItem t = node->timer;
            char buf[64];
            size_t len = snprintf(buf, sizeof(buf), "id:%02d dur:%04d end:%ld, ", t.id, t.duration_in_seconds, t.end_time);
            strbuf_append(strbuf, buf, len);
        }
    }
    
    char *rv = strbuf_to_cstr(strbuf);
    strbuf_free(strbuf);
    return rv;
}


int timer_manager_load_state(TimerManager *tman, const char *state_string)
{
    while (state_string) {
        TimerItem timer;
        int num_items_parsed = sscanf(state_string, "id:%d dur:%d end:%ld", &(timer.id),
            &(timer.duration_in_seconds), &(timer.end_time));
        if (num_items_parsed != 3)
            break;

        ListItem *p = list_find(tman->timers, timer.id);
        if (!p)
            return -1;

        p->timer = timer;

        state_string = strchr(state_string, ',');
        if (!state_string)
            break;

        state_string++;
        while (isspace(*state_string))
            state_string++;
    }

    return 0;
}


int timer_manager_create_timer(TimerManager *tman, unsigned id, unsigned duration_in_seconds)
{
    TimerItem timer;
    timer.id = id;
    timer.duration_in_seconds = duration_in_seconds;
    timer.end_time = 0;

    ListItem *new_list = list_push_front(tman->timers, timer);
    if (new_list == NULL)
        return -1;

    tman->timers = new_list;
    return 0;
}


void timer_manager_delete_timer(TimerManager *tman, unsigned id)
{
    tman->timers = list_remove(tman->timers, id);
}


int timer_manager_set_duration(TimerManager *tman, unsigned id, unsigned duration_in_seconds)
{
    ListItem *node = list_find(tman->timers, id);
    if (!node)
        return -1;

    node->timer.duration_in_seconds = duration_in_seconds;

    if (node->timer.end_time != 0)
        node->timer.end_time = time(NULL) + duration_in_seconds;

    return 0;
}


int timer_manager_start_timer(TimerManager *tman, unsigned id)
{
    ListItem *node = list_find(tman->timers, id);
    if (!node)
        return -1;

    node->timer.end_time = time(NULL) + node->timer.duration_in_seconds;
    return 0;
}


int timer_manager_stop_timer(TimerManager *tman, unsigned id)
{
    ListItem *node = list_find(tman->timers, id);
    if (!node)
        return -1;

    node->timer.end_time = 0;
    return 0;
}
