with Ada.Unchecked_Conversion;
with Ada.Command_Line;
with Interfaces.C;

with HW.Debug;
with HW.PCI.Dev;
with HW.MMIO_Range;
with HW.GFX.GMA;
with HW.GFX.GMA.Display_Probing;

package body HW.GFX.GMA.GFX_Test
is
   pragma Disable_Atomic_Synchronization;

   package Dev is new PCI.Dev (PCI.Address'(0, 2, 0));

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

   function Pixel_To_Word (P : Pixel_Type) return Word32
   with
      SPARK_Mode => Off
   is
      function To_Word is new Ada.Unchecked_Conversion (Pixel_Type, Word32);
   begin
      return To_Word (P);
   end Pixel_To_Word;

   Max_W    : constant := 4096;
   Max_H    : constant := 2160;
   FB_Align : constant := 16#0004_0000#;
   subtype Screen_Index is Natural range
      0 .. 3 * (Max_W * Max_H + FB_Align / 4) - 1;
   type Screen_Type is array (Screen_Index) of Word32;

   package Screen is new MMIO_Range (0, Word32, Screen_Index, Screen_Type);

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
            Screen.Write (Offset, Pixel_To_Word (P));
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

      Offset : Word32 := 0;
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

   procedure Main
   is
      use type HW.GFX.GMA.Port_Type;
      use type HW.Word64;
      use type Interfaces.C.int;

      Res_Addr : Word64;

      Dev_Init,
      Initialized : Boolean;

      function iopl (level : Interfaces.C.int) return Interfaces.C.int;
      pragma Import (C, iopl, "iopl");
   begin
      if iopl (3) /= 0 then
         Debug.Put_Line ("Failed to change i/o privilege level.");
         return;
      end if;

      Dev.Initialize (Dev_Init);
      if not Dev_Init then
         Debug.Put_Line ("Failed to map PCI config.");
         return;
      end if;

      Dev.Map (Res_Addr, PCI.Res2, WC => True);
      if Res_Addr = 0 then
         Debug.Put_Line ("Failed to map PCI resource2.");
         return;
      end if;
      Screen.Set_Base_Address (Res_Addr);

      GMA.Initialize
        (Clean_State => True,
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

end HW.GFX.GMA.GFX_Test;
