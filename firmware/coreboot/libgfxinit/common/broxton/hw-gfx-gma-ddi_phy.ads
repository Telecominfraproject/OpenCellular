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

with HW.GFX.GMA.DP_Info;

private package HW.GFX.GMA.DDI_Phy is

   type T is (BC, A);

   procedure Power_On (Phy : T);
   procedure Power_Off (Phy : T);

   subtype DDI_Phy_Port is GPU_Port range DIGI_A .. DIGI_C;

   procedure Pre_PLL (Port_Cfg : Port_Config);

   Max_V_Swing : constant DP_Info.DP_Voltage_Swing := DP_Info.VS_Level_3;

   type Emph_Array is array (DP_Info.DP_Voltage_Swing) of DP_Info.DP_Pre_Emph;
   Max_Pre_Emph : constant Emph_Array :=
     (DP_Info.VS_Level_0   => DP_Info.Emph_Level_3,
      DP_Info.VS_Level_1   => DP_Info.Emph_Level_2,
      DP_Info.VS_Level_2   => DP_Info.Emph_Level_1,
      others               => DP_Info.Emph_Level_0);

   procedure Set_DP_Signal_Levels
     (Port        : Digital_Port;
      Train_Set   : DP_Info.Train_Set);

   subtype HDMI_Buf_Trans_Range is DDI_HDMI_Buf_Trans_Range range 0 .. 9;
   procedure Set_HDMI_Signal_Levels
     (Port  : DDI_Phy_Port;
      Level : HDMI_Buf_Trans_Range);

end HW.GFX.GMA.DDI_Phy;
