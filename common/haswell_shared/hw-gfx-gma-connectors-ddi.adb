--
-- Copyright (C) 2015-2017 secunet Security Networks AG
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
with HW.GFX.DP_Training;
with HW.GFX.GMA.Config;
with HW.GFX.GMA.PCH.FDI;
with HW.GFX.GMA.PCH.Transcoder;
with HW.GFX.GMA.PCH.VGA;
with HW.GFX.GMA.DP_Info;
with HW.GFX.GMA.DP_Aux_Ch;
with HW.GFX.GMA.SPLL;
with HW.GFX.GMA.DDI_Phy;
with HW.GFX.GMA.Connectors.DDI.Buffers;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.Connectors.DDI is

   DDI_BUF_CTL_BUFFER_ENABLE        : constant :=  1 * 2 ** 31;
   DDI_BUF_CTL_TRANS_SELECT_MASK    : constant := 15 * 2 ** 24;
   DDI_BUF_CTL_PORT_REVERSAL        : constant :=  1 * 2 ** 16;
   DDI_BUF_CTL_IDLE_STATUS          : constant :=  1 * 2 **  7;
   DDI_BUF_CTL_DDI_A_LANE_CAP       : constant :=  1 * 2 **  4;
   DDI_BUF_CTL_PORT_WIDTH_MASK      : constant :=  7 * 2 **  1;
   DDI_BUF_CTL_PORT_WIDTH_1_LANE    : constant :=  0 * 2 **  1;
   DDI_BUF_CTL_PORT_WIDTH_2_LANES   : constant :=  1 * 2 **  1;
   DDI_BUF_CTL_PORT_WIDTH_4_LANES   : constant :=  3 * 2 **  1;
   DDI_BUF_CTL_INIT_DISPLAY_DETECT  : constant :=  1 * 2 **  0;

   subtype DDI_BUF_CTL_TRANS_SELECT_T is Natural range 0 .. 9;
   function DDI_BUF_CTL_TRANS_SELECT
     (Sel : DDI_BUF_CTL_TRANS_SELECT_T)
      return Word32;

   type DDI_BUF_CTL_PORT_WIDTH_T is array (HW.GFX.DP_Lane_Count) of Word32;
   DDI_BUF_CTL_PORT_WIDTH : constant DDI_BUF_CTL_PORT_WIDTH_T :=
      DDI_BUF_CTL_PORT_WIDTH_T'
     (HW.GFX.DP_Lane_Count_1 => DDI_BUF_CTL_PORT_WIDTH_1_LANE,
      HW.GFX.DP_Lane_Count_2 => DDI_BUF_CTL_PORT_WIDTH_2_LANES,
      HW.GFX.DP_Lane_Count_4 => DDI_BUF_CTL_PORT_WIDTH_4_LANES);

   DP_TP_CTL_TRANSPORT_ENABLE       : constant := 1 * 2 ** 31;
   DP_TP_CTL_MODE_SST               : constant := 0 * 2 ** 27;
   DP_TP_CTL_MODE_MST               : constant := 1 * 2 ** 27;
   DP_TP_CTL_FORCE_ACT              : constant := 1 * 2 ** 25;
   DP_TP_CTL_ENHANCED_FRAME_ENABLE  : constant := 1 * 2 ** 18;
   DP_TP_CTL_FDI_AUTOTRAIN          : constant := 1 * 2 ** 15;
   DP_TP_CTL_LINK_TRAIN_MASK        : constant := 7 * 2 **  8;
   DP_TP_CTL_LINK_TRAIN_PAT1        : constant := 0 * 2 **  8;
   DP_TP_CTL_LINK_TRAIN_PAT2        : constant := 1 * 2 **  8;
   DP_TP_CTL_LINK_TRAIN_PAT3        : constant := 4 * 2 **  8;
   DP_TP_CTL_LINK_TRAIN_IDLE        : constant := 2 * 2 **  8;
   DP_TP_CTL_LINK_TRAIN_NORMAL      : constant := 3 * 2 **  8;
   DP_TP_CTL_SCRAMBLE_DISABLE       : constant := 1 * 2 **  7;
   DP_TP_CTL_ALT_SCRAMBLER_RESET    : constant := 1 * 2 **  6;

   type DP_TP_CTL_LINK_TRAIN_Array is
      array (DP_Info.Training_Pattern) of Word32;
   DP_TP_CTL_LINK_TRAIN : constant DP_TP_CTL_LINK_TRAIN_Array :=
      DP_TP_CTL_LINK_TRAIN_Array'
     (DP_Info.TP_1      => DP_TP_CTL_LINK_TRAIN_PAT1 or DP_TP_CTL_SCRAMBLE_DISABLE,
      DP_Info.TP_2      => DP_TP_CTL_LINK_TRAIN_PAT2 or DP_TP_CTL_SCRAMBLE_DISABLE,
      DP_Info.TP_3      => DP_TP_CTL_LINK_TRAIN_PAT3 or DP_TP_CTL_SCRAMBLE_DISABLE,
      DP_Info.TP_Idle   => DP_TP_CTL_LINK_TRAIN_IDLE,
      DP_Info.TP_None   => DP_TP_CTL_LINK_TRAIN_NORMAL);

   DP_TP_STATUS_MIN_IDLES_SENT      : constant := 1 * 2 ** 25;
   DP_TP_STATUS_FDI_AUTO_TRAIN_DONE : constant := 1 * 2 ** 12;

   PORT_CLK_SEL_LCPLL2700  : constant := 0 * 2 ** 29; -- not on ULX
   PORT_CLK_SEL_LCPLL1350  : constant := 1 * 2 ** 29;
   PORT_CLK_SEL_LCPLL810   : constant := 2 * 2 ** 29;
   PORT_CLK_SEL_SPLL       : constant := 3 * 2 ** 29;
   PORT_CLK_SEL_WRPLL1     : constant := 4 * 2 ** 29;
   PORT_CLK_SEL_WRPLL2     : constant := 5 * 2 ** 29;
   PORT_CLK_SEL_NONE       : constant := 7 * 2 ** 29;

   type PORT_CLK_SEL_LCPLL_T is array (HW.GFX.DP_Bandwidth) of Word32;
   PORT_CLK_SEL_LCPLL : constant PORT_CLK_SEL_LCPLL_T :=
      PORT_CLK_SEL_LCPLL_T'
     (HW.GFX.DP_Bandwidth_1_62 => PORT_CLK_SEL_LCPLL810,
      HW.GFX.DP_Bandwidth_2_7  => PORT_CLK_SEL_LCPLL1350,
      HW.GFX.DP_Bandwidth_5_4  => PORT_CLK_SEL_LCPLL2700);

   DISPIO_CR_TX_BLNC_LEG_DISBL_MASK    : constant := 16#1f# * 2 ** 23;
   DISPIO_CR_TX_BLNC_LEG_SCTL_4_SHIFT  : constant :=               20;
   DISPIO_CR_TX_BLNC_LEG_SCTL_4_MASK   : constant := 16#07# * 2 ** 20;
   DISPIO_CR_TX_BLNC_LEG_SCTL_3_SHIFT  : constant :=               17;
   DISPIO_CR_TX_BLNC_LEG_SCTL_3_MASK   : constant := 16#07# * 2 ** 17;
   DISPIO_CR_TX_BLNC_LEG_SCTL_2_SHIFT  : constant :=               14;
   DISPIO_CR_TX_BLNC_LEG_SCTL_2_MASK   : constant := 16#07# * 2 ** 14;
   DISPIO_CR_TX_BLNC_LEG_SCTL_1_SHIFT  : constant :=               11;
   DISPIO_CR_TX_BLNC_LEG_SCTL_1_MASK   : constant := 16#07# * 2 ** 11;
   DISPIO_CR_TX_BLNC_LEG_SCTL_0_SHIFT  : constant :=                8;
   DISPIO_CR_TX_BLNC_LEG_SCTL_0_MASK   : constant := 16#07# * 2 **  8;

   type DDI_Buf_Trans_Regs_Array
      is array (Buf_Trans_Range) of Registers.Registers_Index;

   type DDI_Registers is record
      BUF_CTL        : Registers.Registers_Index;
      BUF_TRANS      : DDI_Buf_Trans_Regs_Array;
      DP_TP_CTL      : Registers.Registers_Index;
      DP_TP_STATUS   : Registers.Registers_Invalid_Index;
      PORT_CLK_SEL   : Registers.Registers_Index;
   end record;

   type DDI_Registers_Array is array (Digital_Port) of DDI_Registers;

   DDI_Regs : constant DDI_Registers_Array := DDI_Registers_Array'
     (DIGI_A => DDI_Registers'
        (BUF_CTL        => Registers.DDI_BUF_CTL_A,
         BUF_TRANS      => DDI_Buf_Trans_Regs_Array'
                          (Registers.DDI_BUF_TRANS_A_S0T1,
                           Registers.DDI_BUF_TRANS_A_S0T2,
                           Registers.DDI_BUF_TRANS_A_S1T1,
                           Registers.DDI_BUF_TRANS_A_S1T2,
                           Registers.DDI_BUF_TRANS_A_S2T1,
                           Registers.DDI_BUF_TRANS_A_S2T2,
                           Registers.DDI_BUF_TRANS_A_S3T1,
                           Registers.DDI_BUF_TRANS_A_S3T2,
                           Registers.DDI_BUF_TRANS_A_S4T1,
                           Registers.DDI_BUF_TRANS_A_S4T2,
                           Registers.DDI_BUF_TRANS_A_S5T1,
                           Registers.DDI_BUF_TRANS_A_S5T2,
                           Registers.DDI_BUF_TRANS_A_S6T1,
                           Registers.DDI_BUF_TRANS_A_S6T2,
                           Registers.DDI_BUF_TRANS_A_S7T1,
                           Registers.DDI_BUF_TRANS_A_S7T2,
                           Registers.DDI_BUF_TRANS_A_S8T1,
                           Registers.DDI_BUF_TRANS_A_S8T2,
                           Registers.DDI_BUF_TRANS_A_S9T1,
                           Registers.DDI_BUF_TRANS_A_S9T2),
         DP_TP_CTL      => Registers.DP_TP_CTL_A,
         DP_TP_STATUS   => Registers.Invalid_Register,
         PORT_CLK_SEL   => Registers.PORT_CLK_SEL_DDIA),
      DIGI_B => DDI_Registers'
        (BUF_CTL        => Registers.DDI_BUF_CTL_B,
         BUF_TRANS      => DDI_Buf_Trans_Regs_Array'
                          (Registers.DDI_BUF_TRANS_B_S0T1,
                           Registers.DDI_BUF_TRANS_B_S0T2,
                           Registers.DDI_BUF_TRANS_B_S1T1,
                           Registers.DDI_BUF_TRANS_B_S1T2,
                           Registers.DDI_BUF_TRANS_B_S2T1,
                           Registers.DDI_BUF_TRANS_B_S2T2,
                           Registers.DDI_BUF_TRANS_B_S3T1,
                           Registers.DDI_BUF_TRANS_B_S3T2,
                           Registers.DDI_BUF_TRANS_B_S4T1,
                           Registers.DDI_BUF_TRANS_B_S4T2,
                           Registers.DDI_BUF_TRANS_B_S5T1,
                           Registers.DDI_BUF_TRANS_B_S5T2,
                           Registers.DDI_BUF_TRANS_B_S6T1,
                           Registers.DDI_BUF_TRANS_B_S6T2,
                           Registers.DDI_BUF_TRANS_B_S7T1,
                           Registers.DDI_BUF_TRANS_B_S7T2,
                           Registers.DDI_BUF_TRANS_B_S8T1,
                           Registers.DDI_BUF_TRANS_B_S8T2,
                           Registers.DDI_BUF_TRANS_B_S9T1,
                           Registers.DDI_BUF_TRANS_B_S9T2),
         DP_TP_CTL      => Registers.DP_TP_CTL_B,
         DP_TP_STATUS   => Registers.DP_TP_STATUS_B,
         PORT_CLK_SEL   => Registers.PORT_CLK_SEL_DDIB),
      DIGI_C => DDI_Registers'
        (BUF_CTL        => Registers.DDI_BUF_CTL_C,
         BUF_TRANS      => DDI_Buf_Trans_Regs_Array'
                          (Registers.DDI_BUF_TRANS_C_S0T1,
                           Registers.DDI_BUF_TRANS_C_S0T2,
                           Registers.DDI_BUF_TRANS_C_S1T1,
                           Registers.DDI_BUF_TRANS_C_S1T2,
                           Registers.DDI_BUF_TRANS_C_S2T1,
                           Registers.DDI_BUF_TRANS_C_S2T2,
                           Registers.DDI_BUF_TRANS_C_S3T1,
                           Registers.DDI_BUF_TRANS_C_S3T2,
                           Registers.DDI_BUF_TRANS_C_S4T1,
                           Registers.DDI_BUF_TRANS_C_S4T2,
                           Registers.DDI_BUF_TRANS_C_S5T1,
                           Registers.DDI_BUF_TRANS_C_S5T2,
                           Registers.DDI_BUF_TRANS_C_S6T1,
                           Registers.DDI_BUF_TRANS_C_S6T2,
                           Registers.DDI_BUF_TRANS_C_S7T1,
                           Registers.DDI_BUF_TRANS_C_S7T2,
                           Registers.DDI_BUF_TRANS_C_S8T1,
                           Registers.DDI_BUF_TRANS_C_S8T2,
                           Registers.DDI_BUF_TRANS_C_S9T1,
                           Registers.DDI_BUF_TRANS_C_S9T2),
         DP_TP_CTL      => Registers.DP_TP_CTL_C,
         DP_TP_STATUS   => Registers.DP_TP_STATUS_C,
         PORT_CLK_SEL   => Registers.PORT_CLK_SEL_DDIC),
      DIGI_D => DDI_Registers'
        (BUF_CTL        => Registers.DDI_BUF_CTL_D,
         BUF_TRANS      => DDI_Buf_Trans_Regs_Array'
                          (Registers.DDI_BUF_TRANS_D_S0T1,
                           Registers.DDI_BUF_TRANS_D_S0T2,
                           Registers.DDI_BUF_TRANS_D_S1T1,
                           Registers.DDI_BUF_TRANS_D_S1T2,
                           Registers.DDI_BUF_TRANS_D_S2T1,
                           Registers.DDI_BUF_TRANS_D_S2T2,
                           Registers.DDI_BUF_TRANS_D_S3T1,
                           Registers.DDI_BUF_TRANS_D_S3T2,
                           Registers.DDI_BUF_TRANS_D_S4T1,
                           Registers.DDI_BUF_TRANS_D_S4T2,
                           Registers.DDI_BUF_TRANS_D_S5T1,
                           Registers.DDI_BUF_TRANS_D_S5T2,
                           Registers.DDI_BUF_TRANS_D_S6T1,
                           Registers.DDI_BUF_TRANS_D_S6T2,
                           Registers.DDI_BUF_TRANS_D_S7T1,
                           Registers.DDI_BUF_TRANS_D_S7T2,
                           Registers.DDI_BUF_TRANS_D_S8T1,
                           Registers.DDI_BUF_TRANS_D_S8T2,
                           Registers.DDI_BUF_TRANS_D_S9T1,
                           Registers.DDI_BUF_TRANS_D_S9T2),
         DP_TP_CTL      => Registers.DP_TP_CTL_D,
         DP_TP_STATUS   => Registers.DP_TP_STATUS_D,
         PORT_CLK_SEL   => Registers.PORT_CLK_SEL_DDID),
      DIGI_E => DDI_Registers'
        (BUF_CTL        => Registers.DDI_BUF_CTL_E,
         BUF_TRANS      => DDI_Buf_Trans_Regs_Array'
                          (Registers.DDI_BUF_TRANS_E_S0T1,
                           Registers.DDI_BUF_TRANS_E_S0T2,
                           Registers.DDI_BUF_TRANS_E_S1T1,
                           Registers.DDI_BUF_TRANS_E_S1T2,
                           Registers.DDI_BUF_TRANS_E_S2T1,
                           Registers.DDI_BUF_TRANS_E_S2T2,
                           Registers.DDI_BUF_TRANS_E_S3T1,
                           Registers.DDI_BUF_TRANS_E_S3T2,
                           Registers.DDI_BUF_TRANS_E_S4T1,
                           Registers.DDI_BUF_TRANS_E_S4T2,
                           Registers.DDI_BUF_TRANS_E_S5T1,
                           Registers.DDI_BUF_TRANS_E_S5T2,
                           Registers.DDI_BUF_TRANS_E_S6T1,
                           Registers.DDI_BUF_TRANS_E_S6T2,
                           Registers.DDI_BUF_TRANS_E_S7T1,
                           Registers.DDI_BUF_TRANS_E_S7T2,
                           Registers.DDI_BUF_TRANS_E_S8T1,
                           Registers.DDI_BUF_TRANS_E_S8T2,
                           Registers.DDI_BUF_TRANS_E_S9T1,
                           Registers.DDI_BUF_TRANS_E_S9T2),
         DP_TP_CTL      => Registers.DP_TP_CTL_E,
         DP_TP_STATUS   => Registers.DP_TP_STATUS_E,
         PORT_CLK_SEL   => Registers.PORT_CLK_SEL_DDIE));

   ----------------------------------------------------------------------------

   type Values is array (Digital_Port) of Word32;
   type Shifts is array (Digital_Port) of Natural;

   DPLL_CTRL2_DDIx_CLOCK_OFF : constant Values := Values'
     (DIGI_A   => 1 * 2 ** 15,
      DIGI_B   => 1 * 2 ** 16,
      DIGI_C   => 1 * 2 ** 17,
      DIGI_D   => 1 * 2 ** 18,
      DIGI_E   => 1 * 2 ** 19);

   DPLL_CTRL2_DDIx_SELECT_MASK : constant Values := Values'
     (DIGI_A   => 3 * 2 **  1,
      DIGI_B   => 3 * 2 **  4,
      DIGI_C   => 3 * 2 **  7,
      DIGI_D   => 3 * 2 ** 10,
      DIGI_E   => 3 * 2 ** 13);
   DPLL_CTRL2_DDIx_SELECT_SHIFT : constant Shifts := Shifts'
     (DIGI_A   =>  1,
      DIGI_B   =>  4,
      DIGI_C   =>  7,
      DIGI_D   => 10,
      DIGI_E   => 13);

   DPLL_CTRL2_DDIx_SELECT_OVERRIDE : constant Values := Values'
     (DIGI_A   => 1 * 2 **  0,
      DIGI_B   => 1 * 2 **  3,
      DIGI_C   => 1 * 2 **  6,
      DIGI_D   => 1 * 2 **  9,
      DIGI_E   => 1 * 2 ** 12);

   ----------------------------------------------------------------------------

   function DDI_BUF_CTL_TRANS_SELECT
     (Sel : DDI_BUF_CTL_TRANS_SELECT_T)
      return Word32
   is
   begin
      return Word32 (Sel) * 2 ** 24;
   end DDI_BUF_CTL_TRANS_SELECT;

   ----------------------------------------------------------------------------

   procedure Program_Buffer_Translations (Port : Digital_Port)
   is
      Buffer_Translations : Buf_Trans_Array;
   begin
      Buffers.Translations (Buffer_Translations, Port);
      for I in Buf_Trans_Range loop
         Registers.Write
           (Register => DDI_Regs (Port).BUF_TRANS (I),
            Value    => Buffer_Translations (I));
      end loop;
   end Program_Buffer_Translations;

   procedure Initialize
   is
      Iboost_Value : constant Word32 := 1;
   begin
      if Config.Has_DDI_Buffer_Trans then
         for Port in Digital_Port range DIGI_A .. Config.Last_Digital_Port loop
            Program_Buffer_Translations (Port);
         end loop;
         if Config.Is_FDI_Port (Analog) then
            Program_Buffer_Translations (DIGI_E);
         end if;
      end if;

      if Config.Has_Iboost_Config then
         Registers.Unset_And_Set_Mask
           (Register    => Registers.DISPIO_CR_TX_BMU_CR0,
            Mask_Unset  => DISPIO_CR_TX_BLNC_LEG_DISBL_MASK or
                           DISPIO_CR_TX_BLNC_LEG_SCTL_4_MASK or
                           DISPIO_CR_TX_BLNC_LEG_SCTL_3_MASK or
                           DISPIO_CR_TX_BLNC_LEG_SCTL_2_MASK or
                           DISPIO_CR_TX_BLNC_LEG_SCTL_1_MASK or
                           DISPIO_CR_TX_BLNC_LEG_SCTL_0_MASK,
            Mask_Set    =>
               Shift_Left (Iboost_Value, DISPIO_CR_TX_BLNC_LEG_SCTL_4_SHIFT) or
               Shift_Left (Iboost_Value, DISPIO_CR_TX_BLNC_LEG_SCTL_3_SHIFT) or
               Shift_Left (Iboost_Value, DISPIO_CR_TX_BLNC_LEG_SCTL_2_SHIFT) or
               Shift_Left (Iboost_Value, DISPIO_CR_TX_BLNC_LEG_SCTL_1_SHIFT) or
               Shift_Left (Iboost_Value, DISPIO_CR_TX_BLNC_LEG_SCTL_0_SHIFT));
      end if;
   end Initialize;

   ----------------------------------------------------------------------------

   function Max_V_Swing
     (Port : Digital_Port)
      return DP_Info.DP_Voltage_Swing
   is
   begin
      return
        (if Config.Has_DDI_PHYs then
            DDI_Phy.Max_V_Swing
         elsif (Config.Has_Low_Voltage_Swing and Config.EDP_Low_Voltage_Swing)
            and then Port = DIGI_A
         then
            DP_Info.VS_Level_3
         else
            DP_Info.VS_Level_2);
   end Max_V_Swing;

   pragma Warnings (GNATprove, Off, "unused variable ""Port""",
                    Reason => "Needed for a common interface");
   function Max_Pre_Emph
     (Port        : Digital_Port;
      Train_Set   : DP_Info.Train_Set)
      return DP_Info.DP_Pre_Emph
   is
   begin
      return
        (if Config.Has_DDI_PHYs then
            DDI_Phy.Max_Pre_Emph (Train_Set.Voltage_Swing)
         else
           (case Train_Set.Voltage_Swing is
               when DP_Info.VS_Level_0 => DP_Info.Emph_Level_3,
               when DP_Info.VS_Level_1 => DP_Info.Emph_Level_2,
               when DP_Info.VS_Level_2 => DP_Info.Emph_Level_1,
               when others             => DP_Info.Emph_Level_0));
   end Max_Pre_Emph;
   pragma Warnings (GNATprove, On, "unused variable ""Port""");

   ----------------------------------------------------------------------------

   procedure Set_TP_CTL
     (Port     : Digital_Port;
      Link     : DP_Link;
      Pattern  : DP_Info.Training_Pattern)
   is
      DP_TP_CTL_Enhanced_Frame : Word32 := 0;
   begin
      if Link.Enhanced_Framing then
         DP_TP_CTL_Enhanced_Frame := DP_TP_CTL_ENHANCED_FRAME_ENABLE;
      end if;

      Registers.Write
        (Register => DDI_Regs (Port).DP_TP_CTL,
         Value    => DP_TP_CTL_TRANSPORT_ENABLE or
                     DP_TP_CTL_Enhanced_Frame or
                     DP_TP_CTL_LINK_TRAIN (Pattern));
   end Set_TP_CTL;

   procedure Set_Training_Pattern
     (Port     : Digital_Port;
      Link     : DP_Link;
      Pattern  : DP_Info.Training_Pattern)
   is
      use type DP_Info.Training_Pattern;
   begin
      if Pattern < DP_Info.TP_Idle then
         Set_TP_CTL (Port, Link, Pattern);
      else
         -- send at least 5 idle patterns
         Set_TP_CTL (Port, Link, DP_Info.TP_Idle);

         -- switch to normal frame delivery
         if Config.End_EDP_Training_Late and then Port = DIGI_A then
            null; -- do it later in Post_On procedure
            -- TODO: if there are problems getting the pipe up,
            --       wait here some time
            -- Time.U_Delay (100);
         else
            if Port /= DIGI_A then
               Registers.Wait_Set_Mask
                 (Register    => DDI_Regs (Port).DP_TP_STATUS,
                  Mask        => DP_TP_STATUS_MIN_IDLES_SENT);
            end if;
            Set_TP_CTL (Port, Link, DP_Info.TP_None);
         end if;
      end if;
   end Set_Training_Pattern;

   procedure Set_Signal_Levels
     (Port        : Digital_Port;
      Link        : DP_Link;
      Train_Set   : DP_Info.Train_Set)
   is
      Was_Enabled : Boolean;
      Trans_Select : DDI_BUF_CTL_TRANS_SELECT_T;
   begin
      Registers.Is_Set_Mask
        (Register => DDI_Regs (Port).BUF_CTL,
         Mask     => DDI_BUF_CTL_BUFFER_ENABLE,
         Result   => Was_Enabled);

      if Config.Has_DDI_PHYs then
         Trans_Select := 0;
      else
         case Train_Set.Voltage_Swing is
            when DP_Info.VS_Level_0 =>
               case Train_Set.Pre_Emph is
                  when DP_Info.Emph_Level_0  => Trans_Select := 0;
                  when DP_Info.Emph_Level_1  => Trans_Select := 1;
                  when DP_Info.Emph_Level_2  => Trans_Select := 2;
                  when DP_Info.Emph_Level_3  => Trans_Select := 3;
               end case;
            when DP_Info.VS_Level_1 =>
               case Train_Set.Pre_Emph is
                  when DP_Info.Emph_Level_0  => Trans_Select := 4;
                  when DP_Info.Emph_Level_1  => Trans_Select := 5;
                  when DP_Info.Emph_Level_2  => Trans_Select := 6;
                  when others                => Trans_Select := 0;
               end case;
            when DP_Info.VS_Level_2 =>
               case Train_Set.Pre_Emph is
                  when DP_Info.Emph_Level_0  => Trans_Select := 7;
                  when DP_Info.Emph_Level_1  => Trans_Select := 8;
                  when others                => Trans_Select := 0;
               end case;
            when DP_Info.VS_Level_3 =>
               case Train_Set.Pre_Emph is
                  when DP_Info.Emph_Level_0  => Trans_Select := 9;
                  when others                => Trans_Select := 0;
               end case;
         end case;
      end if;

      -- enable DDI buffer
      Registers.Unset_And_Set_Mask
        (Register    => DDI_Regs (Port).BUF_CTL,
         Mask_Unset  => DDI_BUF_CTL_TRANS_SELECT_MASK or
                        DDI_BUF_CTL_PORT_REVERSAL or
                        DDI_BUF_CTL_PORT_WIDTH_MASK,
         Mask_Set    => DDI_BUF_CTL_BUFFER_ENABLE or
                        DDI_BUF_CTL_TRANS_SELECT (Trans_Select) or
                        DDI_BUF_CTL_PORT_WIDTH (Link.Lane_Count));
      Registers.Posting_Read (DDI_Regs (Port).BUF_CTL);

      if not Was_Enabled then
         Time.U_Delay (600);  -- wait >= 518us (intel spec)
      end if;

      if Config.Has_DDI_PHYs then
         DDI_Phy.Set_DP_Signal_Levels (Port, Train_Set);
      end if;
   end Set_Signal_Levels;

   ----------------------------------------------------------------------------

   procedure Digital_Off (Port : Digital_Port)
   is
      Enabled : Boolean;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Is_Set_Mask
        (Register => DDI_Regs (Port).BUF_CTL,
         Mask     => DDI_BUF_CTL_BUFFER_ENABLE,
         Result   => Enabled);

      if Enabled then
         Registers.Unset_Mask
           (Register => DDI_Regs (Port).BUF_CTL,
            Mask     => DDI_BUF_CTL_BUFFER_ENABLE);
      end if;

      Registers.Unset_Mask
        (Register => DDI_Regs (Port).DP_TP_CTL,
         Mask     => DP_TP_CTL_TRANSPORT_ENABLE);

      if Enabled then
         Registers.Wait_Set_Mask
           (Register => DDI_Regs (Port).BUF_CTL,
            Mask     => DDI_BUF_CTL_IDLE_STATUS);
      end if;

      if Config.Has_Per_DDI_Clock_Sel then
         Registers.Write
           (Register => DDI_Regs (Port).PORT_CLK_SEL,
            Value    => PORT_CLK_SEL_NONE);
      elsif not Config.Has_DDI_PHYs then
         Registers.Set_Mask
           (Register => Registers.DPLL_CTRL2,
            Mask     => DPLL_CTRL2_DDIx_CLOCK_OFF (Port));
      end if;
   end Digital_Off;

   ----------------------------------------------------------------------------

   procedure Train_FDI
     (Port_Cfg : in     Port_Config;
      Success  :    out Boolean)
   is
   begin
      PCH.FDI.Pre_Train (PCH.FDI_A, Port_Cfg);

      -- always use SPLL for FDI
      SPLL.On;
      Registers.Write
        (Register => DDI_Regs (DIGI_E).PORT_CLK_SEL,
         Value    => PORT_CLK_SEL_SPLL);

      -- try each preemph/voltage pair twice
      for Trans2 in Natural range 0 .. DDI_BUF_CTL_TRANS_SELECT_T'Last * 2 + 1
      loop
         Registers.Write
           (Register => DDI_Regs (DIGI_E).DP_TP_CTL,
            Value    => DP_TP_CTL_TRANSPORT_ENABLE or
                        DP_TP_CTL_ENHANCED_FRAME_ENABLE or
                        DP_TP_CTL_FDI_AUTOTRAIN or
                        DP_TP_CTL_LINK_TRAIN_PAT1);

         Registers.Unset_And_Set_Mask
           (Register    => DDI_Regs (DIGI_E).BUF_CTL,
            Mask_Unset  => DDI_BUF_CTL_TRANS_SELECT_MASK or
                           DDI_BUF_CTL_PORT_REVERSAL or
                           DDI_BUF_CTL_PORT_WIDTH_MASK,
            Mask_Set    => DDI_BUF_CTL_BUFFER_ENABLE or
                           DDI_BUF_CTL_TRANS_SELECT (Trans2 / 2) or
                           DDI_BUF_CTL_PORT_WIDTH (Port_Cfg.FDI.Lane_Count));
         Registers.Posting_Read (DDI_Regs (DIGI_E).BUF_CTL);
         Time.U_Delay (600);  -- wait >= 518us (intel spec)

         PCH.FDI.Auto_Train (PCH.FDI_A);
         Registers.Is_Set_Mask
           (Register => DDI_Regs (DIGI_E).DP_TP_STATUS,
            Mask     => DP_TP_STATUS_FDI_AUTO_TRAIN_DONE,
            Result   => Success);
         exit when Success;

         Registers.Unset_Mask
           (Register => DDI_Regs (DIGI_E).BUF_CTL,
            Mask     => DDI_BUF_CTL_BUFFER_ENABLE);
         Registers.Posting_Read (DDI_Regs (DIGI_E).BUF_CTL);

         Registers.Unset_And_Set_Mask
           (Register    => DDI_Regs (DIGI_E).DP_TP_CTL,
            Mask_Unset  => DP_TP_CTL_TRANSPORT_ENABLE or
                           DP_TP_CTL_LINK_TRAIN_MASK,
            Mask_Set    => DP_TP_CTL_LINK_TRAIN_PAT1);
         Registers.Posting_Read (DDI_Regs (DIGI_E).DP_TP_CTL);

         Registers.Wait_Set_Mask
           (Register => DDI_Regs (DIGI_E).BUF_CTL,
            Mask     => DDI_BUF_CTL_IDLE_STATUS);

         PCH.FDI.Off (PCH.FDI_A, PCH.FDI.Lanes_Off);
      end loop;

      if Success then
         -- start normal frame delivery
         Registers.Write
           (Register => DDI_Regs (DIGI_E).DP_TP_CTL,
            Value    => DP_TP_CTL_TRANSPORT_ENABLE or
                        DP_TP_CTL_ENHANCED_FRAME_ENABLE or
                        DP_TP_CTL_FDI_AUTOTRAIN or
                        DP_TP_CTL_LINK_TRAIN_NORMAL);
      else
         Registers.Write
           (Register => DDI_Regs (DIGI_E).PORT_CLK_SEL,
            Value    => PORT_CLK_SEL_NONE);
         SPLL.Off;

         PCH.FDI.Off (PCH.FDI_A, PCH.FDI.Clock_Off);
      end if;
   end Train_FDI;

   ----------------------------------------------------------------------------

   procedure Pre_On
     (Port_Cfg : in     Port_Config;
      PLL_Hint : in     Word32;
      Success  :    out Boolean)
   is
      function To_DP (Port : Digital_Port) return DP_Port
      is
      begin
         return
           (case Port is
               when DIGI_A => DP_A,
               when DIGI_B => DP_B,
               when DIGI_C => DP_C,
               when DIGI_D => DP_D,
               when others => DP_Port'First);
      end To_DP;
      package Training is new DP_Training
        (TPS3_Supported    => True,
         T                 => Digital_Port,
         Aux_T             => DP_Port,
         Aux_Ch            => DP_Aux_Ch,
         DP_Info           => DP_Info,
         To_Aux            => To_DP,
         Max_V_Swing       => Max_V_Swing,
         Max_Pre_Emph      => Max_Pre_Emph,
         Set_Pattern       => Set_Training_Pattern,
         Set_Signal_Levels => Set_Signal_Levels,
         Off               => Digital_Off);
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Display = VGA then
         Train_FDI (Port_Cfg, Success);
      else
         -- direct configured PLL output to this port
         if Config.Has_Per_DDI_Clock_Sel then
            Registers.Write
              (Register    => DDI_Regs (Port_Cfg.Port).PORT_CLK_SEL,
               Value       => PLL_Hint);
         elsif not Config.Has_DDI_PHYs then
            Registers.Unset_And_Set_Mask
              (Register    => Registers.DPLL_CTRL2,
               Mask_Unset  => DPLL_CTRL2_DDIx_CLOCK_OFF (Port_Cfg.Port) or
                              DPLL_CTRL2_DDIx_SELECT_MASK (Port_Cfg.Port),
               Mask_Set    => Shift_Left
                                (PLL_Hint,
                                 DPLL_CTRL2_DDIx_SELECT_SHIFT (Port_Cfg.Port))
                              or
                              DPLL_CTRL2_DDIx_SELECT_OVERRIDE (Port_Cfg.Port));
         end if;

         if Port_Cfg.Display = DP then
            Training.Train_DP
              (Port        => Port_Cfg.Port,
               Link        => Port_Cfg.DP,
               Success     => Success);
         elsif Config.Has_DDI_PHYs and then
            Port_Cfg.Display = HDMI and then
            Port_Cfg.Port in DDI_Phy.DDI_Phy_Port
         then
            declare
               HDMI_Level : constant DDI_Phy.HDMI_Buf_Trans_Range :=
                 (if Config.DDI_HDMI_Buffer_Translation
                     in DDI_Phy.HDMI_Buf_Trans_Range
                  then Config.DDI_HDMI_Buffer_Translation
                  else Config.Default_DDI_HDMI_Buffer_Translation);
            begin
               DDI_Phy.Set_HDMI_Signal_Levels (Port_Cfg.Port, HDMI_Level);
               Success := True;
            end;
         else
            Success := True;
         end if;
      end if;
   end Pre_On;

   ----------------------------------------------------------------------------

   procedure Post_On (Port_Cfg : Port_Config)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Port = DIGI_A then
         if Config.End_EDP_Training_Late then
            Registers.Unset_And_Set_Mask
              (Register    => DDI_Regs (DIGI_A).DP_TP_CTL,
               Mask_Unset  => DP_TP_CTL_LINK_TRAIN_MASK,
               Mask_Set    => DP_TP_CTL_LINK_TRAIN_NORMAL);
         end if;
      end if;

      case Port_Cfg.Display is
         when HDMI =>
            Registers.Unset_And_Set_Mask
              (Register    => DDI_Regs (Port_Cfg.Port).BUF_CTL,
               Mask_Unset  => DDI_BUF_CTL_TRANS_SELECT_MASK or
                              DDI_BUF_CTL_PORT_REVERSAL,
               Mask_Set    => DDI_BUF_CTL_BUFFER_ENABLE);
            Time.U_Delay (600);  -- wait >= 518us (intel spec)
         when VGA =>
            PCH.VGA.Clock_On (Port_Cfg.Mode);
            PCH.Transcoder.On (Port_Cfg, PCH.FDI_A, 0);
            PCH.VGA.On
              (Port => PCH.FDI_A,
               Mode => Port_Cfg.Mode);
         when others =>
            null;
      end case;
   end Post_On;

   ----------------------------------------------------------------------------

   procedure Off (Port : Digital_Port)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port = DIGI_E then
         PCH.VGA.Off;
         PCH.Transcoder.Off (PCH.FDI_A);
         -- PCH.VGA.Clock_Off; -- Can't tell what Linux does, if anything.
         PCH.FDI.Off (PCH.FDI_A, PCH.FDI.Rx_Off);
      end if;

      Digital_Off (Port);

      if Port = DIGI_E then
         SPLL.Off;
         PCH.FDI.Off (PCH.FDI_A, PCH.FDI.Clock_Off);
      end if;
   end Off;

   ----------------------------------------------------------------------------

   procedure Post_Reset_Off
   is
      Clocks_Off : Word32 := 0;
   begin
      if not Config.Has_Per_DDI_Clock_Sel and not Config.Has_DDI_PHYs then
         for Port in Digital_Port loop
            Clocks_Off := Clocks_Off or DPLL_CTRL2_DDIx_CLOCK_OFF (Port);
         end loop;
         Registers.Set_Mask (Registers.DPLL_CTRL2, Clocks_Off);
      end if;
   end Post_Reset_Off;

end HW.GFX.GMA.Connectors.DDI;
