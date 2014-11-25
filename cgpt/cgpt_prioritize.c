// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "cgpt.h"
#include "cgptlib_internal.h"
#include "vboot_host.h"

//////////////////////////////////////////////////////////////////////////////
// We need a sorted list of priority groups, where each element in the list
// contains an unordered list of GPT partition numbers.

#define MAX_GROUPS 17                   // 0-15, plus one "higher"

typedef struct {
  int priority;                         // priority of this group
  int num_parts;                        // number of partitions in this group
  uint32_t *part;                       // array of partitions in this group
} group_t;

typedef struct {
  int max_parts;                       // max number of partitions in any group
  int num_groups;                      // number of non-empty groups
  group_t group[MAX_GROUPS];           // array of groups
} group_list_t;


static group_list_t *NewGroupList(int max_p) {
  int i;
  group_list_t *gl = (group_list_t *)malloc(sizeof(group_list_t));
  require(gl);
  gl->max_parts = max_p;
  gl->num_groups = 0;
  // reserve space for the maximum number of partitions in every group
  for (i=0; i<MAX_GROUPS; i++) {
    gl->group[i].priority = -1;
    gl->group[i].num_parts = 0;
    gl->group[i].part = (uint32_t *)malloc(sizeof(uint32_t) * max_p);
    require(gl->group[i].part);
  }

  return gl;
}

static void FreeGroups(group_list_t *gl) {
  int i;
  for (i=0; i<MAX_GROUPS; i++)
    free(gl->group[i].part);
  free(gl);
}

static void AddToGroup(group_list_t *gl, int priority, int partition) {
  int i;
  // See if I've already got a group with this priority
  for (i=0; i<gl->num_groups; i++)
    if (gl->group[i].priority == priority)
      break;
  if (i == gl->num_groups) {
    // no, add a group
    require(i < MAX_GROUPS);
    gl->num_groups++;
    gl->group[i].priority = priority;
  }
  // add the partition to it
  int j = gl->group[i].num_parts;
  gl->group[i].part[j] = partition;
  gl->group[i].num_parts++;
}

static void ChangeGroup(group_list_t *gl, int old_priority, int new_priority) {
  int i;
  for (i=0; i<gl->num_groups; i++)
    if (gl->group[i].priority == old_priority) {
      gl->group[i].priority = new_priority;
      break;
    }
}

static void SortGroups(group_list_t *gl) {
  int i, j;
  group_t tmp;

  // straight insertion sort is fast enough
  for (i=1; i<gl->num_groups; i++) {
    tmp = gl->group[i];
    for (j=i; j && (gl->group[j-1].priority < tmp.priority); j--)
      gl->group[j] = gl->group[j-1];
    gl->group[j] = tmp;
  }
}

int CgptPrioritize(CgptPrioritizeParams *params) {
  struct drive drive;

  int priority;

  int gpt_retval;
  uint32_t index;
  uint32_t max_part;
  int num_kernels;
  int i,j;
  group_list_t *groups;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR,
                           params->drive_size))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    return CGPT_FAILED;
  }

  max_part = GetNumberOfEntries(&drive);

  if (params->set_partition) {
    if (params->set_partition < 1 || params->set_partition > max_part) {
      Error("invalid partition number: %d (must be between 1 and %d\n",
            params->set_partition, max_part);
      goto bad;
    }
    index = params->set_partition - 1;
    // it must be a kernel
    if (!IsKernel(&drive, PRIMARY, index)) {
      Error("partition %d is not a ChromeOS kernel\n", params->set_partition);
      goto bad;
    }
  }

  // How many kernel partitions do I have?
  num_kernels = 0;
  for (i = 0; i < max_part; i++) {
    if (IsKernel(&drive, PRIMARY, i))
      num_kernels++;
  }

  if (num_kernels) {
    // Determine the current priority groups
    groups = NewGroupList(num_kernels);
    for (i = 0; i < max_part; i++) {
      if (!IsKernel(&drive, PRIMARY, i))
        continue;

      priority = GetPriority(&drive, PRIMARY, i);

      // Is this partition special?
      if (params->set_partition && (i+1 == params->set_partition)) {
        params->orig_priority = priority;  // remember the original priority
        if (params->set_friends)
          AddToGroup(groups, priority, i); // we'll move them all later
        else
          AddToGroup(groups, 99, i);       // move only this one
      } else {
        AddToGroup(groups, priority, i);   // just remember
      }
    }

    // If we're including friends, then change the original group priority
    if (params->set_partition && params->set_friends) {
      ChangeGroup(groups, params->orig_priority, 99);
    }

    // Sorting gives the new order. Now we just need to reassign the
    // priorities.
    SortGroups(groups);

    // We'll never lower anything to zero, so if the last group is priority zero
    // we can ignore it.
    i = groups->num_groups;
    if (groups->group[i-1].priority == 0)
      groups->num_groups--;

    // Where do we start?
    if (params->max_priority)
      priority = params->max_priority;
    else
      priority = groups->num_groups > 15 ? 15 : groups->num_groups;

    // Figure out what the new values should be
    for (i=0; i<groups->num_groups; i++) {
      groups->group[i].priority = priority;
      if (priority > 1)
        priority--;
    }

    // Now apply the ranking to the GPT
    for (i=0; i<groups->num_groups; i++)
      for (j=0; j<groups->group[i].num_parts; j++)
        SetPriority(&drive, PRIMARY,
                    groups->group[i].part[j], groups->group[i].priority);

    FreeGroups(groups);
  }

  // Write it all out
  UpdateAllEntries(&drive);

  return DriveClose(&drive, 1);

bad:
  (void) DriveClose(&drive, 0);
  return CGPT_FAILED;
}
