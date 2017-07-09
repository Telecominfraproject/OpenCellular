--
-- Copyright (C) 2017 Nico Huber <nico.h@gmx.de>
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

with Interfaces.C;
with Interfaces.C.Strings;

with HW.Debug;

use Interfaces.C;
use Interfaces.C.Strings;

package body HW.File is

   READ  : constant := 16#01#;
   WRITE : constant := 16#02#;

   function c_map
     (addr  : out Word64;
      path  : chars_ptr;
      len   : Word32;
      off   : Word32;
      mode  : Word32;
      copy  : int)
      return int;
   pragma Import (C, c_map, "hw_file_map");

   procedure Map
     (Addr     :    out Word64;
      Path     : in     String;
      Len      : in     Natural := 0;
      Offset   : in     Natural := 0;
      Readable : in     Boolean := False;
      Writable : in     Boolean := False;
      Map_Copy : in     Boolean := False;
      Success  :    out Boolean)
   is
      use type HW.Word32;

      cpath : chars_ptr := New_String (Path);
      ret : constant int := c_map
        (addr  => Addr,
         path  => cpath,
         len   => Word32 (Len),
         off   => Word32 (Offset),
         mode  => (if Readable then READ else 0) or
                  (if Writable then WRITE else 0),
         copy  => (if Map_Copy then 1 else 0));
   begin
      pragma Warnings(GNAT, Off, """cpath"" modified*, but* referenced",
                      Reason => "Free() demands to set it to null_ptr");
      Free (cpath);
      pragma Warnings(GNAT, On, """cpath"" modified*, but* referenced");
      Success := ret = 0;

      pragma Debug (not Success, Debug.Put ("Mapping failed: "));
      pragma Debug (not Success, Debug.Put_Int32 (Int32 (ret)));
      pragma Debug (not Success, Debug.New_Line);
   end Map;

end HW.File;
