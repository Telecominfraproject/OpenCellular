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

private package HW.GFX.GMA.PLLs.LCPLL
is

   type Fixed_LCPLLs_Array is array (HW.GFX.DP_Bandwidth) of LCPLLs;

   Fixed_LCPLLs : constant Fixed_LCPLLs_Array := Fixed_LCPLLs_Array'
     (DP_Bandwidth_5_4  => LCPLL0,
      DP_Bandwidth_2_7  => LCPLL1,
      DP_Bandwidth_1_62 => LCPLL2);

   type Value_Array is array (LCPLLs) of Word32;
   Register_Value : constant Value_Array := Value_Array'
     (LCPLL0 => 0 * 2 ** 29, LCPLL1 => 1 * 2 ** 29, LCPLL2 => 2 * 2 ** 29);

end HW.GFX.GMA.PLLs.LCPLL;
