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

package body HW.GFX.GMA.Power_And_Clocks_Haswell is

   PWR_WELL_CTL_ENABLE_REQUEST   : constant := 1 * 2 ** 31;
   PWR_WELL_CTL_DISABLE_REQUEST  : constant := 0 * 2 ** 31;
   PWR_WELL_CTL_STATE_ENABLED    : constant := 1 * 2 ** 30;

   ----------------------------------------------------------------------------

   SRD_CTL_ENABLE          : constant := 1 * 2 ** 31;
   SRD_STATUS_STATE_MASK   : constant := 7 * 2 ** 29;

   type Pipe is (EDP, A, B, C);
   type SRD_Regs is record
      CTL     : Registers.Registers_Index;
      STATUS  : Registers.Registers_Index;
   end record;
   type SRD_Per_Pipe_Regs is array (Pipe) of SRD_Regs;
   SRD : constant SRD_Per_Pipe_Regs := SRD_Per_Pipe_Regs'
     (A     => SRD_Regs'
        (CTL      => Registers.SRD_CTL_A,
         STATUS   => Registers.SRD_STATUS_A),
      B     => SRD_Regs'
        (CTL      => Registers.SRD_CTL_B,
         STATUS   => Registers.SRD_STATUS_B),
      C     => SRD_Regs'
        (CTL      => Registers.SRD_CTL_C,
         STATUS   => Registers.SRD_STATUS_C),
      EDP   => SRD_Regs'
        (CTL      => Registers.SRD_CTL_EDP,
         STATUS   => Registers.SRD_STATUS_EDP));

   ----------------------------------------------------------------------------

   IPS_CTL_ENABLE          : constant := 1 * 2 ** 31;
   DISPLAY_IPS_CONTROL     : constant := 16#19#;

   GT_MAILBOX_READY        : constant := 1 * 2 ** 31;

   ----------------------------------------------------------------------------

   procedure PSR_Off
   is
      Enabled : Boolean;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Config.Has_Per_Pipe_SRD then
         for P in Pipe loop
            Registers.Is_Set_Mask (SRD (P).CTL, SRD_CTL_ENABLE, Enabled);
            if Enabled then
               Registers.Unset_Mask (SRD (P).CTL, SRD_CTL_ENABLE);
               Registers.Wait_Unset_Mask (SRD (P).STATUS, SRD_STATUS_STATE_MASK);

               pragma Debug (Debug.Put_Line ("Disabled PSR."));
            end if;
         end loop;
      else
         Registers.Is_Set_Mask (Registers.SRD_CTL, SRD_CTL_ENABLE, Enabled);
         if Enabled then
            Registers.Unset_Mask (Registers.SRD_CTL, SRD_CTL_ENABLE);
            Registers.Wait_Unset_Mask (Registers.SRD_STATUS, SRD_STATUS_STATE_MASK);

            pragma Debug (Debug.Put_Line ("Disabled PSR."));
         end if;
      end if;
   end PSR_Off;

   ----------------------------------------------------------------------------

   procedure GT_Mailbox_Write (MBox : Word32; Value : Word32) is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Wait_Unset_Mask (Registers.GT_MAILBOX, GT_MAILBOX_READY);
      Registers.Write (Registers.GT_MAILBOX_DATA, Value);
      Registers.Write (Registers.GT_MAILBOX, GT_MAILBOX_READY or MBox);

      Registers.Wait_Unset_Mask (Registers.GT_MAILBOX, GT_MAILBOX_READY);
      Registers.Write (Registers.GT_MAILBOX_DATA, 0);
   end GT_Mailbox_Write;

   procedure IPS_Off
   is
      Enabled : Boolean;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Config.Has_IPS then
         Registers.Is_Set_Mask (Registers.IPS_CTL, IPS_CTL_ENABLE, Enabled);
         if Enabled then
            if Config.Has_IPS_CTL_Mailbox then
               GT_Mailbox_Write (DISPLAY_IPS_CONTROL, 0);
               Registers.Wait_Unset_Mask
                 (Register => Registers.IPS_CTL,
                  Mask     => IPS_CTL_ENABLE,
                  TOut_MS  => 42);
            else
               Registers.Unset_Mask (Registers.IPS_CTL, IPS_CTL_ENABLE);
            end if;

            pragma Debug (Debug.Put_Line ("Disabled IPS."));
            -- We have to wait until the next vblank here.
            -- 20ms should be enough.
            Time.M_Delay (20);
         end if;
      end if;
   end IPS_Off;

   ----------------------------------------------------------------------------

   procedure PDW_Off
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
          PWR_WELL_CTL_ENABLE_REQUEST) /= 0
      then
         Registers.Wait_Set_Mask
           (Registers.PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_STATE_ENABLED);
      end if;

      if (Ctl1 and PWR_WELL_CTL_ENABLE_REQUEST) /= 0 then
         Registers.Write (Registers.PWR_WELL_CTL_BIOS, PWR_WELL_CTL_DISABLE_REQUEST);
      end if;

      if (Ctl2 and PWR_WELL_CTL_ENABLE_REQUEST) /= 0 then
         Registers.Write (Registers.PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_DISABLE_REQUEST);
      end if;
   end PDW_Off;

   procedure PDW_On
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
          PWR_WELL_CTL_ENABLE_REQUEST) = 0
      then
         Registers.Wait_Unset_Mask
           (Registers.PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_STATE_ENABLED);
      end if;

      if (Ctl2 and PWR_WELL_CTL_ENABLE_REQUEST) = 0 then
         Registers.Write (Registers.PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_ENABLE_REQUEST);
         Registers.Wait_Set_Mask
           (Registers.PWR_WELL_CTL_DRIVER, PWR_WELL_CTL_STATE_ENABLED);
      end if;
   end PDW_On;

   function Need_PDW (Checked_Configs : Pipe_Configs) return Boolean is
   begin
      return (Checked_Configs (Primary).Port /= Disabled and
              Checked_Configs (Primary).Port /= Internal) or
             Checked_Configs (Secondary).Port /= Disabled or
             Checked_Configs (Tertiary).Port /= Disabled;
   end Need_PDW;

   ----------------------------------------------------------------------------

   procedure Pre_All_Off is
   begin
      -- HSW: disable panel self refresh (PSR) on eDP if enabled
         -- wait for PSR idling
      PSR_Off;
      IPS_Off;
   end Pre_All_Off;

   procedure Initialize is
   begin
      -- HSW: disable power down well
      PDW_Off;
   end Initialize;

   procedure Power_Set_To (Configs : Pipe_Configs) is
   begin
      if Need_PDW (Configs) then
         PDW_On;
      else
         PDW_Off;
      end if;
   end Power_Set_To;

   procedure Power_Up (Old_Configs, New_Configs : Pipe_Configs) is
   begin
      if not Need_PDW (Old_Configs) and Need_PDW (New_Configs) then
         PDW_On;
      end if;
   end Power_Up;

   procedure Power_Down (Old_Configs, Tmp_Configs, New_Configs : Pipe_Configs)
   is
   begin
      if (Need_PDW (Old_Configs) or Need_PDW (Tmp_Configs)) and
         not Need_PDW (New_Configs)
      then
         PDW_Off;
      end if;
   end Power_Down;

end HW.GFX.GMA.Power_And_Clocks_Haswell;
