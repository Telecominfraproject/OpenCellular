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

typedef struct Param {
  const char* name;  /* Parameter name */
  int is_string;     /* 0 if integer, 1 if string */
  int can_write;     /* 0 if read-only, 1 if writable */
  const char* desc;  /* Human-readable description */
  const char* format; /* Format string, if non-NULL and 0==is_string*/
} Param;

/* List of parameters, terminated with a param with NULL name */
const Param sys_param_list[] = {
  /* Read-only integers */
  {"devsw_cur",  0, 0, "Developer switch current position"},
  {"devsw_boot", 0, 0, "Developer switch position at boot"},
  {"recoverysw_cur", 0, 0, "Recovery switch current position"},
  {"recoverysw_boot", 0, 0, "Recovery switch position at boot"},
  {"recoverysw_ec_boot", 0, 0, "Recovery switch position at EC boot"},
  {"wpsw_cur",  0, 0, "Firmware write protect switch current position"},
  {"wpsw_boot", 0, 0, "Firmware write protect switch position at boot"},
  {"recovery_reason",  0, 0, "Recovery mode reason for current boot"},
  {"savedmem_base", 0, 0, "RAM debug data area physical address", "0x%08x"},
  {"savedmem_size", 0, 0, "RAM debug data area size in bytes"},
  {"fmap_base", 0, 0, "Main firmware flashmap physical address", "0x%08x"},
  {"tried_fwb", 0, 0, "Tried firmware B before A this boot"},
  /* Read-only strings */
  {"hwid", 1, 0, "Hardware ID"},
  {"fwid", 1, 0, "Active firmware ID"},
  {"ro_fwid", 1, 0, "Read-only firmware ID"},
  {"mainfw_act", 1, 0, "Active main firmware"},
  {"mainfw_type", 1, 0, "Active main firmware type"},
  {"ecfw_act", 1, 0, "Active EC firmware"},
  {"kernkey_vfy", 1, 0, "Type of verification done on kernel key block"},
  /* Writable integers */
  {"nvram_cleared", 0, 1, "Have NV settings been lost?  Write 0 to clear"},
  {"kern_nv", 0, 1, "Non-volatile field for kernel use", "0x%08x"},
  {"recovery_request", 0, 1, "Recovery mode request (writable)"},
  {"dbg_reset", 0, 1, "Debug reset mode request (writable)"},
  {"fwb_tries", 0, 1, "Try firmware B count (writable)"},

  /* TODO: implement the following:
   *   nvram_cleared
   */

  /* Terminate with null name */
  {NULL, 0, 0, NULL}
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
         "\n"
         "Valid parameters:\n", progname, progname, progname);
  for (p = sys_param_list; p->name; p++)
    printf("  %-22s  %s\n", p->name, p->desc);
}


/* Find the parameter in the list.
 *
 * Returns the parameter, or NULL if no match. */
const Param* FindParam(const char* name) {
  const Param* p;
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
  if (!p->can_write)
    return 1;  /* Parameter is read-only */

  if (p->is_string) {
    return (0 == VbSetSystemPropertyString(p->name, value) ? 0 : 1);
  } else {
    char* e;
    int i = strtol(value, &e, 0);
    if (!*value || (e && *e))
      return 1;
    return (0 == VbSetSystemPropertyInt(p->name, i) ? 0 : 1);
  }
}


/* Print the specified parameter.
 *
 * Returns 0 if success, non-zero if error. */
int PrintParam(const Param* p) {
  if (p->is_string) {
    char buf[256];
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
  char buf[256];
  const char* value;

  for (p = sys_param_list; p->name; p++) {
    if (p->is_string) {
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
    printf("%-22s = %-20s # %s\n",
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
    char* name = strtok(argv[i], "=");
    char* value = strtok(NULL, "=");
    const Param* p = FindParam(name);
    if (!p) {
      fprintf(stderr, "Invalid parameter name: %s\n", name);
      PrintHelp(progname);
      return 1;
    }

    if (i > 1)
      printf(" ");  /* Output params space-delimited */
    if (value)
      retval = SetParam(p, value);
    else
      retval = PrintParam(p);
  }

  return retval;
}
