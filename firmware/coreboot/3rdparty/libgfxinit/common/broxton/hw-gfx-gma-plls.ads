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

private package HW.GFX.GMA.PLLs
with
   Abstract_State => (State with Part_Of => GMA.State)
is

   -- Broxton DDI PHY PLLs are tied to the port.
   -- So the allocation will be a stub.

   -- XXX: Types should be private (but that triggers a bug in SPARK GPL 2016)
   type T is (Invalid_PLL, DPLL_A, DPLL_B, DPLL_C);
   Invalid : constant T := Invalid_PLL;

   procedure Initialize is null
   with
      Global => (Output => State);

   procedure Alloc
     (Port_Cfg : in     Port_Config;
      PLL      :    out T;
      Success  :    out Boolean);

   procedure Free (PLL : T);

   procedure All_Off;

   -- Just an interface stub since PLLs are tied to the port.
   type Word_Array is array (T) of Word32;
   Register_Value : constant Word_Array := (others => 0);

end HW.GFX.GMA.PLLs;
