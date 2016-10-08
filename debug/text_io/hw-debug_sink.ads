--
-- Copyright (C) 2015 secunet Security Networks AG
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; version 2 of the License.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--

with Ada.Text_IO;

package HW.Debug_Sink is

   procedure Put (Item : String) renames Ada.Text_IO.Put;
   procedure Put_Char (Item : Character) renames Ada.Text_IO.Put;
   procedure New_Line (Spacing : Ada.Text_IO.Positive_Count := 1)
      renames  Ada.Text_IO.New_Line;

end HW.Debug_Sink;
