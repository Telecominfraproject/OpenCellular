with Ada.Numerics.Discrete_Random;
with Ada.Unchecked_Conversion;
with Ada.Command_Line;
with Interfaces.C;

with HW.Time;
with HW.Debug;
with HW.PCI.Dev;
with HW.MMIO_Range;
with HW.GFX.GMA;
with HW.GFX.GMA.Config;
with HW.GFX.GMA.Display_Probing;

package body HW.GFX.GMA.GFX_Test
is
   pragma Disable_Atomic_Synchronization;

   Primary_Delay_MS     : constant := 8_000;
   Secondary_Delay_MS   : constant := 4_000;
   Seed                 : constant := 12345;

   package Rand_P is new Ada.Numerics.Discrete_Random (Natural);
   Gen : Rand_P.Generator;
   function Rand return Int32 is (Int32 (Rand_P.Random (Gen)));

   Start_X : constant := 0;
   Start_Y : constant := 0;

   package Dev is new PCI.Dev (PCI.Address'(0, 2, 0));

   type GTT_PTE_Type is mod 2 ** (Config.GTT_PTE_Size * 8);
   type GTT_Registers_Type is array (GTT_Range) of GTT_PTE_Type;
   package GTT is new MMIO_Range
     (Base_Addr   => 0,
      Element_T   => GTT_PTE_Type,
      Index_T     => GTT_Range,
      Array_T     => GTT_Registers_Type);

   GTT_Backup : GTT_Registers_Type;

   procedure Backup_GTT
   is
   begin
      for Idx in GTT_Range loop
         GTT.Read (GTT_Backup (Idx), Idx);
      end loop;
   end Backup_GTT;

   procedure Restore_GTT
   is
   begin
      for Idx in GTT_Range loop
         GTT.Write (Idx, GTT_Backup (Idx));
      end loop;
   end Restore_GTT;

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

   White : constant Pixel_Type := (255, 255, 255, 255);
   Black : constant Pixel_Type := (  0,   0,   0, 255);
   Red   : constant Pixel_Type := (255,   0,   0, 255);
   Green : constant Pixel_Type := (  0, 255,   0, 255);
   Blue  : constant Pixel_Type := (  0,   0, 255, 255);

   function Pixel_To_Word (P : Pixel_Type) return Word32
   with
      SPARK_Mode => Off
   is
      function To_Word is new Ada.Unchecked_Conversion (Pixel_Type, Word32);
   begin
      return To_Word (P);
   end Pixel_To_Word;

   Max_W          : constant := 4096;
   Max_H          : constant := 2160;
   FB_Align       : constant := 16#0004_0000#;
   Cursor_Align   : constant := 16#0001_0000#;
   Max_Cursor_Wid : constant := 256;
   subtype Screen_Index is Natural range 0 .. 3 *
      (Max_W * Max_H + FB_Align / 4 +
       3 * Max_Cursor_Wid * Max_Cursor_Wid + Cursor_Align / 4)
      - 1;
   type Screen_Type is array (Screen_Index) of Word32;

   function Screen_Offset (FB : Framebuffer_Type) return Natural is
     (Natural (Phys_Offset (FB) / 4));

   package Screen is new MMIO_Range (0, Word32, Screen_Index, Screen_Type);

   Screen_Backup : Screen_Type;

   procedure Backup_Screen (FB : Framebuffer_Type)
   is
      First : constant Screen_Index := Screen_Offset (FB);
      Last  : constant Screen_Index := First + Natural (FB_Size (FB)) / 4 - 1;
   begin
      for Idx in Screen_Index range First .. Last loop
         Screen.Read (Screen_Backup (Idx), Idx);
      end loop;
   end Backup_Screen;

   procedure Restore_Screen (FB : Framebuffer_Type)
   is
      First : constant Screen_Index := Screen_Offset (FB);
      Last  : constant Screen_Index := First + Natural (FB_Size (FB)) / 4 - 1;
   begin
      for Idx in Screen_Index range First .. Last loop
         Screen.Write (Idx, Screen_Backup (Idx));
      end loop;
   end Restore_Screen;

   function Drawing_Width (FB : Framebuffer_Type) return Natural is
     (Natural (FB.Width + 2 * Start_X));

   function Drawing_Height (FB : Framebuffer_Type) return Natural is
     (Natural (FB.Height + 2 * Start_Y));

   function Corner_Fill
     (X, Y  : Natural;
      FB    : Framebuffer_Type;
      Pipe  : Pipe_Index)
      return Pixel_Type
   is
      Xrel : constant Integer :=
        (if X < 32 then X else X - (Drawing_Width (FB) - 32));
      Yrel : constant Integer :=
        (if Y < 32 then Y else Y - (Drawing_Height (FB) - 32));

      function Color (Idx : Natural) return Pixel_Type is
        (case (Idx + Pipe_Index'Pos (Pipe)) mod 4 is
            when 0 => Blue,   when      1 => Black,
            when 3 => Green,  when others => Red);
   begin
      return
        (if Xrel mod 16 = 0 or Xrel = 31 or Yrel mod 16 = 0 or Yrel = 31 then
            White
         elsif Yrel < 16 then
           (if Xrel < 16 then Color (0) else Color (1))
         else
           (if Xrel < 16 then Color (3) else Color (2)));
   end Corner_Fill;

   function Fill
     (X, Y        : Natural;
      Framebuffer : Framebuffer_Type;
      Pipe        : Pipe_Index)
      return Pixel_Type
   is
      use type HW.Byte;

      Xp : constant Natural := X * 256 / Drawing_Width (Framebuffer);
      Yp : constant Natural := Y * 256 / Drawing_Height (Framebuffer);
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
      Offset_Y : Natural := Screen_Offset (Framebuffer);
      Offset   : Natural;

      function Top_Test (X, Y : Natural) return Boolean
      is
         C     : constant Natural := Drawing_Width (Framebuffer) / 2;
         S_Y   : constant Natural := 3 * (Y - Start_Y) / 2;
         Left  : constant Integer := X - C + S_Y;
         Right : constant Integer := X - C - S_Y;
      begin
         return
            (Y - Start_Y) < 12 and
            ((-1 <= Left and Left <= 0) or
             (0 <= Right and Right <= 1));
      end Top_Test;
   begin
      for Y in 0 .. Drawing_Height (Framebuffer) - 1 loop
         Offset := Offset_Y;
         for X in 0 .. Drawing_Width (Framebuffer) - 1 loop
            if (X < 32 or X >= Drawing_Width (Framebuffer) - 32) and
               (Y < 32 or Y >= Drawing_Height (Framebuffer) - 32)
            then
               P := Corner_Fill (X, Y, Framebuffer, Pipe);
            elsif Framebuffer.Rotation /= No_Rotation and then
                  Top_Test (X, Y)
            then
               P := White;
            elsif Y mod 16 = 0 or X mod 16 = 0 then
               P := Black;
            else
               P := Fill (X, Y, Framebuffer, Pipe);
            end if;
            Screen.Write (Offset, Pixel_To_Word (P));
            Offset := Offset + 1;
         end loop;
         Offset_Y := Offset_Y + Natural (Framebuffer.Stride);
      end loop;
   end Test_Screen;

   function Donut (X, Y, Max : Cursor_Pos) return Byte
   is
      ZZ : constant Int32 := Max * Max * 2;
      Dist_Center : constant Int32 := ((X * X + Y * Y) * 255) / ZZ;
      Dist_Circle : constant Int32 := Dist_Center - 20;
   begin
      return Byte (255 - Int32'Min (255, 6 * abs Dist_Circle + 64));
   end Donut;

   procedure Draw_Cursor (Pipe : Pipe_Index; Cursor : Cursor_Type)
   is
      use type HW.Byte;
      Width : constant Width_Type := Cursor_Width (Cursor.Size);
      Screen_Offset : Natural :=
         Natural (Shift_Left (Word32 (Cursor.GTT_Offset), 12) / 4);
   begin
      if Cursor.Mode /= ARGB_Cursor then
         return;
      end if;
      for Y in Cursor_Pos range -Width / 2 .. Width / 2 - 1 loop
         for X in Cursor_Pos range -Width / 2 .. Width / 2 - 1 loop
            declare
               D : constant Byte := Donut (X, Y, Width / 2);
            begin
               -- Hardware seems to expect pre-multiplied alpha (i.e.
               -- color components already contain the alpha).
               Screen.Write
                 (Index => Screen_Offset,
                  Value => Pixel_To_Word (
                    (Red   => (if Pipe = Secondary then D / 2 else 0),
                     Green => (if Pipe = Tertiary  then D / 2 else 0),
                     Blue  => (if Pipe = Primary   then D / 2 else 0),
                     Alpha => D)));
               Screen_Offset := Screen_Offset + 1;
            end;
         end loop;
      end loop;
   end Draw_Cursor;

   procedure Calc_Framebuffer
     (FB       :    out Framebuffer_Type;
      Mode     : in     Mode_Type;
      Rotation : in     Rotation_Type;
      Offset   : in out Word32)
   is
      Width : constant Width_Type := Width_Type (Mode.H_Visible);
      Height : constant Height_Type := Height_Type (Mode.V_Visible);
   begin
      Offset := (Offset + FB_Align - 1) and not (FB_Align - 1);
      if Rotation = Rotated_90 or Rotation = Rotated_270 then
         FB :=
           (Width    => Width_Type (Height),
            Height   => Height_Type (Width),
            Start_X  => Start_X,
            Start_Y  => Start_Y,
            BPC      => 8,
            Stride   => Div_Round_Up (Pos32 (Height + 2 * Start_X), 32) * 32,
            V_Stride => Div_Round_Up (Pos32 (Width + 2 * Start_Y), 32) * 32,
            Tiling   => Y_Tiled,
            Rotation => Rotation,
            Offset   => Offset + Word32 (GTT_Rotation_Offset) * GTT_Page_Size);
      else
         FB :=
           (Width    => Width,
            Height   => Height,
            Start_X  => Start_X,
            Start_Y  => Start_Y,
            BPC      => 8,
            Stride   => Div_Round_Up (Width + 2 * Start_X, 16) * 16,
            V_Stride => Height + 2 * Start_Y,
            Tiling   => Linear,
            Rotation => Rotation,
            Offset   => Offset);
      end if;
      Offset := Offset + Word32 (FB_Size (FB));
   end Calc_Framebuffer;

   type Cursor_Array is array (Cursor_Size) of Cursor_Type;
   Cursors : array (Pipe_Index) of Cursor_Array;

   procedure Prepare_Cursors
     (Cursors  :    out Cursor_Array;
      Offset   : in out Word32)
   is
      GMA_Phys_Base      : constant PCI.Index := 16#5c#;
      GMA_Phys_Base_Mask : constant := 16#fff0_0000#;

      Phys_Base : Word32;
      Success : Boolean;
   begin
      Dev.Read32 (Phys_Base, GMA_Phys_Base);
      Phys_Base := Phys_Base and GMA_Phys_Base_Mask;
      Success := Phys_Base /= GMA_Phys_Base_Mask and Phys_Base /= 0;
      if not Success then
         Debug.Put_Line ("Failed to read stolen memory base.");
         return;
      end if;

      for Size in Cursor_Size loop
         Offset := (Offset + Cursor_Align - 1) and not (Cursor_Align - 1);
         declare
            Width : constant Width_Type := Cursor_Width (Size);
            GTT_End : constant Word32 := Offset + Word32 (Width * Width) * 4;
         begin
            Cursors (Size) :=
              (Mode        => ARGB_Cursor,
               Size        => Size,
               Center_X    => Width,
               Center_Y    => Width,
               GTT_Offset  => GTT_Range (Shift_Right (Offset, 12)));
            while Offset < GTT_End loop
               GMA.Write_GTT
                 (GTT_Page       => GTT_Range (Offset / GTT_Page_Size),
                  Device_Address => GTT_Address_Type (Phys_Base + Offset),
                  Valid          => True);
               Offset := Offset + GTT_Page_Size;
            end loop;
         end;
      end loop;
   end Prepare_Cursors;

   Pipes : GMA.Pipe_Configs;

   procedure Prepare_Configs (Rotation : Rotation_Type)
   is
      use type HW.GFX.GMA.Port_Type;

      Offset : Word32 := 0;
      Success : Boolean;
   begin
      GMA.Display_Probing.Scan_Ports (Pipes);

      for Pipe in GMA.Pipe_Index loop
         if Pipes (Pipe).Port /= GMA.Disabled then
            Calc_Framebuffer
              (FB       => Pipes (Pipe).Framebuffer,
               Mode     => Pipes (Pipe).Mode,
               Rotation => Rotation,
               Offset   => Offset);
            GMA.Setup_Default_FB
              (FB       => Pipes (Pipe).Framebuffer,
               Clear    => False,
               Success  => Success);
            if not Success then
               Pipes (Pipe).Port := GMA.Disabled;
            end if;
         end if;
         Prepare_Cursors (Cursors (Pipe), Offset);
         Pipes (Pipe).Cursor := Cursors (Pipe) (Cursor_Size'Val (Rand mod 3));
      end loop;

      GMA.Dump_Configs (Pipes);
   end Prepare_Configs;

   procedure Script_Cursors
     (Pipes    : in out GMA.Pipe_Configs;
      Time_MS  : in     Natural)
   is
      type Corner is (UL, UR, LR, LL);
      type Cursor_Script_Entry is record
         Rel : Corner;
         X, Y : Int32;
      end record;
      Cursor_Script : constant array (Natural range 0 .. 19) of Cursor_Script_Entry :=
        ((UL,  16,  16), (UL,  16,  16), (UL,  16,  16), (UL, -32,   0), (UL,  16,  16),
         (UR, -16,  16), (UR, -16,  16), (UR, -16,  16), (UR,   0, -32), (UR, -16,  16),
         (LR, -16, -16), (LR, -16, -16), (LR, -16, -16), (LR,  32,   0), (LR, -16, -16),
         (LL,  16, -16), (LL,  16, -16), (LL,  16, -16), (LL,   0,  32), (LL,  16, -16));

      Deadline : constant Time.T := Time.MS_From_Now (Time_MS);
      Timed_Out : Boolean := False;
      Cnt : Word32 := 0;
   begin
      loop
         for Pipe in Pipe_Index loop
            exit when Pipes (Pipe).Port = GMA.Disabled;
            declare
               C : Cursor_Type renames Pipes (Pipe).Cursor;
               FB : Framebuffer_Type renames Pipes (Pipe).Framebuffer;
               Width : constant Int32 := Int32 (Rotated_Width (FB));
               Height : constant Int32 := Int32 (Rotated_Height (FB));
               CS : Cursor_Script_Entry renames Cursor_Script
                 (Natural (Cnt) mod (Cursor_Script'Last + 1));
            begin
               C.Center_X := CS.X;
               C.Center_Y := CS.Y;
               case CS.Rel is
                  when UL  => null;
                  when UR  => C.Center_X := CS.X + Width;
                  when LR  => C.Center_X := CS.X + Width;
                              C.Center_Y := CS.Y + Height;
                  when LL  => C.Center_Y := CS.Y + Height;
               end case;
               GMA.Place_Cursor (Pipe, C.Center_X, C.Center_Y);
            end;
         end loop;
         Timed_Out := Time.Timed_Out (Deadline);
         exit when Timed_Out;
         Time.M_Delay (160);
         Cnt := Cnt + 1;
      end loop;
   end Script_Cursors;

   type Cursor_Info is record
      X_Velo, Y_Velo : Int32;
      X_Acc, Y_Acc : Int32;
      Color : Pipe_Index;
      Size : Cursor_Size;
   end record;
   function Cursor_Rand return Int32 is (Rand mod 51 - 25);
   Cursor_Infos : array (Pipe_Index) of Cursor_Info :=
     (others =>
        (Color    => Pipe_Index'Val (Rand mod 3),
         Size     => Cursor_Size'Val (Rand mod 3),
         X_Velo   => 3 * Cursor_Rand,
         Y_Velo   => 3 * Cursor_Rand,
         others   => Cursor_Rand));

   procedure Move_Cursors
     (Pipes    : in out GMA.Pipe_Configs;
      Time_MS  : in     Natural)
   is
      procedure Select_New_Cursor
        (P  : in     Pipe_Index;
         C  : in out Cursor_Type;
         CI : in out Cursor_Info)
      is
         Old_C : constant Cursor_Type := C;
      begin
         -- change either size or color
         if Rand mod 2 = 0 then
            CI.Color := Pipe_Index'Val
              ((Pipe_Index'Pos (CI.Color) + 1 + Rand mod 2) mod 3);
         else
            CI.Size := Cursor_Size'Val
              ((Cursor_Size'Pos (CI.Size) + 1 + Rand mod 2) mod 3);
         end if;
         C := Cursors (CI.Color) (CI.Size);
         C.Center_X := Old_C.Center_X;
         C.Center_Y := Old_C.Center_Y;
         GMA.Update_Cursor (P, C);
      end Select_New_Cursor;

      Deadline : constant Time.T := Time.MS_From_Now (Time_MS);
      Timed_Out : Boolean := False;
      Cnt : Word32 := 0;
   begin
      for Pipe in Pipe_Index loop
         exit when Pipes (Pipe).Port = GMA.Disabled;
         Select_New_Cursor (Pipe, Pipes (Pipe).Cursor, Cursor_Infos (Pipe));
      end loop;
      loop
         for Pipe in Pipe_Index loop
            exit when Pipes (Pipe).Port = GMA.Disabled;
            declare
               C : Cursor_Type renames Pipes (Pipe).Cursor;
               CI : Cursor_Info renames Cursor_Infos (Pipe);
               FB : Framebuffer_Type renames Pipes (Pipe).Framebuffer;
               Width : constant Int32 := Int32 (Rotated_Width (FB));
               Height : constant Int32 := Int32 (Rotated_Height (FB));

               Update : Boolean := False;
            begin
               if Cnt mod 16 = 0 then
                  CI.X_Acc := Cursor_Rand;
                  CI.Y_Acc := Cursor_Rand;
               end if;
               CI.X_Velo := CI.X_Velo + CI.X_Acc;
               CI.Y_Velo := CI.Y_Velo + CI.Y_Acc;
               C.Center_X := C.Center_X + CI.X_Velo / 100;
               C.Center_Y := C.Center_Y + CI.Y_Velo / 100;
               if C.Center_X not in 0 .. Width - 1 then
                 C.Center_X := Int32'Max (0, Int32'Min (Width, C.Center_X));
                 CI.X_Velo := -CI.X_Velo;
                 Update := True;
               end if;
               if C.Center_Y not in 0 .. Height - 1 then
                 C.Center_Y := Int32'Max (0, Int32'Min (Height, C.Center_Y));
                 CI.Y_Velo := -CI.Y_Velo;
                 Update := True;
               end if;
               if Update then
                  Select_New_Cursor (Pipe, C, CI);
               else
                  GMA.Place_Cursor (Pipe, C.Center_X, C.Center_Y);
               end if;
            end;
         end loop;
         Timed_Out := Time.Timed_Out (Deadline);
         exit when Timed_Out;
         Time.M_Delay (16);   -- ~60 fps
         Cnt := Cnt + 1;
      end loop;
   end Move_Cursors;

   procedure Print_Usage
   is
   begin
      Debug.Put_Line
        ("Usage: " & Ada.Command_Line.Command_Name &
         " <delay seconds>" &
         " [(0|90|180|270)]");
      Debug.New_Line;
   end Print_Usage;

   procedure Main
   is
      use type HW.GFX.GMA.Port_Type;
      use type HW.Word64;
      use type Interfaces.C.int;

      Res_Addr : Word64;

      Delay_MS : Natural;
      Rotation : Rotation_Type := No_Rotation;

      Dev_Init,
      Initialized : Boolean;

      function iopl (level : Interfaces.C.int) return Interfaces.C.int;
      pragma Import (C, iopl, "iopl");
   begin
      if Ada.Command_Line.Argument_Count < 1 then
         Print_Usage;
         return;
      end if;

      Delay_MS := Natural'Value (Ada.Command_Line.Argument (1)) * 1_000;

      if Ada.Command_Line.Argument_Count >= 2 then
         declare
            Rotation_Degree : constant String := Ada.Command_Line.Argument (2);
         begin
            if    Rotation_Degree =   "0" then Rotation := No_Rotation;
            elsif Rotation_Degree =  "90" then Rotation := Rotated_90;
            elsif Rotation_Degree = "180" then Rotation := Rotated_180;
            elsif Rotation_Degree = "270" then Rotation := Rotated_270;
            else  Print_Usage; return; end if;
         end;
      end if;

      if iopl (3) /= 0 then
         Debug.Put_Line ("Failed to change i/o privilege level.");
         return;
      end if;

      Dev.Initialize (Dev_Init);
      if not Dev_Init then
         Debug.Put_Line ("Failed to map PCI config.");
         return;
      end if;

      Dev.Map (Res_Addr, PCI.Res0, Offset => Config.GTT_Offset);
      if Res_Addr = 0 then
         Debug.Put_Line ("Failed to map PCI resource0.");
         return;
      end if;
      GTT.Set_Base_Address (Res_Addr);

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
         Backup_GTT;

         Prepare_Configs (Rotation);

         GMA.Update_Outputs (Pipes);

         for Pipe in GMA.Pipe_Index loop
            if Pipes (Pipe).Port /= GMA.Disabled then
               Backup_Screen (Pipes (Pipe).Framebuffer);
               Test_Screen
                 (Framebuffer => Pipes (Pipe).Framebuffer,
                  Pipe        => Pipe);
            end if;
            for Size in Cursor_Size loop
               Draw_Cursor (Pipe, Cursors (Pipe) (Size));
            end loop;
         end loop;

         if Delay_MS < Primary_Delay_MS + Secondary_Delay_MS then
            Script_Cursors (Pipes, Delay_MS);
         else -- getting bored?
            Script_Cursors (Pipes, Primary_Delay_MS);
            Delay_MS := Delay_MS - Primary_Delay_MS;
            declare
               New_Pipes : GMA.Pipe_Configs := Pipes;

               function Rand_Div (Num : Position_Type) return Position_Type is
                 (case Rand mod 4 is
                     when 3 => Rand mod Num / 3,
                     when 2 => Rand mod Num / 2,
                     when 1 => Rand mod Num,
                     when others => 0);
            begin
               Rand_P.Reset (Gen, Seed);
               while Delay_MS >= Secondary_Delay_MS loop
                  New_Pipes := Pipes;
                  for Pipe in GMA.Pipe_Index loop
                     exit when Pipes (Pipe).Port = Disabled;
                     declare
                        New_FB : Framebuffer_Type renames
                           New_Pipes (Pipe).Framebuffer;
                        Cursor : Cursor_Type renames New_Pipes (Pipe).Cursor;
                        Width : constant Width_Type :=
                           Pipes (Pipe).Framebuffer.Width;
                        Height : constant Height_Type :=
                           Pipes (Pipe).Framebuffer.Height;
                     begin
                        New_FB.Start_X := Position_Type'Min
                          (Width - 320, Rand_Div (Width));
                        New_FB.Start_Y := Position_Type'Min
                          (Height - 320, Rand_Div (Height));
                        New_FB.Width := Width_Type'Max
                          (320, Width - New_FB.Start_X - Rand_Div (Width));
                        New_FB.Height := Height_Type'Max
                          (320, Height - New_FB.Start_Y - Rand_Div (Height));

                        Cursor.Center_X := Int32 (Rotated_Width (New_FB)) / 2;
                        Cursor.Center_Y := Int32 (Rotated_Height (New_FB)) / 2;
                        GMA.Update_Cursor (Pipe, Cursor);
                     end;
                  end loop;
                  GMA.Dump_Configs (New_Pipes);
                  GMA.Update_Outputs (New_Pipes);
                  Move_Cursors (New_Pipes, Secondary_Delay_MS);
                  Delay_MS := Delay_MS - Secondary_Delay_MS;
               end loop;
               Move_Cursors (New_Pipes, Delay_MS);
            end;
         end if;

         for Pipe in GMA.Pipe_Index loop
            if Pipes (Pipe).Port /= GMA.Disabled then
               Restore_Screen (Pipes (Pipe).Framebuffer);
            end if;
         end loop;
         Restore_GTT;
      end if;
   end Main;

end HW.GFX.GMA.GFX_Test;
