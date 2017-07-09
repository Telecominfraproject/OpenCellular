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

with Ada.Text_IO;
with Ada.Strings.Fixed;

with HW.File;
with HW.PCI.MMConf;

with HW.MMIO_Range;
pragma Elaborate_All (HW.MMIO_Range);

use Ada.Strings.Fixed;

package body HW.PCI.Dev
with
   Refined_State =>
     (Address_State  => MM.Address_State,
      PCI_State      => MM.PCI_State)
is

   -- We map each device's config space individually, hence Address'(0, 0, 0).
   package MM is new HW.PCI.MMConf (Address'(0, 0, 0));

   procedure Read8 (Value : out Word8; Offset : Index) renames MM.Read8;
   procedure Read16 (Value : out Word16; Offset : Index) renames MM.Read16;
   procedure Read32 (Value : out Word32; Offset : Index) renames MM.Read32;

   procedure Write8 (Offset : Index; Value : Word8) renames MM.Write8;
   procedure Write16 (Offset : Index; Value : Word16) renames MM.Write16;
   procedure Write32 (Offset : Index; Value : Word32) renames MM.Write32;

   function Hex (Val : Natural) return Character
   with
      Pre => Val < 16
   is
   begin
      if Val < 10 then
         return Character'Val (Character'Pos ('0') + Val);
      else
         return Character'Val (Character'Pos ('a') + Val - 10);
      end if;
   end Hex;

   subtype String2 is String (1 .. 2);
   function Hex2 (Val : Natural) return String2
   with
      Pre => Val < 256
   is
      Res : constant String (1 .. 2) := (Hex (Val / 16), Hex (Val mod 16));
   begin
      return Res;
   end Hex2;

   procedure Patch_Sysfs_Path (Path : in out String)
   with
      Pre => Path'Length >= 36
   is
   begin
      Path (Path'First + 21 .. Path'First + 22) := Hex2 (Natural (Dev.Bus));
      Path (Path'First + 29 .. Path'First + 30) := Hex2 (Natural (Dev.Bus));
      Path (Path'First + 32 .. Path'First + 33) := Hex2 (Natural (Dev.Slot));
      Path (Path'First + 35) := Hex (Natural (Dev.Func));
   end Patch_Sysfs_Path;

   procedure Map
     (Addr     :    out Word64;
      Res      : in     Resource;
      Length   : in     Natural := 0;
      Offset   : in     Natural := 0;
      WC       : in     Boolean := False)
   is
      Success  : Boolean;
      Path     : String (1 .. 49) :=
         "/sys/devices/pci0000:xx/0000:xx:xx.x/resourcex_wc";
   begin
      Patch_Sysfs_Path (Path);
      Path (46) := Character'Val (Character'Pos ('0') + Resource'Pos (Res));
      if not WC then
         Path (47) := Character'Val (0);
      end if;

      File.Map
        (Addr     => Addr,
         Path     => Path,
         Len      => Length,
         Offset   => Offset,
         Readable => True,
         Writable => True,
         Success  => Success);

      if not Success and WC then -- try again without write-combining
         Path (47) := Character'Val (0);

         File.Map
           (Addr     => Addr,
            Path     => Path,
            Len      => Length,
            Offset   => Offset,
            Readable => True,
            Writable => True,
            Success  => Success);
      end if;

      if not Success then
         Addr := 0;
      end if;
   end Map;

   procedure Resource_Size (Length : out Natural; Res : Resource)
   is
      Path : String (1 .. 46) :=
         "/sys/devices/pci0000:xx/0000:xx:xx.x/resourcex";
   begin
      Patch_Sysfs_Path (Path);
      Path (46) := Character'Val (Character'Pos ('0') + Resource'Pos (Res));

      File.Size (Length, Path);
   end Resource_Size;

   procedure Initialize (Success : out Boolean; MMConf_Base : Word64 := 0)
   is
      Addr  : Word64;
      Path  : String (1 .. 43) := "/sys/devices/pci0000:xx/0000:xx:xx.x/config";
   begin
      Patch_Sysfs_Path (Path);

      File.Map
        (Addr     => Addr,
         Path     => Path,
         Readable => True,
         Map_Copy => True,
         Success  => Success);
      MM.Set_Base_Address (Addr);
   end Initialize;

end HW.PCI.Dev;
