/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of firmware-provided API functions.
 */

#include <execinfo.h>
#include <stdint.h>

#define _STUB_IMPLEMENTATION_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "vboot_api.h"

#define MAX_STACK_LEVELS 10


/* Keep track of nodes that are currently allocated */
struct alloc_node {
	struct alloc_node *next;
	void *ptr;
	size_t size;
	void *bt_buffer[MAX_STACK_LEVELS];
	int bt_levels;
};

static struct alloc_node *alloc_head;

static void print_stacktrace(void)
{
	void *buffer[MAX_STACK_LEVELS];
	int levels = backtrace(buffer, MAX_STACK_LEVELS);

	// print to stderr (fd = 2), and remove this function from the trace
	backtrace_symbols_fd(buffer + 1, levels - 1, 2);
}

void *VbExMalloc(size_t size)
{
	struct alloc_node *node;
	void *p = malloc(size);

	if (!p) {
		/* Fatal Error. We must abort. */
		abort();
	}

	node = malloc(sizeof(*node));
	if (!node)
		abort();
	node->next = alloc_head;
	node->ptr = p;
	node->size = size;
	node->bt_levels = backtrace(node->bt_buffer, MAX_STACK_LEVELS);
	alloc_head = node;

	return p;
}

static struct alloc_node **find_node(void *ptr)
{
	struct alloc_node **nodep;

	for (nodep = &alloc_head; *nodep; nodep = &(*nodep)->next)
		if ((*nodep)->ptr == ptr)
			return nodep;

	return NULL;
}

void VbExFree(void *ptr)
{
	struct alloc_node **nodep, *next;

	nodep = find_node(ptr);
	if (nodep) {
		next = (*nodep)->next;
		free(*nodep);
		*nodep = next;
	} else {
		fprintf(stderr, "\n>>>>>> Invalid VbExFree() %p\n", ptr);
		fflush(stderr);
		print_stacktrace();
		/*
		 * Fall through and do the free() so we get normal error
		 * handling.
		 */
	}

	free(ptr);
}

VbError_t VbExHashFirmwareBody(VbCommonParams *cparams,
                               uint32_t firmware_index)
{
	return VBERROR_SUCCESS;
}

int vboot_api_stub_check_memory(void)
{
	struct alloc_node *node, *next;

	if (!alloc_head)
		return 0;

	/*
	 * Make sure we free all our memory so that valgrind doesn't complain
	 * about leaked memory.
	 */
	fprintf(stderr, "\nWarning, some allocations not freed:");
	for (node = alloc_head; node; node = next) {
		next = node->next;
		fprintf(stderr, "\nptr=%p, size=%zd\n", node->ptr, node->size);
		fflush(stderr);
		backtrace_symbols_fd(node->bt_buffer + 1, node->bt_levels - 1,
				     2);
		free(node);
	}

	return -1;
}
