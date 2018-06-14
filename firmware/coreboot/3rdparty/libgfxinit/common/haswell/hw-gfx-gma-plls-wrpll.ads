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

private package HW.GFX.GMA.PLLs.WRPLL
is

   type Value_Array is array (WRPLLs) of Word32;
   Register_Value : constant Value_Array := Value_Array'
     (WRPLL0 => 4 * 2 ** 29, WRPLL1 => 5 * 2 ** 29);

   procedure On
     (PLL            : in     WRPLLs;
      Target_Clock   : in     Frequency_Type;
      Success        :    out Boolean);

   procedure Off (PLL : WRPLLs);

end HW.GFX.GMA.PLLs.WRPLL;
