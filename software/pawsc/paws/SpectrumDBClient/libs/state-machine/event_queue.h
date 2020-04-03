/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#pragma once

// This file is used internally to the project and is not part of the public interface.


#include <stdbool.h>


#define MAX_EVENTS_QUEUED 512


typedef struct _EventQueue
{
    unsigned read_idx;
    unsigned write_idx;
    int array[MAX_EVENTS_QUEUED];
} EventQueue;


EventQueue *event_queue_create();
void event_queue_delete(EventQueue *q);
bool event_queue_is_empty(EventQueue *q);
void event_queue_push(EventQueue *q, int event_id);
int event_queue_pop(EventQueue *q);
