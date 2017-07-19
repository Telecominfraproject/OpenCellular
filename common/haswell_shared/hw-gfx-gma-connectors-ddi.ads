--
-- Copyright (C) 2015-2017 secunet Security Networks AG
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

private package HW.GFX.GMA.Connectors.DDI
is

   procedure Initialize;

   procedure Pre_On
     (Port_Cfg : in     Port_Config;
      PLL_Hint : in     Word32;
      Success  :    out Boolean);

   procedure Post_On (Port_Cfg : Port_Config);

   procedure Off (Port : Digital_Port);

   procedure Post_Reset_Off;

private
   type Buf_Trans_Range is range 0 .. 19;
   type Buf_Trans_Array is array (Buf_Trans_Range) of Word32;

end HW.GFX.GMA.Connectors.DDI;
