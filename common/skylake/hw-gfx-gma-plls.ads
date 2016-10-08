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

private package HW.GFX.GMA.PLLs
with
   Abstract_State => (State with Part_Of => GMA.State)
is

   -- NOTE: Order of DPLLs is twisted, as DPLL2 (WRPLL1)
   --       should be selected as last choice.

   -- XXX: Types should be private (but that triggers a bug in SPARK GPL 2016)
   type T is (Invalid_PLL, DPLL0, DPLL1, DPLL3, DPLL2);
   subtype Configurable_DPLLs is T range DPLL1 .. DPLL2;
   Invalid : constant T := Invalid_PLL;

   procedure Initialize
   with
      Global => (Output => State);

   procedure Alloc
     (Port_Cfg : in     Port_Config;
      PLL      :    out T;
      Success  :    out Boolean);

   procedure Free (PLL : T);

   procedure All_Off;

   function Register_Value (PLL : T) return Word32;

end HW.GFX.GMA.PLLs;
