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

with HW.GFX.GMA.Config;
with HW.GFX.GMA.Config_Helpers;
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

use type HW.Int32;

package body HW.GFX.GMA
   with Refined_State =>
     (State =>
        (Registers.Address_State,
         PLLs.State, Panel.Panel_State,
         Cur_Configs, Allocated_PLLs,
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

   type HPD_Type is array (Port_Type) of Boolean;
   type HPD_Delay_Type is array (Active_Port_Type) of Time.T;

   Allocated_PLLs : PLLs_Type;
   HPD_Delay : HPD_Delay_Type;
   Wait_For_HPD : HPD_Type;
   Initialized : Boolean := False;

   ----------------------------------------------------------------------------

   PCH_RAWCLK_FREQ_MASK                : constant := 16#3ff# * 2 ** 0;

   function PCH_RAWCLK_FREQ (Freq : Frequency_Type) return Word32
   is
   begin
      return Word32 (Freq / 1_000_000);
   end PCH_RAWCLK_FREQ;

   ----------------------------------------------------------------------------

   procedure Enable_Output
     (Pipe     : in     Pipe_Index;
      Pipe_Cfg : in     Pipe_Config;
      Success  :    out Boolean)
   is
      Port_Cfg : Port_Config;
   begin
      pragma Debug (Debug.New_Line);
      pragma Debug (Debug.Put_Line
        ("Trying to enable port " & Port_Names (Pipe_Cfg.Port)));

      Config_Helpers.Fill_Port_Config
        (Port_Cfg, Pipe, Pipe_Cfg.Port, Pipe_Cfg.Mode, Success);

      if Success then
         Success := Config_Helpers.Validate_Config
           (Pipe_Cfg.Framebuffer, Port_Cfg, Pipe);
      end if;

      if Success then
         Connector_Info.Preferred_Link_Setting (Port_Cfg, Success);
      end if;

      -- loop over all possible DP-lane configurations
      -- (non-DP ports use a single fake configuration)
      while Success loop
         pragma Loop_Invariant
           (Pipe_Cfg.Port in Active_Port_Type and
            Port_Cfg.Mode = Port_Cfg.Mode'Loop_Entry);

         PLLs.Alloc
           (Port_Cfg => Port_Cfg,
            PLL      => Allocated_PLLs (Pipe),
            Success  => Success);

         if Success then
            -- try each DP-lane configuration twice
            for Try in 1 .. 2 loop
               pragma Loop_Invariant
                 (Pipe_Cfg.Port in Active_Port_Type);

               -- Clear pending hot-plug events before every try
               Port_Detect.Clear_Hotplug_Detect (Pipe_Cfg.Port);

               Connectors.Pre_On
                 (Pipe        => Pipe,
                  Port_Cfg    => Port_Cfg,
                  PLL_Hint    => PLLs.Register_Value (Allocated_PLLs (Pipe)),
                  Success     => Success);

               if Success then
                  Display_Controller.On
                    (Pipe        => Pipe,
                     Port_Cfg    => Port_Cfg,
                     Framebuffer => Pipe_Cfg.Framebuffer);

                  Connectors.Post_On
                    (Port_Cfg => Port_Cfg,
                     PLL_Hint => PLLs.Register_Value (Allocated_PLLs (Pipe)),
                     Success  => Success);

                  if not Success then
                     Display_Controller.Off (Pipe);
                     Connectors.Post_Off (Port_Cfg);
                  end if;
               end if;

               exit when Success;
            end loop;
            exit when Success;   -- connection established => stop loop

            -- connection failed
            PLLs.Free (Allocated_PLLs (Pipe));
         end if;

         Connector_Info.Next_Link_Setting (Port_Cfg, Success);
      end loop;

      if Success then
         pragma Debug (Debug.Put_Line
           ("Enabled port " & Port_Names (Pipe_Cfg.Port)));
      else
         Wait_For_HPD (Pipe_Cfg.Port) := True;
         if Pipe_Cfg.Port = Internal then
            Panel.Off;
         end if;
      end if;
   end Enable_Output;

   procedure Disable_Output (Pipe : Pipe_Index; Pipe_Cfg : Pipe_Config)
   is
      Port_Cfg : Port_Config;
      Success  : Boolean;
   begin
      Config_Helpers.Fill_Port_Config
        (Port_Cfg, Pipe, Pipe_Cfg.Port, Pipe_Cfg.Mode, Success);
      if Success then
         pragma Debug (Debug.New_Line);
         pragma Debug (Debug.Put_Line
           ("Disabling port " & Port_Names (Pipe_Cfg.Port)));
         pragma Debug (Debug.New_Line);

         Connectors.Pre_Off (Port_Cfg);
         Display_Controller.Off (Pipe);
         Connectors.Post_Off (Port_Cfg);

         PLLs.Free (Allocated_PLLs (Pipe));
      end if;
   end Disable_Output;

   procedure Update_Outputs (Configs : Pipe_Configs)
   is
      procedure Check_HPD (Port : in Active_Port_Type; Detected : out Boolean)
      is
         HPD_Delay_Over : constant Boolean := Time.Timed_Out (HPD_Delay (Port));
      begin
         if HPD_Delay_Over then
            Port_Detect.Hotplug_Detect (Port, Detected);
            HPD_Delay (Port) := Time.MS_From_Now (333);
         else
            Detected := False;
         end if;
      end Check_HPD;

      Power_Changed  : Boolean := False;
      Old_Configs    : Pipe_Configs;

      -- Only called when we actually tried to change something
      -- so we don't congest the log with unnecessary messages.
      procedure Update_Power
      is
      begin
         if not Power_Changed then
            Power_And_Clocks.Power_Up (Old_Configs, Configs);
            Power_Changed := True;
         end if;
      end Update_Power;
   begin
      Old_Configs := Cur_Configs;

      -- disable all pipes that changed or had a hot-plug event
      for Pipe in Pipe_Index loop
         declare
            Unplug_Detected : Boolean;
            Cur_Config : Pipe_Config renames Cur_Configs (Pipe);
            New_Config : Pipe_Config renames Configs (Pipe);
         begin
            if Cur_Config.Port /= Disabled then
               Check_HPD (Cur_Config.Port, Unplug_Detected);

               if Cur_Config.Port /= New_Config.Port or
                  Cur_Config.Mode /= New_Config.Mode or
                  Unplug_Detected
               then
                  Disable_Output (Pipe, Cur_Config);
                  Cur_Config.Port := Disabled;
                  Update_Power;
               end if;
            end if;
         end;
      end loop;

      -- enable all pipes that changed and should be active
      for Pipe in Pipe_Index loop
         declare
            Success : Boolean;
            Cur_Config : Pipe_Config renames Cur_Configs (Pipe);
            New_Config : Pipe_Config renames Configs (Pipe);
         begin
            if New_Config.Port /= Disabled and then
               (Cur_Config.Port /= New_Config.Port or
                Cur_Config.Mode /= New_Config.Mode)
            then
               if Wait_For_HPD (New_Config.Port) then
                  Check_HPD (New_Config.Port, Success);
                  Wait_For_HPD (New_Config.Port) := not Success;
               else
                  Success := True;
               end if;

               if Success then
                  Update_Power;
                  Enable_Output (Pipe, New_Config, Success);
               end if;

               if Success then
                  Cur_Config := New_Config;
               end if;

            -- update framebuffer offset only
            elsif New_Config.Port /= Disabled and
                  Cur_Config.Framebuffer /= New_Config.Framebuffer
            then
               Display_Controller.Update_Offset (Pipe, New_Config.Framebuffer);
               Cur_Config := New_Config;
            end if;
         end;
      end loop;

      if Power_Changed then
         Power_And_Clocks.Power_Down (Old_Configs, Configs, Cur_Configs);
      end if;
   end Update_Outputs;

   ----------------------------------------------------------------------------

   procedure Initialize
     (MMIO_Base   : in     Word64 := 0;
      Write_Delay : in     Word64 := 0;
      Clean_State : in     Boolean := False;
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
            Cur_Configs, Allocated_PLLs,
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
               when Broxton      => Audio_VID_DID = 16#8086_280a#,
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

      if Clean_State then
         Power_And_Clocks.Pre_All_Off;
         Connectors.Pre_All_Off;
         Display_Controller.All_Off;
         Connectors.Post_All_Off;
         PLLs.All_Off;
         Power_And_Clocks.Post_All_Off;
      else
         -- According to PRMs, VGA plane is the only thing
         -- that's enabled by default after reset.
         Display_Controller.Legacy_VGA_Off;
      end if;

      -------------------- Now restart from a clean state ---------------------
      Power_And_Clocks.Initialize;

      if Config.Has_PCH then
         Registers.Unset_And_Set_Mask
           (Register    => Registers.PCH_RAWCLK_FREQ,
            Mask_Unset  => PCH_RAWCLK_FREQ_MASK,
            Mask_Set    => PCH_RAWCLK_FREQ (Config.Default_RawClk_Freq));
      end if;

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
      Debug.Put_Line ("CONFIG =>");
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
