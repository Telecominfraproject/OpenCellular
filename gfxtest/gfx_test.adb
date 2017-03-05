with System.Storage_Elements;
with Ada.Command_Line;
with Interfaces.C;

with HW.File;
with HW.Debug;
with HW.GFX.GMA;
with HW.GFX.GMA.Display_Probing;

use HW;
use HW.GFX;

package body GFX_Test is

   MMIO_Size : constant := 2 * 1024 * 1024;
   subtype MMIO_Range is Natural range 0 .. MMIO_Size - 1;
   subtype MMIO_Buffer is Buffer (MMIO_Range);
   MMIO_Dummy : MMIO_Buffer
   with
      Alignment => 16#1000#,
      Volatile;

   type Pixel_Type is record
      Red   : Byte;
      Green : Byte;
      Blue  : Byte;
      Alpha : Byte;
   end record;

   for Pixel_Type use record
      Blue  at 0 range 0 .. 7;
      Green at 1 range 0 .. 7;
      Red   at 2 range 0 .. 7;
      Alpha at 3 range 0 .. 7;
   end record;

   Max_W    : constant := 4096;
   Max_H    : constant := 2160;
   FB_Align : constant := 16#0004_0000#;
   type Screen_Type is
      array (0 .. 3 * (Max_W * Max_H + FB_Align / 4) - 1) of Pixel_Type;

   Screen : Screen_Type
   with
      Alignment => 16#1000#,
      Volatile;

   Pipes : GMA.Pipe_Configs;

   function Fill
     (X, Y        : Natural;
      Framebuffer : Framebuffer_Type;
      Pipe        : GMA.Pipe_Index)
      return Pixel_Type
   is
      use type HW.Byte;

      Xp : constant Natural := X * 256 / Natural (Framebuffer.Width);
      Yp : constant Natural := Y * 256 / Natural (Framebuffer.Height);
      Xn : constant Natural := 255 - Xp;
      Yn : constant Natural := 255 - Yp;

      function Map (X, Y : Natural) return Byte is
      begin
         return Byte (X * Y / 255);
      end Map;
   begin
      return
        (case Pipe is
         when GMA.Primary   => (Map (Xn, Yn), Map (Xp, Yn), Map (Xp, Yp), 255),
         when GMA.Secondary => (Map (Xn, Yp), Map (Xn, Yn), Map (Xp, Yn), 255),
         when GMA.Tertiary  => (Map (Xp, Yp), Map (Xn, Yp), Map (Xn, Yn), 255));
   end Fill;

   procedure Test_Screen
     (Framebuffer : Framebuffer_Type;
      Pipe        : GMA.Pipe_Index)
   is
      use type HW.Word32;
      P        : Pixel_Type;
      -- We have pixel offset wheras the framebuffer has a byte offset
      Offset_Y : Natural := Natural (Framebuffer.Offset / 4);
      Offset   : Natural;
   begin
      for Y in 0 .. Natural (Framebuffer.Height) - 1 loop
         Offset := Offset_Y;
         for X in 0 .. Natural (Framebuffer.Width) - 1 loop
            if Y mod 16 = 0 or X mod 16 = 0 then
               P := (0, 0, 0, 0);
            else
               P := Fill (X, Y, Framebuffer, Pipe);
            end if;
            Screen (Offset) := P;
            Offset := Offset + 1;
         end loop;
         Offset_Y := Offset_Y + Natural (Framebuffer.Stride);
      end loop;
   end Test_Screen;

   procedure Calc_Framebuffer
     (FB       :    out Framebuffer_Type;
      Mode     : in     Mode_Type;
      Offset   : in out Word32)
   is
      use type HW.Int32;
      use type HW.Word32;
   begin
      Offset := (Offset + FB_Align - 1) and not (FB_Align - 1);
      FB :=
        (Width    => Width_Type (Mode.H_Visible),
         Height   => Height_Type (Mode.V_Visible),
         BPC      => 8,
         Stride   => Width_Type ((Word32 (Mode.H_Visible) + 15) and not 15),
         Offset   => Offset);
      Offset := Offset + Word32 (FB.Stride * FB.Height * 4);
   end Calc_Framebuffer;

   procedure Prepare_Configs
   is
      use type HW.GFX.GMA.Port_Type;

      Offset : HW.Word32 := 0;
   begin
      GMA.Display_Probing.Scan_Ports (Pipes);

      for Pipe in GMA.Pipe_Index loop
         if Pipes (Pipe).Port /= GMA.Disabled then
            Calc_Framebuffer
              (FB       => Pipes (Pipe).Framebuffer,
               Mode     => Pipes (Pipe).Mode,
               Offset   => Offset);
         end if;
      end loop;

      GMA.Dump_Configs (Pipes);
   end Prepare_Configs;

   procedure Print_Usage is
   begin
      Debug.Put_Line
        ("Usage: " & Ada.Command_Line.Command_Name & " <sysfs-pci-path>");
      Debug.New_Line;
   end Print_Usage;

   procedure Main
   is
      use System.Storage_Elements;

      use type HW.GFX.GMA.Port_Type;
      use type Interfaces.C.int;

      MMIO_Mapped,
      Screen_Mapped,
      Initialized : Boolean;

      function iopl (level : Interfaces.C.int) return Interfaces.C.int;
      pragma Import (C, iopl, "iopl");
   begin
      if Ada.Command_Line.Argument_Count /= 1 then
         Print_Usage;
         return;
      end if;

      if iopl (3) /= 0 then
         Debug.Put_Line ("Failed to change i/o privilege level.");
         return;
      end if;

      File.Map
        (Path     => Ada.Command_Line.Argument (1) & "/resource0",
         Addr     => Word64 (To_Integer (MMIO_Dummy'Address)),
         Len      => MMIO_Dummy'Size / 8,
         Readable => True,
         Writable => True,
         Success  => MMIO_Mapped);
      if not MMIO_Mapped then
         Debug.Put_Line
           ("Failed to map '" & Ada.Command_Line.Argument (1) & "/resource0'.");
         return;
      end if;

      File.Map
        (Path     => Ada.Command_Line.Argument (1) & "/resource2",
         Addr     => Word64 (To_Integer (Screen'Address)),
         Len      => Screen'Size / 8,
         Readable => True,
         Writable => True,
         Success  => Screen_Mapped);
      if not Screen_Mapped then
         Debug.Put_Line
           ("Failed to map '" & Ada.Command_Line.Argument (1) & "/resource2'.");
         return;
      end if;

      GMA.Initialize
        (MMIO_Base   => Word64 (To_Integer (MMIO_Dummy'Address)),
         Clean_State => True,
         Success     => Initialized);

      if Initialized then
         Prepare_Configs;

         GMA.Update_Outputs (Pipes);

         for Pipe in GMA.Pipe_Index loop
            if Pipes (Pipe).Port /= GMA.Disabled then
               Test_Screen
                 (Framebuffer => Pipes (Pipe).Framebuffer,
                  Pipe        => Pipe);
            end if;
         end loop;
      end if;
   end Main;

end GFX_Test;
