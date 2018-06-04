--
-- Copyright (C) 2015 secunet Security Networks AG
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
with HW.GFX.GMA.Registers;

package body HW.GFX.GMA.PLLs.WRPLL is

   ----------------------------------------------------------------------------
   --
   --  Divider calculation as found in Linux' i915 driver
   --
   --  Copyright (C) 2012 Intel Corporation
   --
   --  Permission is hereby granted, free of charge, to any person obtaining a
   --  copy of this software and associated documentation files (the "Software"),
   --  to deal in the Software without restriction, including without limitation
   --  the rights to use, copy, modify, merge, publish, distribute, sublicense,
   --  and/or sell copies of the Software, and to permit persons to whom the
   --  Software is furnished to do so, subject to the following conditions:
   --
   --  The above copyright notice and this permission notice (including the next
   --  paragraph) shall be included in all copies or substantial portions of the
   --  Software.
   --
   --  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   --  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   --  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   --  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   --  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   --  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   --  IN THE SOFTWARE.
   --
   --  Authors:
   --     Eugeni Dodonov <eugeni.dodonov@intel.com>
   --

   LC_FREQ     : constant := 2700;           -- in MHz
   LC_FREQ_2K  : constant := LC_FREQ * 2000; -- in 500Hz

   P_MIN       : constant := 2;
   P_MAX       : constant := 62; -- i915 says 64, but this would overflow 6-bit
   P_INC       : constant := 2;

   -- Constraints for PLL good behavior
   REF_MIN     : constant := 48;
   REF_MAX     : constant := 400;
   VCO_MIN     : constant := 2400;
   VCO_MAX     : constant := 4800;

   type R2_Range is new Natural range 0 .. LC_FREQ * 2 / REF_MIN;
   type N2_Range is new Natural range 0 .. VCO_MAX * Natural (R2_Range'Last) / LC_FREQ;
   type P_Range is new Natural range 0 .. P_MAX;

   type RNP is record
      P  : P_Range;
      N2 : N2_Range;
      R2 : R2_Range;
   end record;
   Invalid_RNP : constant RNP := RNP'(0, 0, 0);

   function Get_Budget_For_Freq
     (Clock : HW.GFX.Frequency_Type)
      return Word64
   is
      Result : Word64;
   begin
      case Clock is
         when  25175000 |
               25200000 |
               27000000 |
               27027000 |
               37762500 |
               37800000 |
               40500000 |
               40541000 |
               54000000 |
               54054000 |
               59341000 |
               59400000 |
               72000000 |
               74176000 |
               74250000 |
               81000000 |
               81081000 |
               89012000 |
               89100000 |
              108000000 |
              108108000 |
              111264000 |
              111375000 |
              148352000 |
              148500000 |
              162000000 |
              162162000 |
              222525000 |
              222750000 |
              296703000 |
              297000000 =>
            Result := 0;
         when 233500000 |
              245250000 |
              247750000 |
              253250000 |
              298000000 =>
            Result := 1500;
         when 169128000 |
              169500000 |
              179500000 |
              202000000 =>
            Result := 2000;
         when 256250000 |
              262500000 |
              270000000 |
              272500000 |
              273750000 |
              280750000 |
              281250000 |
              286000000 |
              291750000 =>
            Result := 4000;
         when 267250000 |
              268500000 =>
            Result := 5000;
         when others =>
            Result := 1000;
      end case;
      return Result;
   end Get_Budget_For_Freq;

   procedure Update_RNP
     (Freq_2K  : in     Word64;
      Budget   : in     Word64;
      R2       : in     R2_Range;
      N2       : in     N2_Range;
      P        : in     P_Range;
      Best     : in out RNP)
   with
      Depends => (Best =>+ (Freq_2K, Budget, R2, N2, P))
   is
      use type HW.Word64;

      function Abs_Diff (A, B : Word64) return Word64
      is
         Result : Word64;
      begin
         if A > B then
            Result := A - B;
         else
            Result := B - A;
         end if;
         return Result;
      end Abs_Diff;

      A, B, C, D, Diff, Diff_Best : Word64;
   begin
      -- No best (r,n,p) yet */
      if Best.P = 0 then
         Best.P   := P;
         Best.N2  := N2;
         Best.R2  := R2;
      else
         -- Config clock is (LC_FREQ_2K / 2000) * N / (P * R), which compares to
         -- freq2k.
         --
         -- delta = 1e6 *
         --         abs(freq2k - (LC_FREQ_2K * n2/(p * r2))) /
         --         freq2k;
         --
         -- and we would like delta <= budget.
         --
         -- If the discrepancy is above the PPM-based budget, always prefer to
         -- improve upon the previous solution.  However, if you're within the
         -- budget, try to maximize Ref * VCO, that is N / (P * R^2).
         A := Freq_2K * Budget * Word64 (P) * Word64 (R2);
         B := Freq_2K * Budget * Word64 (Best.P) * Word64 (Best.R2);
         Diff := Abs_Diff
           (Freq_2K * Word64 (P) * Word64 (R2),
            LC_FREQ_2K * Word64 (N2));
         Diff_Best := Abs_Diff
           (Freq_2K * Word64 (Best.P) * Word64 (Best.R2),
            LC_FREQ_2K * Word64 (Best.N2));
         C := 1000000 * Diff;
         D := 1000000 * Diff_Best;

         if A < C and B < D then
            -- If both are above the Budget, pick the closer
            if Word64 (Best.P) * Word64 (Best.R2) * Diff
                  < Word64 (P) * Word64 (R2) * Diff_Best
            then
               Best.P := P;
               Best.N2 := N2;
               Best.R2 := R2;
            end if;
         elsif A >= C and B < D then
            -- If A is below the threshold but B is above it?  Update.
            Best.P := P;
            Best.N2 := N2;
            Best.R2 := R2;
         elsif A >= C and B >= D then
            -- Both are below the limit, so pick the higher N2/(R2*R2)
            if Word64 (N2) * Word64 (Best.R2) * Word64 (Best.R2)
                  > Word64 (Best.N2) * Word64 (R2) * Word64 (R2)
            then
               Best.P := P;
               Best.N2 := N2;
               Best.R2 := R2;
            end if;
         end if;
         -- Otherwise A < C && B >= D, do nothing
      end if;
   end Update_RNP;

   procedure Calculate_WRPLL
     (Clock    : in     HW.GFX.Frequency_Type;
      R2_Out   :    out R2_Range;
      N2_Out   :    out N2_Range;
      P_Out    :    out P_Range)
   with
      Global => null,
      Pre => True,
      Post => True
   is
      use type HW.Word64;

      Freq_2K  : Word64;
      Budget   : Word64;
      Best     : RNP := Invalid_RNP;
   begin
      Freq_2K  := Word64 (Clock) / 100;   -- PLL output should be 5x
                                                -- the pixel clock
      Budget   := Get_Budget_For_Freq (Clock);

      -- Special case handling for 540MHz pixel clock: bypass WR PLL entirely
      -- and directly pass the LC PLL to it. */
      if Freq_2K = 5400000 then
         N2_Out   := 2;
         P_Out    := 1;
         R2_Out   := 2;
      else
         -- Ref = LC_FREQ / R, where Ref is the actual reference input seen by
         -- the WR PLL.
         --
         -- We want R so that REF_MIN <= Ref <= REF_MAX.
         -- Injecting R2 = 2 * R gives:
         --   REF_MAX * r2 > LC_FREQ * 2 and
         --   REF_MIN * r2 < LC_FREQ * 2
         --
         -- Which means the desired boundaries for r2 are:
         --  LC_FREQ * 2 / REF_MAX < r2 < LC_FREQ * 2 / REF_MIN
         --
         for R2 in R2_Range range
            LC_FREQ * 2 / REF_MAX + 1 .. LC_FREQ * 2 / REF_MIN
         loop
            -- VCO = N * Ref, that is: VCO = N * LC_FREQ / R
            --
            -- Once again we want VCO_MIN <= VCO <= VCO_MAX.
            -- Injecting R2 = 2 * R and N2 = 2 * N, we get:
            --   VCO_MAX * r2 > n2 * LC_FREQ and
            --   VCO_MIN * r2 < n2 * LC_FREQ)
            --
            -- Which means the desired boundaries for n2 are:
            -- VCO_MIN * r2 / LC_FREQ < n2 < VCO_MAX * r2 / LC_FREQ
            for N2 in N2_Range range
               N2_Range (VCO_MIN * Natural (R2) / LC_FREQ + 1)
                  .. N2_Range (VCO_MAX * Natural (R2) / LC_FREQ)
            loop
               for P_Fract in Natural range P_MIN / P_INC .. P_MAX / P_INC
               loop
                  Update_RNP
                    (Freq_2K, Budget, R2, N2, P_Range (P_Fract * P_INC), Best);
               end loop;
            end loop;
         end loop;

         N2_Out   := Best.N2;
         P_Out    := Best.P;
         R2_Out   := Best.R2;
      end if;

   end Calculate_WRPLL;

   --
   ----------------------------------------------------------------------------

   type Regs is array (WRPLLs) of Registers.Registers_Index;

   WRPLL_CTL : constant Regs := Regs'(Registers.WRPLL_CTL_1, Registers.WRPLL_CTL_2);
   WRPLL_CTL_PLL_ENABLE    : constant := 1 * 2 ** 31;
   WRPLL_CTL_SELECT_LCPLL  : constant := 3 * 2 ** 28;

   function WRPLL_CTL_DIVIDER_FEEDBACK (N2 : N2_Range) return Word32
   is
   begin
      return Word32 (N2) * 2 ** 16;
   end WRPLL_CTL_DIVIDER_FEEDBACK;

   function WRPLL_CTL_DIVIDER_POST (P : P_Range) return Word32
   is
   begin
      return Word32 (P) * 2 ** 8;
   end WRPLL_CTL_DIVIDER_POST;

   function WRPLL_CTL_DIVIDER_REFERENCE (R2 : R2_Range) return Word32
   is
   begin
      return Word32 (R2) * 2 ** 0;
   end WRPLL_CTL_DIVIDER_REFERENCE;

   ----------------------------------------------------------------------------

   procedure On
     (PLL            : in     WRPLLs;
      Target_Clock   : in     Frequency_Type;
      Success        :    out Boolean)
   is
      R2 : R2_Range;
      N2 : N2_Range;
      P  : P_Range;
   begin
      Calculate_WRPLL (Target_Clock, R2, N2, P);
      Registers.Write
        (Register => WRPLL_CTL (PLL),
         Value    => WRPLL_CTL_PLL_ENABLE or
                     WRPLL_CTL_SELECT_LCPLL or
                     WRPLL_CTL_DIVIDER_FEEDBACK (N2) or
                     WRPLL_CTL_DIVIDER_POST (P) or
                     WRPLL_CTL_DIVIDER_REFERENCE (R2));
      Registers.Posting_Read (WRPLL_CTL (PLL));
      Time.U_Delay (20);

      Success := True;
   end On;

   procedure Off (PLL : WRPLLs)
   is
   begin
      Registers.Unset_Mask (WRPLL_CTL (PLL), WRPLL_CTL_PLL_ENABLE);
   end Off;

end HW.GFX.GMA.PLLs.WRPLL;
