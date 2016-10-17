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

with HW.Debug;
with GNAT.Source_Info;
with System.Storage_Elements;
with System.Address_To_Access_Conversions;

package body HW.MMIO_Range
with
   Refined_State =>
     (State => Range_A,       -- the contents accessed
      Base_Address => null)   -- the address, so actually Range_A too
is
   pragma Warnings (Off, "implicit dereference",
                    Reason => "This is what this package is about.");

   Debug_Reads    : constant Boolean := False;
   Debug_Writes   : constant Boolean := False;

   type Range_Access is access all Array_T;
   package Conv_Range is new System.Address_To_Access_Conversions (Array_T);

   Range_A : Range_Access :=
      Range_Access (Conv_Range.To_Pointer (System'To_Address (Base_Addr)))
   with Volatile;

   procedure Read (Value : out Element_T; Index : in Index_T)
   is
      use type Word32;
   begin
      Value := Range_A (Index);
      pragma Debug (Debug_Reads, Debug.Put
        (GNAT.Source_Info.Enclosing_Entity & ":  "));
      pragma Debug (Debug_Reads, Debug.Put_Word32 (Word32 (Value)));
      pragma Debug (Debug_Reads, Debug.Put (" <- "));
      pragma Debug (Debug_Reads, Debug.Put_Word32
        (Word32 (System.Storage_Elements.To_Integer
           (Conv_Range.To_Address (Conv_Range.Object_Pointer (Range_A)))) +
         Word32 (Index) * (Element_T'Size / 8)));
      pragma Debug (Debug_Reads, Debug.New_Line);
   end Read;

   procedure Write (Index : in Index_T; Value : in Element_T)
   is
      use type Word32;
   begin
      pragma Debug (Debug_Writes, Debug.Put
        (GNAT.Source_Info.Enclosing_Entity & ": "));
      pragma Debug (Debug_Writes, Debug.Put_Word32 (Word32 (Value)));
      pragma Debug (Debug_Writes, Debug.Put (" -> "));
      pragma Debug (Debug_Writes, Debug.Put_Word32
        (Word32 (System.Storage_Elements.To_Integer
           (Conv_Range.To_Address (Conv_Range.Object_Pointer (Range_A)))) +
         Word32 (Index) * (Element_T'Size / 8)));
      pragma Debug (Debug_Writes, Debug.New_Line);
      Range_A (Index) := Value;
   end Write;

   procedure Set_Base_Address (Base : Word64) is
   begin
      Range_A := Range_Access
        (Conv_Range.To_Pointer (System'To_Address (Base)));
   end Set_Base_Address;

end HW.MMIO_Range;
