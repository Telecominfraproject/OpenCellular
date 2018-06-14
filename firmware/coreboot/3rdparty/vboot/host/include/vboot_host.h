/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * vboot-related functions exported for use by userspace programs
 */

#ifndef VBOOT_HOST_H_
#define VBOOT_HOST_H_
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

/****************************************************************************/
/* EFI GPT manipulation */

#include "cgpt_params.h"

/* partition table manipulation */
int CgptCreate(CgptCreateParams *params);
int CgptAdd(CgptAddParams *params);
int CgptSetAttributes(CgptAddParams *params);
int CgptGetPartitionDetails(CgptAddParams *params);
int CgptBoot(CgptBootParams *params);
int CgptGetBootPartitionNumber(CgptBootParams *params);
int CgptShow(CgptShowParams *params);
int CgptGetNumNonEmptyPartitions(CgptShowParams *params);
int CgptRepair(CgptRepairParams *params);
int CgptPrioritize(CgptPrioritizeParams *params);
void CgptFind(CgptFindParams *params);
int CgptLegacy(CgptLegacyParams *params);

/* GUID conversion functions. Accepted format:
 *
 *   "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
 *
 * At least GUID_STRLEN bytes should be reserved in 'str' (included the tailing
 * '\0').
 */
#define GUID_STRLEN 37
int StrToGuid(const char *str, Guid *guid);
void GuidToStr(const Guid *guid, char *str, unsigned int buflen);
int GuidEqual(const Guid *guid1, const Guid *guid2);
int GuidIsZero(const Guid *guid);


/****************************************************************************/
/* Kernel command line */

/* TODO(wfrichar): This needs a better location */
#define MAX_KERNEL_CONFIG_SIZE     4096

/* Use this to obtain the body load address from the kernel preamble */
#define USE_PREAMBLE_LOAD_ADDR     (~0)

/* Returns a new copy of the kernel cmdline. The caller must free it. */
char *FindKernelConfig(const char *filename,
                       uint64_t kernel_body_load_address);

/****************************************************************************/
/* Kernel partition */

/* Used to get a bootable vmlinuz from the kernel partition. vmlinuz_out must
 * be free'd after this function returns success. Success is indicated by a
 * zero return value.
 */
int ExtractVmlinuz(void *kpart_data, size_t kpart_size,
		   void **vmlinuz_out, size_t *vmlinuz_size);


#endif  /* VBOOT_HOST_H_ */
