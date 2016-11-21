--
-- Copyright (C) 2014-2016 secunet Security Networks AG
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

with HW.Time;
with HW.Debug;
with HW.GFX.GMA.Config;
with HW.GFX.GMA.Registers;
with HW.GFX.GMA.Power_And_Clocks_Haswell;

use type HW.Word64;

package body HW.GFX.GMA.Power_And_Clocks_Skylake is

   type Power_Domain is (MISC_IO, PW1, PW2, DDI_AE, DDI_B, DDI_C, DDI_D);
   subtype Power_Well is Power_Domain range PW1 .. PW2;
   subtype Dynamic_Domain is Power_Domain range PW2 .. DDI_D;

   NDE_RSTWRN_OPT_RST_PCH_Handshake_En : constant := 1 * 2 **  4;

   FUSE_STATUS_DOWNLOAD_STATUS         : constant := 1 * 2 ** 31;
   FUSE_STATUS_PG0_DIST_STATUS         : constant := 1 * 2 ** 27;

   type Power_Domain_Values is array (Power_Domain) of Word32;
   PWR_WELL_CTL_POWER_REQUEST : constant Power_Domain_Values :=
     (MISC_IO  => 1 * 2 **  1,
      DDI_AE   => 1 * 2 **  3,
      DDI_B    => 1 * 2 **  5,
      DDI_C    => 1 * 2 **  7,
      DDI_D    => 1 * 2 **  9,
      PW1      => 1 * 2 ** 29,
      PW2      => 1 * 2 ** 31);
   PWR_WELL_CTL_POWER_STATE : constant Power_Domain_Values :=
     (MISC_IO  => 1 * 2 **  0,
      DDI_AE   => 1 * 2 **  2,
      DDI_B    => 1 * 2 **  4,
      DDI_C    => 1 * 2 **  6,
      DDI_D    => 1 * 2 **  8,
      PW1      => 1 * 2 ** 28,
      PW2      => 1 * 2 ** 30);

   type Power_Well_Values is array (Power_Well) of Word32;
   FUSE_STATUS_PGx_DIST_STATUS : constant Power_Well_Values :=
     (PW1   => 1 * 2 ** 26,
      PW2   => 1 * 2 ** 25);

   DBUF_CTL_DBUF_POWER_REQUEST         : constant := 1 * 2 ** 31;
   DBUF_CTL_DBUF_POWER_STATE           : constant := 1 * 2 ** 30;

   ----------------------------------------------------------------------------

   DPLL_CTRL1_DPLL0_LINK_RATE_MASK     : constant := 7 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_2700MHZ  : constant := 0 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_1350MHZ  : constant := 1 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_810MHZ   : constant := 2 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_1620MHZ  : constant := 3 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_1080MHZ  : constant := 4 * 2 **  1;
   DPLL_CTRL1_DPLL0_LINK_RATE_2160MHZ  : constant := 5 * 2 **  1;
   DPLL_CTRL1_DPLL0_OVERRIDE           : constant := 1 * 2 **  0;

   LCPLL1_CTL_PLL_ENABLE               : constant := 1 * 2 ** 31;
   LCPLL1_CTL_PLL_LOCK                 : constant := 1 * 2 ** 30;

   ----------------------------------------------------------------------------

   CDCLK_CTL_CD_FREQ_SELECT_MASK       : constant := 3 * 2 ** 26;
   CDCLK_CTL_CD_FREQ_SELECT_450MHZ     : constant := 0 * 2 ** 26;
   CDCLK_CTL_CD_FREQ_SELECT_540MHZ     : constant := 1 * 2 ** 26;
   CDCLK_CTL_CD_FREQ_SELECT_337_5MHZ   : constant := 2 * 2 ** 26;
   CDCLK_CTL_CD_FREQ_SELECT_675MHZ     : constant := 3 * 2 ** 26;
   CDCLK_CTL_CD_FREQ_DECIMAL_MASK      : constant :=     16#7ff#;

   SKL_PCODE_CDCLK_CONTROL             : constant := 7;
   SKL_CDCLK_PREPARE_FOR_CHANGE        : constant := 3;
   SKL_CDCLK_READY_FOR_CHANGE          : constant := 1;

   GT_MAILBOX_READY                    : constant := 1 * 2 ** 31;

   function CDCLK_CTL_CD_FREQ_DECIMAL
     (Freq        : Positive;
      Plus_Half   : Boolean)
      return Word32 is
   begin
      return Word32 (2 * (Freq - 1)) or (if Plus_Half then 1 else 0);
   end CDCLK_CTL_CD_FREQ_DECIMAL;

   ----------------------------------------------------------------------------

   procedure GT_Mailbox_Write (MBox : Word32; Value : Word64) is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Wait_Unset_Mask (Registers.GT_MAILBOX, GT_MAILBOX_READY);
      Registers.Write
        (Registers.GT_MAILBOX_DATA, Word32 (Value and 16#ffff_ffff#));
      Registers.Write
        (Registers.GT_MAILBOX_DATA_1, Word32 (Shift_Right (Value, 32)));
      Registers.Write (Registers.GT_MAILBOX, GT_MAILBOX_READY or MBox);

      Registers.Wait_Unset_Mask (Registers.GT_MAILBOX, GT_MAILBOX_READY);
   end GT_Mailbox_Write;

   ----------------------------------------------------------------------------

   procedure PD_Off (PD : Power_Domain)
   is
      Ctl1, Ctl2, Ctl3, Ctl4 : Word32;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Read (Registers.PWR_WELL_CTL_BIOS, Ctl1);
      Registers.Read (Registers.PWR_WELL_CTL_DRIVER, Ctl2);
      Registers.Read (Registers.PWR_WELL_CTL_KVMR, Ctl3);
      Registers.Read (Registers.PWR_WELL_CTL_DEBUG, Ctl4);
      pragma Debug (Registers.Posting_Read (Registers.PWR_WELL_CTL5)); --  Result for debugging only
      pragma Debug (Registers.Posting_Read (Registers.PWR_WELL_CTL6)); --  Result for debugging only

      if ((Ctl1 or Ctl2 or Ctl3 or Ctl4) and
          PWR_WELL_CTL_POWER_REQUEST (PD)) /= 0
      then
         Registers.Wait_Set_Mask
           (Register => Registers.PWR_WELL_CTL_DRIVER,
            Mask     => PWR_WELL_CTL_POWER_STATE (PD));
      end if;

      if (Ctl1 and PWR_WELL_CTL_POWER_REQUEST (PD)) /= 0 then
         Registers.Unset_Mask
           (Register => Registers.PWR_WELL_CTL_BIOS,
            Mask     => PWR_WELL_CTL_POWER_REQUEST (PD));
      end if;

      if (Ctl2 and PWR_WELL_CTL_POWER_REQUEST (PD)) /= 0 then
         Registers.Unset_Mask
           (Register => Registers.PWR_WELL_CTL_DRIVER,
            Mask     => PWR_WELL_CTL_POWER_REQUEST (PD));
      end if;
   end PD_Off;

   procedure PD_On (PD : Power_Domain)
   with
      Pre => True
   is
      Ctl1, Ctl2, Ctl3, Ctl4 : Word32;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Read (Registers.PWR_WELL_CTL_BIOS, Ctl1);
      Registers.Read (Registers.PWR_WELL_CTL_DRIVER, Ctl2);
      Registers.Read (Registers.PWR_WELL_CTL_KVMR, Ctl3);
      Registers.Read (Registers.PWR_WELL_CTL_DEBUG, Ctl4);
      pragma Debug (Registers.Posting_Read (Registers.PWR_WELL_CTL5)); --  Result for debugging only
      pragma Debug (Registers.Posting_Read (Registers.PWR_WELL_CTL6)); --  Result for debugging only

      if ((Ctl1 or Ctl2 or Ctl3 or Ctl4) and
          PWR_WELL_CTL_POWER_REQUEST (PD)) = 0
      then
         Registers.Wait_Unset_Mask
           (Register => Registers.PWR_WELL_CTL_DRIVER,
            Mask     => PWR_WELL_CTL_POWER_STATE (PD));
      end if;

      if (Ctl2 and PWR_WELL_CTL_POWER_REQUEST (PD)) = 0 then
         Registers.Set_Mask
           (Register => Registers.PWR_WELL_CTL_DRIVER,
            Mask     => PWR_WELL_CTL_POWER_REQUEST (PD));
         Registers.Wait_Set_Mask
           (Register => Registers.PWR_WELL_CTL_DRIVER,
            Mask     => PWR_WELL_CTL_POWER_STATE (PD));

         if PD in Power_Well then
            Registers.Wait_Set_Mask
              (Register => Registers.FUSE_STATUS,
               Mask     => FUSE_STATUS_PGx_DIST_STATUS (PD));
         end if;
      end if;
   end PD_On;

   function Need_PD (PD : Dynamic_Domain; Configs : Pipe_Configs) return Boolean
   is
   begin
      return (case PD is
         when DDI_AE    => Configs (Primary).Port = Internal or
                           Configs (Secondary).Port = Internal or
                           Configs (Tertiary).Port = Internal,
         when DDI_B     => Configs (Primary).Port = HDMI1 or
                           Configs (Primary).Port = DP1 or
                           Configs (Secondary).Port = HDMI1 or
                           Configs (Secondary).Port = DP1 or
                           Configs (Tertiary).Port = HDMI1 or
                           Configs (Tertiary).Port = DP1,
         when DDI_C     => Configs (Primary).Port = HDMI2 or
                           Configs (Primary).Port = DP2 or
                           Configs (Secondary).Port = HDMI2 or
                           Configs (Secondary).Port = DP2 or
                           Configs (Tertiary).Port = HDMI2 or
                           Configs (Tertiary).Port = DP2,
         when DDI_D     => Configs (Primary).Port = HDMI3 or
                           Configs (Primary).Port = DP3 or
                           Configs (Secondary).Port = HDMI3 or
                           Configs (Secondary).Port = DP3 or
                           Configs (Tertiary).Port = HDMI3 or
                           Configs (Tertiary).Port = DP3,
         when PW2       => (Configs (Primary).Port /= Disabled and
                            Configs (Primary).Port /= Internal) or
                           Configs (Secondary).Port /= Disabled or
                           Configs (Tertiary).Port /= Disabled);
   end Need_PD;

   ----------------------------------------------------------------------------

   procedure Pre_All_Off is
   begin
      Power_And_Clocks_Haswell.PSR_Off;
   end Pre_All_Off;

   procedure Post_All_Off is
   begin
      for PD in reverse Dynamic_Domain loop
         PD_Off (PD);
      end loop;

      Registers.Unset_Mask
        (Register    => Registers.DBUF_CTL,
         Mask        => DBUF_CTL_DBUF_POWER_REQUEST);
      Registers.Wait_Unset_Mask
        (Register    => Registers.DBUF_CTL,
         Mask        => DBUF_CTL_DBUF_POWER_STATE);

      Registers.Unset_Mask
        (Register    => Registers.LCPLL1_CTL,
         Mask        => LCPLL1_CTL_PLL_ENABLE);
      Registers.Wait_Unset_Mask
        (Register    => Registers.LCPLL1_CTL,
         Mask        => LCPLL1_CTL_PLL_LOCK);

      PD_Off (MISC_IO);
      PD_Off (PW1);
   end Post_All_Off;

   procedure Initialize
   is
      CDClk_Change_Timeout : Time.T;
      Timed_Out : Boolean;

      MBox_Data0 : Word32;
   begin
      Registers.Set_Mask
        (Register    => Registers.NDE_RSTWRN_OPT,
         Mask        => NDE_RSTWRN_OPT_RST_PCH_Handshake_En);

      Registers.Wait_Set_Mask
        (Register    => Registers.FUSE_STATUS,
         Mask        => FUSE_STATUS_PG0_DIST_STATUS);
      PD_On (PW1);
      PD_On (MISC_IO);

      Registers.Write
        (Register    => Registers.CDCLK_CTL,
         Value       => CDCLK_CTL_CD_FREQ_SELECT_337_5MHZ or
                        CDCLK_CTL_CD_FREQ_DECIMAL (337, True));
      -- TODO: Set to preferred eDP rate:
      -- Registers.Unset_And_Set_Mask
      --   (Register    => Registers.DPLL_CTRL1,
      --    Unset_Mask  => DPLL_CTRL1_DPLL0_LINK_RATE_MASK,
      --    Set_Mask    => DPLL_CTRL1_DPLL0_LINK_RATE_...);
      Registers.Set_Mask
        (Register    => Registers.LCPLL1_CTL,
         Mask        => LCPLL1_CTL_PLL_ENABLE);
      Registers.Wait_Set_Mask
        (Register    => Registers.LCPLL1_CTL,
         Mask        => LCPLL1_CTL_PLL_LOCK);

      CDClk_Change_Timeout := Time.MS_From_Now (3);
      loop
         GT_Mailbox_Write
           (MBox        => SKL_PCODE_CDCLK_CONTROL,
            Value       => SKL_CDCLK_PREPARE_FOR_CHANGE);
         Timed_Out := Time.Timed_Out (CDClk_Change_Timeout);
         Registers.Read (Registers.GT_MAILBOX_DATA, MBox_Data0);
         if (MBox_Data0 and SKL_CDCLK_READY_FOR_CHANGE) =
            SKL_CDCLK_READY_FOR_CHANGE
         then
            Timed_Out := False;
            exit;
         end if;
         exit when Timed_Out;
      end loop;

      if not Timed_Out then
         GT_Mailbox_Write
           (MBox        => SKL_PCODE_CDCLK_CONTROL,
            Value       => 16#0000_0000#);   -- 0 - 337.5MHz
                                             -- 1 - 450.0MHz
                                             -- 2 - 540.0MHz
                                             -- 3 - 675.0MHz
         Registers.Set_Mask
           (Register    => Registers.DBUF_CTL,
            Mask        => DBUF_CTL_DBUF_POWER_REQUEST);
         Registers.Wait_Set_Mask
           (Register    => Registers.DBUF_CTL,
            Mask        => DBUF_CTL_DBUF_POWER_STATE);
      end if;
   end Initialize;

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

end HW.GFX.GMA.Power_And_Clocks_Skylake;
