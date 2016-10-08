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

private package HW.GFX.GMA.PLLs.DPLL_0 is

   Register_Value : constant Word32 := 0;

   procedure Check_Link_Rate
     (Link_Rate   : in     DP_Bandwidth;
      Success     :    out Boolean);

end HW.GFX.GMA.PLLs.DPLL_0;
