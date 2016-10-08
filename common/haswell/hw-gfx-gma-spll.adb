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

with HW.Time;
with HW.GFX.GMA.Registers;

package body HW.GFX.GMA.SPLL is

   SPLL_CTL_PLL_ENABLE        : constant := 1 * 2 ** 31;
   SPLL_CTL_REF_SEL_MASK      : constant := 3 * 2 ** 28;
   SPLL_CTL_REF_SEL_SSC       : constant := 1 * 2 ** 28;
   SPLL_CTL_REF_SEL_NON_SSC   : constant := 2 * 2 ** 28;
   SPLL_CTL_FREQ_SEL_MASK     : constant := 3 * 2 ** 26;
   SPLL_CTL_FREQ_SEL_810      : constant := 0 * 2 ** 26;
   SPLL_CTL_FREQ_SEL_1350     : constant := 1 * 2 ** 26;

   procedure On is
   begin
      Registers.Write
        (Register => Registers.SPLL_CTL,
         Value    => SPLL_CTL_PLL_ENABLE or
                     SPLL_CTL_REF_SEL_SSC or
                     SPLL_CTL_FREQ_SEL_1350);
      Registers.Posting_Read (Registers.SPLL_CTL);
      Time.U_Delay (20);
   end On;

   procedure Off is
   begin
      Registers.Unset_Mask (Registers.SPLL_CTL, SPLL_CTL_PLL_ENABLE);
   end Off;

end HW.GFX.GMA.SPLL;
