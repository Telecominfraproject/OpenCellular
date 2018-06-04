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

with HW.GFX.GMA.Registers;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.GMCH.HDMI
is

   GMCH_HDMI_ENABLE               : constant := 1 * 2 ** 31;
   GMCH_HDMI_COLOR_FORMAT_8BPC    : constant := 0 * 2 ** 26;
   GMCH_HDMI_COLOR_FORMAT_12BPC   : constant := 3 * 2 ** 26;
   GMCH_HDMI_COLOR_FORMAT_MASK    : constant := 7 * 2 ** 26;
   GMCH_HDMI_SDVO_ENCODING_SDVO   : constant := 0 * 2 ** 10;
   GMCH_HDMI_SDVO_ENCODING_HDMI   : constant := 2 * 2 ** 10;
   GMCH_HDMI_SDVO_ENCODING_MASK   : constant := 3 * 2 ** 10;
   GMCH_HDMI_MODE_SELECT_HDMI     : constant := 1 * 2 **  9;
   GMCH_HDMI_MODE_SELECT_DVI      : constant := 0 * 2 **  9;
   GMCH_HDMI_VSYNC_ACTIVE_HIGH    : constant := 1 * 2 **  4;
   GMCH_HDMI_HSYNC_ACTIVE_HIGH    : constant := 1 * 2 **  3;
   GMCH_HDMI_PORT_DETECT          : constant := 1 * 2 **  2;

   GMCH_HDMI_MASK : constant Word32 :=
     GMCH_PORT_PIPE_SELECT_MASK or
     GMCH_HDMI_ENABLE or
     GMCH_HDMI_COLOR_FORMAT_MASK or
     GMCH_HDMI_SDVO_ENCODING_MASK or
     GMCH_HDMI_MODE_SELECT_HDMI or
     GMCH_HDMI_HSYNC_ACTIVE_HIGH or
     GMCH_HDMI_VSYNC_ACTIVE_HIGH;

   type GMCH_HDMI_Array is array (GMCH_HDMI_Port) of Registers.Registers_Index;
   GMCH_HDMI : constant GMCH_HDMI_Array := GMCH_HDMI_Array'
     (DIGI_B => Registers.GMCH_HDMIB,
      DIGI_C => Registers.GMCH_HDMIC);

   ----------------------------------------------------------------------------

   procedure On (Port_Cfg : in Port_Config;
                 Pipe     : in Pipe_Index)
   is
      Polarity : constant Word32 :=
        (if Port_Cfg.Mode.H_Sync_Active_High then
            GMCH_HDMI_HSYNC_ACTIVE_HIGH else 0) or
        (if Port_Cfg.Mode.V_Sync_Active_High then
            GMCH_HDMI_VSYNC_ACTIVE_HIGH else 0);
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Unset_And_Set_Mask
         (Register   => GMCH_HDMI (Port_Cfg.Port),
          Mask_Unset => GMCH_HDMI_MASK,
          Mask_Set   => GMCH_HDMI_ENABLE or
                        GMCH_PORT_PIPE_SELECT (Pipe) or
                        GMCH_HDMI_SDVO_ENCODING_HDMI or
                        GMCH_HDMI_MODE_SELECT_DVI or
                        Polarity);
   end On;

   ----------------------------------------------------------------------------

   procedure Off (Port : GMCH_HDMI_Port)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Unset_And_Set_Mask
         (Register   => GMCH_HDMI (Port),
          Mask_Unset => GMCH_HDMI_MASK,
          Mask_Set   => GMCH_HDMI_HSYNC_ACTIVE_HIGH or
                        GMCH_HDMI_VSYNC_ACTIVE_HIGH);
   end Off;

   procedure All_Off
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      for Port in GMCH_HDMI_Port loop
         Off (Port);
      end loop;
   end All_Off;

end HW.GFX.GMA.GMCH.HDMI;
