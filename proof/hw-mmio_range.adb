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

package body HW.MMIO_Range
with
   Refined_State =>
     (State => Range_A,
      Base_Address => Address)
is

   Range_A : Array_T
   with
      Volatile,
      Async_Readers, Async_Writers, Effective_Reads, Effective_Writes,
      Import;

   Address : Word64 := Base_Addr;

   procedure Read (Value : out Element_T; Index : in Index_T) is
   begin
      Value := Range_A (Index);
   end Read;

   procedure Write (Index : in Index_T; Value: in Element_T) is
   begin
      Range_A (Index) := Value;
   end Write;

   procedure Set_Base_Address (Base : Word64) is
   begin
      Address := Base;
   end Set_Base_Address;

end HW.MMIO_Range;
