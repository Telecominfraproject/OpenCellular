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

with HW.GFX.I2C;
with HW.GFX.EDID;
with HW.GFX.GMA.Config;
with HW.GFX.GMA.I2C;
with HW.GFX.GMA.DP_Aux_Ch;
with HW.GFX.GMA.DP_Info;
with HW.GFX.GMA.Registers;
with HW.GFX.GMA.Power_And_Clocks;
with HW.GFX.GMA.Panel;
with HW.GFX.GMA.PLLs;
with HW.GFX.GMA.Port_Detect;
with HW.GFX.GMA.Connectors;
with HW.GFX.GMA.Connector_Info;
with HW.GFX.GMA.Pipe_Setup;

with System;

with HW.Debug;
with GNAT.Source_Info;

use type HW.Word8;
use type HW.Int32;

package body HW.GFX.GMA
   with Refined_State =>
     (State =>
        (Registers.Address_State,
         PLLs.State, Panel.Panel_State,
         Cur_Configs, Allocated_PLLs, DP_Links,
         HPD_Delay, Wait_For_HPD),
      Init_State => Initialized,
      Config_State => Config.Valid_Port_GPU,
      Device_State =>
        (Registers.Register_State, Registers.GTT_State))
is

   subtype Port_Name is String (1 .. 8);
   type Port_Name_Array is array (Port_Type) of Port_Name;
   Port_Names : constant Port_Name_Array :=
     (Disabled => "Disabled",
      Internal => "Internal",
      DP1      => "DP1     ",
      DP2      => "DP2     ",
      DP3      => "DP3     ",
      HDMI1    => "HDMI1   ",
      HDMI2    => "HDMI2   ",
      HDMI3    => "HDMI3   ",
      Analog   => "Analog  ");

   package Display_Controller renames Pipe_Setup;

   type PLLs_Type is array (Pipe_Index) of PLLs.T;

   type Links_Type is array (Pipe_Index) of DP_Link;

   type HPD_Type is array (Port_Type) of Boolean;
   type HPD_Delay_Type is array (Port_Type) of Time.T;

   Cur_Configs : Pipe_Configs;
   Allocated_PLLs : PLLs_Type;
   DP_Links : Links_Type;
   HPD_Delay : HPD_Delay_Type;
   Wait_For_HPD : HPD_Type;
   Initialized : Boolean := False;

   subtype Active_Port_Type is Port_Type range Port_Type'Succ (Disabled) .. Port_Type'Last;

   ----------------------------------------------------------------------------

   PCH_RAWCLK_FREQ_MASK                : constant := 16#3ff# * 2 ** 0;

   function PCH_RAWCLK_FREQ (Freq : Frequency_Type) return Word32
   is
   begin
      return Word32 (Freq / 1_000_000);
   end PCH_RAWCLK_FREQ;

   ----------------------------------------------------------------------------

   function To_GPU_Port
     (Pipe : Pipe_Index;
      Port : Active_Port_Type)
      return GPU_Port
   is
   begin
      return
        (case Config.CPU is
            when Ironlake .. Ivybridge => -- everything but eDP through FDI/PCH
              (if Config.Internal_Is_EDP and then Port = Internal then
                  DIGI_A
               else
                 (case Pipe is
                     -- FDIs are fixed to the CPU pipe
                     when Primary   => DIGI_B,
                     when Secondary => DIGI_C,
                     when Tertiary  => DIGI_D)),
            when Haswell .. Skylake =>    -- everything but VGA directly on CPU
              (case Port is
                  when Internal     => DIGI_A,  -- LVDS not available
                  when HDMI1 | DP1  => DIGI_B,
                  when HDMI2 | DP2  => DIGI_C,
                  when HDMI3 | DP3  => DIGI_D,
                  when Analog       => DIGI_E));
   end To_GPU_Port;

   function To_PCH_Port (Port : Active_Port_Type) return PCH_Port
   with Pre => True
   is
   begin
      return
        (case Port is
            when Internal  => PCH_LVDS,   -- will be ignored if Internal is DP
            when Analog    => PCH_DAC,
            when HDMI1     => PCH_HDMI_B,
            when HDMI2     => PCH_HDMI_C,
            when HDMI3     => PCH_HDMI_D,
            when DP1       => PCH_DP_B,
            when DP2       => PCH_DP_C,
            when DP3       => PCH_DP_D);
   end To_PCH_Port;

   function To_Display_Type (Port : Active_Port_Type) return Display_Type
   with Pre => True
   is
   begin
      return Display_Type'
        (case Port is
            when Internal        => Config.Internal_Display,
            when Analog          => VGA,
            when HDMI1 .. HDMI3  => HDMI,
            when DP1 .. DP3      => DP);
   end To_Display_Type;

   -- Prepares link rate and lane count settings for an FDI connection.
   procedure Configure_FDI_Link
     (Port_Cfg : in out Port_Config;
      Success  :    out Boolean)
   with Pre => True
   is
      procedure Limit_Lane_Count
      is
         FDI_TX_CTL_FDI_TX_ENABLE : constant := 1 * 2 ** 31;
         Enabled : Boolean;
      begin
         -- if DIGI_D enabled: (FDI names are off by one)
         Registers.Is_Set_Mask
           (Register => Registers.FDI_TX_CTL_C,
            Mask     => FDI_TX_CTL_FDI_TX_ENABLE,
            Result   => Enabled);
         if Enabled then
            Port_Cfg.FDI.Receiver_Caps.Max_Lane_Count := DP_Lane_Count_2;
         end if;
      end Limit_Lane_Count;
   begin
      Port_Cfg.FDI.Receiver_Caps.Max_Link_Rate    := DP_Bandwidth_2_7;
      Port_Cfg.FDI.Receiver_Caps.Max_Lane_Count   :=
         Config.FDI_Lane_Count (Port_Cfg.Port);
      Port_Cfg.FDI.Receiver_Caps.Enhanced_Framing := True;
      if Config.Has_FDI_C and then Port_Cfg.Port = DIGI_C then
         Limit_Lane_Count;
      end if;
      DP_Info.Preferred_Link_Setting (Port_Cfg.FDI, Port_Cfg.Mode, Success);
   end Configure_FDI_Link;

   -- Validates that a given configuration should work with
   -- a given framebuffer.
   function Validate_Config
     (Framebuffer : Framebuffer_Type;
      Port_Cfg    : Port_Config;
      I           : Pipe_Index)
      return Boolean
   with
      Post =>
        (if Validate_Config'Result then
            Framebuffer.Width <= Pos32 (Port_Cfg.Mode.H_Visible) and
            Framebuffer.Height <= Pos32 (Port_Cfg.Mode.V_Visible))
   is
   begin
      -- No downscaling
      -- Respect maximum scalable width
      -- VGA plane is only allowed on the primary pipe
      -- Only 32bpp RGB (ignored for VGA plane)
      -- Stride must be a multiple of 64 (ignored for VGA plane)
      return
         ((Framebuffer.Width = Pos32 (Port_Cfg.Mode.H_Visible) and
           Framebuffer.Height = Pos32 (Port_Cfg.Mode.V_Visible)) or
          (Framebuffer.Width <= Config.Maximum_Scalable_Width (I) and
           Framebuffer.Width <= Pos32 (Port_Cfg.Mode.H_Visible) and
           Framebuffer.Height <= Pos32 (Port_Cfg.Mode.V_Visible))) and
         (Framebuffer.Offset /= VGA_PLANE_FRAMEBUFFER_OFFSET or I = Primary) and
         (Framebuffer.Offset = VGA_PLANE_FRAMEBUFFER_OFFSET or
          (Framebuffer.BPC = 8 and
           Framebuffer.Stride mod 64 = 0));
   end Validate_Config;

   -- Derives an internal port config.
   --
   -- This is where the magic happens that hides the hardware details
   -- from libgfxinit's users. We have to map the pipe (Pipe_Index),
   -- the user visible port (Port_Type) and the modeline (Mode_Type)
   -- that we are supposed to output to an internal representation
   -- (Port_Config) that applies to the selected hardware generation
   -- (in GMA.Config).
   procedure Fill_Port_Config
     (Port_Cfg :    out Port_Config;
      Pipe     : in     Pipe_Index;
      Port     : in     Port_Type;
      Mode     : in     Mode_Type;
      Success  :    out Boolean)
   with Pre => True
   is
   begin
      Success :=
         GMA.Config.Supported_Pipe (Pipe) and then
         GMA.Config.Valid_Port (Port) and then
         Port /= Disabled; -- Valid_Port should already cover this, but the
                           -- array is writeable, so it's hard to prove this.

      if Success then
         declare
            Link : constant DP_Link := DP_Links (Pipe);
         begin
            Port_Cfg := Port_Config'
              (Port     => To_GPU_Port (Pipe, Port),
               PCH_Port => To_PCH_Port (Port),
               Display  => To_Display_Type (Port),
               Mode     => Mode,
               Is_FDI   => GMA.Config.Is_FDI_Port (Port),
               FDI      => Default_DP,
               DP       => Link);
         end;

         if Port_Cfg.Is_FDI then
            Configure_FDI_Link (Port_Cfg, Success);
         end if;

         if Success then
            if Port_Cfg.Mode.BPC = Auto_BPC then
               Port_Cfg.Mode.BPC := Connector_Info.Default_BPC (Port_Cfg);
            end if;

            if Port_Cfg.Display = HDMI then
               declare
                  pragma Assert (Config.HDMI_Max_Clock_24bpp * 8
                                 / Port_Cfg.Mode.BPC >= Frequency_Type'First);
                  Max_Dotclock : constant Frequency_Type :=
                     Config.HDMI_Max_Clock_24bpp * 8 / Port_Cfg.Mode.BPC;
               begin
                  if Port_Cfg.Mode.Dotclock > Max_Dotclock then
                     pragma Debug (Debug.Put ("Dotclock "));
                     pragma Debug (Debug.Put_Int64 (Port_Cfg.Mode.Dotclock));
                     pragma Debug (Debug.Put (" too high, limiting to "));
                     pragma Debug (Debug.Put_Int64 (Max_Dotclock));
                     pragma Debug (Debug.Put_Line ("."));
                     Port_Cfg.Mode.Dotclock := Max_Dotclock;
                  end if;
               end;
            end if;
         end if;
      else
         Port_Cfg := Port_Config'
           (Port     => GPU_Port'First,
            PCH_Port => PCH_Port'First,
            Display  => Display_Type'First,
            Mode     => Invalid_Mode,
            Is_FDI   => False,
            FDI      => Default_DP,
            DP       => Default_DP);
      end if;
   end Fill_Port_Config;

   ----------------------------------------------------------------------------

   function To_Controller
      (Dsp_Config : Pipe_Index) return Display_Controller.Controller_Type
   is
      Result : Display_Controller.Controller_Type;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      case Dsp_Config is
         when Primary =>
            Result := Display_Controller.Controllers (Display_Controller.A);
         when Secondary =>
            Result := Display_Controller.Controllers (Display_Controller.B);
         when Tertiary =>
            Result := Display_Controller.Controllers (Display_Controller.C);
      end case;
      return Result;
   end To_Controller;

   ----------------------------------------------------------------------------

   function To_Head
     (N_Config : Pipe_Index;
      Port     : Active_Port_Type)
      return Display_Controller.Head_Type
   is
      Result : Display_Controller.Head_Type;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Config.Has_EDP_Pipe and then Port = Internal then
         Result := Display_Controller.Heads (Display_Controller.Head_EDP);
      else
         case N_Config is
            when Primary =>
               Result := Display_Controller.Heads (Display_Controller.Head_A);
            when Secondary =>
               Result := Display_Controller.Heads (Display_Controller.Head_B);
            when Tertiary =>
               Result := Display_Controller.Heads (Display_Controller.Head_C);
         end case;
      end if;
      return Result;
   end To_Head;

   ----------------------------------------------------------------------------

   procedure Legacy_VGA_Off
   is
      Reg8 : Word8;
   begin
      -- disable legacy VGA plane, taking over control now
      Port_IO.OutB (VGA_SR_INDEX, VGA_SR01);
      Port_IO.InB  (Reg8, VGA_SR_DATA);
      Port_IO.OutB (VGA_SR_DATA, Reg8 or 1 * 2 ** 5);
      Time.U_Delay (100); -- PRM says 100us, Linux does 300
      Registers.Set_Mask (Registers.VGACNTRL, 1 * 2 ** 31);
   end Legacy_VGA_Off;

   ----------------------------------------------------------------------------

   function Port_Configured
     (Configs  : Pipe_Configs;
      Port     : Port_Type)
      return Boolean
   with
      Global => null
   is
   begin
      return Configs (Primary).Port    = Port or
             Configs (Secondary).Port  = Port or
             Configs (Tertiary).Port   = Port;
   end Port_Configured;

   -- DP and HDMI share physical pins.
   function Sibling_Port (Port : Port_Type) return Port_Type
   is
   begin
      return
        (case Port is
            when HDMI1 => DP1,
            when HDMI2 => DP2,
            when HDMI3 => DP3,
            when DP1 => HDMI1,
            when DP2 => HDMI2,
            when DP3 => HDMI3,
            when others => Disabled);
   end Sibling_Port;

   function Has_Sibling_Port (Port : Port_Type) return Boolean
   is
   begin
      return Sibling_Port (Port) /= Disabled;
   end Has_Sibling_Port;

   procedure Read_EDID
     (Raw_EDID :    out EDID.Raw_EDID_Data;
      Port     : in     Active_Port_Type;
      Success  :    out Boolean)
   with
      Post => (if Success then EDID.Valid (Raw_EDID))
   is
      Raw_EDID_Length : GFX.I2C.Transfer_Length := Raw_EDID'Length;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      for I in 1 .. 2 loop
         if To_Display_Type (Port) = DP then
            -- May need power to read edid
            declare
               Temp_Configs : Pipe_Configs := Cur_Configs;
            begin
               Temp_Configs (Primary).Port := Port;
               Power_And_Clocks.Power_Up (Cur_Configs, Temp_Configs);
            end;

            declare
               DP_Port : constant GMA.DP_Port :=
                 (case Port is
                     when Internal  => DP_A,
                     when DP1       => DP_B,
                     when DP2       => DP_C,
                     when DP3       => DP_D,
                     when others    => GMA.DP_Port'First);
            begin
               DP_Aux_Ch.I2C_Read
                 (Port     => DP_Port,
                  Address  => 16#50#,
                  Length   => Raw_EDID_Length,
                  Data     => Raw_EDID,
                  Success  => Success);
            end;
         else
            I2C.I2C_Read
              (Port     => (if Port = Analog
                            then Config.Analog_I2C_Port
                            else To_PCH_Port (Port)),
               Address  => 16#50#,
               Length   => Raw_EDID_Length,
               Data     => Raw_EDID,
               Success  => Success);
         end if;
         exit when not Success;  -- don't retry if reading itself failed

         pragma Debug (Debug.Put_Buffer ("EDID", Raw_EDID, Raw_EDID_Length));
         EDID.Sanitize (Raw_EDID, Success);
         exit when Success;
      end loop;
   end Read_EDID;

   procedure Probe_Port
     (Pipe_Cfg : in out Pipe_Config;
      Port     : in     Active_Port_Type;
      Success  :    out Boolean)
   with Pre => True
   is
      Raw_EDID : EDID.Raw_EDID_Data := (others => 16#00#);
   begin
      Success := Config.Valid_Port (Port);

      if Success then
         if Port = Internal then
            Panel.On;
         end if;
         Read_EDID (Raw_EDID, Port, Success);
      end if;

      if Success and then
         (EDID.Compatible_Display (Raw_EDID, To_Display_Type (Port)) and
          EDID.Has_Preferred_Mode (Raw_EDID))
      then
         Pipe_Cfg.Port := Port;
         Pipe_Cfg.Mode := EDID.Preferred_Mode (Raw_EDID);

         pragma Warnings (GNATprove, Off, "unused assignment to ""Raw_EDID""",
            Reason => "We just want to check if it's readable.");
         if Has_Sibling_Port (Port) then
            -- Probe sibling port too and bail out if something is detected.
            -- This is a precaution for adapters that expose the pins of a
            -- port for both HDMI/DVI and DP (like some ThinkPad docks). A
            -- user might have attached both by accident and there are ru-
            -- mors of displays that got fried by applying the wrong signal.
            declare
               Have_Sibling_EDID : Boolean;
            begin
               Read_EDID (Raw_EDID, Sibling_Port (Port), Have_Sibling_EDID);
               if Have_Sibling_EDID then
                  Pipe_Cfg.Port := Disabled;
                  Success := False;
               end if;
            end;
         end if;
         pragma Warnings (GNATprove, On, "unused assignment to ""Raw_EDID""");
      else
         Success := False;
         if Port = Internal then
            Panel.Off;
         end if;
      end if;
   end Probe_Port;

   procedure Scan_Ports
     (Configs        :    out Pipe_Configs;
      Ports          : in     Port_List;
      Max_Pipe       : in     Pipe_Index := Pipe_Index'Last)
   is
      Port_Idx : Port_List_Range := Port_List_Range'First;
      Success  : Boolean;
   begin
      Configs := (Pipe_Index =>
                    (Port        => Disabled,
                     Mode        => Invalid_Mode,
                     Framebuffer => Default_FB));

      for Pipe in Pipe_Index range
         Pipe_Index'First .. Pipe_Index'Min (Max_Pipe, Config.Max_Pipe)
      loop
         while Ports (Port_Idx) /= Disabled loop
            if not Port_Configured (Configs, Ports (Port_Idx)) and
               (not Has_Sibling_Port (Ports (Port_Idx)) or
                not Port_Configured (Configs, Sibling_Port (Ports (Port_Idx))))
            then
               Probe_Port (Configs (Pipe), Ports (Port_Idx), Success);
            else
               Success := False;
            end if;

            exit when Port_Idx = Port_List_Range'Last;
            Port_Idx := Port_List_Range'Succ (Port_Idx);

            exit when Success;
         end loop;
      end loop;

      -- Restore power settings
      Power_And_Clocks.Power_Set_To (Cur_Configs);
   end Scan_Ports;

   ----------------------------------------------------------------------------

   procedure Update_Outputs (Configs : Pipe_Configs)
   is
      Did_Power_Up : Boolean := False;

      HPD, HPD_Delay_Over, Success : Boolean;
      Old_Config, New_Config : Pipe_Config;
      Old_Configs : Pipe_Configs;
      Port_Cfg : Port_Config;

      procedure Check_HPD
        (Port_Cfg : in     Port_Config;
         Port     : in     Port_Type;
         Detected :    out Boolean)
      is
      begin
         HPD_Delay_Over := Time.Timed_Out (HPD_Delay (Port));
         if HPD_Delay_Over then
            Port_Detect.Hotplug_Detect (Port_Cfg, Detected);
            HPD_Delay (Port) := Time.MS_From_Now (333);
         else
            Detected := False;
         end if;
      end Check_HPD;
   begin
      Old_Configs := Cur_Configs;

      for I in Pipe_Index loop
         HPD := False;

         Old_Config := Cur_Configs (I);
         New_Config := Configs (I);

         Fill_Port_Config
           (Port_Cfg, I, Old_Configs (I).Port, Old_Configs (I).Mode, Success);
         if Success then
            Check_HPD (Port_Cfg, Old_Config.Port, HPD);
         end if;

         -- Connector changed?
         if (Success and then HPD) or
            Old_Config.Port /= New_Config.Port or
            Old_Config.Mode /= New_Config.Mode
         then
            if Old_Config.Port /= Disabled then
               if Success then
                  pragma Debug (Debug.New_Line);
                  pragma Debug (Debug.Put_Line
                    ("Disabling port " & Port_Names (Old_Config.Port)));

                  Connectors.Pre_Off (Port_Cfg);

                  Display_Controller.Off
                    (To_Controller (I), To_Head (I, Old_Config.Port));

                  Connectors.Post_Off (Port_Cfg);
               end if;

               -- Free PLL
               PLLs.Free (Allocated_PLLs (I));

               Cur_Configs (I).Port := Disabled;
            end if;

            if New_Config.Port /= Disabled then
               Fill_Port_Config
                 (Port_Cfg, I, Configs (I).Port, Configs (I).Mode, Success);

               Success := Success and then
                          Validate_Config (New_Config.Framebuffer, Port_Cfg, I);

               if Success and then Wait_For_HPD (New_Config.Port) then
                  Check_HPD (Port_Cfg, New_Config.Port, Success);
                  Wait_For_HPD (New_Config.Port) := not Success;
               end if;

               if Success then
                  pragma Debug (Debug.New_Line);
                  pragma Debug (Debug.Put_Line
                    ("Trying to enable port " & Port_Names (New_Config.Port)));

                  if not Did_Power_Up then
                     Power_And_Clocks.Power_Up (Old_Configs, Configs);
                     Did_Power_Up := True;
                  end if;
               end if;

               if Success then
                  Connector_Info.Preferred_Link_Setting
                    (Port_Cfg => Port_Cfg,
                     Success  => Success);
               end if;

               while Success loop
                  pragma Loop_Invariant
                    (New_Config.Port in Active_Port_Type and
                     Port_Cfg.Mode = Port_Cfg.Mode'Loop_Entry);

                  PLLs.Alloc
                    (Port_Cfg => Port_Cfg,
                     PLL      => Allocated_PLLs (I),
                     Success  => Success);

                  if Success then
                     for Try in 1 .. 2 loop
                        pragma Loop_Invariant
                          (New_Config.Port in Active_Port_Type);

                        Connectors.Pre_On
                          (Port_Cfg    => Port_Cfg,
                           PLL_Hint    => PLLs.Register_Value
                                            (Allocated_PLLs (I)),
                           Pipe_Hint   => Display_Controller.Get_Pipe_Hint
                                            (To_Head (I, New_Config.Port)),
                           Success     => Success);

                        if Success then
                           Display_Controller.On
                             (Controller  => To_Controller (I),
                              Head        => To_Head (I, New_Config.Port),
                              Port_Cfg    => Port_Cfg,
                              Framebuffer => New_Config.Framebuffer);

                           Connectors.Post_On
                             (Port_Cfg => Port_Cfg,
                              PLL_Hint => PLLs.Register_Value
                                            (Allocated_PLLs (I)),
                              Success  => Success);

                           if not Success then
                              Display_Controller.Off
                                (To_Controller (I),
                                 To_Head (I, New_Config.Port));
                              Connectors.Post_Off (Port_Cfg);
                           end if;
                        end if;

                        exit when Success;
                     end loop;
                     exit when Success;   -- connection established => stop loop

                     -- connection failed
                     PLLs.Free (Allocated_PLLs (I));
                  end if;

                  Connector_Info.Next_Link_Setting
                    (Port_Cfg => Port_Cfg,
                     Success  => Success);
               end loop;

               if Success then
                  pragma Debug (Debug.Put_Line
                    ("Enabled port " & Port_Names (New_Config.Port)));
                  Cur_Configs (I) := New_Config;
                  DP_Links (I) := Port_Cfg.DP;
               else
                  Wait_For_HPD (New_Config.Port) := True;
                  if New_Config.Port = Internal then
                     Panel.Off;
                  end if;
               end if;
            else
               Cur_Configs (I) := New_Config;
            end if;
         elsif Old_Config.Framebuffer /= New_Config.Framebuffer and
               Old_Config.Port /= Disabled
         then
            Display_Controller.Update_Offset
              (Controller  => To_Controller (I),
               Framebuffer => New_Config.Framebuffer);
            Cur_Configs (I) := New_Config;
         end if;
      end loop;

      if Did_Power_Up then
         Power_And_Clocks.Power_Down (Old_Configs, Configs, Cur_Configs);
      end if;

   end Update_Outputs;

   ----------------------------------------------------------------------------

   procedure Initialize
     (MMIO_Base   : in     Word64 := 0;
      Write_Delay : in     Word64 := 0;
      Success     :    out Boolean)
   with
      Refined_Global =>
        (In_Out =>
           (Config.Valid_Port_GPU,
            Registers.Register_State, Port_IO.State),
         Input =>
           (Time.State),
         Output =>
           (Registers.Address_State,
            PLLs.State, Panel.Panel_State,
            Cur_Configs, Allocated_PLLs, DP_Links,
            HPD_Delay, Wait_For_HPD, Initialized))
   is
      use type HW.Word64;

      Now : constant Time.T := Time.Now;

      procedure Check_Platform (Success : out Boolean)
      is
         Audio_VID_DID : Word32;
      begin
         case Config.CPU is
            when Haswell .. Skylake =>
               Registers.Read (Registers.AUD_VID_DID, Audio_VID_DID);
            when Ironlake .. Ivybridge =>
               Registers.Read (Registers.PCH_AUD_VID_DID, Audio_VID_DID);
         end case;
         Success :=
           (case Config.CPU is
               when Skylake      => Audio_VID_DID = 16#8086_2809#,
               when Broadwell    => Audio_VID_DID = 16#8086_2808#,
               when Haswell      => Audio_VID_DID = 16#8086_2807#,
               when Ivybridge |
                    Sandybridge  => Audio_VID_DID = 16#8086_2806# or
                                    Audio_VID_DID = 16#8086_2805#,
               when Ironlake     => Audio_VID_DID = 16#0000_0000#);
      end Check_Platform;
   begin
      pragma Warnings (GNATprove, Off, "unused variable ""Write_Delay""",
         Reason => "Write_Delay is used for debugging only");

      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      pragma Debug (Debug.Set_Register_Write_Delay (Write_Delay));

      Wait_For_HPD := HPD_Type'(others => False);
      HPD_Delay := HPD_Delay_Type'(others => Now);
      DP_Links := Links_Type'(others => HW.GFX.Default_DP);
      Allocated_PLLs := (others => PLLs.Invalid);
      Cur_Configs := Pipe_Configs'
        (others => Pipe_Config'
           (Port        => Disabled,
            Framebuffer => HW.GFX.Default_FB,
            Mode        => HW.GFX.Invalid_Mode));
      Registers.Set_Register_Base
        (if MMIO_Base /= 0 then
            MMIO_Base
         else
            Config.Default_MMIO_Base);
      PLLs.Initialize;

      Check_Platform (Success);
      if not Success then
         pragma Debug (Debug.Put_Line ("ERROR: Incompatible CPU or PCH."));

         Panel.Static_Init;   -- for flow analysis

         Initialized := False;
         return;
      end if;

      Panel.Setup_PP_Sequencer;
      Port_Detect.Initialize;

      Power_And_Clocks.Pre_All_Off;

      Legacy_VGA_Off;

      Connectors.Pre_All_Off;
      Display_Controller.All_Off;
      Connectors.Post_All_Off;
      PLLs.All_Off;

      Power_And_Clocks.Post_All_Off;

      -------------------- Now restart from a clean state ---------------------
      Power_And_Clocks.Initialize;

      Registers.Unset_And_Set_Mask
        (Register    => Registers.PCH_RAWCLK_FREQ,
         Mask_Unset  => PCH_RAWCLK_FREQ_MASK,
         Mask_Set    => PCH_RAWCLK_FREQ (Config.Default_RawClk_Freq));

      Initialized := True;

   end Initialize;

   function Is_Initialized return Boolean
   with
      Refined_Post => Is_Initialized'Result = Initialized
   is
   begin
      return Initialized;
   end Is_Initialized;

   ----------------------------------------------------------------------------

   procedure Write_GTT
     (GTT_Page       : GTT_Range;
      Device_Address : GTT_Address_Type;
      Valid          : Boolean) is
   begin
      Registers.Write_GTT (GTT_Page, Device_Address, Valid);
   end Write_GTT;

   procedure Setup_Default_GTT (FB : Framebuffer_Type; Phys_FB : Word32)
   is
      FB_Size : constant Pos32 :=
         FB.Stride * FB.Height * Pos32 (((FB.BPC * 4) / 8));
      Phys_Addr : GTT_Address_Type := GTT_Address_Type (Phys_FB);
   begin
      for Idx in GTT_Range range 0 .. GTT_Range (((FB_Size + 4095) / 4096) - 1)
      loop
         Registers.Write_GTT
           (GTT_Page       => Idx,
            Device_Address => Phys_Addr,
            Valid          => True);
         Phys_Addr := Phys_Addr + 4096;
      end loop;
   end Setup_Default_GTT;

   ----------------------------------------------------------------------------

   procedure Dump_Configs (Configs : Pipe_Configs)
   is
      subtype Pipe_Name is String (1 .. 9);
      type Pipe_Name_Array is array (Pipe_Index) of Pipe_Name;
      Pipe_Names : constant Pipe_Name_Array :=
        (Primary     => "Primary  ",
         Secondary   => "Secondary",
         Tertiary    => "Tertiary ");
   begin
      Debug.New_Line;
      Debug.Put_Line ("CONFIG => ");
      for Pipe in Pipe_Index loop
         if Pipe = Pipe_Index'First then
            Debug.Put ("  (");
         else
            Debug.Put ("   ");
         end if;
         Debug.Put_Line (Pipe_Names (Pipe) & " =>");
         Debug.Put_Line
           ("     (Port => " & Port_Names (Configs (Pipe).Port) & ",");
         Debug.Put_Line ("      Framebuffer =>");
         Debug.Put ("        (Width  => ");
         Debug.Put_Int32 (Configs (Pipe).Framebuffer.Width);
         Debug.Put_Line (",");
         Debug.Put ("         Height => ");
         Debug.Put_Int32 (Configs (Pipe).Framebuffer.Height);
         Debug.Put_Line (",");
         Debug.Put ("         Stride => ");
         Debug.Put_Int32 (Configs (Pipe).Framebuffer.Stride);
         Debug.Put_Line (",");
         Debug.Put ("         Offset => ");
         Debug.Put_Word32 (Configs (Pipe).Framebuffer.Offset);
         Debug.Put_Line (",");
         Debug.Put ("         BPC    => ");
         Debug.Put_Int64 (Configs (Pipe).Framebuffer.BPC);
         Debug.Put_Line ("),");
         Debug.Put_Line ("      Mode =>");
         Debug.Put ("        (Dotclock           => ");
         Debug.Put_Int64 (Configs (Pipe).Mode.Dotclock);
         Debug.Put_Line (",");
         Debug.Put ("         H_Visible          => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.H_Visible);
         Debug.Put_Line (",");
         Debug.Put ("         H_Sync_Begin       => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.H_Sync_Begin);
         Debug.Put_Line (",");
         Debug.Put ("         H_Sync_End         => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.H_Sync_End);
         Debug.Put_Line (",");
         Debug.Put ("         H_Total            => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.H_Total);
         Debug.Put_Line (",");
         Debug.Put ("         V_Visible          => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.V_Visible);
         Debug.Put_Line (",");
         Debug.Put ("         V_Sync_Begin       => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.V_Sync_Begin);
         Debug.Put_Line (",");
         Debug.Put ("         V_Sync_End         => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.V_Sync_End);
         Debug.Put_Line (",");
         Debug.Put ("         V_Total            => ");
         Debug.Put_Int16 (Configs (Pipe).Mode.V_Total);
         Debug.Put_Line (",");
         Debug.Put_Line ("         H_Sync_Active_High => " &
           (if Configs (Pipe).Mode.H_Sync_Active_High
            then "True,"
            else "False,"));
         Debug.Put_Line ("         V_Sync_Active_High => " &
           (if Configs (Pipe).Mode.V_Sync_Active_High
            then "True,"
            else "False,"));
         Debug.Put ("         BPC                => ");
         Debug.Put_Int64 (Configs (Pipe).Mode.BPC);
         if Pipe /= Pipe_Index'Last then
            Debug.Put_Line (")),");
         else
            Debug.Put_Line (")));");
         end if;
      end loop;
   end Dump_Configs;

end HW.GFX.GMA;
