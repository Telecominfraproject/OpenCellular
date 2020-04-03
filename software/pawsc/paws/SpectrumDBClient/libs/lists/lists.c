/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Platform headers
#ifdef _MSC_VER
#include <winsock2.h>
typedef unsigned socklen_t;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// Standard headers
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "lists.h"
//#include "tcps1_globals.h"



//#######################################################################################
// static prototypes
static llist_item_t* llist_item_new(void* data);
static void llist_item_free(llist_item_t* item, data_callback_func1 free_func);



//#######################################################################################
static llist_item_t* llist_item_new(void* data)
{
	llist_item_t* item = malloc(sizeof(llist_item_t));
	if (item)
	{
		memset(item, 0, sizeof(llist_item_t));
		item->data = data;
	}
	return item;
}


//#######################################################################################
static void llist_item_free(llist_item_t* item, data_callback_func1 free_func)
{
	if (item)
	{
		if (item->data)
		{
			if (free_func)
			{
				free_func(item->data);
			}
		}
	}
	free(item);
}


//#######################################################################################
// create a new list
llist_t* llist_new(void)
{
	llist_t* ll = malloc(sizeof(llist_t));
	if (ll)
	{
		memset(ll, 0, sizeof(llist_t));
	}
	return ll;
}



//#######################################################################################
// free all entries from a list,  BUT NOT the list itself
void llist_clear(llist_t* ll, data_callback_func1 free_func)
{
	if (ll)
	{
		llist_item_t* item = ll->head;
		while (item)
		{
			llist_item_t* next = item->next;
			llist_item_free(item, free_func);
			item = next;
		}
	}
}


//#######################################################################################
// free all entries from a list, and free the list itself
void llist_free(llist_t** ll, data_callback_func1 free_func)
{
	if ((ll) && (*ll))
	{
		llist_clear(*ll, free_func);
		free(*ll);
		*ll = NULL;
	}
}



//#######################################################################################
// insert entry at head of list
llist_item_t* llist_prepend(llist_t* ll, void* data)
{
	if ((!ll) || (!data))
		return NULL;

	// create the item
	llist_item_t* item = llist_item_new(data);
	if (!item)
	{
		return NULL;
	}

	// add at front
	item->next = ll->head;
	if (ll->head)
		ll->head->prev = item;
	ll->head = item;
	if (!ll->tail)
		ll->tail = item;

	return item;
}


//#######################################################################################
// add entry at tail of list
llist_item_t* llist_append(llist_t* ll, void* data)
{
	if ((!ll) || (!data))
		return NULL;

	// create the item
	llist_item_t* item = llist_item_new(data);
	if (!item)
	{
		return NULL;
	}

	// add at tail
	if (ll->tail)
	{
		ll->tail->next = item;
		item->prev = ll->tail;
		ll->tail = item;
	}
	else
	{
		ll->head = item;
		ll->tail = item;
	}

	return item;
}


//#######################################################################################
// find entry point where in insertion will take place, based on "comparison_data" and "compare_func".   "compare_func" to return <0, 0, >0, as per strcmp 
llist_item_t* llist_find_insert_before(llist_t* ll, void* data, data_callback_func2 compare_func, anytype_u comparison_data)
{
	if ((!ll) || (!data) || (!compare_func))
		return NULL;

	// if the Q is empty, just add it 
	if (!ll->head)
	{
		return NULL;
	}

	// otherwise walk through the list, and stop once the comparison is <= that of the items context
	llist_item_t *item = ll->head;
	while (item)
	{
		llist_item_t*next = item->next;
		if (compare_func(item->data, comparison_data) > 0)
			break;
		item = next;
	}

	return item;
}


//#######################################################################################
// insert entry before specified item
llist_item_t* llist_insert_before(llist_t* ll, void* data, llist_item_t* before_this)
{
	if (!before_this)
	{
		return llist_append(ll, data);
	}
	else
	{
		llist_item_t* prev = before_this->prev;

		// create the item
		llist_item_t* item = llist_item_new(data);
		if (!item)
			return NULL;

		if (prev)
			prev->next = item;
		item->prev = prev;
		before_this->prev = item;
		item->next = before_this;

		// if we're inserting in front of head, item becomes the new head
		if (ll->head == before_this)
			ll->head = item;

		return item;
	}

	return NULL;
}


//#######################################################################################
// insert entry into list based on "comparison_data" and "compare_func".   "compare_func" to return <0, 0, >0, as per strcmp 
llist_item_t* llist_insert(llist_t* ll, void* data, data_callback_func2 compare_func, anytype_u comparison_data)
{
	if ((!ll) || (!data) || (!compare_func))
		return NULL;

	llist_item_t* before_this = llist_find_insert_before(ll, data, compare_func, comparison_data);

	// if it was greater than everything in the list, add at end
	if (!before_this)
		return llist_append(ll, data);
	else
	{
		// otherwise, insert before 
		return llist_insert_before(ll, data, before_this);
	}
}


//#######################################################################################
// return the data entity pointed to be the head item
void* llist_get_head(llist_t* ll)
{
	// create the item
	void* data = NULL;
	
	if ((ll) && (ll->head))
	{
		data = ll->head->data;
	}
	return data;
}



//#######################################################################################
// remove head from the list
void llist_free_head(llist_t* ll, data_callback_func1 free_func)
{
	if ((ll) && (ll->head))
	{
		llist_item_t* item = ll->head;

		// remove it from the list
		ll->head = ll->head->next;
		if (ll->head)
			ll->head->prev = NULL;
		if (ll->head == NULL)
			ll->tail = NULL;

		// free the item
		llist_item_free(item, free_func);
	}
}



//#######################################################################################
static llist_item_t* llist_get_item(llist_t* ll, data_callback_func2 compare_func, anytype_u comparison_data)
{
	if ((ll) && (ll->head) && (compare_func))
	{
		llist_item_t*item = ll->head;
		while (item)
		{
			llist_item_t*next = item->next;
			if (compare_func(item->data, comparison_data) == 0)
				return item;
			item = next;
		}
	}

	return NULL;
}



//#######################################################################################
// get entry, based on "comparison_data" and "compare_func".   "compare_func" to return <0, 0, >0, as per strcmp 
void* llist_get_entry(llist_t* ll, data_callback_func2 compare_func, anytype_u comparison_data)
{
	if (ll)
	{
		llist_item_t* item = NULL;
		if ((item = llist_get_item(ll, compare_func, comparison_data)))
		{
			return item->data;
		}
	}
	return NULL;
}



//#######################################################################################
// delele entries which match "comparison data", using "compare func".   "compare_func" to return <0, 0, >0, as per strcmp 
// "single_entry" = TRUE means only the first matched entry will be deleted, otherwise all entries match the comparson_data will be deleted 
void llist_delete_entries(llist_t* ll, data_callback_func2 compare_func, anytype_u comparison_data, data_callback_func1 free_func, bool single_entry)
{
	if ((ll) && (ll->head) && (compare_func))
	{
		llist_item_t* prev = NULL;
		llist_item_t* item = ll->head;
		while (item)
		{
			llist_item_t* next = item->next;
			if (compare_func(item->data, comparison_data) == 0)
			{
				if (prev)
					prev->next = next;

				if (next)
					next->prev = prev;

				if (item == ll->head)
				{
					ll->head = item->next;
					if (ll->head == NULL)
						ll->tail = NULL;
				}
				if (item == ll->tail)
					ll->tail = prev;

				// free the item and record
				llist_item_free(item, free_func);
				
				if (single_entry)
					break;
			}
			prev = item;
			item = next;
		}
	}
}


//#######################################################################################
// for "action" func for all entries in the list
void llist_foreach(llist_t* ll, data_callback_func1 action)
{
	if ((ll) && (action))
	{
		llist_item_t*item = ll->head;
		while (item)
		{
			llist_item_t*next = item->next;
			if (item->data)
				action(item->data);
			item = next;
		}
	}
}


//#######################################################################################
// get next "item" in the list.
void* get_next_item_entry(llist_item_t *item)
{
	if ((!item) || (!item->next))
		return NULL;

	return item->next->data;
}