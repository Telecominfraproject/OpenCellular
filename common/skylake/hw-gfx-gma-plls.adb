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

with HW.GFX.GMA.PLLs.DPLL_0;
with HW.GFX.GMA.PLLs.DPLL;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.PLLs
with
   Refined_State => (State => PLLs)
is

   type Count_Range is new Natural range 0 .. 2;

   type PLL_State is record
      Use_Count   : Count_Range;
      Used_For_DP : Boolean;
      Link_Rate   : DP_Bandwidth;
      Mode        : Mode_Type;
   end record;

   type PLL_State_Array is array (Configurable_DPLLs) of PLL_State;

   PLLs : PLL_State_Array;

   procedure Initialize
   is
   begin
      PLLs :=
        (Configurable_DPLLs =>
           (Use_Count   => 0,
            Used_For_DP => False,
            Link_Rate   => DP_Bandwidth'First,
            Mode        => Invalid_Mode));
   end Initialize;

   procedure Alloc_Configurable
     (Port_Cfg : in     Port_Config;
      PLL      :    out T;
      Success  :    out Boolean)
   with
      Pre => True
   is
      function Config_Matches (PE : PLL_State) return Boolean
      is
      begin
         return
            PE.Used_For_DP = (Port_Cfg.Display = DP) and
            ((PE.Used_For_DP and PE.Link_Rate = Port_Cfg.DP.Bandwidth) or
             (not PE.Used_For_DP and PE.Mode = Port_Cfg.Mode));
      end Config_Matches;
   begin
      -- try to find shareable PLL
      for P in Configurable_DPLLs loop
         Success := PLLs (P).Use_Count /= 0 and
                     PLLs (P).Use_Count /= Count_Range'Last and
                     Config_Matches (PLLs (P));
         if Success then
            PLL := P;
            PLLs (PLL).Use_Count := PLLs (PLL).Use_Count + 1;
            return;
         end if;
      end loop;

      -- try to find free PLL
      for P in Configurable_DPLLs loop
         if PLLs (P).Use_Count = 0 then
            PLL := P;
            DPLL.On (PLL, Port_Cfg, Success);
            if Success then
               PLLs (PLL) :=
                 (Use_Count   => 1,
                  Used_For_DP => Port_Cfg.Display = DP,
                  Link_Rate   => Port_Cfg.DP.Bandwidth,
                  Mode        => Port_Cfg.Mode);
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

      if Port_Cfg.Port = DIGI_A then
         DPLL_0.Check_Link_Rate (Port_Cfg.DP.Bandwidth, Success);
      else
         Success := False;
      end if;

      if Success then
         PLL := DPLL0;
      else
         Alloc_Configurable (Port_Cfg, PLL, Success);
      end if;
   end Alloc;

   procedure Free (PLL : T)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if PLL in Configurable_DPLLs then
         if PLLs (PLL).Use_Count /= 0 then
            PLLs (PLL).Use_Count := PLLs (PLL).Use_Count - 1;
            if PLLs (PLL).Use_Count = 0 then
               DPLL.Off (PLL);
            end if;
         end if;
      end if;
   end Free;

   procedure All_Off
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      for PLL in Configurable_DPLLs loop
         DPLL.Off (PLL);
      end loop;
   end All_Off;

   function Register_Value (PLL : T) return Word32
   is
   begin
      return
        (if    PLL = DPLL0                then DPLL_0.Register_Value
         elsif PLL in Configurable_DPLLs  then DPLL.Register_Value (PLL)
         else 0);
   end Register_Value;

end HW.GFX.GMA.PLLs;
