--
-- Copyright (C) 2015 secunet Security Networks AG
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

with HW;
with HW.Time;
with HW.Debug_Sink;

use type HW.Word64;
use type HW.Int64;

package body HW.Debug
with
   SPARK_Mode => Off
is

   Start_Of_Line : Boolean := True;
   Register_Write_Delay_Nanoseconds : Word64 := 0;

   type Base_Range is new Positive range 2 .. 16;
   type Width_Range is new Natural range 0 .. 64;

   procedure Put_By_Base
     (Item        : Word64;
      Min_Width   : Width_Range;
      Base        : Base_Range);

   procedure Do_Put_Int64
     (Item        : Int64);

   ----------------------------------------------------------------------------

   procedure Put_Time
   is
      Now_US : Int64;
   begin
      if Start_Of_Line then
         Start_Of_Line := False;
         Now_US := Time.Now_US;
         Debug_Sink.Put_Char ('[');
         Do_Put_Int64 ((Now_US / 1_000_000) mod 1_000_000);
         Debug_Sink.Put_Char ('.');
         Put_By_Base (Word64 (Now_US mod 1_000_000), 6, 10);
         Debug_Sink.Put ("] ");
      end if;
   end Put_Time;

   ----------------------------------------------------------------------------

   procedure Put (Item : String) is
   begin
      Put_Time;
      HW.Debug_Sink.Put (Item);
   end Put;

   procedure Put_Line (Item : String) is
   begin
      Put (Item);
      New_Line;
   end Put_Line;

   procedure New_Line is
   begin
      HW.Debug_Sink.New_Line;
      Start_Of_Line := True;
   end New_Line;

   ----------------------------------------------------------------------------

   procedure Put_By_Base
     (Item        : Word64;
      Min_Width   : Width_Range;
      Base        : Base_Range)
   is
      Temp : Word64 := Item;

      subtype Chars_Range is Width_Range range 0 .. 63;
      Index : Width_Range := 0;

      type Chars_Array is array (Chars_Range) of Character;
      Chars : Chars_Array := (others => '0');

      Digit : Natural;
   begin
      while Temp > 0 loop
         Digit := Natural (Temp rem Word64 (Base));
         if Digit < 10 then
            Chars (Index) := Character'Val (Character'Pos ('0') + Digit);
         else
            Chars (Index) := Character'Val (Character'Pos ('a') + Digit - 10);
         end if;
         Temp := Temp / Word64 (Base);
         Index := Index + 1;
      end loop;
      if Index < Min_Width then
         Index := Min_Width;
      end if;
      for I in reverse Width_Range range 0 .. Index - 1 loop
         HW.Debug_Sink.Put_Char (Chars (I));
      end loop;
   end Put_By_Base;

   ----------------------------------------------------------------------------

   procedure Put_Word
     (Item        : Word64;
      Min_Width   : Width_Range;
      Print_Ox    : Boolean := True) is
   begin
      Put_Time;
      if Print_Ox then
         Put ("0x");
      end if;
      Put_By_Base (Item, Min_Width, 16);
   end Put_Word;

   procedure Put_Word8 (Item : Word8) is
   begin
      Put_Word (Word64 (Item), 2);
   end Put_Word8;

   procedure Put_Word16 (Item : Word16) is
   begin
      Put_Word (Word64 (Item), 4);
   end Put_Word16;

   procedure Put_Word32 (Item : Word32) is
   begin
      Put_Word (Word64 (Item), 8);
   end Put_Word32;

   procedure Put_Word64 (Item : Word64) is
   begin
      Put_Word (Item, 16);
   end Put_Word64;

   ----------------------------------------------------------------------------

   procedure Do_Put_Int64 (Item : Int64)
   is
      Temp : Word64;
   begin
      if Item < 0 then
         Debug_Sink.Put_Char ('-');
         Temp := Word64 (-Item);
      else
         Temp := Word64 (Item);
      end if;
      Put_By_Base (Temp, 1, 10);
   end Do_Put_Int64;

   procedure Put_Int64 (Item : Int64)
   is
   begin
      Put_Time;
      Do_Put_Int64 (Item);
   end Put_Int64;

   procedure Put_Int8 (Item : Int8) is
   begin
      Put_Int64 (Int64 (Item));
   end Put_Int8;

   procedure Put_Int16 (Item : Int16) is
   begin
      Put_Int64 (Int64 (Item));
   end Put_Int16;

   procedure Put_Int32 (Item : Int32) is
   begin
      Put_Int64 (Int64 (Item));
   end Put_Int32;

   ----------------------------------------------------------------------------

   procedure Put_Reg8 (Name : String; Item : Word8) is
   begin
      Put (Name);
      Put (": ");
      Put_Word8 (Item);
      New_Line;
   end Put_Reg8;

   procedure Put_Reg16 (Name : String; Item : Word16)
   is
   begin
      Put (Name);
      Put (": ");
      Put_Word16 (Item);
      New_Line;
   end Put_Reg16;

   procedure Put_Reg32 (Name : String; Item : Word32)
   is
   begin
      Put (Name);
      Put (": ");
      Put_Word32 (Item);
      New_Line;
   end Put_Reg32;

   procedure Put_Reg64 (Name : String; Item : Word64)
   is
   begin
      Put (Name);
      Put (": ");
      Put_Word64 (Item);
      New_Line;
   end Put_Reg64;

   ----------------------------------------------------------------------------

   procedure Put_Buffer
     (Name  : String;
      Buf   : Buffer;
      Len   : Buffer_Range)
   is
      Line_Start, Left : Natural;
   begin
      if Len = 0 then
         if Name'Length > 0 then
            Put (Name);
            Put_Line ("+0x00:");
         end if;
      else
         Line_Start  := 0;
         Left        := Len - 1;
         for I in Natural range 1 .. ((Len + 15) / 16) loop
            if Name'Length > 0 then
               Put (Name);
               Debug_Sink.Put_Char ('+');
               Put_Word16 (Word16 (Line_Start));
               Put (":  ");
            end if;
            for J in Natural range 0 .. Natural'Min (7, Left)
            loop
               Put_Word (Word64 (Buf (Line_Start + J)), 2, False);
               Debug_Sink.Put_Char (' ');
            end loop;

            Debug_Sink.Put_Char (' ');
            for J in Natural range 8 .. Natural'Min (15, Left)
            loop
               Put_Word (Word64(Buf (Line_Start + J)), 2, False);
               Debug_Sink.Put_Char (' ');
            end loop;
            New_Line;

            Line_Start  := Line_Start + 16;
            Left        := Left - Natural'Min (Left, 16);
         end loop;
      end if;
   end Put_Buffer;

   ----------------------------------------------------------------------------

   procedure Set_Register_Write_Delay (Value : Word64)
   is
   begin
      Register_Write_Delay_Nanoseconds := Value;
   end Set_Register_Write_Delay;

   ----------------------------------------------------------------------------

   Procedure Register_Write_Wait
   is
   begin
      if Register_Write_Delay_Nanoseconds > 0 then
         Time.U_Delay (Natural ((Register_Write_Delay_Nanoseconds + 999) / 1000));
      end if;
   end Register_Write_Wait;

end HW.Debug;

--  vim: set ts=8 sts=3 sw=3 et:
