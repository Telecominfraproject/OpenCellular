--
-- Copyright (C) 2015-2016 secunet Security Networks AG
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
with HW.GFX.GMA.Transcoder;

package body HW.GFX.GMA.Pipe_Setup is

   ILK_DISPLAY_CHICKEN1_VGA_MASK       : constant := 7 * 2 ** 29;
   ILK_DISPLAY_CHICKEN1_VGA_ENABLE     : constant := 5 * 2 ** 29;
   ILK_DISPLAY_CHICKEN2_VGA_MASK       : constant := 1 * 2 ** 25;
   ILK_DISPLAY_CHICKEN2_VGA_ENABLE     : constant := 0 * 2 ** 25;

   DSPCNTR_ENABLE                      : constant :=  1 * 2 ** 31;
   DSPCNTR_GAMMA_CORRECTION            : constant :=  1 * 2 ** 30;
   DSPCNTR_DISABLE_TRICKLE_FEED        : constant :=  1 * 2 ** 14;
   DSPCNTR_FORMAT_MASK                 : constant := 15 * 2 ** 26;

   DSPCNTR_MASK : constant Word32 :=
      DSPCNTR_ENABLE or
      DSPCNTR_GAMMA_CORRECTION or
      DSPCNTR_FORMAT_MASK or
      DSPCNTR_DISABLE_TRICKLE_FEED;

   PLANE_CTL_PLANE_ENABLE              : constant := 1 * 2 ** 31;
   PLANE_CTL_SRC_PIX_FMT_RGB_32B_8888  : constant := 4 * 2 ** 24;
   PLANE_CTL_PLANE_GAMMA_DISABLE       : constant := 1 * 2 ** 13;

   PLANE_WM_ENABLE                     : constant :=        1 * 2 ** 31;
   PLANE_WM_LINES_SHIFT                : constant :=                 14;
   PLANE_WM_LINES_MASK                 : constant := 16#001f# * 2 ** 14;
   PLANE_WM_BLOCKS_MASK                : constant := 16#03ff# * 2 **  0;

   VGA_SR_INDEX                        : constant :=   16#03c4#;
   VGA_SR_DATA                         : constant :=   16#03c5#;
   VGA_SR01                            : constant :=     16#01#;
   VGA_SR01_SCREEN_OFF                 : constant := 1 * 2 ** 5;

   VGA_CONTROL_VGA_DISPLAY_DISABLE     : constant :=        1 * 2 ** 31;
   VGA_CONTROL_BLINK_DUTY_CYCLE_MASK   : constant := 16#0003# * 2 **  6;
   VGA_CONTROL_BLINK_DUTY_CYCLE_50     : constant :=        2 * 2 **  6;
   VGA_CONTROL_VSYNC_BLINK_RATE_MASK   : constant := 16#003f# * 2 **  0;

   subtype VGA_Cycle_Count is Pos32 range 2 .. 128;
   function VGA_CONTROL_VSYNC_BLINK_RATE
     (Cycles : VGA_Cycle_Count)
      return Word32
   is
   begin
      return Word32 (Cycles) / 2 - 1;
   end VGA_CONTROL_VSYNC_BLINK_RATE;

   PF_CTRL_ENABLE                      : constant := 1 * 2 ** 31;
   PF_CTRL_PIPE_SELECT_MASK            : constant := 3 * 2 ** 29;
   PF_CTRL_FILTER_MED                  : constant := 1 * 2 ** 23;

   PS_CTRL_ENABLE_SCALER               : constant := 1 * 2 ** 31;
   PS_CTRL_SCALER_MODE_7X5_EXTENDED    : constant := 1 * 2 ** 28;
   PS_CTRL_FILTER_SELECT_MEDIUM_2      : constant := 1 * 2 ** 23;

   ---------------------------------------------------------------------------

   function PLANE_WM_LINES (Lines : Natural) return Word32 is
   begin
      return Shift_Left (Word32 (Lines), PLANE_WM_LINES_SHIFT)
               and PLANE_WM_LINES_MASK;
   end PLANE_WM_LINES;

   function PLANE_WM_BLOCKS (Blocks : Natural) return Word32 is
   begin
      return Word32 (Blocks) and PLANE_WM_BLOCKS_MASK;
   end PLANE_WM_BLOCKS;

   ---------------------------------------------------------------------------

   function Encode (LSW, MSW : Pos16) return Word32 is
   begin
      return Shift_Left (Word32 (MSW) - 1, 16) or (Word32 (LSW) - 1);
   end Encode;

   ----------------------------------------------------------------------------

   procedure Clear_Watermarks (Controller : Controller_Type) is
   begin
      Registers.Write
        (Register    => Controller.PLANE_BUF_CFG,
         Value       => 16#0000_0000#);
      for Level in WM_Levels range 0 .. WM_Levels'Last loop
         Registers.Write
           (Register => Controller.PLANE_WM (Level),
            Value    => 16#0000_0000#);
      end loop;
      Registers.Write
        (Register    => Controller.WM_LINETIME,
         Value       => 16#0000_0000#);
   end Clear_Watermarks;

   procedure Setup_Watermarks (Controller : Controller_Type)
   is
      type Per_Plane_Buffer_Range is array (Pipe_Index) of Word32;
      Buffer_Range : constant Per_Plane_Buffer_Range :=
        (Primary     => Shift_Left (159, 16) or   0,
         Secondary   => Shift_Left (319, 16) or 160,
         Tertiary    => Shift_Left (479, 16) or 320);
   begin
      Registers.Write
        (Register    => Controller.PLANE_BUF_CFG,
         Value       => Buffer_Range (Controller.Pipe));
      Registers.Write
        (Register    => Controller.PLANE_WM (0),
         Value       => PLANE_WM_ENABLE or
                        PLANE_WM_LINES (2) or
                        PLANE_WM_BLOCKS (160));
   end Setup_Watermarks;

   ----------------------------------------------------------------------------

   procedure Setup_Hires_Plane
     (Controller  : Controller_Type;
      Framebuffer : HW.GFX.Framebuffer_Type)
   with
      Global => (In_Out => Registers.Register_State),
      Depends =>
        (Registers.Register_State
            =>+
              (Registers.Register_State,
               Controller,
               Framebuffer))
   is
      -- FIXME: setup correct format, based on framebuffer RGB format
      Format : constant Word32 := 6 * 2 ** 26;
      PRI : Word32 := DSPCNTR_ENABLE or Format;

      function To_Bytes (Pixels : Width_Type) return Word32
      with
         Pre => (Word32 (Pixels) <= Word32'Last / 4 / Word32 (BPC_Type'Last) * 8)
      is
      begin
         return Word32 (Pos64 (Pixels) * 4 * Framebuffer.BPC / 8);
      end To_Bytes;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Config.Has_Plane_Control then
         Registers.Write
           (Register    => Controller.PLANE_CTL,
            Value       => PLANE_CTL_PLANE_ENABLE or
                           PLANE_CTL_SRC_PIX_FMT_RGB_32B_8888 or
                           PLANE_CTL_PLANE_GAMMA_DISABLE);
         Registers.Write (Controller.PLANE_OFFSET, 16#0000_0000#);
         Registers.Write
           (Controller.PLANE_SIZE,
            Encode (Pos16 (Framebuffer.Width), Pos16 (Framebuffer.Height)));
         Registers.Write (Controller.PLANE_STRIDE, To_Bytes (Framebuffer.Stride) / 64);
         Registers.Write (Controller.PLANE_POS,    16#0000_0000#);
         Registers.Write (Controller.PLANE_SURF,   Framebuffer.Offset and 16#ffff_f000#);
      else
         if Config.Disable_Trickle_Feed then
            PRI := PRI or DSPCNTR_DISABLE_TRICKLE_FEED;
         end if;
         -- for now, just disable gamma LUT (can't do anything
         -- useful without colorimetry information from display)
         Registers.Unset_And_Set_Mask
            (Register   => Controller.DSPCNTR,
             Mask_Unset => DSPCNTR_MASK,
             Mask_Set   => PRI);

         Registers.Write (Controller.DSPSTRIDE, To_Bytes (Framebuffer.Stride));
         Registers.Write (Controller.DSPSURF, Framebuffer.Offset and 16#ffff_f000#);
         if Config.Has_DSP_Linoff then
            Registers.Write (Controller.DSPLINOFF, 0);
         end if;
         Registers.Write (Controller.DSPTILEOFF, 0);
      end if;
   end Setup_Hires_Plane;

   procedure Setup_Display
     (Controller  : Controller_Type;
      Framebuffer : Framebuffer_Type;
      Dither_BPC  : BPC_Type;
      Dither      : Boolean)
   with
      Global => (In_Out => (Registers.Register_State, Port_IO.State)),
      Depends =>
        (Registers.Register_State
            =>+
              (Registers.Register_State,
               Controller,
               Framebuffer,
               Dither_BPC,
               Dither),
         Port_IO.State
            =>+
               (Framebuffer))
   is
      use type Word8;

      Reg8 : Word8;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Config.Has_Plane_Control then
         Setup_Watermarks (Controller);
      end if;

      if Framebuffer.Offset = VGA_PLANE_FRAMEBUFFER_OFFSET then
         if Config.VGA_Plane_Workaround then
            Registers.Unset_And_Set_Mask
              (Register    => Registers.ILK_DISPLAY_CHICKEN1,
               Mask_Unset  => ILK_DISPLAY_CHICKEN1_VGA_MASK,
               Mask_Set    => ILK_DISPLAY_CHICKEN1_VGA_ENABLE);
            Registers.Unset_And_Set_Mask
              (Register    => Registers.ILK_DISPLAY_CHICKEN2,
               Mask_Unset  => ILK_DISPLAY_CHICKEN2_VGA_MASK,
               Mask_Set    => ILK_DISPLAY_CHICKEN2_VGA_ENABLE);
         end if;

         Registers.Unset_And_Set_Mask
           (Register    => Registers.VGACNTRL,
            Mask_Unset  => VGA_CONTROL_VGA_DISPLAY_DISABLE or
                           VGA_CONTROL_BLINK_DUTY_CYCLE_MASK or
                           VGA_CONTROL_VSYNC_BLINK_RATE_MASK,
            Mask_Set    => VGA_CONTROL_BLINK_DUTY_CYCLE_50 or
                           VGA_CONTROL_VSYNC_BLINK_RATE (30));

         Port_IO.OutB (VGA_SR_INDEX, VGA_SR01);
         Port_IO.InB  (Reg8, VGA_SR_DATA);
         Port_IO.OutB (VGA_SR_DATA, Reg8 and not (VGA_SR01_SCREEN_OFF));
      else
         Setup_Hires_Plane (Controller, Framebuffer);
      end if;

      Registers.Write
        (Register => Controller.PIPESRC,
         Value    => Encode
           (Pos16 (Framebuffer.Height), Pos16 (Framebuffer.Width)));

      if Config.Has_Pipeconf_Misc then
         Registers.Write
           (Register => Controller.PIPEMISC,
            Value    => Transcoder.BPC_Conf (Dither_BPC, Dither));
      end if;
   end Setup_Display;

   ----------------------------------------------------------------------------

   procedure Scale_Keep_Aspect
     (Width       :    out Pos32;
      Height      :    out Pos32;
      Max_Width   : in     Pos32;
      Max_Height  : in     Pos32;
      Framebuffer : in     Framebuffer_Type)
   with
      Pre =>
         Max_Width <= Pos32 (Pos16'Last) and
         Max_Height <= Pos32 (Pos16'Last) and
         Framebuffer.Width <= Max_Width and
         Framebuffer.Height <= Max_Height,
      Post =>
         Width <= Max_Width and Height <= Max_Height
   is
   begin
      if (Max_Width * Framebuffer.Height) / Framebuffer.Width <= Max_Height then
         Width  := Max_Width;
         Height := (Max_Width * Framebuffer.Height) / Framebuffer.Width;
      else
         Height := Max_Height;
         Width  := Pos32'Min (Max_Width,  -- could prove, it's <= Max_Width
            (Max_Height * Framebuffer.Width) / Framebuffer.Height);
      end if;
   end Scale_Keep_Aspect;

   procedure Setup_Skylake_Pipe_Scaler
     (Controller  : in     Controller_Type;
      Mode        : in     HW.GFX.Mode_Type;
      Framebuffer : in     HW.GFX.Framebuffer_Type)
   with
      Pre =>
         Framebuffer.Width <= Pos32 (Mode.H_Visible) and
         Framebuffer.Height <= Pos32 (Mode.V_Visible)
   is
      use type Registers.Registers_Invalid_Index;

      -- Enable 7x5 extended mode where possible:
      Scaler_Mode : constant Word32 :=
        (if Controller.PS_CTRL_2 /= Registers.Invalid_Register then
            PS_CTRL_SCALER_MODE_7X5_EXTENDED else 0);

      -- We can scale up to 2.99x horizontally:
      Horizontal_Limit : constant Pos32 := ((Framebuffer.Width * 299) / 100);
      -- The third scaler is limited to 1.99x
      -- vertical scaling for source widths > 2048:
      Vertical_Limit : constant Pos32 :=
        (Framebuffer.Height *
           (if Controller.PS_CTRL_2 = Registers.Invalid_Register and
               Framebuffer.Width > 2048
            then
               199
            else
               299)) / 100;

      Width, Height : Pos32;
   begin
      -- Writes to WIN_SZ arm the PS registers.

      Scale_Keep_Aspect
        (Width       => Width,
         Height      => Height,
         Max_Width   => Pos32'Min (Horizontal_Limit, Pos32 (Mode.H_Visible)),
         Max_Height  => Pos32'Min (Vertical_Limit, Pos32 (Mode.V_Visible)),
         Framebuffer => Framebuffer);

      Registers.Write
        (Register => Controller.PS_CTRL_1,
         Value    => PS_CTRL_ENABLE_SCALER or Scaler_Mode);
      Registers.Write
        (Register => Controller.PS_WIN_POS_1,
         Value    =>
            Shift_Left (Word32 (Pos32 (Mode.H_Visible) - Width) / 2, 16) or
            Word32 (Pos32 (Mode.V_Visible) - Height) / 2);
      Registers.Write
        (Register => Controller.PS_WIN_SZ_1,
         Value    => Shift_Left (Word32 (Width), 16) or Word32 (Height));
   end Setup_Skylake_Pipe_Scaler;

   procedure Setup_Ironlake_Panel_Fitter
     (Controller  : in     Controller_Type;
      Mode        : in     HW.GFX.Mode_Type;
      Framebuffer : in     HW.GFX.Framebuffer_Type)
   with
      Pre =>
         Framebuffer.Width <= Pos32 (Mode.H_Visible) and
         Framebuffer.Height <= Pos32 (Mode.V_Visible)
   is
      -- Force 1:1 mapping of panel fitter:pipe
      PF_Ctrl_Pipe_Sel : constant Word32 :=
        (if Config.Has_PF_Pipe_Select then
           (case Controller.PF_CTRL is
               when Registers.PFA_CTL_1 => 0 * 2 ** 29,
               when Registers.PFB_CTL_1 => 1 * 2 ** 29,
               when Registers.PFC_CTL_1 => 2 * 2 ** 29,
               when others              => 0) else 0);

      Width, Height : Pos32;
   begin
      -- Writes to WIN_SZ arm the PF registers.

      Scale_Keep_Aspect
        (Width       => Width,
         Height      => Height,
         Max_Width   => Pos32 (Mode.H_Visible),
         Max_Height  => Pos32 (Mode.V_Visible),
         Framebuffer => Framebuffer);

      Registers.Write
        (Register => Controller.PF_CTRL,
         Value    => PF_CTRL_ENABLE or PF_Ctrl_Pipe_Sel or PF_CTRL_FILTER_MED);
      Registers.Write
        (Register => Controller.PF_WIN_POS,
         Value    =>
            Shift_Left (Word32 (Pos32 (Mode.H_Visible) - Width) / 2, 16) or
            Word32 (Pos32 (Mode.V_Visible) - Height) / 2);
      Registers.Write
        (Register => Controller.PF_WIN_SZ,
         Value    => Shift_Left (Word32 (Width), 16) or Word32 (Height));
   end Setup_Ironlake_Panel_Fitter;

   procedure Setup_Scaling
     (Controller  : in     Controller_Type;
      Mode        : in     HW.GFX.Mode_Type;
      Framebuffer : in     HW.GFX.Framebuffer_Type)
   with
      Pre =>
         Framebuffer.Width <= Pos32 (Mode.H_Visible) and
         Framebuffer.Height <= Pos32 (Mode.V_Visible)
   is
   begin
      if Framebuffer.Width /= Pos32 (Mode.H_Visible) or
         Framebuffer.Height /= Pos32 (Mode.V_Visible)
      then
         if Config.Has_Plane_Control then
            Setup_Skylake_Pipe_Scaler (Controller, Mode, Framebuffer);
         else
            Setup_Ironlake_Panel_Fitter (Controller, Mode, Framebuffer);
         end if;
      end if;
   end Setup_Scaling;

   ----------------------------------------------------------------------------

   procedure On
     (Pipe        : Pipe_Index;
      Port_Cfg    : Port_Config;
      Framebuffer : Framebuffer_Type)
   is
      -- Enable dithering if framebuffer BPC differs from port BPC,
      -- as smooth gradients look really bad without.
      Dither : constant Boolean := Framebuffer.BPC /= Port_Cfg.Mode.BPC;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Transcoder.Setup (Pipe, Port_Cfg);

      Setup_Display
        (Controllers (Pipe), Framebuffer, Port_Cfg.Mode.BPC, Dither);

      Setup_Scaling (Controllers (Pipe), Port_Cfg.Mode, Framebuffer);

      Transcoder.On (Pipe, Port_Cfg, Dither);
   end On;

   ----------------------------------------------------------------------------

   procedure Planes_Off (Controller : Controller_Type) is
   begin
      Registers.Unset_Mask (Controller.SPCNTR, DSPCNTR_ENABLE);
      if Config.Has_Plane_Control then
         Clear_Watermarks (Controller);
         Registers.Unset_Mask (Controller.PLANE_CTL, PLANE_CTL_PLANE_ENABLE);
         Registers.Write (Controller.PLANE_SURF, 16#0000_0000#);
      else
         Registers.Unset_Mask (Controller.DSPCNTR, DSPCNTR_ENABLE);
      end if;
   end Planes_Off;

   procedure Panel_Fitter_Off (Controller : Controller_Type)
   is
      use type HW.GFX.GMA.Registers.Registers_Invalid_Index;
   begin
      -- Writes to WIN_SZ arm the PS/PF registers.
      if Config.Has_Plane_Control then
         Registers.Unset_Mask (Controller.PS_CTRL_1, PS_CTRL_ENABLE_SCALER);
         Registers.Write (Controller.PS_WIN_SZ_1, 16#0000_0000#);
         if Controller.PS_CTRL_2 /= Registers.Invalid_Register and
            Controller.PS_WIN_SZ_2 /= Registers.Invalid_Register
         then
            Registers.Unset_Mask (Controller.PS_CTRL_2, PS_CTRL_ENABLE_SCALER);
            Registers.Write (Controller.PS_WIN_SZ_2, 16#0000_0000#);
         end if;
      else
         Registers.Unset_Mask (Controller.PF_CTRL, PF_CTRL_ENABLE);
         Registers.Write (Controller.PF_WIN_SZ, 16#0000_0000#);
      end if;
   end Panel_Fitter_Off;

   procedure Off (Pipe : Pipe_Index)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Planes_Off (Controllers (Pipe));
      Transcoder.Off (Pipe);
      Panel_Fitter_Off (Controllers (Pipe));
      Transcoder.Clk_Off (Pipe);
   end Off;

   procedure Legacy_VGA_Off
   is
      use type HW.Word8;
      Reg8 : Word8;
   begin
      Port_IO.OutB (VGA_SR_INDEX, VGA_SR01);
      Port_IO.InB  (Reg8, VGA_SR_DATA);
      Port_IO.OutB (VGA_SR_DATA, Reg8 or VGA_SR01_SCREEN_OFF);
      Time.U_Delay (100); -- PRM says 100us, Linux does 300
      Registers.Set_Mask (Registers.VGACNTRL, VGA_CONTROL_VGA_DISPLAY_DISABLE);
   end Legacy_VGA_Off;

   procedure All_Off
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Legacy_VGA_Off;

      for Pipe in Pipe_Index loop
         Planes_Off (Controllers (Pipe));
         Transcoder.Off (Pipe);
         Panel_Fitter_Off (Controllers (Pipe));
         Transcoder.Clk_Off (Pipe);
      end loop;
   end All_Off;

   ----------------------------------------------------------------------------

   procedure Update_Offset (Pipe : Pipe_Index; Framebuffer : Framebuffer_Type)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      Registers.Write
        (Controllers (Pipe).DSPSURF, Framebuffer.Offset and 16#ffff_f000#);
   end Update_Offset;

   ----------------------------------------------------------------------------

   function Get_Pipe_Hint (Pipe : Pipe_Index) return Word32
   is
   begin
      return
        (case Pipe is
            when Primary      => 0,
            when Secondary    => 1,
            when Tertiary     => 2);
   end Get_Pipe_Hint;

end HW.GFX.GMA.Pipe_Setup;
