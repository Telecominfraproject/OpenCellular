--
-- Copyright (C) 2016 secunet Security Networks AG
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

with HW.GFX.GMA.Registers;

package body HW.GFX.GMA.PLLs.DPLL_0 is

   DPLL_CTRL1_DPLL0_LINK_RATE_MASK     : constant := 7 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_2700MHZ  : constant := 0 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_1350MHZ  : constant := 1 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_810MHZ   : constant := 2 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_1620MHZ  : constant := 3 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_1080MHZ  : constant := 4 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_2160MHZ  : constant := 5 * 2 **  1;
   DPLL_CTRL1_DPLL0_OVERRIDE           : constant := 1 * 2 **  0;

   procedure Check_Link_Rate
     (Link_Rate   : in     DP_Bandwidth;
      Success     :    out Boolean)
   is
      DPLL_Ctrl1 : Word32;
   begin
      Registers.Read (Registers.DPLL_CTRL1, DPLL_Ctrl1);

      case DPLL_Ctrl1 and DPLL_CTRL1_DPLL0_LINK_RATE_MASK is
         when DPLL_CTRL1_DPLL0_LINK_RATE_2700MHZ =>
            Success := Link_Rate = DP_Bandwidth_5_4;
         when DPLL_CTRL1_DPLL0_LINK_RATE_1350MHZ =>
            Success := Link_Rate = DP_Bandwidth_2_7;
         when DPLL_CTRL1_DPLL0_LINK_RATE_810MHZ =>
            Success := Link_Rate = DP_Bandwidth_1_62;
         when others =>
            Success := False;
      end case;
      Success := Success and (DPLL_Ctrl1 and DPLL_CTRL1_DPLL0_OVERRIDE) /= 0;
   end Check_Link_Rate;

end HW.GFX.GMA.PLLs.DPLL_0;
