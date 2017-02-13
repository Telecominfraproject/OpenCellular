--
-- Copyright (C) 2015-2016 secunet Security Networks AG
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

with System;

with Musinfo;
with Muschedinfo;

package body HW.Time.Timer
   with Refined_State => (Timer_State => null,
                          Abstract_Time => (Sinfo, Sched_Info))
is
   Sinfo_Base_Address : constant := 16#000e_0000_0000#;
   Sinfo_Page_Size    : constant
     := ((Musinfo.Subject_Info_Type_Size + (16#1000# - 1))
         / 16#1000#) * 16#1000#;

   Sinfo : Musinfo.Subject_Info_Type
   with
      Address => System'To_Address (Sinfo_Base_Address);

   Sched_Info : Muschedinfo.Scheduling_Info_Type
   with
      Volatile,
      Async_Writers,
      Address => System'To_Address (Sinfo_Base_Address + Sinfo_Page_Size);

   function Raw_Value_Min return T
   is
      TSC_Schedule_Start : constant Interfaces.Unsigned_64
         := Sched_Info.TSC_Schedule_Start;
   begin
      return T (TSC_Schedule_Start);
   end Raw_Value_Min;

   function Raw_Value_Max return T
   is
      TSC_Schedule_End : constant Interfaces.Unsigned_64
         := Sched_Info.TSC_Schedule_End;
   begin
      return T (TSC_Schedule_End);
   end Raw_Value_Max;

   function Hz return T is
   begin
      return T (Sinfo.TSC_Khz) * 1000;
   end Hz;

end HW.Time.Timer;
