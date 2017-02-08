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

private package HW.GFX.GMA.DDI_Phy is

   type T is (BC, A);

   procedure Power_On (Phy : T);
   procedure Power_Off (Phy : T);

   subtype DDI_Phy_Port is GPU_Port range DIGI_A .. DIGI_C;

   procedure Pre_PLL (Port_Cfg : Port_Config);

end HW.GFX.GMA.DDI_Phy;
