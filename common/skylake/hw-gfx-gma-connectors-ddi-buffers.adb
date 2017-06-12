--
-- Copyright (C) 2017 secunet Security Networks AG
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

package body HW.GFX.GMA.Connectors.DDI.Buffers
is

   subtype Skylake_HDMI_Range is DDI_HDMI_Buf_Trans_Range range 0 .. 10;

   type HDMI_Buf_Trans is record
      Trans1 : Word32;
      Trans2 : Word32;
   end record;
   type HDMI_Buf_Trans_Array is array (Skylake_HDMI_Range) of HDMI_Buf_Trans;

   ----------------------------------------------------------------------------

   Skylake_Trans_EDP : constant Buf_Trans_Array :=
     (16#0000_0018#, 16#0000_00a8#,
      16#0000_4013#, 16#0000_00a9#,
      16#0000_7011#, 16#0000_00a2#,
      16#0000_9010#, 16#0000_009c#,
      16#0000_0018#, 16#0000_00a9#,
      16#0000_6013#, 16#0000_00a2#,
      16#0000_7011#, 16#0000_00a6#,
      16#0000_0018#, 16#0000_00ab#,
      16#0000_7013#, 16#0000_009f#,
      16#0000_0018#, 16#0000_00df#);

   Skylake_U_Trans_EDP : constant Buf_Trans_Array :=
     (16#0000_0018#, 16#0000_00a8#,
      16#0000_4013#, 16#0000_00a9#,
      16#0000_7011#, 16#0000_00a2#,
      16#0000_9010#, 16#0000_009c#,
      16#0000_0018#, 16#0000_00a9#,
      16#0000_6013#, 16#0000_00a2#,
      16#0000_7011#, 16#0000_00a6#,
      16#0000_2016#, 16#0000_00ab#,
      16#0000_5013#, 16#0000_009f#,
      16#0000_0018#, 16#0000_00df#);

   Skylake_Trans_DP : constant Buf_Trans_Array :=
     (16#0000_2016#, 16#0000_00a0#,
      16#0000_5012#, 16#0000_009b#,
      16#0000_7011#, 16#0000_0088#,
      16#8000_9010#, 16#0000_00c0#,
      16#0000_2016#, 16#0000_009b#,
      16#0000_5012#, 16#0000_0088#,
      16#8000_7011#, 16#0000_00c0#,
      16#0000_2016#, 16#0000_00df#,
      16#8000_5012#, 16#0000_00c0#,
      others => 0);

   Skylake_U_Trans_DP : constant Buf_Trans_Array :=
     (16#0000_201b#, 16#0000_00a2#,
      16#0000_5012#, 16#0000_0088#,
      16#8000_7011#, 16#0000_00cd#,
      16#8000_9010#, 16#0000_00c0#,
      16#0000_201b#, 16#0000_009d#,
      16#8000_5012#, 16#0000_00c0#,
      16#8000_7011#, 16#0000_00c0#,
      16#0000_2016#, 16#0000_0088#,
      16#8000_5012#, 16#0000_00c0#,
      others => 0);

   Skylake_Trans_HDMI : constant HDMI_Buf_Trans_Array :=
     ((16#0000_0018#, 16#0000_00ac#),
      (16#0000_5012#, 16#0000_009d#),
      (16#0000_7011#, 16#0000_0088#),
      (16#0000_0018#, 16#0000_00a1#),
      (16#0000_0018#, 16#0000_0098#),
      (16#0000_4013#, 16#0000_0088#),
      (16#8000_6012#, 16#0000_00cd#),
      (16#0000_0018#, 16#0000_00df#),
      (16#8000_3015#, 16#0000_00cd#),
      (16#8000_3015#, 16#0000_00c0#),
      (16#8000_0018#, 16#0000_00c0#));

   ----------------------------------------------------------------------------

   procedure Translations (Trans : out Buf_Trans_Array; Port : Digital_Port)
   is
      DDIA_Low_Voltage_Swing : constant Boolean :=
         Config.EDP_Low_Voltage_Swing and then Port = DIGI_A;

      HDMI_Trans : constant Skylake_HDMI_Range :=
        (if Config.DDI_HDMI_Buffer_Translation in Skylake_HDMI_Range
         then Config.DDI_HDMI_Buffer_Translation
         else Config.Default_DDI_HDMI_Buffer_Translation);
   begin
      Trans :=
        (case Config.CPU_Var is
            when Normal =>
              (if DDIA_Low_Voltage_Swing
               then Skylake_Trans_EDP
               else Skylake_Trans_DP),
            when ULT =>
              (if DDIA_Low_Voltage_Swing
               then Skylake_U_Trans_EDP
               else Skylake_U_Trans_DP));
      if not DDIA_Low_Voltage_Swing then
         Trans (18) := Skylake_Trans_HDMI (HDMI_Trans).Trans1;
         Trans (19) := Skylake_Trans_HDMI (HDMI_Trans).Trans2;
      end if;
   end Translations;

end HW.GFX.GMA.Connectors.DDI.Buffers;
