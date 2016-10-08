--
-- Copyright (C) 2016 secunet Security Networks AG
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; version 2 of the License.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--

with HW.GFX.GMA.Registers;

package body HW.GFX.GMA.PLLs.DPLL is

   -- NOTE: Order of DPLLs is twisted => always use named associations!

   type Regs is array (Configurable_DPLLs) of Registers.Registers_Index;

   DPLL_CTL : constant Regs := Regs'
     (DPLL1 => Registers.LCPLL2_CTL,
      DPLL2 => Registers.WRPLL_CTL_1,
      DPLL3 => Registers.WRPLL_CTL_2);
   DPLL_CTL_PLL_ENABLE                 : constant := 1 * 2 ** 31;

   ----------------------------------------------------------------------------

   DPLL_CFGR1 : constant Regs := Regs'
     (DPLL1 => Registers.DPLL1_CFGR1,
      DPLL2 => Registers.DPLL2_CFGR1,
      DPLL3 => Registers.DPLL3_CFGR1);
   DPLL_CFGR1_FREQUENCY_ENABLE         : constant :=        1 * 2 ** 31;
   DPLL_CFGR1_DCO_FRACTION_SHIFT       : constant :=                  9;
   DPLL_CFGR1_DCO_FRACTION_MASK        : constant := 16#7fff# * 2 **  9;
   DPLL_CFGR1_DCO_INTEGER_MASK         : constant := 16#01ff# * 2 **  0;

   DPLL_CFGR2 : constant Regs := Regs'
     (DPLL1 => Registers.DPLL1_CFGR2,
      DPLL2 => Registers.DPLL2_CFGR2,
      DPLL3 => Registers.DPLL3_CFGR2);
   DPLL_CFGR2_QDIV_RATIO_SHIFT         : constant :=            8;
   DPLL_CFGR2_QDIV_RATIO_MASK          : constant := 255 * 2 ** 8;
   DPLL_CFGR2_QDIV_MODE                : constant :=   1 * 2 ** 7;
   DPLL_CFGR2_KDIV_SHIFT               : constant :=            5;
   DPLL_CFGR2_KDIV_MASK                : constant :=   3 * 2 ** 5;
   DPLL_CFGR2_PDIV_SHIFT               : constant :=            2;
   DPLL_CFGR2_PDIV_MASK                : constant :=   7 * 2 ** 2;
   DPLL_CFGR2_CENTRAL_FREQ_MASK        : constant :=   3 * 2 ** 0;
   DPLL_CFGR2_CENTRAL_FREQ_9600MHZ     : constant :=   0 * 2 ** 0;
   DPLL_CFGR2_CENTRAL_FREQ_9000MHZ     : constant :=   1 * 2 ** 0;
   DPLL_CFGR2_CENTRAL_FREQ_8400MHZ     : constant :=   3 * 2 ** 0;

   ----------------------------------------------------------------------------

   HDMI_MODE            : constant := 1 * 2 **  5;
   SSC                  : constant := 1 * 2 **  4;
   LINK_RATE_MASK       : constant := 7 * 2 **  1;
   LINK_RATE_2700MHZ    : constant := 0 * 2 **  1;
   LINK_RATE_1350MHZ    : constant := 1 * 2 **  1;
   LINK_RATE_810MHZ     : constant := 2 * 2 **  1;
   LINK_RATE_1620MHZ    : constant := 3 * 2 **  1;
   LINK_RATE_1080MHZ    : constant := 4 * 2 **  1;
   LINK_RATE_2160MHZ    : constant := 5 * 2 **  1;
   OVERRIDE             : constant := 1 * 2 **  0;

   LOCK                 : constant := 1 * 2 **  0;

   type Shifts is array (Configurable_DPLLs) of Natural;
   DPLL_CTRL1_SHIFT : constant Shifts :=
     (DPLL1 => 6, DPLL2 => 12, DPLL3 => 18);
   DPLL_STATUS_SHIFT : constant Shifts :=
     (DPLL1 => 8, DPLL2 => 16, DPLL3 => 24);

   function LINK_RATE (Link_Rate : DP_Bandwidth) return Word32 is
   begin
      return (case Link_Rate is
               when DP_Bandwidth_5_4   => LINK_RATE_2700MHZ,
               when DP_Bandwidth_2_7   => LINK_RATE_1350MHZ,
               when DP_Bandwidth_1_62  => LINK_RATE_810MHZ);
   end LINK_RATE;

   function DPLL_CTRL1_DPLLx
     (Value : Word32;
      PLL   : Configurable_DPLLs)
      return Word32 is
   begin
      return Shift_Left (Value, DPLL_CTRL1_SHIFT (PLL));
   end DPLL_CTRL1_DPLLx;

   function DPLL_STATUS_DPLLx_LOCK (PLL : Configurable_DPLLs) return Word32 is
   begin
      return Shift_Left (LOCK, DPLL_STATUS_SHIFT (PLL));
   end DPLL_STATUS_DPLLx_LOCK;

   ----------------------------------------------------------------------------

   subtype PDiv_Range is Pos64 range 1 ..   7;
   subtype QDiv_Range is Pos64 range 1 .. 255;
   subtype KDiv_Range is Pos64 range 1 ..   5;

   type Central_Frequency is (CF_INVALID, CF_9600MHZ, CF_9000MHZ, CF_8400MHZ);
   subtype Valid_Central_Freq is
      Central_Frequency range CF_9600MHZ .. CF_8400MHZ;

   type CF_Pos is array (Valid_Central_Freq) of Pos64;
   CF_Pos64 : constant CF_Pos := CF_Pos'
     (CF_9600MHZ  => 9_600_000_000,
      CF_9000MHZ  => 9_000_000_000,
      CF_8400MHZ  => 8_400_000_000);

   subtype DCO_Frequency is
      Pos64 range 1 .. CF_Pos64 (CF_9600MHZ) + CF_Pos64 (CF_9600MHZ) / 100;

   function DPLL_CFGR1_DCO_FRACTION (DCO_Freq : DCO_Frequency) return Word32
   with
      Pre => True
   is
   begin
      return Shift_Left
        (Word32 ((DCO_Freq * 2 ** 15) / 24_000_000) and 16#7fff#,
         DPLL_CFGR1_DCO_FRACTION_SHIFT);
   end DPLL_CFGR1_DCO_FRACTION;

   function DPLL_CFGR1_DCO_INTEGER (DCO_Freq : DCO_Frequency) return Word32
   with
      Pre => True
   is
   begin
      return Word32 (DCO_Freq / 24_000_000);
   end DPLL_CFGR1_DCO_INTEGER;

   function DPLL_CFGR2_PDIV (PDiv : PDiv_Range) return Word32 is
   begin
      return Shift_Left
        ((case PDiv is
            when      1 => 0,
            when      2 => 1,
            when      3 => 2,
            when      7 => 4,
            when others => 4),
         DPLL_CFGR2_PDIV_SHIFT);
   end DPLL_CFGR2_PDIV;

   function DPLL_CFGR2_QDIV (QDiv : QDiv_Range) return Word32 is
   begin
      return Shift_Left (Word32 (QDiv), DPLL_CFGR2_QDIV_RATIO_SHIFT) or
             (if QDiv /= 1 then DPLL_CFGR2_QDIV_MODE else 0);
   end DPLL_CFGR2_QDIV;

   function DPLL_CFGR2_KDIV (KDiv : KDiv_Range) return Word32 is
   begin
      return Shift_Left
        ((case KDiv is
            when      5 => 0,
            when      2 => 1,
            when      3 => 2,
            when      1 => 3,
            when others => 0),
         DPLL_CFGR2_KDIV_SHIFT);
   end DPLL_CFGR2_KDIV;

   function DPLL_CFGR2_CENTRAL_FREQ (CF : Valid_Central_Freq) return Word32 is
   begin
      return (case CF is
               when CF_9600MHZ   => DPLL_CFGR2_CENTRAL_FREQ_9600MHZ,
               when CF_9000MHZ   => DPLL_CFGR2_CENTRAL_FREQ_9000MHZ,
               when CF_8400MHZ   => DPLL_CFGR2_CENTRAL_FREQ_8400MHZ);
   end DPLL_CFGR2_CENTRAL_FREQ;

   ----------------------------------------------------------------------------

   procedure Calculate_DPLL
     (Dotclock       : in     Frequency_Type;
      Central_Freq   :    out Central_Frequency;
      DCO_Freq       :    out DCO_Frequency;
      PDiv           :    out PDiv_Range;
      QDiv           :    out QDiv_Range;
      KDiv           :    out KDiv_Range)
   with
      Pre => True
   is
      Max_Pos_Deviation : constant := 1;
      Max_Neg_Deviation : constant := 6;

      subtype Div_Range is Pos64 range 1 .. 98;
      subtype Candidate_Index is Positive range 1 .. 36;
      type Candidate_Array is array (Candidate_Index) of Div_Range;
      type Candidate_List is record
         Divs  : Candidate_Array;
         Count : Candidate_Index;
      end record;
      type Parity_Type is (Even, Odd);
      type Candidates_Type is array (Parity_Type) of Candidate_List;

      Candidates : constant Candidates_Type := Candidates_Type'
        (Even  => Candidate_List'
           (Divs  => Candidate_Array'
              (4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 30, 32, 36, 40, 42, 44,
               48, 52, 54, 56, 60, 64, 66, 68, 70, 72, 76, 78, 80, 84, 88, 90,
               92, 96, 98),
            Count => 36),
         Odd   => Candidate_List'
           (Divs  => Candidate_Array'(3, 5, 7, 9, 15, 21, 35, others => 1),
            Count => 7));

      Temp_Freq,
      Allowed_Deviation : Pos64;
      Deviation         : Int64;
      Temp_Central      : DCO_Frequency;
      Min_Deviation     : Int64 := Int64'Last;
      Div               : Div_Range := Div_Range'Last;
   begin
      Central_Freq   := CF_INVALID;
      DCO_Freq       := 1;
      PDiv           := 1;
      QDiv           := 1;
      KDiv           := 1;

      for Parity in Parity_Type loop
         for CF in Valid_Central_Freq loop
            Temp_Central := CF_Pos64 (CF);
            for I in Candidate_Index range 1 .. Candidates (Parity).Count loop
               Temp_Freq := Candidates (Parity).Divs (I) * 5 * Dotclock;
               if Temp_Freq > Temp_Central then
                  Deviation         := Temp_Freq - Temp_Central;
                  Allowed_Deviation := (Max_Pos_Deviation * Temp_Central) / 100;
               else
                  Deviation         := Temp_Central - Temp_Freq;
                  Allowed_Deviation := (Max_Neg_Deviation * Temp_Central) / 100;
               end if;
               if Deviation < Min_Deviation and
                  Deviation < Allowed_Deviation
               then
                  Min_Deviation  := Deviation;
                  Central_Freq   := CF;
                  DCO_Freq       := Temp_Freq;
                  Div            := Candidates (Parity).Divs (I);
               end if;
            end loop;
         end loop;
         exit when Central_Freq /= CF_INVALID;
      end loop;

      if Central_Freq /= CF_INVALID then
         if Div mod 2 = 0 then
            pragma Assert (Div /= 1);
            pragma Assert (Div > 1);
            Div := Div / 2;
            if Div = 1 or Div = 3 or Div = 5 then
               -- 2, 6 and 10
               PDiv := 2;
               QDiv := 1;
               KDiv := Div;
            elsif Div mod 2 = 0 then
               -- divisible by 4
               PDiv := 2;
               QDiv := Div / 2;
               KDiv := 2;
            elsif Div mod 3 = 0 then
               -- divisible by 6
               PDiv := 3;
               QDiv := Div / 3;
               KDiv := 2;
            elsif Div mod 7 = 0  then
               -- divisible by 14
               PDiv := 7;
               QDiv := Div / 7;
               KDiv := 2;
            end if;
         elsif Div = 7 or Div = 21 or Div = 35 then
            -- 7, 21 and 35
            PDiv := 7;
            QDiv := 1;
            KDiv := Div / 7;
         elsif Div = 3 or Div = 9 or Div = 15 then
            -- 3, 9 and 15
            PDiv := 3;
            QDiv := 1;
            KDiv := Div / 3;
         elsif Div = 5 then
            -- 5
            PDiv := 5;
            QDiv := 1;
            KDiv := 1;
         end if;
      end if;
   end Calculate_DPLL;

   ----------------------------------------------------------------------------

   procedure On
     (PLL      : in     Configurable_DPLLs;
      Port_Cfg : in     Port_Config;
      Success  :    out Boolean)
   is
      Central_Freq   : Central_Frequency;
      DCO_Freq       : DCO_Frequency;
      PDiv           : PDiv_Range;
      QDiv           : QDiv_Range;
      KDiv           : KDiv_Range;
   begin
      if Port_Cfg.Display = DP then
         Registers.Unset_And_Set_Mask
           (Register    => Registers.DPLL_CTRL1,
            Mask_Unset  => DPLL_CTRL1_DPLLx (HDMI_MODE, PLL) or
                           DPLL_CTRL1_DPLLx (SSC, PLL) or
                           DPLL_CTRL1_DPLLx (LINK_RATE_MASK, PLL),
            Mask_Set    => DPLL_CTRL1_DPLLx (LINK_RATE
                             (Port_Cfg.DP.Bandwidth), PLL) or
                           DPLL_CTRL1_DPLLx (OVERRIDE, PLL));
         Registers.Posting_Read (Registers.DPLL_CTRL1);
         Success := True;
      else
         Calculate_DPLL
           (Port_Cfg.Mode.Dotclock, Central_Freq, DCO_Freq, PDiv, QDiv, KDiv);
         Success := Central_Freq /= CF_INVALID;
         if Success then
            Registers.Unset_And_Set_Mask
              (Register    => Registers.DPLL_CTRL1,
               Mask_Unset  => DPLL_CTRL1_DPLLx (SSC, PLL),
               Mask_Set    => DPLL_CTRL1_DPLLx (HDMI_MODE, PLL) or
                              DPLL_CTRL1_DPLLx (OVERRIDE, PLL));
            Registers.Write
              (Register    => DPLL_CFGR1 (PLL),
               Value       => DPLL_CFGR1_FREQUENCY_ENABLE or
                              DPLL_CFGR1_DCO_FRACTION (DCO_Freq) or
                              DPLL_CFGR1_DCO_INTEGER (DCO_Freq));
            Registers.Write
              (Register    => DPLL_CFGR2 (PLL),
               Value       => DPLL_CFGR2_QDIV (QDiv) or
                              DPLL_CFGR2_KDIV (KDiv) or
                              DPLL_CFGR2_PDIV (PDiv) or
                              DPLL_CFGR2_CENTRAL_FREQ (Central_Freq));
            Registers.Posting_Read (Registers.DPLL_CTRL1);
            Registers.Posting_Read (DPLL_CFGR1 (PLL));
            Registers.Posting_Read (DPLL_CFGR2 (PLL));
         end if;
      end if;

      if Success then
         Registers.Write
           (Register => DPLL_CTL (PLL),
            Value    => DPLL_CTL_PLL_ENABLE);
         Registers.Wait_Set_Mask
           (Register => Registers.DPLL_STATUS,
            Mask     => DPLL_STATUS_DPLLx_LOCK (PLL));
      end if;
   end On;

   procedure Off (PLL : Configurable_DPLLs) is
   begin
      Registers.Unset_Mask (DPLL_CTL (PLL), DPLL_CTL_PLL_ENABLE);
   end Off;

end HW.GFX.GMA.PLLs.DPLL;
