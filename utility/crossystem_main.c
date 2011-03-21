/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Chrome OS firmware/system interface utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crossystem.h"

/* Max length of a string parameter */
#define MAX_STRING 8192

/* Flags for Param */
#define IS_STRING      0x01  /* String (not present = integer) */
#define CAN_WRITE      0x02  /* Writable (not present = read-only */
#define NO_PRINT_ALL   0x04  /* Don't print contents of parameter when
                              * doing a print-all */

typedef struct Param {
  const char* name;  /* Parameter name */
  int flags;         /* Flags (see above) */
  const char* desc;  /* Human-readable description */
  const char* format; /* Format string, if non-NULL and 0==is_string*/
} Param;

/* List of parameters, terminated with a param with NULL name */
const Param sys_param_list[] = {
  /* Read-only integers */
  {"devsw_cur",  0, "Developer switch current position"},
  {"devsw_boot", 0, "Developer switch position at boot"},
  {"recoverysw_cur", 0, "Recovery switch current position"},
  {"recoverysw_boot", 0, "Recovery switch position at boot"},
  {"recoverysw_ec_boot", 0, "Recovery switch position at EC boot"},
  {"wpsw_cur", 0, "Firmware write protect hardware switch current position"},
  {"wpsw_boot", 0, "Firmware write protect hardware switch position at boot"},
  {"recovery_reason", 0, "Recovery mode reason for current boot"},
  {"savedmem_base", 0, "RAM debug data area physical address", "0x%08x"},
  {"savedmem_size", 0, "RAM debug data area size in bytes"},
  {"fmap_base", 0, "Main firmware flashmap physical address", "0x%08x"},
  {"tried_fwb", 0, "Tried firmware B before A this boot"},
  {"cros_debug", 0, "OS should allow debug features"},
  {"vdat_flags", 0, "Flags from VbSharedData", "0x%08x"},
  {"tpm_fwver", 0, "Firmware version stored in TPM", "0x%08x"},
  {"tpm_kernver", 0, "Kernel version stored in TPM", "0x%08x"},
  /* Read-only strings */
  {"hwid", IS_STRING, "Hardware ID"},
  {"fwid", IS_STRING, "Active firmware ID"},
  {"ro_fwid", IS_STRING, "Read-only firmware ID"},
  {"mainfw_act", IS_STRING, "Active main firmware"},
  {"mainfw_type", IS_STRING, "Active main firmware type"},
  {"ecfw_act", IS_STRING, "Active EC firmware"},
  {"kernkey_vfy", IS_STRING, "Type of verification done on kernel key block"},
  {"vdat_timers", IS_STRING, "Timer values from VbSharedData"},
  /* Writable integers */
  {"nvram_cleared", CAN_WRITE, "Have NV settings been lost?  Write 0 to clear"},
  {"kern_nv", CAN_WRITE, "Non-volatile field for kernel use", "0x%08x"},
  {"recovery_request", CAN_WRITE, "Recovery mode request (writable)"},
  {"dbg_reset", CAN_WRITE, "Debug reset mode request (writable)"},
  {"fwb_tries", CAN_WRITE, "Try firmware B count (writable)"},
  {"vbtest_errfunc", CAN_WRITE, "Verified boot test error function (writable)"},
  {"vbtest_errno", CAN_WRITE, "Verified boot test error number (writable)"},
  /* Fields not shown in a print-all list */
  {"vdat_lfdebug", IS_STRING|NO_PRINT_ALL,
   "LoadFirmware() debug data (not in print-all)"},
  {"vdat_lkdebug", IS_STRING|NO_PRINT_ALL,
   "LoadKernel() debug data (not in print-all)"},
  /* Terminate with null name */
  {NULL, 0, NULL}
};


/* Print help */
void PrintHelp(const char *progname) {
  const Param *p;

  printf("\nUsage:\n"
         "  %s\n"
         "    Prints all parameters with descriptions and current values.\n"
         "  %s [param1 [param2 [...]]]\n"
         "    Prints the current value(s) of the parameter(s).\n"
         "  %s [param1=value1] [param2=value2 [...]]]\n"
         "    Sets the parameter(s) to the specified value(s).\n"
         "  %s [param1?value1] [param2?value2 [...]]]\n"
         "    Checks if the parameter(s) all contain the specified value(s).\n"
         "Stops at the first error."
         "\n"
         "Valid parameters:\n", progname, progname, progname, progname);
  for (p = sys_param_list; p->name; p++)
    printf("  %-22s  %s\n", p->name, p->desc);
}


/* Find the parameter in the list.
 *
 * Returns the parameter, or NULL if no match. */
const Param* FindParam(const char* name) {
  const Param* p;
  if (!name)
    return NULL;
  for (p = sys_param_list; p->name; p++) {
    if (!strcasecmp(p->name, name))
      return p;
  }
  return NULL;
}


/* Set the specified parameter.
 *
 * Returns 0 if success, non-zero if error. */
int SetParam(const Param* p, const char* value) {
  if (!(p->flags & CAN_WRITE))
    return 1;  /* Parameter is read-only */

  if (p->flags & IS_STRING) {
    return (0 == VbSetSystemPropertyString(p->name, value) ? 0 : 1);
  } else {
    char* e;
    int i = (int)strtol(value, &e, 0);
    if (!*value || (e && *e))
      return 1;
    return (0 == VbSetSystemPropertyInt(p->name, i) ? 0 : 1);
  }
}


/* Compares the parameter with the expected value.
 *
 * Returns 0 if success (match), non-zero if error (mismatch). */
int CheckParam(const Param* p, char* expect) {
  if (p->flags & IS_STRING) {
    char buf[MAX_STRING];
    const char* v = VbGetSystemPropertyString(p->name, buf, sizeof(buf));
    if (!v || 0 != strcmp(v, expect))
      return 1;
  } else {
    char* e;
    int i = (int)strtol(expect, &e, 0);
    int v = VbGetSystemPropertyInt(p->name);
    if (!*expect || (e && *e))
      return 1;
    if (v == -1 || i != v)
      return 1;
  }
  return 0;
}


/* Print the specified parameter.
 *
 * Returns 0 if success, non-zero if error. */
int PrintParam(const Param* p) {
  if (p->flags & IS_STRING) {
    char buf[MAX_STRING];
    const char* v = VbGetSystemPropertyString(p->name, buf, sizeof(buf));
    if (!v)
      return 1;
    printf("%s", v);
  } else {
    int v = VbGetSystemPropertyInt(p->name);
    if (v == -1)
      return 1;
    printf(p->format ? p->format : "%d", v);
  }
  return 0;
}


/* Print all parameters with descriptions,
 *
 * Returns 0 if success, non-zero if error. */
int PrintAllParams(void) {
  const Param* p;
  int retval = 0;
  char buf[MAX_STRING];
  const char* value;

  for (p = sys_param_list; p->name; p++) {
    if (p->flags & NO_PRINT_ALL)
      continue;
    if (p->flags & IS_STRING) {
      value = VbGetSystemPropertyString(p->name, buf, sizeof(buf));
    } else {
      int v = VbGetSystemPropertyInt(p->name);
      if (v == -1)
        value = NULL;
      else {
        snprintf(buf, sizeof(buf), p->format ? p->format : "%d", v);
        value = buf;
      }
    }
    printf("%-22s = %-30s # %s\n",
           p->name, (value ? value : "(error)"), p->desc);
  }
  return retval;
}


int main(int argc, char* argv[]) {
  int retval = 0;
  int i;

  char* progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  /* If no args specified, print all params */
  if (argc == 1)
    return PrintAllParams();

  /* Print help if needed */
  if (!strcasecmp(argv[1], "-h") || !strcmp(argv[1], "-?")) {
    PrintHelp(progname);
    return 0;
  }

  /* Otherwise, loop through params and get/set them */
  for (i = 1; i < argc && retval == 0; i++) {
    char* has_set = strchr(argv[i], '=');
    char* has_expect = strchr(argv[i], '?');
    char* name = strtok(argv[i], "=?");
    char* value = strtok(NULL, "=?");
    const Param* p;

    /* Make sure args are well-formed. '' or '=foo' or '?foo' not allowed. */
    if (!name || has_set == argv[i] || has_expect == argv[i]) {
      fprintf(stderr, "Poorly formed parameter\n");
      PrintHelp(progname);
      return 1;
    }
    if (!value)
      value=""; /* Allow setting/checking an empty string ('foo=' or 'foo?') */
    if (has_set && has_expect) {
      fprintf(stderr, "Use either = or ? in a parameter, but not both.\n");
      PrintHelp(progname);
      return 1;
    }

    /* Find the parameter */
    p = FindParam(name);
    if (!p) {
      fprintf(stderr, "Invalid parameter name: %s\n", name);
      PrintHelp(progname);
      return 1;
    }

    if (i > 1)
      printf(" ");  /* Output params space-delimited */
    if (has_set)
      retval = SetParam(p, value);
    else if (has_expect)
      retval = CheckParam(p, value);
    else
      retval = PrintParam(p);
  }

  return retval;
}
