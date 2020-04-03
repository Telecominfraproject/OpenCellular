/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#pragma once

#include "utils/types.h"

typedef int(*data_callback_func1)(void* p1);						// generic callback 1 param
typedef int(*data_callback_func2)(void* p1, anytype_u p2);			// generic callback 2 params

typedef struct llist_item
{
	struct llist_item*	prev;
	struct llist_item*	next;
	void*				data;
} llist_item_t;

typedef struct
{
	llist_item_t*		head;
	llist_item_t*		tail;
} llist_t;



//#######################################################################################

// create a new list
extern llist_t* llist_new(void);

// free all entries from a list,  BUT NOT the list itself
extern void llist_clear(llist_t* ll, data_callback_func1 free_func);

// free all entries from a list, and free the list itself
extern void llist_free(llist_t** ll, data_callback_func1 free_func);

// insert entry at head of list
extern llist_item_t* llist_prepend(llist_t* ll, void* data);

// add entry at tail of list
extern llist_item_t* llist_append(llist_t* ll, void* data);

// find entry point where in insertion will take place, based on "comparison_data" and "compare_func".   "compare_func" to return <0, 0, >0, as per strcmp 
extern llist_item_t* llist_find_insert_before(llist_t* ll, void* data, data_callback_func2 compare_func, anytype_u comparison_data);

// insert entry before specified item
extern llist_item_t* llist_insert_before(llist_t* ll, void* data, llist_item_t* before_this);

// insert entry into list based on "comparison_data" and "compare_func".   "compare_func" to return <0, 0, >0, as per strcmp 
extern llist_item_t* llist_insert(llist_t* ll, void* data, data_callback_func2 compare_func, anytype_u comparison_data);

// return the data entity pointed to be the head item
extern void* llist_get_head(llist_t* ll);

// remove head from the list
extern void llist_free_head(llist_t* ll, data_callback_func1 free_func);

// get entry, based on "comparison_data" and "compare_func".   "compare_func" to return <0, 0, >0, as per strcmp 
extern void* llist_get_entry(llist_t* ll, data_callback_func2 compare_func, anytype_u comparison_data);

// delele entries which match "comparison data", using "compare func".   "compare_func" to return <0, 0, >0, as per strcmp 
// "single_entry" = TRUE means only the first matched entry will be deleted, otherwise all entries match the comparson_data will be deleted 
extern void llist_delete_entries(llist_t* ll, data_callback_func2 compare_func, anytype_u comparison_data, data_callback_func1 free_func, bool single_entry);

// for "action" func for all entries in the list
extern void llist_foreach(llist_t* ll, data_callback_func1 action);

// get next "item" in the list.
extern void* get_next_item_entry(llist_item_t *item);			// used for cases when item ptr is stored in the data structure itself