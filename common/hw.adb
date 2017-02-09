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

package body HW is

   function Div_Round_Closest (N, M : Pos8) return Int8 is
   begin
      return (N + M / 2) / M;
   end Div_Round_Closest;

   function Div_Round_Closest (N, M : Pos16) return Int16 is
   begin
      return (N + M / 2) / M;
   end Div_Round_Closest;

   function Div_Round_Closest (N, M : Pos32) return Int32 is
   begin
      return (N + M / 2) / M;
   end Div_Round_Closest;

   function Div_Round_Closest (N, M : Pos64) return Int64 is
   begin
      return (N + M / 2) / M;
   end Div_Round_Closest;

end HW;
