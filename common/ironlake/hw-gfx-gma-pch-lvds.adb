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

with HW.GFX.GMA.Config;
with HW.GFX.GMA.Registers;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.PCH.LVDS is

   PCH_LVDS_ENABLE               : constant :=  1 * 2 ** 31;
   PCH_LVDS_TWO_CHANNEL          : constant := 15 * 2 **  2;

   PCH_LVDS_MASK : constant Word32 :=
      PCH_TRANSCODER_SELECT_MASK or
      PCH_LVDS_ENABLE or
      PCH_LVDS_TWO_CHANNEL;

   ----------------------------------------------------------------------------

   procedure On (Port_Cfg : Port_Config; FDI_Port : FDI_Port_Type)
   is
      Two_Channel : constant Word32 :=
        (if Port_Cfg.Mode.Dotclock >= Config.LVDS_Dual_Threshold then
            PCH_LVDS_TWO_CHANNEL else 0);
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Unset_And_Set_Mask
         (Register   => Registers.PCH_LVDS,
          Mask_Unset => PCH_LVDS_MASK,
          Mask_Set   => PCH_LVDS_ENABLE or
                        PCH_TRANSCODER_SELECT (FDI_Port) or
                        Two_Channel);
   end On;

   ----------------------------------------------------------------------------

   procedure Off
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Unset_Mask (Registers.PCH_LVDS, PCH_LVDS_ENABLE);
   end Off;

end HW.GFX.GMA.PCH.LVDS;
