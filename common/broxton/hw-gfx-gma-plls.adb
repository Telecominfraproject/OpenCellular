--
-- Copyright (C) 2017 secunet Security Networks AG
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

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.PLLs
with
   Refined_State => (State => null)
is

   procedure Alloc
     (Port_Cfg : in     Port_Config;
      PLL      :    out T;
      Success  :    out Boolean)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      case Port_Cfg.Port is
         when DIGI_A => PLL := DPLL_A;
         when DIGI_B => PLL := DPLL_B;
         when DIGI_C => PLL := DPLL_C;
         when others => PLL := Invalid_PLL;
      end case;

      Success := PLL /= Invalid_PLL;
   end Alloc;

   procedure Free (PLL : T) is
   begin
      null; -- FIXME
   end Free;

   procedure All_Off is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      null; -- FIXME
   end All_Off;

   function Register_Value (PLL : T) return Word32 is
   begin
      return 0;   -- FIXME
   end Register_Value;

end HW.GFX.GMA.PLLs;
