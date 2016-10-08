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

with HW;
with HW.Time.Timer;

use type HW.Word64;

package body HW.Time
   with Refined_State => (State => (Timer.Timer_State, Timer.Abstract_Time))
is

   function Now return T
   with
      Refined_Global => (Input => Timer.Abstract_Time)
   is
      Current : constant T := Timer.Raw_Value_Min;
   begin
      return Current;
   end Now;

   function US_From_Now (US : Natural) return T
   with
      Refined_Global => (Input => (Timer.Abstract_Time, Timer.Timer_State))
   is
      Current : constant T := Timer.Raw_Value_Max;
   begin
      return Current + (T (US) * Timer.Hz + 999_999) / 1_000_000;
   end US_From_Now;

   function MS_From_Now (MS : Natural) return T
   with
      Refined_Global => (Input => (Timer.Abstract_Time, Timer.Timer_State))
   is
      Current : constant T := Timer.Raw_Value_Max;
   begin
      return Current + (T (MS) * Timer.Hz + 999) / 1_000;
   end MS_From_Now;

   function Now_US return Int64
   with
      Refined_Global => (Input => (Timer.Abstract_Time, Timer.Timer_State))
   is
      MHz : constant T := Timer.Hz / 1_000_000;
      Current : constant T := Timer.Raw_Value_Min;
   begin
      return Int64 ((Current and (2 ** 63 - 1)) / (if MHz = 0 then 1 else MHz));
   end Now_US;

   ----------------------------------------------------------------------------

   procedure Delay_Until (Deadline : T)
   with
      Refined_Global => (Input => (Timer.Abstract_Time))
   is
      Current: T;
   begin
      loop
         Current := Timer.Raw_Value_Min;
         exit when Current >= Deadline;
      end loop;
   end Delay_Until;

   procedure U_Delay (US : Natural)
   with
      Refined_Global => (Input => (Timer.Abstract_Time, Timer.Timer_State))
   is
      Deadline : constant T := US_From_Now (US);
   begin
      Delay_Until (Deadline);
   end U_Delay;

   procedure M_Delay (MS : Natural)
   with
      Refined_Global => (Input => (Timer.Abstract_Time, Timer.Timer_State))
   is
      Deadline : constant T := MS_From_Now (MS);
   begin
      Delay_Until (Deadline);
   end M_Delay;

   ----------------------------------------------------------------------------

   function Timed_Out (Deadline : T) return Boolean
   with
      Refined_Global => (Input => (Timer.Abstract_Time))
   is
      Current : constant T := Timer.Raw_Value_Min;
   begin
      return Current >= Deadline;
   end Timed_Out;

end HW.Time;

--  vim: set ts=8 sts=3 sw=3 et:
