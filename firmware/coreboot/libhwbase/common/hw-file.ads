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

package HW.File is

   --
   -- Map a file's content into our address space
   --
   -- If `Map_Copy` is `False`, `Len` bytes at `Offset` from the start
   -- of the file given by `Path` should be mapped into the application's
   -- address space using mmap().
   --
   -- If `Map_Copy` is `True`, anonymous memory should be mapped instead
   -- and be filled with a copy of the file's content using read().
   --
   -- If `Len` is zero, the whole file should be mapped.
   --
   procedure Map
     (Addr     :    out Word64;
      Path     : in     String;
      Len      : in     Natural := 0;
      Offset   : in     Natural := 0;
      Readable : in     Boolean := False;
      Writable : in     Boolean := False;
      Map_Copy : in     Boolean := False;
      Success  :    out Boolean)
   with
      Pre => (Readable or Writable) and
             (if Map_Copy then Readable and not Writable);

   -- Sets `Length` to the size of the file given by `Path` or 0 if an
   -- error occurs.
   procedure Size (Length : out Natural; Path : String);

end HW.File;
