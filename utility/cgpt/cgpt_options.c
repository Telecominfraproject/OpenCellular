/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Update GPT attribute bits.
 */
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt.h"
#include "utility.h"

/* Special validator. Set 'integer' as 1 to indicate the option is present. */
int AssignTrue(const char *argument, void *pointer, void *integer) {
  *(int*)integer = 1;
  return CGPT_OK;
}

/* Special validator. Copy string to 'parsed' with max 'valid_range' bytes. */
int CopyString(const char *argument, void *max_len, void *dst) {
  Memcpy(dst, argument, (intptr_t)max_len);
  return CGPT_OK;
}

/* Validator function. Returns 1 if 'argument' is between 'max' and 'min'
 * in 'valid_range'. */
int InNumberRange(const char *argument, void *valid_range, void *parsed) {
  struct number_range *range = valid_range;
  char *endptr;
  int number;

  number = strtol(argument, &endptr, 10);
  if (*endptr) {
    printf("[ERROR] argument '%s' is not a number.\n", argument);
    return CGPT_FAILED;
  }

  if (range) {
    if (number < range->min) {
      printf("[ERROR] argument is too small (min is %d, but you gave: %d).\n",
             range->min, number);
      return CGPT_FAILED;
    } else if (number > range->max) {
      printf("[ERROR] argument is too large (max is %d, but you gave: %d).\n",
             range->max, number);
      return CGPT_FAILED;
    } else {
      if (parsed) *(int*)parsed = number;
      return CGPT_OK;
    }
  } else {
    /* no range to check, assign integer. */
    if (parsed) *(int*)parsed = number;
    return CGPT_OK;
  }
}

void ShowOptions(const struct option *opts,
                 const struct option_details *details,
                 const int num) {
  int i;
  for (i = 0; i < num; ++i) {
    char buf[32];
    if (!opts[i].name) break;
    snprintf(buf, sizeof(buf), "--%s %s", opts[i].name,
                                          opts[i].has_arg ? "ARG" : "");
    printf("  %-20s (-%c)  %s\n", buf, opts[i].val, details[i].comment);
  }
}

int HandleOptions(const int argc,
                  char *const *argv,
                  const char *short_options,
                  const int option_count,
                  const struct option *options,
                  const struct option_details *details) {
  while (1) {
    int index;
    int option;

    /* We assume every short option has an entry in long option (for validator).
     * So please add corresponding entry in attribute_options if you add short
     * option. */
    index = NOT_INITED;
    option = getopt_long(argc, argv, short_options, options, &index);
    if (option == -1) {
      break;
    } else if (option == 0) {
      /* option 'val' has been saved in 'flag'. We do nothing here. */
    } else if (option == ':') {
      printf("[ERROR] Missing parameter for option.\n");
      ShowOptions(options, details, option_count);
      return CGPT_FAILED;
    } else if (option == '?') {
      printf("[ERROR] unknown option name: %s\n", argv[optind - 1]);
      ShowOptions(options, details, option_count);
      return CGPT_FAILED;
    } else {
      /* Short option doesn't update 'index'. We search whole array to find out
       * the corresponding long option. */
      if (index == NOT_INITED) {
        for (index = 0; index < option_count; ++index)
          if (option == options[index].val) break;
        /* assumes every short option has a corresponding long option. */
        assert(index < option_count);
      }
      assert(option == options[index].val);

      /* Calls validator if an argument is provided. */
      if (details[index].validator &&
          CGPT_OK != details[index].validator(
              optarg ? argv[optind - 1] : 0,
              details[index].valid_range,
              details[index].parsed)) {
        printf("[ERROR] The argument of '%s' is invalid.\n",
               options[index].name);
        return CGPT_FAILED;
      }
    }
  }
  return CGPT_OK;
}

int OpenDriveInLastArgument(const int argc,
                            char *const *argv,
                            struct drive *drive) {
  if (optind != (argc - 1)) {
    printf("[ERROR] One (and only one) non-option argument is required.\n");
    return CGPT_FAILED;
  }

  return DriveOpen(argv[optind], drive);
}
