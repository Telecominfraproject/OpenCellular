--
-- Copyright (C) 2014-2017 secunet Security Networks AG
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
with HW.GFX.GMA.Power_And_Clocks_Haswell;
with HW.GFX.GMA.DDI_Phy;

use HW.GFX.GMA.Registers;

package body HW.GFX.GMA.Power_And_Clocks is

   type Power_Domain is (PW1, PW2, DDI_A, DDI_BC);
   subtype Power_Well is Power_Domain range PW1 .. PW2;
   subtype Dynamic_Domain is Power_Domain range PW2 .. DDI_BC;
   subtype DDI_Domain is Dynamic_Domain range DDI_A .. DDI_BC;

   type DDI_Phy_Array is array (DDI_Domain) of DDI_Phy.T;
   Phy : constant DDI_Phy_Array := (DDI_A => DDI_Phy.A, DDI_BC => DDI_Phy.BC);

   NDE_RSTWRN_OPT_RST_PCH_Handshake_En : constant := 1 * 2 **  4;

   FUSE_STATUS_DOWNLOAD_STATUS         : constant := 1 * 2 ** 31;
   FUSE_STATUS_PG0_DIST_STATUS         : constant := 1 * 2 ** 27;

   type Power_Well_Values is array (Power_Well) of Word32;
   PWR_WELL_CTL_POWER_REQUEST : constant Power_Well_Values :=
     (PW1      => 1 * 2 ** 29,
      PW2      => 1 * 2 ** 31);
   PWR_WELL_CTL_POWER_STATE : constant Power_Well_Values :=
     (PW1      => 1 * 2 ** 28,
      PW2      => 1 * 2 ** 30);

   FUSE_STATUS_PGx_DIST_STATUS : constant Power_Well_Values :=
     (PW1   => 1 * 2 ** 26,
      PW2   => 1 * 2 ** 25);

   DBUF_CTL_DBUF_POWER_REQUEST         : constant := 1 * 2 ** 31;
   DBUF_CTL_DBUF_POWER_STATE           : constant := 1 * 2 ** 30;

   ----------------------------------------------------------------------------

   BXT_DE_PLL_RATIO_MASK               : constant :=      16#ff#;
   BXT_DE_PLL_PLL_ENABLE               : constant := 1 * 2 ** 31;
   BXT_DE_PLL_PLL_LOCK                 : constant := 1 * 2 ** 30;

   CDCLK_CD2X_DIV_SEL_MASK             : constant := 3 * 2 ** 22;
   CDCLK_CD2X_DIV_SEL_1                : constant := 0 * 2 ** 22;
   CDCLK_CD2X_DIV_SEL_1_5              : constant := 1 * 2 ** 22;
   CDCLK_CD2X_DIV_SEL_2                : constant := 2 * 2 ** 22;
   CDCLK_CD2X_DIV_SEL_4                : constant := 3 * 2 ** 22;
   CDCLK_CD2X_PIPE_NONE                : constant := 3 * 2 ** 20;
   CDCLK_CD2X_SSA_PRECHARGE_ENABLE     : constant := 1 * 2 ** 16;
   CDCLK_CTL_CD_FREQ_DECIMAL_MASK      : constant :=     16#7ff#;

   function CDCLK_CTL_CD_FREQ_DECIMAL (Freq : Positive) return Word32 is
   begin
      return Word32 (2 * (Freq / 1_000_000 - 1));
   end CDCLK_CTL_CD_FREQ_DECIMAL;

   BXT_PCODE_CDCLK_CONTROL             : constant := 16#17#;
   BXT_CDCLK_PREPARE_FOR_CHANGE        : constant := 16#8000_0000#;

   ----------------------------------------------------------------------------

   procedure PW_Off (PD : Power_Well)
   is
      Ctl1, Ctl2, Ctl3, Ctl4 : Word32;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Read (PWR_WELL_CTL_BIOS, Ctl1);
      Read (PWR_WELL_CTL_DRIVER, Ctl2);
      Read (PWR_WELL_CTL_KVMR, Ctl3);
      Read (PWR_WELL_CTL_DEBUG, Ctl4);
      pragma Debug (Posting_Read (PWR_WELL_CTL5)); --  Result for debugging only
      pragma Debug (Posting_Read (PWR_WELL_CTL6)); --  Result for debugging only

      if ((Ctl1 or Ctl2 or Ctl3 or Ctl4) and
          PWR_WELL_CTL_POWER_REQUEST (PD)) /= 0
      then
         Wait_Set_Mask (PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_POWER_STATE (PD));
      end if;

      if (Ctl1 and PWR_WELL_CTL_POWER_REQUEST (PD)) /= 0 then
         Unset_Mask (PWR_WELL_CTL_BIOS, PWR_WELL_CTL_POWER_REQUEST (PD));
      end if;

      if (Ctl2 and PWR_WELL_CTL_POWER_REQUEST (PD)) /= 0 then
         Unset_Mask (PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_POWER_REQUEST (PD));
      end if;
   end PW_Off;

   procedure PW_On (PD : Power_Well)
   with
      Pre => True
   is
      Ctl1, Ctl2, Ctl3, Ctl4 : Word32;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Read (PWR_WELL_CTL_BIOS, Ctl1);
      Read (PWR_WELL_CTL_DRIVER, Ctl2);
      Read (PWR_WELL_CTL_KVMR, Ctl3);
      Read (PWR_WELL_CTL_DEBUG, Ctl4);
      pragma Debug (Posting_Read (PWR_WELL_CTL5)); --  Result for debugging only
      pragma Debug (Posting_Read (PWR_WELL_CTL6)); --  Result for debugging only

      if ((Ctl1 or Ctl2 or Ctl3 or Ctl4) and
          PWR_WELL_CTL_POWER_REQUEST (PD)) = 0
      then
         Wait_Unset_Mask (PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_POWER_STATE (PD));
      end if;

      if (Ctl2 and PWR_WELL_CTL_POWER_REQUEST (PD)) = 0 then
         Set_Mask (PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_POWER_REQUEST (PD));
         Wait_Set_Mask (PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_POWER_STATE (PD));

         Wait_Set_Mask (FUSE_STATUS, FUSE_STATUS_PGx_DIST_STATUS (PD));
      end if;
   end PW_On;

   procedure PD_On (PD : Power_Domain) is
   begin
      if PD in Power_Well then
         PW_On (PD);
      else
         DDI_Phy.Power_On (Phy (PD));
      end if;
   end PD_On;

   procedure PD_Off (PD : Power_Domain) is
   begin
      if PD in Power_Well then
         PW_Off (PD);
      else
         DDI_Phy.Power_Off (Phy (PD));
      end if;
   end PD_Off;

   function Need_PD (PD : Dynamic_Domain; Configs : Pipe_Configs) return Boolean
   is
   begin
      return (case PD is
         when DDI_A     => Configs (Primary).Port = Internal or
                           Configs (Secondary).Port = Internal or
                           Configs (Tertiary).Port = Internal,
         when DDI_BC    => Configs (Primary).Port = HDMI1 or
                           Configs (Primary).Port = DP1 or
                           Configs (Secondary).Port = HDMI1 or
                           Configs (Secondary).Port = DP1 or
                           Configs (Tertiary).Port = HDMI1 or
                           Configs (Tertiary).Port = DP1 or
                           Configs (Primary).Port = HDMI2 or
                           Configs (Primary).Port = DP2 or
                           Configs (Secondary).Port = HDMI2 or
                           Configs (Secondary).Port = DP2 or
                           Configs (Tertiary).Port = HDMI2 or
                           Configs (Tertiary).Port = DP2,
         when PW2       => (Configs (Primary).Port /= Disabled and
                            Configs (Primary).Port /= Internal) or
                           Configs (Secondary).Port /= Disabled or
                           Configs (Tertiary).Port /= Disabled);
   end Need_PD;

   procedure Power_Set_To (Configs : Pipe_Configs) is
   begin
      for PD in reverse Dynamic_Domain loop
         if not Need_PD (PD, Configs) then
            PD_Off (PD);
         end if;
      end loop;
      for PD in Dynamic_Domain loop
         if Need_PD (PD, Configs) then
            PD_On (PD);
         end if;
      end loop;
   end Power_Set_To;

   procedure Power_Up (Old_Configs, New_Configs : Pipe_Configs) is
   begin
      for PD in Dynamic_Domain loop
         if not Need_PD (PD, Old_Configs) and Need_PD (PD, New_Configs) then
            PD_On (PD);
         end if;
      end loop;
   end Power_Up;

   procedure Power_Down (Old_Configs, Tmp_Configs, New_Configs : Pipe_Configs)
   is
   begin
      for PD in reverse Dynamic_Domain loop
         if (Need_PD (PD, Old_Configs) or Need_PD (PD, Tmp_Configs)) and
            not Need_PD (PD, New_Configs)
         then
            PD_Off (PD);
         end if;
      end loop;
   end Power_Down;

   ----------------------------------------------------------------------------

   CDClk_Ref : constant := 19_200_000;

   procedure Set_CDClk (Freq : Positive)
   with
      Pre =>
         Freq =   CDClk_Ref or Freq = 144_000_000 or Freq = 288_000_000 or
         Freq = 384_000_000 or Freq = 576_000_000 or Freq = 624_000_000
   is
      VCO : constant Natural :=
         CDClk_Ref *
           (if Freq = CDClk_Ref then
               0
            elsif Freq = 624_000_000 then
               65
            else
               60);
      CDCLK_CD2X_Div_Sel : constant Word32 :=
        (case VCO / Freq is   -- CDClk = VCO / 2 / Div
            when 2      => CDCLK_CD2X_DIV_SEL_1,
            when 3      => CDCLK_CD2X_DIV_SEL_1_5,
            when 4      => CDCLK_CD2X_DIV_SEL_2,
            when 8      => CDCLK_CD2X_DIV_SEL_4,
            when others => CDCLK_CD2X_DIV_SEL_1);  -- for CDClk = CDClk_Ref
      CDCLK_CD2X_SSA_Precharge : constant Word32 :=
        (if Freq >= 500_000_000 then CDCLK_CD2X_SSA_PRECHARGE_ENABLE else 0);
   begin
      Power_And_Clocks_Haswell.GT_Mailbox_Write
        (MBox        => BXT_PCODE_CDCLK_CONTROL,
         Value       => BXT_CDCLK_PREPARE_FOR_CHANGE);

      Write
        (Register => BXT_DE_PLL_ENABLE,
         Value    => 16#0000_0000#);
      Wait_Unset_Mask
        (Register => BXT_DE_PLL_ENABLE,
         Mask     => BXT_DE_PLL_PLL_LOCK,
         TOut_MS  => 1);   -- 200us

      Unset_And_Set_Mask
        (Register    => BXT_DE_PLL_CTL,
         Mask_Unset  => BXT_DE_PLL_RATIO_MASK,
         Mask_Set    => Word32 (VCO / CDClk_Ref));
      Write
        (Register => BXT_DE_PLL_ENABLE,
         Value    => BXT_DE_PLL_PLL_ENABLE);
      Wait_Set_Mask
        (Register => BXT_DE_PLL_ENABLE,
         Mask     => BXT_DE_PLL_PLL_LOCK,
         TOut_MS  => 1);   -- 200us

      Write
        (Register => CDCLK_CTL,
         Value    => CDCLK_CD2X_Div_Sel or
                     CDCLK_CD2X_PIPE_NONE or
                     CDCLK_CD2X_SSA_Precharge or
                     CDCLK_CTL_CD_FREQ_DECIMAL (Freq));

      Power_And_Clocks_Haswell.GT_Mailbox_Write
        (MBox        => BXT_PCODE_CDCLK_CONTROL,
         Value       => Word32 ((Freq + (25_000_000 - 1)) / 25_000_000));
   end Set_CDClk;

   ----------------------------------------------------------------------------

   procedure Pre_All_Off is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));
      Power_And_Clocks_Haswell.PSR_Off;
   end Pre_All_Off;

   procedure Post_All_Off is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      for PD in reverse Dynamic_Domain loop
         PD_Off (PD);
      end loop;

      Unset_Mask (DBUF_CTL, DBUF_CTL_DBUF_POWER_REQUEST);
      Wait_Unset_Mask (DBUF_CTL, DBUF_CTL_DBUF_POWER_STATE);

      -- Linux' i915 never keeps the PLL disabled but runs it
      -- at a "ratio" of 0 with CDClk at its reference clock.
      Set_CDClk (CDClk_Ref);

      PW_Off (PW1);
   end Post_All_Off;

   procedure Initialize is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      -- no PCH for Broxton
      Unset_Mask (NDE_RSTWRN_OPT, NDE_RSTWRN_OPT_RST_PCH_Handshake_En);

      Wait_Set_Mask (FUSE_STATUS, FUSE_STATUS_PG0_DIST_STATUS);
      PW_On (PW1);

      Set_CDClk (Positive (Config.Default_CDClk_Freq));

      Set_Mask (DBUF_CTL, DBUF_CTL_DBUF_POWER_REQUEST);
      Wait_Set_Mask (DBUF_CTL, DBUF_CTL_DBUF_POWER_STATE);
   end Initialize;

end HW.GFX.GMA.Power_And_Clocks;
