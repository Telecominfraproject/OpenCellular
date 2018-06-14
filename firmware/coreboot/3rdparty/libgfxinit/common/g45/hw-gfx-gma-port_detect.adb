--
-- Copyright (C) 2016-2017 secunet Security Networks AG
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
with HW.GFX.GMA.Config_Helpers;

package body HW.GFX.GMA.Port_Detect
is

   PORT_DETECTED                    : constant := 1 * 2 **  2;

   PORTB_HOTPLUG_INT_EN             : constant := 1 * 2 ** 29;
   PORTC_HOTPLUG_INT_EN             : constant := 1 * 2 ** 28;
   PORTD_HOTPLUG_INT_EN             : constant := 1 * 2 ** 27;
   SDVOB_HOTPLUG_INT_EN             : constant := 1 * 2 ** 26;
   SDVOC_HOTPLUG_INT_EN             : constant := 1 * 2 ** 25;
   CRT_HOTPLUG_INT_EN               : constant := 1 * 2 ** 9;
   CRT_HOTPLUG_ACTIVATION_PERIOD_64 : constant := 1 * 2 ** 8;

   type HDMI_Port_Value is array (GMCH_HDMI_Port) of Word32;
   type DP_Port_Value is array (GMCH_DP_Port) of Word32;
   HDMI_PORT_HOTPLUG_EN : constant HDMI_Port_Value :=
     (DIGI_B => SDVOB_HOTPLUG_INT_EN,
      DIGI_C => SDVOC_HOTPLUG_INT_EN);
   DP_PORT_HOTPLUG_EN : constant DP_Port_Value :=
     (DIGI_B => PORTB_HOTPLUG_INT_EN,
      DIGI_C => PORTC_HOTPLUG_INT_EN,
      DIGI_D => PORTD_HOTPLUG_INT_EN);

   type HDMI_Regs is array (GMCH_HDMI_Port) of Registers.Registers_Index;
   type DP_Regs is array (GMCH_DP_Port) of Registers.Registers_Index;
   GMCH_HDMI : constant HDMI_Regs :=
     (DIGI_B => Registers.GMCH_HDMIB,
      DIGI_C => Registers.GMCH_HDMIC);
   GMCH_DP : constant DP_Regs :=
     (DIGI_B => Registers.GMCH_DP_B,
      DIGI_C => Registers.GMCH_DP_C,
      DIGI_D => Registers.GMCH_DP_D);

   HOTPLUG_INT_STATUS : constant array (Active_Port_Type) of word32 :=
     (DP1    => 3 * 2 ** 17,
      DP2    => 3 * 2 ** 19,
      DP3    => 3 * 2 ** 21,
      HDMI1  => 1 * 2 **  2,
      HDMI2  => 1 * 2 **  3,
      Analog => 1 * 2 ** 11,
      others => 0);

   procedure Initialize
   is
      Detected : Boolean;
      hotplug_mask_set : Word32 :=
        CRT_HOTPLUG_INT_EN or CRT_HOTPLUG_ACTIVATION_PERIOD_64;

      To_HDMI_Port : constant array (GMCH_HDMI_Port) of Port_Type :=
        (DIGI_B => HDMI1,
         DIGI_C => HDMI2);
      To_DP_Port : constant array (GMCH_DP_Port) of Port_Type :=
        (DIGI_B => DP1,
         DIGI_C => DP2,
         DIGI_D => DP3);

   begin
      for HDMI_Port in GMCH_HDMI_Port loop
         Registers.Is_Set_Mask
           (Register => GMCH_HDMI (HDMI_Port),
            Mask     => PORT_DETECTED,
            Result   => Detected);
         Config.Valid_Port (To_HDMI_Port (HDMI_Port)) := Detected;
         hotplug_mask_set := hotplug_mask_set or
           (if Detected then HDMI_PORT_HOTPLUG_EN (HDMI_Port) else 0);
      end loop;
      for DP_Port in GMCH_DP_Port loop
         Registers.Is_Set_Mask
           (Register => GMCH_DP (DP_Port),
            Mask     => PORT_DETECTED,
            Result   => Detected);
         Config.Valid_Port (To_DP_Port (DP_Port)) := Detected;
         hotplug_mask_set := hotplug_mask_set or
           (if Detected then DP_PORT_HOTPLUG_EN (DP_Port) else 0);
      end loop;
      Registers.Write
        (Register => Registers.PORT_HOTPLUG_EN,
         Value => hotplug_mask_set);
   end Initialize;

   procedure Hotplug_Detect (Port : in Active_Port_Type; Detected : out Boolean)
   is
      Ctl32 : Word32;
   begin
      Registers.Read (Register => Registers.PORT_HOTPLUG_STAT,
                      Value    => Ctl32);
      Detected := (Ctl32 and HOTPLUG_INT_STATUS (Port)) /= 0;

      if Detected then
         registers.Set_Mask
           (Register => Registers.PORT_HOTPLUG_STAT,
            Mask     => HOTPLUG_INT_STATUS (Port));
      end if;
   end Hotplug_Detect;

   procedure Clear_Hotplug_Detect (Port : Active_Port_Type)
   is
      Ignored_HPD : Boolean;
   begin
      pragma Warnings (GNATprove, Off, "unused assignment to ""Ignored_HPD""",
                       Reason => "We want to clear pending events only");
      Port_Detect.Hotplug_Detect (Port, Ignored_HPD);
      pragma Warnings (GNATprove, On, "unused assignment to ""Ignored_HPD""");
   end Clear_Hotplug_Detect;

end HW.GFX.GMA.Port_Detect;
