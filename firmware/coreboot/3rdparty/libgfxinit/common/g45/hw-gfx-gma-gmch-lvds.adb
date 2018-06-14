--
-- Copyright (C) 2015-2016 secunet Security Networks AG
-- Copyright (C) 2016 Nico Huber <nico.h@gmx.de>
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

with HW.GFX.GMA.Config;
with HW.GFX.GMA.Registers;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.GMCH.LVDS is

   LVDS_ENABLE                     : constant :=  1 * 2 ** 31;
   LVDS_DITHER_EN                  : constant :=  1 * 2 ** 25;
   LVDS_VSYNC_POLARITY_INVERT      : constant :=  1 * 2 ** 21;
   LVDS_HSYNC_POLARITY_INVERT      : constant :=  1 * 2 ** 20;
   LVDS_CLK_A_DATA_A0A2_POWER_MASK : constant :=  3 * 2 **  8;
   LVDS_CLK_A_DATA_A0A2_POWER_DOWN : constant :=  0 * 2 **  8;
   LVDS_CLK_A_DATA_A0A2_POWER_UP   : constant :=  3 * 2 **  8;
   LVDS_CLK_B_POWER_MASK           : constant :=  3 * 2 **  4;
   LVDS_CLK_B_POWER_DOWN           : constant :=  0 * 2 **  4;
   LVDS_CLK_B_POWER_UP             : constant :=  3 * 2 **  4;
   LVDS_DATA_B0B2_POWER_MASK       : constant :=  3 * 2 **  2;
   LVDS_DATA_B0B2_POWER_DOWN       : constant :=  0 * 2 **  2;
   LVDS_DATA_B0B2_POWER_UP         : constant :=  3 * 2 **  2;

   ----------------------------------------------------------------------------

   procedure On (Port_Cfg : in Port_Config;
                 Pipe     : in Pipe_Index)
   is
      Sync_Polarity : constant Word32 :=
        (if Port_Cfg.Mode.H_Sync_Active_High then 0
         else LVDS_HSYNC_POLARITY_INVERT) or
        (if Port_Cfg.Mode.V_Sync_Active_High then 0
         else LVDS_VSYNC_POLARITY_INVERT);

      Two_Channel : constant Word32 :=
        (if Port_Cfg.Mode.Dotclock >= Config.LVDS_Dual_Threshold then
            LVDS_CLK_B_POWER_UP or LVDS_DATA_B0B2_POWER_UP else 0);
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));
      pragma Debug (Port_Cfg.Mode.BPC /= 6, Debug.Put_Line
        ("WARNING: Only 18bpp LVDS mode implemented."));

      Registers.Write
        (Register => Registers.GMCH_LVDS,
         Value    => LVDS_ENABLE or
                     GMCH_PORT_PIPE_SELECT(Pipe) or
                     Sync_Polarity or
                     LVDS_CLK_A_DATA_A0A2_POWER_UP or
                     Two_Channel or
                     LVDS_DITHER_EN);
   end On;

   ----------------------------------------------------------------------------

   procedure Off
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Write
        (Register => Registers.GMCH_LVDS,
         Value    => LVDS_CLK_A_DATA_A0A2_POWER_DOWN or
                     LVDS_CLK_B_POWER_DOWN or
                     LVDS_DATA_B0B2_POWER_DOWN);
   end Off;

end HW.GFX.GMA.GMCH.LVDS;
