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

with GNAT.Source_Info;
with HW.Debug;

with HW.GFX.GMA.Config;
with HW.GFX.GMA.Registers;

use HW.GFX.GMA.Registers;

package body HW.GFX.GMA.DDI_Phy is

   subtype Dual_Channel is T range BC .. BC;

   type DDI_Range is record
      First : DDI_Phy_Port;
      Last  : DDI_Phy_Port;
   end record;
   type DDI_Range_Array is array (T) of DDI_Range;
   DDIs : constant DDI_Range_Array :=
     (A  => (DIGI_A, DIGI_A),
      BC => (DIGI_B, DIGI_C));

   ----------------------------------------------------------------------------

   type CL1CM is record
      DW0   : Registers_Index;
      DW9   : Registers_Index;
      DW10  : Registers_Index;
      DW28  : Registers_Index;
      DW30  : Registers_Index;
   end record;
   type CL1CM_Array is array (T) of CL1CM;
   PORT_CL1CM : constant CL1CM_Array :=
     (A  =>
        (DW0   => BXT_PORT_CL1CM_DW0_A,
         DW9   => BXT_PORT_CL1CM_DW9_A,
         DW10  => BXT_PORT_CL1CM_DW10_A,
         DW28  => BXT_PORT_CL1CM_DW28_A,
         DW30  => BXT_PORT_CL1CM_DW30_A),
      BC =>
        (DW0   => BXT_PORT_CL1CM_DW0_BC,
         DW9   => BXT_PORT_CL1CM_DW9_BC,
         DW10  => BXT_PORT_CL1CM_DW10_BC,
         DW28  => BXT_PORT_CL1CM_DW28_BC,
         DW30  => BXT_PORT_CL1CM_DW30_BC));

   type CL2CM is record
      DW6 : Registers_Index;
   end record;
   type CL2CM_Array is array (Dual_Channel) of CL2CM;
   PORT_CL2CM : constant CL2CM_Array :=
     (BC => (DW6 => BXT_PORT_CL2CM_DW6_BC));

   type Port_Ref_Regs is record
      DW3 : Registers_Index;
      DW6 : Registers_Index;
      DW8 : Registers_Index;
   end record;
   type Port_Ref_Array is array (T) of Port_Ref_Regs;
   PORT_REF : constant Port_Ref_Array :=
     (A  =>
        (DW3 => BXT_PORT_REF_DW3_A,
         DW6 => BXT_PORT_REF_DW6_A,
         DW8 => BXT_PORT_REF_DW8_A),
      BC =>
        (DW3 => BXT_PORT_REF_DW3_BC,
         DW6 => BXT_PORT_REF_DW6_BC,
         DW8 => BXT_PORT_REF_DW8_BC));

   type Regs is array (T) of Registers_Index;
   PHY_CTL_FAMILY : constant Regs :=
     (A => BXT_PHY_CTL_FAM_EDP, BC => BXT_PHY_CTL_FAM_DDI);

   type DDI_Regs is array (DDI_Phy_Port) of Registers_Index;
   PHY_CTL : constant DDI_Regs :=
     (DIGI_A => BXT_PHY_CTL_A,
      DIGI_B => BXT_PHY_CTL_B,
      DIGI_C => BXT_PHY_CTL_C);

   ----------------------------------------------------------------------------

   type Values is array (T) of Word32;
   GT_DISPLAY_POWER_ON : constant Values :=
     (A  => 1 * 2 ** 1,
      BC => 1 * 2 ** 0);

   PORT_CL1CM_PHY_POWER_GOOD           : constant :=      1 * 2 ** 16;
   PORT_CL1CM_PHY_RESERVED             : constant :=      1 * 2 **  7;

   PORT_CL1CM_IREFxRC_OFFSET_SHIFT     : constant :=                8;
   PORT_CL1CM_IREFxRC_OFFSET_MASK      : constant := 16#ff# * 2 **  8;

   PORT_CL1CM_OCL1_POWER_DOWN_EN       : constant :=      1 * 2 ** 23;
   PORT_CL1CM_OLDO_DYN_POWER_DOWN_EN   : constant :=      1 * 2 ** 22;
   PORT_CL1CM_SUS_CLK_CONFIG           : constant :=      3 * 2 **  0;

   PORT_CL2CM_OLDO_DYN_POWER_DOWN_EN   : constant :=      1 * 2 ** 28;

   PORT_REF_GRC_DONE                   : constant :=      1 * 2 ** 22;

   PORT_REF_GRC_CODE_SHIFT             : constant :=               24;
   PORT_REF_GRC_FAST_SHIFT             : constant :=               16;
   PORT_REF_GRC_SLOW_SHIFT             : constant :=                8;

   PORT_REF_GRC_DISABLE                : constant :=      1 * 2 ** 15;
   PORT_REF_GRC_READY_OVERRIDE         : constant :=      1 * 2 **  1;

   PHY_CTL_FAM_CMN_RESET_DIS           : constant :=      1 * 2 ** 31;

   PHY_CTL_CMNLANE_POWERDOWN_ACK       : constant :=      1 * 2 ** 10;

   ----------------------------------------------------------------------------

   procedure Is_Enabled (Phy : in T; Enabled : out Boolean)
   is
      Phy_Pwr : Word32;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Is_Set_Mask (BXT_P_CR_GT_DISP_PWRON, GT_DISPLAY_POWER_ON (Phy), Enabled);

      if Enabled then
         Read (PORT_CL1CM (Phy).DW0, Phy_Pwr);
         Enabled :=
            (Phy_Pwr and (PORT_CL1CM_PHY_POWER_GOOD or PORT_CL1CM_PHY_RESERVED))
            = PORT_CL1CM_PHY_POWER_GOOD;
         pragma Debug (not Enabled, Debug.Put_Line ("DDI PHY power unsettled"));
      end if;

      if Enabled then
         Is_Set_Mask (PHY_CTL_FAMILY (Phy), PHY_CTL_FAM_CMN_RESET_DIS, Enabled);
         pragma Debug (not Enabled, Debug.Put_Line ("DDI PHY still in reset"));
      end if;

      for DDI in DDIs (Phy).First .. DDIs (Phy).Last loop
         if Enabled then
            declare
               Common_Lane_Powerdown : Boolean;
            begin
               Is_Set_Mask
                 (Register => PHY_CTL (DDI),
                  Mask     => PHY_CTL_CMNLANE_POWERDOWN_ACK,
                  Result   => Common_Lane_Powerdown);
               Enabled := not Common_Lane_Powerdown;
               pragma Debug
                 (not Enabled, Debug.Put_Line ("Common lane powered down"));
            end;
         end if;
      end loop;
   end Is_Enabled;

   procedure Power_On_Phy (Phy : T)
   with
      Pre => True
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Set_Mask (BXT_P_CR_GT_DISP_PWRON, GT_DISPLAY_POWER_ON (Phy));

      Wait
        (Register    => PORT_CL1CM (Phy).DW0,
         Mask        => PORT_CL1CM_PHY_POWER_GOOD or
                        PORT_CL1CM_PHY_RESERVED,
         Value       => PORT_CL1CM_PHY_POWER_GOOD,
         TOut_MS     => 1);   -- 50~100us

      Unset_And_Set_Mask
        (Register    => PORT_CL1CM (Phy).DW9,
         Mask_Unset  => PORT_CL1CM_IREFxRC_OFFSET_MASK,
         Mask_Set    => Shift_Left
                          (16#e4#, PORT_CL1CM_IREFxRC_OFFSET_SHIFT));
      Unset_And_Set_Mask
        (Register    => PORT_CL1CM (Phy).DW10,
         Mask_Unset  => PORT_CL1CM_IREFxRC_OFFSET_MASK,
         Mask_Set    => Shift_Left
                          (16#e4#, PORT_CL1CM_IREFxRC_OFFSET_SHIFT));

      Set_Mask
        (Register    => PORT_CL1CM (Phy).DW28,
         Mask        => PORT_CL1CM_OCL1_POWER_DOWN_EN or
                        PORT_CL1CM_OLDO_DYN_POWER_DOWN_EN or
                        PORT_CL1CM_SUS_CLK_CONFIG);

      if Phy in Dual_Channel then
         Set_Mask (PORT_CL2CM (Phy).DW6, PORT_CL2CM_OLDO_DYN_POWER_DOWN_EN);
      end if;

      if Phy = BC then
         declare
            GRC_Val : Word32;
         begin
            -- take RCOMP calibration result from A

            Wait_Set_Mask (PORT_REF (A).DW3, PORT_REF_GRC_DONE, TOut_MS => 10);
            Read (PORT_REF (A).DW6, GRC_Val);
            GRC_Val := Shift_Right (GRC_Val, PORT_REF_GRC_CODE_SHIFT);

            -- use it for BC too
            GRC_Val :=  Shift_Left (GRC_Val, PORT_REF_GRC_FAST_SHIFT) or
                        Shift_Left (GRC_Val, PORT_REF_GRC_SLOW_SHIFT) or
                        GRC_Val;
            Write (PORT_REF (Phy).DW6, GRC_Val);

            Set_Mask
              (Register => PORT_REF (Phy).DW8,
               Mask     => PORT_REF_GRC_DISABLE or
                           PORT_REF_GRC_READY_OVERRIDE);
         end;
      end if;

      Set_Mask (PHY_CTL_FAMILY (Phy), PHY_CTL_FAM_CMN_RESET_DIS);
   end Power_On_Phy;

   procedure Power_On (Phy : T)
   is
      Phy_A_Enabled : Boolean := False;
      Enabled : Boolean;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Is_Enabled (Phy, Enabled);
      pragma Debug (Enabled, Debug.Put_Line ("DDI PHY already enabled"));

      if not Enabled then
         if Phy = BC then
            -- PHY BC needs RCOMP calibration results from PHY A
            Is_Enabled (A, Phy_A_Enabled);
            if not Phy_A_Enabled then
               Power_On_Phy (A);
            end if;
         end if;

         Power_On_Phy (Phy);

         if Phy = BC and then not Phy_A_Enabled then
            Power_Off (A);
         end if;
      end if;
   end Power_On;

   procedure Power_Off (Phy : T) is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Unset_Mask (PHY_CTL_FAMILY (Phy), PHY_CTL_FAM_CMN_RESET_DIS);
      Unset_Mask (BXT_P_CR_GT_DISP_PWRON, GT_DISPLAY_POWER_ON (Phy));
   end Power_Off;

   ----------------------------------------------------------------------------

   type Lanes is range 0 .. 3;
   type Lane_Reg_Array is array (Lanes) of Registers_Index;

   type Port_TX_Regs is record
      DW2_LN0 : Registers_Index;
      DW2_GRP : Registers_Index;
      DW3_LN0 : Registers_Index;
      DW3_GRP : Registers_Index;
      DW4_LN0 : Registers_Index;
      DW4_GRP : Registers_Index;
      DW14_LN : Lane_Reg_Array;
   end record;
   type Port_TX_Array is array (DDI_Phy_Port) of Port_TX_Regs;

   PORT_TX : constant Port_TX_Array :=
     (DIGI_A =>
        (DW2_LN0 => BXT_PORT_TX_DW2_LN0_A,
         DW2_GRP => BXT_PORT_TX_DW2_GRP_A,
         DW3_LN0 => BXT_PORT_TX_DW3_LN0_A,
         DW3_GRP => BXT_PORT_TX_DW3_GRP_A,
         DW4_LN0 => BXT_PORT_TX_DW4_LN0_A,
         DW4_GRP => BXT_PORT_TX_DW4_GRP_A,
         DW14_LN =>
           (BXT_PORT_TX_DW14_LN0_A,
            BXT_PORT_TX_DW14_LN1_A,
            BXT_PORT_TX_DW14_LN2_A,
            BXT_PORT_TX_DW14_LN3_A)),
      DIGI_B =>
        (DW2_LN0 => BXT_PORT_TX_DW2_LN0_B,
         DW2_GRP => BXT_PORT_TX_DW2_GRP_B,
         DW3_LN0 => BXT_PORT_TX_DW3_LN0_B,
         DW3_GRP => BXT_PORT_TX_DW3_GRP_B,
         DW4_LN0 => BXT_PORT_TX_DW4_LN0_B,
         DW4_GRP => BXT_PORT_TX_DW4_GRP_B,
         DW14_LN =>
           (BXT_PORT_TX_DW14_LN0_B,
            BXT_PORT_TX_DW14_LN1_B,
            BXT_PORT_TX_DW14_LN2_B,
            BXT_PORT_TX_DW14_LN3_B)),
      DIGI_C =>
        (DW2_LN0 => BXT_PORT_TX_DW2_LN0_C,
         DW2_GRP => BXT_PORT_TX_DW2_GRP_C,
         DW3_LN0 => BXT_PORT_TX_DW3_LN0_C,
         DW3_GRP => BXT_PORT_TX_DW3_GRP_C,
         DW4_LN0 => BXT_PORT_TX_DW4_LN0_C,
         DW4_GRP => BXT_PORT_TX_DW4_GRP_C,
         DW14_LN =>
           (BXT_PORT_TX_DW14_LN0_C,
            BXT_PORT_TX_DW14_LN1_C,
            BXT_PORT_TX_DW14_LN2_C,
            BXT_PORT_TX_DW14_LN3_C)));

   PORT_TX_DW2_MARGIN_000_SHIFT        : constant :=               16;
   PORT_TX_DW2_MARGIN_000_MASK         : constant := 16#ff# * 2 ** 16;
   PORT_TX_DW2_UNIQ_TRANS_SCALE_SHIFT  : constant :=                8;
   PORT_TX_DW2_UNIQ_TRANS_SCALE_MASK   : constant := 16#ff# * 2 **  8;
   function PORT_TX_DW2_MARGIN_000 (Margin : Word8) return Word32 is
   begin
      return Shift_Left (Word32 (Margin), PORT_TX_DW2_MARGIN_000_SHIFT);
   end PORT_TX_DW2_MARGIN_000;
   function PORT_TX_DW2_UNIQ_TRANS_SCALE (Scale : Word8) return Word32 is
   begin
      return Shift_Left (Word32 (Scale), PORT_TX_DW2_UNIQ_TRANS_SCALE_SHIFT);
   end PORT_TX_DW2_UNIQ_TRANS_SCALE;

   PORT_TX_DW3_UNIQUE_TRANGE_EN_METHOD : constant :=      1 * 2 ** 27;
   PORT_TX_DW3_SCALE_DCOMP_METHOD      : constant :=      1 * 2 ** 26;

   PORT_TX_DW4_DE_EMPHASIS_SHIFT       : constant :=               24;
   PORT_TX_DW4_DE_EMPHASIS_MASK        : constant := 16#ff# * 2 ** 24;
   function PORT_TX_DW4_DE_EMPHASIS (De_Emph : Word8) return Word32 is
   begin
      return Shift_Left (Word32 (De_Emph), PORT_TX_DW4_DE_EMPHASIS_SHIFT);
   end PORT_TX_DW4_DE_EMPHASIS;

   PORT_TX_DW14_LN_LATENCY_OPTIM       : constant :=      1 * 2 ** 30;

   ----------------------------------------------------------------------------

   type Port_PCS_Regs is record
      DW10_LN01   : Registers_Index;
      DW10_GRP    : Registers_Index;
   end record;
   type Port_PCS_Array is array (DDI_Phy_Port) of Port_PCS_Regs;

   PORT_PCS : constant Port_PCS_Array :=
     (DIGI_A =>
        (DW10_LN01   => BXT_PORT_PCS_DW10_01_A,
         DW10_GRP    => BXT_PORT_PCS_DW10_GRP_A),
      DIGI_B =>
        (DW10_LN01   => BXT_PORT_PCS_DW10_01_B,
         DW10_GRP    => BXT_PORT_PCS_DW10_GRP_B),
      DIGI_C =>
        (DW10_LN01   => BXT_PORT_PCS_DW10_01_C,
         DW10_GRP    => BXT_PORT_PCS_DW10_GRP_C));

   PORT_PCS_TX2_SWING_CALC_INIT        : constant := 1 * 2 ** 31;
   PORT_PCS_TX1_SWING_CALC_INIT        : constant := 1 * 2 ** 30;

   ----------------------------------------------------------------------------

   procedure Pre_PLL (Port_Cfg : Port_Config)
   is
      type Lane_Values is array (Lanes) of Word32;
      Lane_Optim : constant Lane_Values :=
        (if Port_Cfg.Display = HDMI or
            Port_Cfg.DP.Lane_Count = DP_Lane_Count_4
         then
            (0 => PORT_TX_DW14_LN_LATENCY_OPTIM,
             1 => 0,
             2 => PORT_TX_DW14_LN_LATENCY_OPTIM,
             3 => PORT_TX_DW14_LN_LATENCY_OPTIM)
         elsif Port_Cfg.DP.Lane_Count = DP_Lane_Count_2 then
            (0 => PORT_TX_DW14_LN_LATENCY_OPTIM,
             1 => 0,
             2 => PORT_TX_DW14_LN_LATENCY_OPTIM,
             3 => 0)
         else
            (Lanes => 0));
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Port in DDI_Phy_Port then
         for Lane in Lanes loop
            Unset_And_Set_Mask
              (Register    => PORT_TX (Port_Cfg.Port).DW14_LN (Lane),
               Mask_Unset  => PORT_TX_DW14_LN_LATENCY_OPTIM,
               Mask_Set    => Lane_Optim (Lane));
         end loop;
      end if;
   end Pre_PLL;

   ----------------------------------------------------------------------------

   type DDI_Buf_Trans is record
      Margin   : Word8;
      Scale    : Word8;
      Scale_En : Boolean;
      De_Emph  : Word8;
   end record;
   Invalid_Buf_Trans : constant DDI_Buf_Trans := (0, 0, False, 0);

   function DP_Buf_Trans (TS : DP_Info.Train_Set) return DDI_Buf_Trans
   with
      Pre => True
   is
   begin
      return
        (case TS.Voltage_Swing is
            when DP_Info.VS_Level_0 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => ( 52, 16#9a#, False, 128),
                  when DP_Info.Emph_Level_1  => ( 78, 16#9a#, False,  85),
                  when DP_Info.Emph_Level_2  => (104, 16#9a#, False,  64),
                  when DP_Info.Emph_Level_3  => (154, 16#9a#, False,  43)),
            when DP_Info.VS_Level_1 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => ( 77, 16#9a#, False, 128),
                  when DP_Info.Emph_Level_1  => (116, 16#9a#, False,  85),
                  when DP_Info.Emph_Level_2  => (154, 16#9a#, False,  64),
                  when others                => Invalid_Buf_Trans),
            when DP_Info.VS_Level_2 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => (102, 16#9a#, False, 128),
                  when DP_Info.Emph_Level_1  => (154, 16#9a#, False,  85),
                  when others                => Invalid_Buf_Trans),
            when DP_Info.VS_Level_3 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => (154, 16#9a#,  True, 128),
                  when others                => Invalid_Buf_Trans));
   end DP_Buf_Trans;

   function eDP_Buf_Trans (TS : DP_Info.Train_Set) return DDI_Buf_Trans
   with
      Pre => True
   is
   begin
      return
        (case TS.Voltage_Swing is
            when DP_Info.VS_Level_0 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => ( 26, 16#00#, False, 128),
                  when DP_Info.Emph_Level_1  => ( 38, 16#00#, False, 112),
                  when DP_Info.Emph_Level_2  => ( 48, 16#00#, False,  96),
                  when DP_Info.Emph_Level_3  => ( 54, 16#00#, False,  69)),
            when DP_Info.VS_Level_1 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => ( 32, 16#00#, False, 128),
                  when DP_Info.Emph_Level_1  => ( 48, 16#00#, False, 104),
                  when DP_Info.Emph_Level_2  => ( 54, 16#00#, False,  85),
                  when others                => Invalid_Buf_Trans),
            when DP_Info.VS_Level_2 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => ( 43, 16#00#, False, 128),
                  when DP_Info.Emph_Level_1  => ( 54, 16#00#, False, 101),
                  when others                => Invalid_Buf_Trans),
            when DP_Info.VS_Level_3 =>
              (case TS.Pre_Emph is
                  when DP_Info.Emph_Level_0  => ( 48, 16#00#, False, 128),
                  when others                => Invalid_Buf_Trans));
   end eDP_Buf_Trans;

   type HDMI_Buf_Trans_Array is array (HDMI_Buf_Trans_Range) of DDI_Buf_Trans;
   HDMI_Buf_Trans : constant HDMI_Buf_Trans_Array :=
     (0 => ( 52, 16#9a#, False, 128),
      1 => ( 52, 16#9a#, False,  85),
      2 => ( 52, 16#9a#, False,  64),
      3 => ( 42, 16#9a#, False,  43), -- XXX: typo in i915?
      4 => ( 77, 16#9a#, False, 128),
      5 => ( 77, 16#9a#, False,  85),
      6 => ( 77, 16#9a#, False,  64),
      7 => (102, 16#9a#, False, 128),
      8 => (102, 16#9a#, False,  85),
      9 => (154, 16#9a#,  True, 128));

   procedure Set_Signal_Levels
     (Port  : DDI_Phy_Port;
      Trans : DDI_Buf_Trans)
   with
      Pre => True
   is
      -- We read from / write to different registers
      -- to program all lanes in a group at once.
      Val32 : Word32;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Read (PORT_PCS (Port).DW10_LN01, Val32);
      Val32 := Val32 and
         not PORT_PCS_TX2_SWING_CALC_INIT and
         not PORT_PCS_TX1_SWING_CALC_INIT;
      Write (PORT_PCS (Port).DW10_GRP, Val32);

      Read (PORT_TX (Port).DW2_LN0, Val32);
      Val32 := Val32 and
         not PORT_TX_DW2_MARGIN_000_MASK and
         not PORT_TX_DW2_UNIQ_TRANS_SCALE_MASK;
      Val32 := Val32 or
         PORT_TX_DW2_MARGIN_000 (Trans.Margin) or
         PORT_TX_DW2_UNIQ_TRANS_SCALE (Trans.Scale);
      Write (PORT_TX (Port).DW2_GRP, Val32);

      Read (PORT_TX (Port).DW3_LN0, Val32);
      Val32 := Val32 and not PORT_TX_DW3_SCALE_DCOMP_METHOD;
      if Trans.Scale_En then
         Val32 := Val32 or PORT_TX_DW3_SCALE_DCOMP_METHOD;
      end if;
      Write (PORT_TX (Port).DW3_GRP, Val32);

      pragma Debug
        ((Val32 and PORT_TX_DW3_UNIQUE_TRANGE_EN_METHOD) /= 0 and
         (Val32 and PORT_TX_DW3_SCALE_DCOMP_METHOD) = 0,
         Debug.Put_Line ("XXX: Unique trange enabled but scaling disabled."));

      Read (PORT_TX (Port).DW4_LN0, Val32);
      Val32 := Val32 and not PORT_TX_DW4_DE_EMPHASIS_MASK;
      Val32 := Val32 or PORT_TX_DW4_DE_EMPHASIS (Trans.De_Emph);
      Write (PORT_TX (Port).DW4_GRP, Val32);

      Read (PORT_PCS (Port).DW10_LN01, Val32);
      Val32 := Val32 or
         PORT_PCS_TX2_SWING_CALC_INIT or
         PORT_PCS_TX1_SWING_CALC_INIT;
      Write (PORT_PCS (Port).DW10_GRP, Val32);
   end Set_Signal_Levels;

   procedure Set_DP_Signal_Levels
     (Port        : Digital_Port;
      Train_Set   : DP_Info.Train_Set)
   is
      Trans : constant DDI_Buf_Trans :=
        (if Port = DIGI_A and Config.EDP_Low_Voltage_Swing then
            eDP_Buf_Trans (Train_Set)
         else
            DP_Buf_Trans (Train_Set));
   begin
      if Port in DDI_Phy_Port then
         Set_Signal_Levels (Port, Trans);
      end if;
   end Set_DP_Signal_Levels;

   procedure Set_HDMI_Signal_Levels
     (Port  : DDI_Phy_Port;
      Level : HDMI_Buf_Trans_Range)
   is
   begin
      Set_Signal_Levels (Port, HDMI_Buf_Trans (Level));
   end Set_HDMI_Signal_Levels;

end HW.GFX.GMA.DDI_Phy;
