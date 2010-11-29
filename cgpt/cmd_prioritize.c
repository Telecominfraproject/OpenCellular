// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "cgptlib_internal.h"

static void Usage(void)
{
  printf("\nUsage: %s prioritize [OPTIONS] DRIVE\n\n"
         "Reorder the priority of all active ChromeOS Kernel partitions.\n\n"
         "Options:\n"
         "  -P NUM       Highest priority to use in the new ordering. The\n"
         "                 other partitions will be ranked in decreasing\n"
         "                 priority while preserving their original order.\n"
         "                 If necessary the lowest ranks will be coalesced.\n"
         "                 No active kernels will be lowered to priority 0.\n"
         "  -i NUM       Specify the partition to make the highest in the new\n"
         "                 order.\n"
         "  -f           Friends of the given partition (those with the same\n"
         "                 starting priority) are also updated to the new\n"
         "                 highest priority.\n"
         "\n"
         "With no options this will set the lowest active kernel to\n"
         "priority 1 while maintaining the original order.\n"
         "\n", progname);
}

//////////////////////////////////////////////////////////////////////////////
// I want a sorted list of priority groups, where each element in the list
// contains an unordered list of GPT partition numbers. This is a stupid
// implementation, but our needs are simple and don't justify the time or space
// it would take to write a "better" one.
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


//////////////////////////////////////////////////////////////////////////////

int cmd_prioritize(int argc, char *argv[]) {
  struct drive drive;
  uint32_t set_partition = 0;
  int set_friends = 0;
  int max_priority = 0;
  int priority;
  int orig_priority = 0;
  int gpt_retval;
  GptEntry *entry;
  uint32_t index;
  uint32_t max_part;
  int num_kernels;
  int i,j;
  group_list_t *groups;

  int c;
  int errorcnt = 0;
  char *e = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hi:fP:")) != -1)
  {
    switch (c)
    {
    case 'i':
      set_partition = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'f':
      set_friends = 1;
      break;
    case 'P':
      max_priority = (int)strtol(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      if (max_priority < 1 || max_priority > 15) {
        Error("value for -%c must be between 1 and 15\n", c);
        errorcnt++;
      }
      break;

    case 'h':
      Usage();
      return CGPT_OK;
    case '?':
      Error("unrecognized option: -%c\n", optopt);
      errorcnt++;
      break;
    case ':':
      Error("missing argument to -%c\n", optopt);
      errorcnt++;
      break;
    default:
      errorcnt++;
      break;
    }
  }
  if (errorcnt)
  {
    Usage();
    return CGPT_FAILED;
  }

  if (set_friends && !set_partition) {
    Error("the -f option is only useful with the -i option\n");
    Usage();
    return CGPT_FAILED;
  }

  if (optind >= argc) {
    Error("missing drive argument\n");
    return CGPT_FAILED;
  }

  if (CGPT_OK != DriveOpen(argv[optind], &drive))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    return CGPT_FAILED;
  }

  max_part = GetNumberOfEntries(&drive.gpt);

  if (set_partition) {
    if (set_partition < 1 || set_partition > max_part) {
      Error("invalid partition number: %d (must be between 1 and %d\n",
            set_partition, max_part);
      goto bad;
    }
    index = set_partition - 1;
    // it must be a kernel
    entry = GetEntry(&drive.gpt, PRIMARY, index);
    if (!GuidEqual(&entry->type, &guid_chromeos_kernel)) {
      Error("partition %d is not a ChromeOS kernel\n", set_partition);
      goto bad;
    }
  }

  // How many kernel partitions do I have?
  num_kernels = 0;
  for (i = 0; i < max_part; i++) {
    entry = GetEntry(&drive.gpt, PRIMARY, i);
    if (GuidEqual(&entry->type, &guid_chromeos_kernel))
      num_kernels++;
  }

  if (!num_kernels)
    // nothing to do, so don't
    goto good;

  // Determine the current priority groups
  groups = NewGroupList(num_kernels);
  for (i = 0; i < max_part; i++) {
    entry = GetEntry(&drive.gpt, PRIMARY, i);
    if (!GuidEqual(&entry->type, &guid_chromeos_kernel))
      continue;

    priority = GetPriority(&drive.gpt, PRIMARY, i);

    // Is this partition special?
    if (set_partition && (i+1 == set_partition)) {
      orig_priority = priority;         // remember the original priority
      if (set_friends)
        AddToGroup(groups, priority, i); // we'll move them all later
      else
        AddToGroup(groups, 99, i);      // move only this one
    } else {
      AddToGroup(groups, priority, i);  // just remember
    }
  }

  // If we're including friends, then change the original group priority
  if (set_partition && set_friends) {
    ChangeGroup(groups, orig_priority, 99);
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
  if (max_priority)
    priority = max_priority;
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
      SetPriority(&drive.gpt, PRIMARY,
                  groups->group[i].part[j], groups->group[i].priority);

  FreeGroups(groups);


  // Write it all out
good:
  RepairEntries(&drive.gpt, MASK_PRIMARY);
  RepairHeader(&drive.gpt, MASK_PRIMARY);

  drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                         GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
  UpdateCrc(&drive.gpt);

  return DriveClose(&drive, 1);

bad:
  (void) DriveClose(&drive, 0);
  return CGPT_FAILED;
}
