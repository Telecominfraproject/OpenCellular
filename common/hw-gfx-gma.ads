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

with HW.Time;
with HW.Port_IO;

package HW.GFX.GMA
with
   Abstract_State =>
     (State,
      Init_State,
      Config_State,
      (Device_State with External)),
   Initializes =>
     (Init_State,
      Config_State)
is

   type CPU_Type is
     (Ironlake,
      Sandybridge,
      Ivybridge,
      Haswell,
      Broadwell,
      Skylake);

   type CPU_Variant is (Normal, ULT);

   type Port_Type is
     (Disabled,
      Internal,
      DP1,
      DP2,
      DP3,
      Digital1,
      Digital2,
      Digital3,
      Analog);
   type Port_List_Range is range 0 .. 7;
   type Port_List is array (Port_List_Range) of Port_Type;

   type Pipe_Config is record
      Port        : Port_Type;
      Framebuffer : Framebuffer_Type;
      Mode        : Mode_Type;
   end record;
   type Pipe_Index is (Primary, Secondary, Tertiary);
   type Pipe_Configs is array (Pipe_Index) of Pipe_Config;

   -- Special framebuffer offset to indicate legacy VGA plane.
   -- Only valid on primary pipe.
   VGA_PLANE_FRAMEBUFFER_OFFSET : constant := 16#ffff_ffff#;

   procedure Initialize
     (MMIO_Base   : in     Word64 := 0;
      Write_Delay : in     Word64 := 0;
      Success     :    out Boolean)
   with
      Global =>
        (In_Out => (Config_State, Device_State, Port_IO.State),
         Output => (State, Init_State),
         Input  => (Time.State)),
      Post => Success = Is_Initialized;
   function Is_Initialized return Boolean
   with
      Global => (Input => Init_State);

   procedure Legacy_VGA_Off;

   procedure Scan_Ports
     (Configs  :    out Pipe_Configs;
      Ports    : in     Port_List;
      Max_Pipe : in     Pipe_Index := Pipe_Index'Last);
   procedure Auto_Configure
     (Configs     : in out Pipe_Configs;
      Keep_Power  : in     Boolean := False);
   procedure Update_Outputs (Configs : Pipe_Configs);

   pragma Warnings (GNATprove, Off, "subprogram ""Dump_Configs"" has no effect",
                    Reason => "It's only used for debugging");
   procedure Dump_Configs (Configs : Pipe_Configs);

   type GTT_Address_Type is mod 2 ** 39;
   type GTT_Range is range 0 .. 16#8_0000# - 1;
   procedure Write_GTT
     (GTT_Page       : GTT_Range;
      Device_Address : GTT_Address_Type;
      Valid          : Boolean);

   procedure Setup_Default_GTT (FB : Framebuffer_Type; Phys_FB : Word32);

private

   type GPU_Port is (DIGI_A, DIGI_B, DIGI_C, DIGI_D, DIGI_E);

   subtype Digital_Port is GPU_Port range DIGI_A .. DIGI_E;

   type PCH_Port is
     (PCH_DAC, PCH_LVDS,
      PCH_HDMI_B, PCH_HDMI_C, PCH_HDMI_D,
      PCH_DP_B, PCH_DP_C, PCH_DP_D);

   subtype PCH_HDMI_Port is PCH_Port range PCH_HDMI_B .. PCH_HDMI_D;
   subtype PCH_DP_Port is PCH_Port range PCH_DP_B .. PCH_DP_D;

   type Port_Config is
      record
         Port     : GPU_Port;
         PCH_Port : GMA.PCH_Port;
         Display  : Display_Type;
         Mode     : Mode_Type;
         Is_FDI   : Boolean;
         FDI      : DP_Link;
         DP       : DP_Link;
      end record;

   type FDI_Training_Type is (Simple_Training, Full_Training, Auto_Training);

   ----------------------------------------------------------------------------

   type DP_Port is (DP_A, DP_B, DP_C, DP_D);

   ----------------------------------------------------------------------------

   VGA_SR_INDEX   : constant Port_IO.Port_Type  := 16#03c4#;
   VGA_SR_DATA    : constant Port_IO.Port_Type  := 16#03c5#;
   VGA_SR01       : constant Word8 := 16#01#;

end HW.GFX.GMA;
