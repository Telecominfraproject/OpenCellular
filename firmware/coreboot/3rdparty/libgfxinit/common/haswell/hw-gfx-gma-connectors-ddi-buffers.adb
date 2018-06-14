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

   subtype Haswell_HDMI_Range is DDI_HDMI_Buf_Trans_Range range 0 .. 11;
   subtype Broadwell_HDMI_Range is DDI_HDMI_Buf_Trans_Range range 0 .. 9;

   type HDMI_Buf_Trans is record
      Trans1 : Word32;
      Trans2 : Word32;
   end record;
   type HDMI_Buf_Trans_Array is array (Haswell_HDMI_Range) of HDMI_Buf_Trans;

   ----------------------------------------------------------------------------

   Haswell_Trans_DP : constant Buf_Trans_Array :=
     (16#00ff_ffff#, 16#0006_000e#,
      16#00d7_5fff#, 16#0005_000a#,
      16#00c3_0fff#, 16#0004_0006#,
      16#80aa_afff#, 16#000b_0000#,
      16#00ff_ffff#, 16#0005_000a#,
      16#00d7_5fff#, 16#000c_0004#,
      16#80c3_0fff#, 16#000b_0000#,
      16#00ff_ffff#, 16#0004_0006#,
      16#80d7_5fff#, 16#000b_0000#,
      others => 0);

   Haswell_Trans_FDI : constant Buf_Trans_Array :=
     (16#00ff_ffff#, 16#0007_000e#,
      16#00d7_5fff#, 16#000f_000a#,
      16#00c3_0fff#, 16#0006_0006#,
      16#00aa_afff#, 16#001e_0000#,
      16#00ff_ffff#, 16#000f_000a#,
      16#00d7_5fff#, 16#0016_0004#,
      16#00c3_0fff#, 16#001e_0000#,
      16#00ff_ffff#, 16#0006_0006#,
      16#00d7_5fff#, 16#001e_0000#,
      others => 0);

   Haswell_Trans_HDMI : constant HDMI_Buf_Trans_Array :=
     ((16#00ff_ffff#, 16#0006_000e#),
      (16#00e7_9fff#, 16#000e_000c#),
      (16#00d7_5fff#, 16#0005_000a#),
      (16#00ff_ffff#, 16#0005_000a#),
      (16#00e7_9fff#, 16#001d_0007#),
      (16#00d7_5fff#, 16#000c_0004#),
      (16#00ff_ffff#, 16#0004_0006#),
      (16#80e7_9fff#, 16#0003_0002#),
      (16#00ff_ffff#, 16#0014_0005#),
      (16#00ff_ffff#, 16#000c_0004#),
      (16#00ff_ffff#, 16#001c_0003#),
      (16#80ff_ffff#, 16#0003_0002#));

   Broadwell_Trans_EDP : constant Buf_Trans_Array :=
     (16#00ff_ffff#, 16#0000_0012#,
      16#00eb_afff#, 16#0002_0011#,
      16#00c7_1fff#, 16#0006_000f#,
      16#00aa_afff#, 16#000e_000a#,
      16#00ff_ffff#, 16#0002_0011#,
      16#00db_6fff#, 16#0005_000f#,
      16#00be_efff#, 16#000a_000c#,
      16#00ff_ffff#, 16#0005_000f#,
      16#00db_6fff#, 16#000a_000c#,
      others => 0);

   Broadwell_Trans_DP : constant Buf_Trans_Array :=
     (16#00ff_ffff#, 16#0007_000e#,
      16#00d7_5fff#, 16#000e_000a#,
      16#00be_ffff#, 16#0014_0006#,
      16#80b2_cfff#, 16#001b_0002#,
      16#00ff_ffff#, 16#000e_000a#,
      16#00db_6fff#, 16#0016_0005#,
      16#80c7_1fff#, 16#001a_0002#,
      16#00f7_dfff#, 16#0018_0004#,
      16#80d7_5fff#, 16#001b_0002#,
      others => 0);

   Broadwell_Trans_FDI : constant Buf_Trans_Array :=
     (16#00ff_ffff#, 16#0001_000e#,
      16#00d7_5fff#, 16#0004_000a#,
      16#00c3_0fff#, 16#0007_0006#,
      16#00aa_afff#, 16#000c_0000#,
      16#00ff_ffff#, 16#0004_000a#,
      16#00d7_5fff#, 16#0009_0004#,
      16#00c3_0fff#, 16#000c_0000#,
      16#00ff_ffff#, 16#0007_0006#,
      16#00d7_5fff#, 16#000c_0000#,
      others => 0);

   Broadwell_Trans_HDMI : constant HDMI_Buf_Trans_Array :=
     ((16#00ff_ffff#, 16#0007_000e#),
      (16#00d7_5fff#, 16#000e_000a#),
      (16#00be_ffff#, 16#0014_0006#),
      (16#00ff_ffff#, 16#0009_000d#),
      (16#00ff_ffff#, 16#000e_000a#),
      (16#00d7_ffff#, 16#0014_0006#),
      (16#80cb_2fff#, 16#001b_0002#),
      (16#00ff_ffff#, 16#0014_0006#),
      (16#80e7_9fff#, 16#001b_0002#),
      (16#80ff_ffff#, 16#001b_0002#),
      others => (0, 0));

   ----------------------------------------------------------------------------

   procedure Translations (Trans : out Buf_Trans_Array; Port : Digital_Port)
   is
      HDMI_Trans : constant DDI_HDMI_Buf_Trans_Range :=
        (if (Config.CPU = Broadwell and
             Config.DDI_HDMI_Buffer_Translation in Broadwell_HDMI_Range)
            or
            (Config.CPU /= Broadwell and
             Config.DDI_HDMI_Buffer_Translation in Haswell_HDMI_Range)
         then Config.DDI_HDMI_Buffer_Translation
         else Config.Default_DDI_HDMI_Buffer_Translation);
   begin
      Trans :=
        (case Config.CPU is
            when Broadwell =>
              (case Port is
                  when DIGI_A =>
                    (if Config.EDP_Low_Voltage_Swing
                     then Broadwell_Trans_EDP
                     else Broadwell_Trans_DP),
                  when DIGI_B .. DIGI_D   => Broadwell_Trans_DP,
                  when DIGI_E             => Broadwell_Trans_FDI),
            when others =>
              (case Port is
                  when DIGI_A .. DIGI_D   => Haswell_Trans_DP,
                  when DIGI_E             => Haswell_Trans_FDI));
      case Config.CPU is
         when Broadwell =>
            Trans (18) := Broadwell_Trans_HDMI (HDMI_Trans).Trans1;
            Trans (19) := Broadwell_Trans_HDMI (HDMI_Trans).Trans2;
         when others =>
            Trans (18) := Haswell_Trans_HDMI (HDMI_Trans).Trans1;
            Trans (19) := Haswell_Trans_HDMI (HDMI_Trans).Trans2;
      end case;
   end Translations;

end HW.GFX.GMA.Connectors.DDI.Buffers;
