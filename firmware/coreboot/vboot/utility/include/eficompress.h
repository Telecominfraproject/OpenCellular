/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define EFI_STATUS int
#define VOID void
#define INT16 int16_t
#define UINT16 uint16_t
#define INT8 int8_t
#define UINT8 uint8_t
#define INT32 int32_t
#define UINT32 uint32_t
#define STATIC static
#define IN /**/
#define OUT /**/
#define EFIAPI /**/

#define EFIERR(a) (a)
#define EFI_SUCCESS               0
#define EFI_LOAD_ERROR            EFIERR (1)
#define EFI_INVALID_PARAMETER     EFIERR (2)
#define EFI_UNSUPPORTED           EFIERR (3)
#define EFI_BAD_BUFFER_SIZE       EFIERR (4)
#define EFI_BUFFER_TOO_SMALL      EFIERR (5)
#define EFI_NOT_READY             EFIERR (6)
#define EFI_DEVICE_ERROR          EFIERR (7)
#define EFI_WRITE_PROTECTED       EFIERR (8)
#define EFI_OUT_OF_RESOURCES      EFIERR (9)
#define EFI_VOLUME_CORRUPTED      EFIERR (10)
#define EFI_VOLUME_FULL           EFIERR (11)
#define EFI_NO_MEDIA              EFIERR (12)
#define EFI_MEDIA_CHANGED         EFIERR (13)
#define EFI_NOT_FOUND             EFIERR (14)
#define EFI_ACCESS_DENIED         EFIERR (15)
#define EFI_NO_RESPONSE           EFIERR (16)
#define EFI_NO_MAPPING            EFIERR (17)
#define EFI_TIMEOUT               EFIERR (18)
#define EFI_NOT_STARTED           EFIERR (19)
#define EFI_ALREADY_STARTED       EFIERR (20)
#define EFI_ABORTED               EFIERR (21)
#define EFI_ICMP_ERROR            EFIERR (22)
#define EFI_TFTP_ERROR            EFIERR (23)
#define EFI_PROTOCOL_ERROR        EFIERR (24)
#define EFI_INCOMPATIBLE_VERSION  EFIERR (25)
#define EFI_SECURITY_VIOLATION    EFIERR (26)
#define EFI_CRC_ERROR             EFIERR (27)
#define EFI_END_OF_MEDIA          EFIERR (28)
#define EFI_END_OF_FILE           EFIERR (31)
#define EFI_INVALID_LANGUAGE      EFIERR (32)

#define EFIWARN(a) ((a)+EFI_INVALID_LANGUAGE)
#define EFI_WARN_UNKNOWN_GLYPH    EFIWARN (1)
#define EFI_WARN_DELETE_FAILURE   EFIWARN (2)
#define EFI_WARN_WRITE_FAILURE    EFIWARN (3)
#define EFI_WARN_BUFFER_TOO_SMALL EFIWARN (4)

#define EFI_ERROR(Status) (Status != 0 && Status < EFIWARN(1))

EFI_STATUS
EfiCompress (
  IN      UINT8   *SrcBuffer,
  IN      UINT32  SrcSize,
  IN      UINT8   *DstBuffer,
  IN OUT  UINT32  *DstSize
  );

EFI_STATUS
EFIAPI
EfiGetInfo (
  IN      VOID                    *Source,
  IN      UINT32                  SrcSize,
  OUT     UINT32                  *DstSize,
  OUT     UINT32                  *ScratchSize
  );

EFI_STATUS
EFIAPI
EfiDecompress (
  IN      VOID                    *Source,
  IN      UINT32                  SrcSize,
  IN OUT  VOID                    *Destination,
  IN      UINT32                  DstSize,
  IN OUT  VOID                    *Scratch,
  IN      UINT32                  ScratchSize
  );
