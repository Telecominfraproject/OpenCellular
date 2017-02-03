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

end HW.GFX.GMA.DDI_Phy;
