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

private package HW.GFX.GMA.PLLs.DPLL is

   type Value_Array is array (Configurable_DPLLs) of Word32;
   Register_Value : constant Value_Array := Value_Array'
     (DPLL1 => 1, DPLL2 => 2, DPLL3 => 3);

   procedure On
     (PLL      : in     Configurable_DPLLs;
      Port_Cfg : in     Port_Config;
      Success  :    out Boolean);

   procedure Off (PLL : Configurable_DPLLs);

end HW.GFX.GMA.PLLs.DPLL;
