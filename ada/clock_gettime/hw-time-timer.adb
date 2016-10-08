--
-- Copyright (C) 2016 secunet Security Networks AG
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; version 2 of the License.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--

with Interfaces.C;

use type Interfaces.C.long;

package body HW.Time.Timer
with
   Refined_State => (Timer_State => null,
                     Abstract_Time => null)
is
   CLOCK_MONOTONIC_RAW : constant := 4;

   subtype Clock_ID_T is Interfaces.C.int;
   subtype Time_T is Interfaces.C.long;

   type Struct_Timespec is record
      TV_Sec   : aliased Time_T;
      TV_NSec  : aliased Interfaces.C.long;
   end record;
   pragma Convention (C_Pass_By_Copy, Struct_Timespec);

   function Clock_Gettime
     (Clock_ID :        Clock_ID_T;
      Timespec : access Struct_Timespec)
      return Interfaces.C.int;
   pragma Import (C, Clock_Gettime, "clock_gettime");

   function Raw_Value_Min return T
   is
      Ignored : Interfaces.C.int;
      Timespec : aliased Struct_Timespec;
   begin
      Ignored := Clock_Gettime (CLOCK_MONOTONIC_RAW, Timespec'Access);
      return T (Timespec.TV_Sec * 1_000_000_000 + Timespec.TV_NSec);
   end Raw_Value_Min;

   function Raw_Value_Max return T
   is
   begin
      return Raw_Value_Min + 1;
   end Raw_Value_Max;

   function Hz return T
   is
   begin
      return 1_000_000_000; -- clock_gettime(2) is fixed to nanoseconds
   end Hz;

end HW.Time.Timer;
