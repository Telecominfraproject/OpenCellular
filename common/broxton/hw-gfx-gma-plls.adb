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

with HW.Debug;
with GNAT.Source_Info;

with HW.GFX.GMA.Config;
with HW.GFX.GMA.Registers;

use HW.GFX.GMA.Registers;

package body HW.GFX.GMA.PLLs
with
   Refined_State => (State => null)
is

   -- DPLL clock equation:
   -- 5 * Target_Dotclock = Ref_Clk * M1 * (M2 / 2^22) / N / (P1 * P2)
   --
   -- Where
   -- M1 = 2,
   -- Ref_Clk = 100MHz,
   -- VCO = Ref_Clk * 2 * (M2 / 2^22),
   -- N = 1 and
   -- P = P1 * P2.

   Ref_Clk  : constant := 100_000_000;
   M1       : constant := 2;
   N        : constant := 1;

   -- i915 has a fixme for the M2 range. But the VCO range is very
   -- limited, giving us a narrow range for M2: 24 .. 33.5
   subtype VCO_Range is Pos64 range 4_800_000_000 .. 6_700_000_000;
   subtype M2_Range is Pos64 range
      N * VCO_Range'First * 2 ** 22 / Ref_Clk / M1 ..
      N * VCO_Range'Last * 2 ** 22 / Ref_Clk / M1;

   subtype N_Range is Pos64 range 1 .. 1;

   subtype P1_Range is Pos64 range 2 ..  4;
   subtype P2_Range is Pos64 range 1 .. 20;

   subtype Clock_Range is Frequency_Type range
      Frequency_Type'First .. 540_000_000;
   subtype HDMI_Clock_Range is Clock_Range range
      25_000_000 .. Config.HDMI_Max_Clock_24bpp;
   subtype Clock_Gap is Clock_Range range 223_333_333 + 1 .. 240_000_000 - 1;

   type Clock_Type is record
      M2       : M2_Range;
      P1       : P1_Range;
      P2       : P2_Range;
      VCO      : VCO_Range;
      Dotclock : Clock_Range;
   end record;

   Invalid_Clock : constant Clock_Type :=
     (M2       => M2_Range'Last,
      P1       => P1_Range'Last,
      P2       => P2_Range'Last,
      VCO      => VCO_Range'Last,
      Dotclock => Clock_Range'Last);

   procedure Calculate_Clock_Parameters
     (Target_Dotclock   : in     HDMI_Clock_Range;
      Best_Clock        :    out Clock_Type;
      Valid             :    out Boolean)
   with
      Pre => True
   is
      Target_Clock : constant Pos64 := 5 * Target_Dotclock;

      M2, VCO, Current_Clock : Pos64;
      P2 : P2_Range;

      Valid_Clk : Boolean;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Valid      := False;
      Best_Clock := Invalid_Clock;

      -- reverse loops as hardware prefers higher values
      for P1 in reverse P1_Range loop
         -- find the highest P2 that results in valid clock
         P2 := P2_Range'Last;
         loop
            M2 := Div_Round_Closest
              (Target_Clock * P2 * P1 * N * 2 ** 22, Ref_Clk * M1);
            VCO := Div_Round_Closest (Ref_Clk * M1 * M2, 2 ** 22 * N);
            Current_Clock := Div_Round_Closest (VCO, P1 * P2);

            Valid_Clk := M2 in M2_Range and then
                           Div_Round_Closest (Current_Clock, 5) in Clock_Range;
            if Valid_Clk then
               -- the error is always below 2^-22, higher P takes precedence
               if not Valid or P1 * P2 > Best_Clock.P1 * Best_Clock.P2 then
                  Best_Clock := Clock_Type'
                    (M2       => M2,
                     P1       => P1,
                     P2       => P2,
                     VCO      => VCO,
                     Dotclock => Div_Round_Closest (Current_Clock, 5));
                  Valid := True;
               end if;
            end if;

            -- Prefer higher P2 over marginal lower error. This is
            -- just an optimization, since lower P2 values would get
            -- filtered above anyway.
            exit when Valid_Clk;

            -- If M2 got too low, it won't get any better. Another
            -- optimization.
            exit when M2 < M2_Range'First;

            exit when P2 = P2_Range'First;

            if P2 > 10 then
               P2 := P2 - 2;
            else
               P2 := P2 - 1;
            end if;
         end loop;
      end loop;

      pragma Debug (Valid,     Debug.Put_Line ("Valid clock found."));
      pragma Debug (Valid,     Debug.Put ("M2 / P1 / P2: "));
      pragma Debug (Valid,     Debug.Put_Word32 (Word32 (Best_Clock.M2)));
      pragma Debug (Valid,     Debug.Put (" / "));
      pragma Debug (Valid,     Debug.Put_Int8 (Pos8 (Best_Clock.P1)));
      pragma Debug (Valid,     Debug.Put (" / "));
      pragma Debug (Valid,     Debug.Put_Int8 (Pos8 (Best_Clock.P2)));
      pragma Debug (Valid,     Debug.New_Line);
      pragma Debug (Valid,     Debug.Put ("Best / Target: "));
      pragma Debug (Valid,     Debug.Put_Int64 (Best_Clock.Dotclock));
      pragma Debug (Valid,     Debug.Put (" / "));
      pragma Debug (Valid,     Debug.Put_Int64 (Target_Dotclock));
      pragma Debug (Valid,     Debug.New_Line);
      pragma Debug (not Valid, Debug.Put_Line ("No valid clock found."));
   end Calculate_Clock_Parameters;

   ----------------------------------------------------------------------------

   subtype Valid_PLLs is T range DPLL_A .. DPLL_C;

   type Port_PLL_Regs is record
      PLL_ENABLE     : Registers_Index;
      PLL_EBB_0      : Registers_Index;
      PLL_EBB_4      : Registers_Index;
      PLL_0          : Registers_Index;
      PLL_1          : Registers_Index;
      PLL_2          : Registers_Index;
      PLL_3          : Registers_Index;
      PLL_6          : Registers_Index;
      PLL_8          : Registers_Index;
      PLL_9          : Registers_Index;
      PLL_10         : Registers_Index;
      PCS_DW12_LN01  : Registers_Index;
      PCS_DW12_GRP   : Registers_Index;
   end record;
   type Port_PLL_Array is array (Valid_PLLs) of Port_PLL_Regs;

   PORT : constant Port_PLL_Array :=
     (DPLL_A =>
        (PLL_ENABLE     => BXT_PORT_PLL_ENABLE_A,
         PLL_EBB_0      => BXT_PORT_PLL_EBB_0_A,
         PLL_EBB_4      => BXT_PORT_PLL_EBB_4_A,
         PLL_0          => BXT_PORT_PLL_0_A,
         PLL_1          => BXT_PORT_PLL_1_A,
         PLL_2          => BXT_PORT_PLL_2_A,
         PLL_3          => BXT_PORT_PLL_3_A,
         PLL_6          => BXT_PORT_PLL_6_A,
         PLL_8          => BXT_PORT_PLL_8_A,
         PLL_9          => BXT_PORT_PLL_9_A,
         PLL_10         => BXT_PORT_PLL_10_A,
         PCS_DW12_LN01  => BXT_PORT_PCS_DW12_01_A,
         PCS_DW12_GRP   => BXT_PORT_PCS_DW12_GRP_A),
      DPLL_B =>
        (PLL_ENABLE     => BXT_PORT_PLL_ENABLE_B,
         PLL_EBB_0      => BXT_PORT_PLL_EBB_0_B,
         PLL_EBB_4      => BXT_PORT_PLL_EBB_4_B,
         PLL_0          => BXT_PORT_PLL_0_B,
         PLL_1          => BXT_PORT_PLL_1_B,
         PLL_2          => BXT_PORT_PLL_2_B,
         PLL_3          => BXT_PORT_PLL_3_B,
         PLL_6          => BXT_PORT_PLL_6_B,
         PLL_8          => BXT_PORT_PLL_8_B,
         PLL_9          => BXT_PORT_PLL_9_B,
         PLL_10         => BXT_PORT_PLL_10_B,
         PCS_DW12_LN01  => BXT_PORT_PCS_DW12_01_B,
         PCS_DW12_GRP   => BXT_PORT_PCS_DW12_GRP_B),
      DPLL_C =>
        (PLL_ENABLE     => BXT_PORT_PLL_ENABLE_C,
         PLL_EBB_0      => BXT_PORT_PLL_EBB_0_C,
         PLL_EBB_4      => BXT_PORT_PLL_EBB_4_C,
         PLL_0          => BXT_PORT_PLL_0_C,
         PLL_1          => BXT_PORT_PLL_1_C,
         PLL_2          => BXT_PORT_PLL_2_C,
         PLL_3          => BXT_PORT_PLL_3_C,
         PLL_6          => BXT_PORT_PLL_6_C,
         PLL_8          => BXT_PORT_PLL_8_C,
         PLL_9          => BXT_PORT_PLL_9_C,
         PLL_10         => BXT_PORT_PLL_10_C,
         PCS_DW12_LN01  => BXT_PORT_PCS_DW12_01_C,
         PCS_DW12_GRP   => BXT_PORT_PCS_DW12_GRP_C));

   PORT_PLL_ENABLE                     : constant :=      1 * 2 ** 31;
   PORT_PLL_ENABLE_LOCK                : constant :=      1 * 2 ** 30;
   PORT_PLL_ENABLE_REF_SEL             : constant :=      1 * 2 ** 27;

   PORT_PLL_EBB0_P1_SHIFT              : constant :=               13;
   PORT_PLL_EBB0_P1_MASK               : constant := 16#07# * 2 ** 13;
   PORT_PLL_EBB0_P2_SHIFT              : constant :=                8;
   PORT_PLL_EBB0_P2_MASK               : constant := 16#1f# * 2 **  8;
   function PORT_PLL_EBB0_P1 (P1 : P1_Range) return Word32 is
   begin
      return Shift_Left (Word32 (P1), PORT_PLL_EBB0_P1_SHIFT);
   end PORT_PLL_EBB0_P1;
   function PORT_PLL_EBB0_P2 (P2 : P2_Range) return Word32 is
   begin
      return Shift_Left (Word32 (P2), PORT_PLL_EBB0_P2_SHIFT);
   end PORT_PLL_EBB0_P2;

   PORT_PLL_EBB4_RECALIBRATE           : constant :=      1 * 2 ** 14;
   PORT_PLL_EBB4_10BIT_CLK_ENABLE      : constant :=      1 * 2 ** 13;

   PORT_PLL_0_M2_INT_MASK              : constant := 16#ff# * 2 **  0;
   function PORT_PLL_0_M2_INT (M2 : M2_Range) return Word32 is
   begin
      return Shift_Right (Word32 (M2), 22);
   end PORT_PLL_0_M2_INT;

   PORT_PLL_1_N_SHIFT                  : constant :=                8;
   PORT_PLL_1_N_MASK                   : constant := 16#0f# * 2 **  8;
   function PORT_PLL_1_N (N : N_Range) return Word32 is
   begin
      return Shift_Left (Word32 (N), PORT_PLL_1_N_SHIFT);
   end PORT_PLL_1_N;

   PORT_PLL_2_M2_FRAC_MASK             : constant :=      16#3f_ffff#;
   function PORT_PLL_2_M2_FRAC (M2 : M2_Range) return Word32 is
   begin
      return Word32 (M2) and PORT_PLL_2_M2_FRAC_MASK;
   end PORT_PLL_2_M2_FRAC;

   PORT_PLL_3_M2_FRAC_EN_MASK          : constant :=      1 * 2 ** 16;
   function PORT_PLL_3_M2_FRAC_EN (M2 : M2_Range) return Word32 is
   begin
      return
        (if (Word32 (M2) and PORT_PLL_2_M2_FRAC_MASK) /= 0 then
            PORT_PLL_3_M2_FRAC_EN_MASK else 0);
   end PORT_PLL_3_M2_FRAC_EN;

   PORT_PLL_6_GAIN_CTL_SHIFT           : constant :=               16;
   PORT_PLL_6_GAIN_CTL_MASK            : constant := 16#07# * 2 ** 16;
   PORT_PLL_6_INT_COEFF_SHIFT          : constant :=                8;
   PORT_PLL_6_INT_COEFF_MASK           : constant := 16#1f# * 2 **  8;
   PORT_PLL_6_PROP_COEFF_MASK          : constant := 16#0f# * 2 **  0;
   function PORT_PLL_6_GAIN_COEFF (VCO : VCO_Range) return Word32 is
   begin
      return
        (if VCO >= 6_200_000_000 then
            Shift_Left (Word32'(3), PORT_PLL_6_GAIN_CTL_SHIFT) or
            Shift_Left (Word32'(9), PORT_PLL_6_INT_COEFF_SHIFT) or
            Word32'(4)
         elsif VCO /= 5_400_000_000 then
            Shift_Left (Word32'(3), PORT_PLL_6_GAIN_CTL_SHIFT) or
            Shift_Left (Word32'(11), PORT_PLL_6_INT_COEFF_SHIFT) or
            Word32'(5)
         else
            Shift_Left (Word32'(1), PORT_PLL_6_GAIN_CTL_SHIFT) or
            Shift_Left (Word32'(8), PORT_PLL_6_INT_COEFF_SHIFT) or
            Word32'(3));
   end PORT_PLL_6_GAIN_COEFF;

   PORT_PLL_8_TARGET_CNT_MASK          : constant :=          16#3ff#;
   function PORT_PLL_8_TARGET_CNT (VCO : VCO_Range) return Word32 is
   begin
      return (if VCO >= 6_200_000_000 then 8 else 9);
   end PORT_PLL_8_TARGET_CNT;

   PORT_PLL_9_LOCK_THRESHOLD_SHIFT     : constant :=                1;
   PORT_PLL_9_LOCK_THRESHOLD_MASK      : constant := 16#07# * 2 **  1;
   function PORT_PLL_9_LOCK_THRESHOLD (Threshold : Natural) return Word32 is
   begin
      return
         Shift_Left (Word32 (Threshold), PORT_PLL_9_LOCK_THRESHOLD_SHIFT) and
         PORT_PLL_9_LOCK_THRESHOLD_MASK;
   end PORT_PLL_9_LOCK_THRESHOLD;

   PORT_PLL_10_DCO_AMP_OVR_EN_H        : constant :=          2 ** 27;
   PORT_PLL_10_DCO_AMP_SHIFT           : constant :=               10;
   PORT_PLL_10_DCO_AMP_MASK            : constant := 16#0f# * 2 ** 10;
   function PORT_PLL_10_DCO_AMP (Amp : Natural) return Word32 is
   begin
      return
         Shift_Left (Word32 (Amp), PORT_PLL_10_DCO_AMP_SHIFT) and
         PORT_PLL_10_DCO_AMP_MASK;
   end PORT_PLL_10_DCO_AMP;

   PORT_PCS_LANE_STAGGER_STRAP_OVRD    : constant :=          2 **  6;
   PORT_PCS_LANE_STAGGER_MASK          : constant := 16#1f# * 2 **  0;
   function PORT_PCS_LANE_STAGGER (Dotclock : Clock_Range) return Word32 is
   begin
      return Word32'(PORT_PCS_LANE_STAGGER_STRAP_OVRD) or
        (if Dotclock > 270_000_000 then
            16#18#
         elsif Dotclock > 135_000_000 then
            16#0d#
         elsif Dotclock >  67_000_000 then
            16#07#
         elsif Dotclock >  33_000_000 then
            16#04#
         else
            16#02#);
   end PORT_PCS_LANE_STAGGER;

   ----------------------------------------------------------------------------

   procedure Program_DPLL (P : T; Clock : Clock_Type)
   is
      PCS : Word32;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Set_Mask    (PORT (P).PLL_ENABLE, PORT_PLL_ENABLE_REF_SEL);   -- non-SSC ref
      Unset_Mask  (PORT (P).PLL_EBB_4,  PORT_PLL_EBB4_10BIT_CLK_ENABLE);

      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_EBB_0,
         Mask_Unset  => PORT_PLL_EBB0_P1_MASK or
                        PORT_PLL_EBB0_P2_MASK,
         Mask_Set    => PORT_PLL_EBB0_P1 (Clock.P1) or
                        PORT_PLL_EBB0_P2 (Clock.P2));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_0,
         Mask_Unset  => PORT_PLL_0_M2_INT_MASK,
         Mask_Set    => PORT_PLL_0_M2_INT (Clock.M2));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_1,
         Mask_Unset  => PORT_PLL_1_N_MASK,
         Mask_Set    => PORT_PLL_1_N (N));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_2,
         Mask_Unset  => PORT_PLL_2_M2_FRAC_MASK,
         Mask_Set    => PORT_PLL_2_M2_FRAC (Clock.M2));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_3,
         Mask_Unset  => PORT_PLL_3_M2_FRAC_EN_MASK,
         Mask_Set    => PORT_PLL_3_M2_FRAC_EN (Clock.M2));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_6,
         Mask_Unset  => PORT_PLL_6_GAIN_CTL_MASK or
                        PORT_PLL_6_INT_COEFF_MASK or
                        PORT_PLL_6_PROP_COEFF_MASK,
         Mask_Set    => PORT_PLL_6_GAIN_COEFF (Clock.VCO));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_8,
         Mask_Unset  => PORT_PLL_8_TARGET_CNT_MASK,
         Mask_Set    => PORT_PLL_8_TARGET_CNT (Clock.VCO));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_9,
         Mask_Unset  => PORT_PLL_9_LOCK_THRESHOLD_MASK,
         Mask_Set    => PORT_PLL_9_LOCK_THRESHOLD (5));
      Unset_And_Set_Mask
        (Register    => PORT (P).PLL_10,
         Mask_Unset  => PORT_PLL_10_DCO_AMP_MASK,
         Mask_Set    => PORT_PLL_10_DCO_AMP_OVR_EN_H or
                        PORT_PLL_10_DCO_AMP (15));

      Set_Mask (PORT (P).PLL_EBB_4,  PORT_PLL_EBB4_RECALIBRATE);
      Set_Mask (PORT (P).PLL_EBB_4,  PORT_PLL_EBB4_10BIT_CLK_ENABLE);

      Set_Mask (PORT (P).PLL_ENABLE, PORT_PLL_ENABLE);
      Wait_Set_Mask
        (Register => PORT (P).PLL_ENABLE,
         Mask     => PORT_PLL_ENABLE_LOCK,
         TOut_MS  => 1);   -- 100us

      Read (PORT (P).PCS_DW12_LN01, PCS);
      PCS := PCS and not PORT_PCS_LANE_STAGGER_MASK;
      PCS := PCS or PORT_PCS_LANE_STAGGER (Clock.Dotclock);
      Write (PORT (P).PCS_DW12_GRP, PCS);
   end Program_DPLL;

   ----------------------------------------------------------------------------

   procedure Alloc
     (Port_Cfg : in     Port_Config;
      PLL      :    out T;
      Success  :    out Boolean)
   is
      Clock : Clock_Type := Invalid_Clock;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      case Port_Cfg.Port is
         when DIGI_A => PLL := DPLL_A;
         when DIGI_B => PLL := DPLL_B;
         when DIGI_C => PLL := DPLL_C;
         when others => PLL := Invalid_PLL;
      end case;

      Success := PLL /= Invalid_PLL;

      if Success then
         case Port_Cfg.Display is
            when DP =>
               Success := True;
               -- we use static values for DP
               case Port_Cfg.DP.Bandwidth is
                  when DP_Bandwidth_1_62 =>
                     Clock.M2       := 32 * 2 ** 22 + 1677722;
                     Clock.P1       := 4;
                     Clock.P2       := 2;
                     Clock.VCO      := 6_480_000_019;
                     Clock.Dotclock :=   162_000_000;
                  when DP_Bandwidth_2_7 =>
                     Clock.M2       := 27 * 2 ** 22;
                     Clock.P1       := 4;
                     Clock.P2       := 1;
                     Clock.VCO      := 5_400_000_000;
                     Clock.Dotclock :=   270_000_000;
                  when DP_Bandwidth_5_4 =>
                     Clock.M2       := 27 * 2 ** 22;
                     Clock.P1       := 2;
                     Clock.P2       := 1;
                     Clock.VCO      := 5_400_000_000;
                     Clock.Dotclock :=   540_000_000;
               end case;
            when HDMI =>
               if Port_Cfg.Mode.Dotclock in HDMI_Clock_Range and
                  (Port_Cfg.Mode.Dotclock *  99 / 100 < Clock_Gap'First or
                   Port_Cfg.Mode.Dotclock * 101 / 100 > Clock_Gap'Last)
               then
                  Calculate_Clock_Parameters
                    (Target_Dotclock   => Port_Cfg.Mode.Dotclock,
                     Best_Clock        => Clock,
                     Valid             => Success);
               else
                  Success := False;
                  pragma Debug (Debug.Put_Line
                     ("Mode's dotclock is out of range."));
               end if;
            when others =>
               Success := False;
               pragma Debug (Debug.Put_Line ("Invalid display type!"));
         end case;
      end if;

      if Success then
         Program_DPLL (PLL, Clock);
      end if;
   end Alloc;

   procedure Free (PLL : T) is
   begin
      if PLL in Valid_PLLs then
         Unset_Mask (PORT (PLL).PLL_ENABLE, PORT_PLL_ENABLE);
      end if;
   end Free;

   procedure All_Off is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      for PLL in Valid_PLLs loop
         Free (PLL);
      end loop;
   end All_Off;

end HW.GFX.GMA.PLLs;
