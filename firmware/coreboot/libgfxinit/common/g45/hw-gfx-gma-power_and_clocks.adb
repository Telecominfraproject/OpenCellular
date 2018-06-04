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

with HW.Time;
with HW.GFX.GMA.Config;
with HW.GFX.GMA.Registers;

package body HW.GFX.GMA.Power_And_Clocks is

   FSB_FREQ_SEL_MASK : constant := 7 * 2 ** 0;
   CLKCFG_FSB_400    : constant Frequency_Type := 100_000_000;
   CLKCFG_FSB_533    : constant Frequency_Type := 133_333_333;
   CLKCFG_FSB_667    : constant Frequency_Type := 166_666_666;
   CLKCFG_FSB_800    : constant Frequency_Type := 200_000_000;
   CLKCFG_FSB_1067   : constant Frequency_Type := 266_666_666;
   CLKCFG_FSB_1333   : constant Frequency_Type := 333_333_333;

   -- The Raw Freq is 1/4 of the FSB freq
   procedure Initialize
   is
      CLK_CFG : Word32;
      type Freq_Sel is new Natural range 0 .. 7;
   begin
      Registers.Read
        (Register => Registers.GMCH_CLKCFG,
         Value => CLK_CFG);
      case Freq_Sel (CLK_CFG and FSB_FREQ_SEL_MASK) is
         when 0      => Config.Raw_Clock := CLKCFG_FSB_1067;
         when 1      => Config.Raw_Clock := CLKCFG_FSB_533;
         when 2      => Config.Raw_Clock := CLKCFG_FSB_800;
         when 3      => Config.Raw_Clock := CLKCFG_FSB_667;
         when 4      => Config.Raw_Clock := CLKCFG_FSB_1333;
         when 5      => Config.Raw_Clock := CLKCFG_FSB_400;
         when 6      => Config.Raw_Clock := CLKCFG_FSB_1067;
         when 7      => Config.Raw_Clock := CLKCFG_FSB_1333;
      end case;
   end Initialize;

end HW.GFX.GMA.Power_And_Clocks;
