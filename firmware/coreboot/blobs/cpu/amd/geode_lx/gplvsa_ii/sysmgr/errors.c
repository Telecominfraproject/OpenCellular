/*
* Copyright (c) 2006-2008 Advanced Micro Devices,Inc. ("AMD").
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General
* Public License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place, Suite 330,
* Boston, MA 02111-1307 USA 
*/

//*****************************************************************************
//*    Implements the error reporting code.  
//*****************************************************************************




#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"

extern void pascal write_flat_size(ULONG, ULONG, UCHAR);
extern ULONG pascal Get_SysCall_Address(ULONG, UCHAR);
extern ULONG VSMs_EAX;
extern ULONG Saved_EAX, Saved_EBX, Saved_ECX, Saved_EDX, Saved_ESI, Saved_EDI;
extern ULONG Current_VSM;


#define MAX_ERROR_BUFFER  500
UCHAR ErrorStrings[MAX_ERROR_BUFFER];
UCHAR * ErrorBuffer = ErrorStrings;
UCHAR * CurrentError;
UCHAR * PreviousError = ErrorStrings;
UCHAR DuplicateFlag;

int letbase;


//***************************************************************************
// Input:
//    BH - 1 to clear error log
//    CL - Error number (0 for 1st)
// Exit:
//    AL - 0 if last error
//***************************************************************************
void Get_Errors()
{ USHORT ErrorNumber, Flag;
  ULONG Destination;

  // Get parameters passed in from INFO
  Flag = (UCHAR)(Saved_EBX >> 8);
  ErrorNumber = (UCHAR)Saved_ECX;
  Destination = Saved_EDI;

  (UCHAR)Saved_EAX = 0x00;

  // If no errors, return AL = 0;
  if (ErrorBuffer == ErrorStrings) {
	 return;
  }  

  // If request for 1st error, initialize buffer ptr
  if (ErrorNumber == 0) {
    CurrentError = ErrorStrings;
  } 


  // Copy next message to EDI
  while (CurrentError < ErrorBuffer) {
    // Copy 1 byte of error message
    write_flat_size(Destination++, (ULONG)*CurrentError, BYTE_IO);
    // If end of this message, return
    if (*CurrentError++ == 0) {

       write_flat_size(Destination-1, '$', BYTE_IO);

      (UCHAR)Saved_EAX = 1;

      // Is this the last message ?
      if (*CurrentError == 0) {
        // Clear error log ? (/E flag)
        if (Flag == 1) {
          PreviousError = ErrorBuffer = ErrorStrings;
        }
      }
      return;
    }
  }
} 






//*****************************************************************************
//*****************************************************************************
static void printchar(char **str, int c)
{
  out_8(DBG_PORT, (UCHAR)c);
  in_8(0x80);
  if (ErrorBuffer < &ErrorStrings[MAX_ERROR_BUFFER-2]) {
    *ErrorBuffer = c;
    if (*ErrorBuffer++ != *PreviousError++) {
	  DuplicateFlag = 0;
    }
  }	 
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

//*****************************************************************************
//*****************************************************************************
static int prints(char **out, const char *string, int width, int pad)
{ register int pc = 0, padchar = ' ';

  if (width > 0) {
    register int len = 0;
    register const char *ptr;

    for (ptr = string; *ptr; ++ptr) {
      ++len;
    }

    if (len >= width) {
      width = 0;
    } else {
      width -= len;
    }

    if (pad & PAD_ZERO) {
      padchar = '0';
    }
  }


  if (!(pad & PAD_RIGHT)) {
    for ( ; width > 0; --width) {
      printchar(out, padchar);
      ++pc;
    }
  }

  for ( ; *string ; ++string) {
    printchar(out, *string);
    ++pc;
  }

  for ( ; width > 0; --width) {
    printchar(out, padchar);
    ++pc;
  }

  return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

//*****************************************************************************
// Prints an integer with the specified formatting
//*****************************************************************************
static int printi(char **out, unsigned long u, int base, int sg, int width, int pad)
{ char print_buf[PRINT_BUF_LEN];
  register char *s;
  register int t, neg = 0, pc = 0;

  if (u == 0) {
    print_buf[0] = '0';
    print_buf[1] = '\0';
    return prints (out, print_buf, width, pad);
  }
  if (sg && (base == 10) && (u & 0x8000)) {
    neg = 1;
    u = ~u;
    u++;
  }

  s = print_buf + PRINT_BUF_LEN-1;
  *s = '\0';

  while (u) {
    t = (int)(u % base);
    if (t >= 10 ) {
      t += letbase - '0' - 10;
    }
    *--s = t + '0';
    u /= base;
  }

  if (neg) {
    if (width && (pad & PAD_ZERO) ) {
      printchar(out, '-');
      ++pc;
      --width;
    } else {
      *--s = '-';
    }
  }

  return pc + prints(out, s, width, pad);
}

//*****************************************************************************
//*****************************************************************************
static int print(char **out, int *varg)
{ register int width, pad;
  register int pc = 0;
  register char *format = (char *)(*varg++);
  char scr[2];
  unsigned long number;
  char *s;


  for (; *format != 0; ++format) {
    if (*format == '%') {
      ++format;
      pad = 0;
      if (*format == '\0') {
        break;
      }

      switch (*format) {
        case '%':
          printchar(out, *format);
          ++pc;
          continue;
      
        case '-':
          ++format;
          pad = PAD_RIGHT;
          break;
      }

      while (*format == '0') {
        ++format;
        pad |= PAD_ZERO;
      }

      for (width = 0; *format >= '0' && *format <= '9'; ++format) {
        width *= 10;
        width += *format - '0';
      }

      letbase = 'a';
      switch (*format) {

        case 's':
          s = *((char **)varg++);
          pc += prints (out, s ? s :"(null)", width, pad);
          break;

        case 'u':
          number = *varg++;
          pc += printi (out, number, 10, 0, width, pad);
          break;

        case 'd':
          number = *varg++;
          pc += printi (out, number, 10, 1, width, pad);
          break;

        case 'x':
//        pc += printi (out, number, 16, 0, width, pad);
//        break;

        case 'X':
          letbase = 'A';
          number = (USHORT)*varg++;
          if (width > 4) {
            number |= (ULONG)(*varg++) << 16;
          }
          pc += printi (out, number, 16, 0, width, pad);
          break;

        case 'c':
          // char are converted to int then pushed on the stack
          scr[0] = *varg++;
          scr[1] = '\0';
          pc += prints (out, scr, width, pad);
          break;

      } // end switch

    } else {
      printchar(out, *format);
      ++pc;
    }
  } // end for

	if (out) {
      **out = '\0';
    }
	return pc;
}

//*****************************************************************************
//*****************************************************************************
void Log_Error(const char *format, ...)
{ register int *varg = (int *)(&format);
  UCHAR * SavedErrorBuffer;
  UCHAR * SavedPrevious;

  SavedErrorBuffer = ErrorBuffer;
  SavedPrevious = PreviousError;
  if (PreviousError == ErrorStrings) {
    DuplicateFlag = 0;
  } else {
    DuplicateFlag = 1;
  }

  // Store a formatted error message
  print(0, varg);

  // Terminate the error buffer
  if (ErrorBuffer < &ErrorStrings[MAX_ERROR_BUFFER]) {
    *(ErrorBuffer++) = 0;
  }

  // Same error as previous?
  if (DuplicateFlag) {
    // Yes, then restore ptrs
	PreviousError = SavedPrevious;
    ErrorBuffer = SavedErrorBuffer;
    *(ErrorBuffer) = 0;
  }	else {
    // No, then advance previous error buffer ptr
    PreviousError = SavedErrorBuffer;
  }

}



//*****************************************************************************
// Inserts an error entry into the error log.
//*****************************************************************************
void pascal Error_Report(UCHAR ErrorCode, ULONG Info1, ULONG Info2, ULONG Vsm, UCHAR Depth)
{ ULONG CallingAddress;
  static UCHAR ErrorString[50];

  CallingAddress = Get_SysCall_Address(Vsm, Depth);

  Log_Error("Error code 0x%02X reported at %08X. Parameters: 0x%08X and 0x%08X.", \
            ErrorCode, CallingAddress, Info1, Info2);
}


//*****************************************************************************
// Reports an error from a VSM.
//*****************************************************************************
void pascal Report_VSM_Error(UCHAR ErrorCode, ULONG Info1, ULONG Info2)
{
  if (Current_VSM == 0) {
    Error_Report(ErrorCode, Info1, Info2, SysMgr_VSM,  0);
  }	else {
    Error_Report(ErrorCode, Info1, Info2, Current_VSM, 1);
  }
}


