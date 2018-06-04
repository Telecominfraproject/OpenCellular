--
-- Copyright (C) 2015-2016 secunet Security Networks AG
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

use type HW.Word64;

package body HW.GFX.GMA.GMCH.VGA is

   ADPA_DAC_ENABLE         : constant := 1 * 2 ** 31;
   ADPA_USE_VGA_HVPOLARITY : constant := 1 * 2 ** 15;
   ADPA_VSYNC_DISABLE      : constant := 1 * 2 ** 11;
   ADPA_HSYNC_DISABLE      : constant := 1 * 2 ** 10;
   ADPA_VSYNC_ACTIVE_HIGH  : constant := 1 * 2 **  4;
   ADPA_HSYNC_ACTIVE_HIGH  : constant := 1 * 2 **  3;

   ADPA_MASK : constant Word32 :=
      GMCH_PORT_PIPE_SELECT_MASK or
      ADPA_DAC_ENABLE        or
      ADPA_VSYNC_DISABLE     or
      ADPA_HSYNC_DISABLE     or
      ADPA_VSYNC_ACTIVE_HIGH or
      ADPA_HSYNC_ACTIVE_HIGH or
      ADPA_USE_VGA_HVPOLARITY;

   ----------------------------------------------------------------------------

   procedure On
     (Pipe     : Pipe_Index;
      Mode     : in Mode_Type)
   is
      Polarity : Word32 := 0;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Mode.H_Sync_Active_High then
         Polarity := Polarity or ADPA_HSYNC_ACTIVE_HIGH;
      end if;
      if Mode.V_Sync_Active_High then
         Polarity := Polarity or ADPA_VSYNC_ACTIVE_HIGH;
      end if;

      Registers.Unset_And_Set_Mask
        (Register    => Registers.GMCH_ADPA,
         Mask_Unset  => ADPA_MASK,
         Mask_Set    => ADPA_DAC_ENABLE or
                        GMCH_PORT_PIPE_SELECT (Pipe) or
                        Polarity);
   end On;

   ----------------------------------------------------------------------------

   procedure Off
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Unset_And_Set_Mask
        (Register    => Registers.GMCH_ADPA,
         Mask_Unset  => ADPA_DAC_ENABLE,
         Mask_Set    => ADPA_HSYNC_DISABLE or
                        ADPA_VSYNC_DISABLE);
   end Off;

end HW.GFX.GMA.GMCH.VGA;
