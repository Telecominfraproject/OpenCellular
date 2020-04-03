/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Own header
#include "event_queue.h"

// Standard headers
#include <assert.h>
#include <stdlib.h>


EventQueue *event_queue_create()
{
    return calloc(1, sizeof(EventQueue));
}


void event_queue_delete(EventQueue *q)
{
    free(q);
}


bool event_queue_is_empty(EventQueue *q)
{
    return q->read_idx == q->write_idx;
}


static bool event_queue_is_full(EventQueue *q)
{
    return (q->write_idx - q->read_idx) == MAX_EVENTS_QUEUED;
}


void event_queue_push(EventQueue *q, int event_id)
{
    assert(!event_queue_is_full(q));
    q->write_idx++;
    q->array[q->write_idx & (MAX_EVENTS_QUEUED - 1)] = event_id;
}


int event_queue_pop(EventQueue *q)
{
    assert(!event_queue_is_empty(q));
    q->read_idx++;
    return q->array[q->read_idx & (MAX_EVENTS_QUEUED - 1)];
}
