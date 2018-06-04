--
-- Copyright (C) 2015-2016 secunet Security Networks AG
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--

with HW.GFX.GMA.PLLs.LCPLL;
with HW.GFX.GMA.PLLs.WRPLL;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.PLLs
with
   Refined_State => (State => PLLs)
is

   type Count_Range is new Natural range 0 .. 2;

   type PLL_State is record
      Use_Count   : Count_Range;
      Mode        : Mode_Type;
   end record;

   type PLL_State_Array is array (WRPLLs) of PLL_State;

   PLLs : PLL_State_Array;

   procedure Initialize
   is
   begin
      PLLs := (WRPLLs => (Use_Count => 0, Mode => Invalid_Mode));
   end Initialize;

   procedure Alloc_Configurable
     (Mode     : in     Mode_Type;
      PLL      :    out T;
      Success  :    out Boolean)
   with
      Pre => True
   is
   begin
      -- try to find shareable PLL
      for P in WRPLLs loop
         Success := PLLs (P).Use_Count /= 0 and
                     PLLs (P).Use_Count /= Count_Range'Last and
                     PLLs (P).Mode = Mode;
         if Success then
            PLL := P;
            PLLs (PLL).Use_Count := PLLs (PLL).Use_Count + 1;
            return;
         end if;
      end loop;

      -- try to find free PLL
      for P in WRPLLs loop
         if PLLs (P).Use_Count = 0 then
            PLL := P;
            WRPLL.On (PLL, Mode.Dotclock, Success);
            if Success then
               PLLs (PLL) := (Use_Count => 1, Mode => Mode);
            end if;
            return;
         end if;
      end loop;

      PLL := Invalid;
   end Alloc_Configurable;

   procedure Alloc
     (Port_Cfg : in     Port_Config;
      PLL      :    out T;
      Success  :    out Boolean)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Port = DIGI_E then
         PLL := Invalid;
         Success := True;
      elsif Port_Cfg.Display = DP then
         PLL := LCPLL.Fixed_LCPLLs (Port_Cfg.DP.Bandwidth);
         Success := True;
      else
         Alloc_Configurable (Port_Cfg.Mode, PLL, Success);
      end if;
   end Alloc;

   procedure Free (PLL : T)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if PLL in WRPLLs then
         if PLLs (PLL).Use_Count /= 0 then
            PLLs (PLL).Use_Count := PLLs (PLL).Use_Count - 1;
            if PLLs (PLL).Use_Count = 0 then
               WRPLL.Off (PLL);
            end if;
         end if;
      end if;
   end Free;

   procedure All_Off
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      for PLL in WRPLLs loop
         WRPLL.Off (PLL);
      end loop;
   end All_Off;

   function Register_Value (PLL : T) return Word32
   is
   begin
      return
        (if    PLL in LCPLLs then LCPLL.Register_Value (PLL)
         elsif PLL in WRPLLs then WRPLL.Register_Value (PLL)
         else  0);
   end Register_Value;

end HW.GFX.GMA.PLLs;
