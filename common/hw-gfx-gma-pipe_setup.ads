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

with HW.GFX.GMA.Registers;

use type HW.Int32;

private package HW.GFX.GMA.Pipe_Setup
is

   procedure On
     (Pipe        : Pipe_Index;
      Port_Cfg    : Port_Config;
      Framebuffer : Framebuffer_Type)
   with
      Pre =>
         Framebuffer.Width <= Pos32 (Port_Cfg.Mode.H_Visible) and
         Framebuffer.Height <= Pos32 (Port_Cfg.Mode.V_Visible);

   procedure Off (Pipe : Pipe_Index);

   procedure Legacy_VGA_Off;

   procedure All_Off;

   function Get_Pipe_Hint (Pipe : Pipe_Index) return Word32;

   procedure Update_Offset (Pipe : Pipe_Index; Framebuffer : Framebuffer_Type);

private

   subtype WM_Levels is Natural range 0 .. 7;
   type PLANE_WM_Type is array (WM_Levels) of Registers.Registers_Index;

   type Controller_Type is
      record
         Pipe              : Pipe_Index;
         PIPESRC           : Registers.Registers_Index;
         PIPEMISC          : Registers.Registers_Index;
         PF_CTRL           : Registers.Registers_Index;
         PF_WIN_POS        : Registers.Registers_Index;
         PF_WIN_SZ         : Registers.Registers_Index;
         DSPCNTR           : Registers.Registers_Index;
         DSPLINOFF         : Registers.Registers_Index;
         DSPSTRIDE         : Registers.Registers_Index;
         DSPSURF           : Registers.Registers_Index;
         DSPTILEOFF        : Registers.Registers_Index;
         SPCNTR            : Registers.Registers_Index;
         -- Skylake registers (partially aliased)
         PLANE_CTL         : Registers.Registers_Index;
         PLANE_OFFSET      : Registers.Registers_Index;
         PLANE_POS         : Registers.Registers_Index;
         PLANE_SIZE        : Registers.Registers_Index;
         PLANE_STRIDE      : Registers.Registers_Index;
         PLANE_SURF        : Registers.Registers_Index;
         PS_CTRL_1         : Registers.Registers_Index;
         PS_WIN_POS_1      : Registers.Registers_Index;
         PS_WIN_SZ_1       : Registers.Registers_Index;
         PS_CTRL_2         : Registers.Registers_Invalid_Index;
         PS_WIN_SZ_2       : Registers.Registers_Invalid_Index;
         WM_LINETIME       : Registers.Registers_Index;
         PLANE_BUF_CFG     : Registers.Registers_Index;
         PLANE_WM          : PLANE_WM_Type;
      end record;

   type Controller_Array is array (Pipe_Index) of Controller_Type;

   Controllers : constant Controller_Array :=
     (Primary => Controller_Type'
        (Pipe              => Primary,
         PIPESRC           => Registers.PIPEASRC,
         PIPEMISC          => Registers.PIPEAMISC,
         PF_CTRL           => Registers.PFA_CTL_1,
         PF_WIN_POS        => Registers.PFA_WIN_POS,
         PF_WIN_SZ         => Registers.PFA_WIN_SZ,
         DSPCNTR           => Registers.DSPACNTR,
         DSPLINOFF         => Registers.DSPALINOFF,
         DSPSTRIDE         => Registers.DSPASTRIDE,
         DSPSURF           => Registers.DSPASURF,
         DSPTILEOFF        => Registers.DSPATILEOFF,
         SPCNTR            => Registers.SPACNTR,
         PLANE_CTL         => Registers.DSPACNTR,
         PLANE_OFFSET      => Registers.DSPATILEOFF,
         PLANE_POS         => Registers.PLANE_POS_1_A,
         PLANE_SIZE        => Registers.PLANE_SIZE_1_A,
         PLANE_STRIDE      => Registers.DSPASTRIDE,
         PLANE_SURF        => Registers.DSPASURF,
         PS_CTRL_1         => Registers.PS_CTRL_1_A,
         PS_WIN_POS_1      => Registers.PS_WIN_POS_1_A,
         PS_WIN_SZ_1       => Registers.PS_WIN_SZ_1_A,
         PS_CTRL_2         => Registers.PS_CTRL_2_A,
         PS_WIN_SZ_2       => Registers.PS_WIN_SZ_2_A,
         WM_LINETIME       => Registers.WM_LINETIME_A,
         PLANE_BUF_CFG     => Registers.PLANE_BUF_CFG_1_A,
         PLANE_WM          => PLANE_WM_Type'(
                              Registers.PLANE_WM_1_A_0,
                              Registers.PLANE_WM_1_A_1,
                              Registers.PLANE_WM_1_A_2,
                              Registers.PLANE_WM_1_A_3,
                              Registers.PLANE_WM_1_A_4,
                              Registers.PLANE_WM_1_A_5,
                              Registers.PLANE_WM_1_A_6,
                              Registers.PLANE_WM_1_A_7)),
      Secondary => Controller_Type'
        (Pipe              => Secondary,
         PIPESRC           => Registers.PIPEBSRC,
         PIPEMISC          => Registers.PIPEBMISC,
         PF_CTRL           => Registers.PFB_CTL_1,
         PF_WIN_POS        => Registers.PFB_WIN_POS,
         PF_WIN_SZ         => Registers.PFB_WIN_SZ,
         DSPCNTR           => Registers.DSPBCNTR,
         DSPLINOFF         => Registers.DSPBLINOFF,
         DSPSTRIDE         => Registers.DSPBSTRIDE,
         DSPSURF           => Registers.DSPBSURF,
         DSPTILEOFF        => Registers.DSPBTILEOFF,
         SPCNTR            => Registers.SPBCNTR,
         PLANE_CTL         => Registers.DSPBCNTR,
         PLANE_OFFSET      => Registers.DSPBTILEOFF,
         PLANE_POS         => Registers.PLANE_POS_1_B,
         PLANE_SIZE        => Registers.PLANE_SIZE_1_B,
         PLANE_STRIDE      => Registers.DSPBSTRIDE,
         PLANE_SURF        => Registers.DSPBSURF,
         PS_CTRL_1         => Registers.PS_CTRL_1_B,
         PS_WIN_POS_1      => Registers.PS_WIN_POS_1_B,
         PS_WIN_SZ_1       => Registers.PS_WIN_SZ_1_B,
         PS_CTRL_2         => Registers.PS_CTRL_2_B,
         PS_WIN_SZ_2       => Registers.PS_WIN_SZ_2_B,
         WM_LINETIME       => Registers.WM_LINETIME_B,
         PLANE_BUF_CFG     => Registers.PLANE_BUF_CFG_1_B,
         PLANE_WM          => PLANE_WM_Type'(
                              Registers.PLANE_WM_1_B_0,
                              Registers.PLANE_WM_1_B_1,
                              Registers.PLANE_WM_1_B_2,
                              Registers.PLANE_WM_1_B_3,
                              Registers.PLANE_WM_1_B_4,
                              Registers.PLANE_WM_1_B_5,
                              Registers.PLANE_WM_1_B_6,
                              Registers.PLANE_WM_1_B_7)),
      Tertiary => Controller_Type'
        (Pipe              => Tertiary,
         PIPESRC           => Registers.PIPECSRC,
         PIPEMISC          => Registers.PIPECMISC,
         PF_CTRL           => Registers.PFC_CTL_1,
         PF_WIN_POS        => Registers.PFC_WIN_POS,
         PF_WIN_SZ         => Registers.PFC_WIN_SZ,
         DSPCNTR           => Registers.DSPCCNTR,
         DSPLINOFF         => Registers.DSPCLINOFF,
         DSPSTRIDE         => Registers.DSPCSTRIDE,
         DSPSURF           => Registers.DSPCSURF,
         DSPTILEOFF        => Registers.DSPCTILEOFF,
         SPCNTR            => Registers.SPCCNTR,
         PLANE_CTL         => Registers.DSPCCNTR,
         PLANE_OFFSET      => Registers.DSPCTILEOFF,
         PLANE_POS         => Registers.PLANE_POS_1_C,
         PLANE_SIZE        => Registers.PLANE_SIZE_1_C,
         PLANE_STRIDE      => Registers.DSPCSTRIDE,
         PLANE_SURF        => Registers.DSPCSURF,
         PS_CTRL_1         => Registers.PS_CTRL_1_C,
         PS_WIN_POS_1      => Registers.PS_WIN_POS_1_C,
         PS_WIN_SZ_1       => Registers.PS_WIN_SZ_1_C,
         PS_CTRL_2         => Registers.Invalid_Register,
         PS_WIN_SZ_2       => Registers.Invalid_Register,
         WM_LINETIME       => Registers.WM_LINETIME_C,
         PLANE_BUF_CFG     => Registers.PLANE_BUF_CFG_1_C,
         PLANE_WM          => PLANE_WM_Type'(
                              Registers.PLANE_WM_1_C_0,
                              Registers.PLANE_WM_1_C_1,
                              Registers.PLANE_WM_1_C_2,
                              Registers.PLANE_WM_1_C_3,
                              Registers.PLANE_WM_1_C_4,
                              Registers.PLANE_WM_1_C_5,
                              Registers.PLANE_WM_1_C_6,
                              Registers.PLANE_WM_1_C_7)));

end HW.GFX.GMA.Pipe_Setup;
