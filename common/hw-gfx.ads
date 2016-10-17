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

with HW;

use type HW.Pos64;
use type HW.Word32;

package HW.GFX is

   -- implementation only supports 4800p for now ;-)
   subtype Width_Type  is Pos32 range 1 .. 4800;
   subtype Height_Type is Pos32 range 1 .. 7680;

   Auto_BPC : constant := 5;
   subtype BPC_Type    is Int64 range Auto_BPC .. 16;

   type Framebuffer_Type is
   record
      Width  : Width_Type;
      Height : Height_Type;
      BPC    : BPC_Type;
      Stride : Width_Type;
      Offset : Word32;
   end record;

   Default_FB : constant Framebuffer_Type := Framebuffer_Type'
      (Width  => 1,
       Height => 1,
       BPC    => 8,
       Stride => 1,
       Offset => 0);

   subtype Frequency_Type is Pos64 range 25_000_000 .. 600_000_000;

   type DP_Lane_Count is (DP_Lane_Count_1, DP_Lane_Count_2, DP_Lane_Count_4);
   subtype DP_Lane_Count_Type is Pos64 range 1 .. 4;
   type DP_Lane_Count_Integers is array (DP_Lane_Count) of DP_Lane_Count_Type;
   Lane_Count_As_Integer : constant DP_Lane_Count_Integers :=
      DP_Lane_Count_Integers'
        (DP_Lane_Count_1 => 1, DP_Lane_Count_2 => 2, DP_Lane_Count_4 => 4);

   type DP_Bandwidth is (DP_Bandwidth_1_62, DP_Bandwidth_2_7, DP_Bandwidth_5_4);
   for DP_Bandwidth use
     (DP_Bandwidth_1_62 => 6, DP_Bandwidth_2_7 => 10, DP_Bandwidth_5_4 => 20);
   for DP_Bandwidth'Size use 8;
   subtype DP_Symbol_Rate_Type is Pos64 range 1 .. 810_000_000;
   type DP_Symbol_Rate_Array is array (DP_Bandwidth) of DP_Symbol_Rate_Type;
   DP_Symbol_Rate : constant DP_Symbol_Rate_Array := DP_Symbol_Rate_Array'
     (DP_Bandwidth_1_62 => 162_000_000,
      DP_Bandwidth_2_7  => 270_000_000,
      DP_Bandwidth_5_4  => 540_000_000);

   type DP_Caps is record
      Rev               : Word8;
      Max_Link_Rate     : DP_Bandwidth;
      Max_Lane_Count    : DP_Lane_Count;
      TPS3_Supported    : Boolean;
      Enhanced_Framing  : Boolean;
      No_Aux_Handshake  : Boolean;
      Aux_RD_Interval   : Word8;
   end record;

   type DP_Link is
      record
         Receiver_Caps           : DP_Caps;
         Lane_Count              : DP_Lane_Count;
         Bandwidth               : DP_Bandwidth;
         Enhanced_Framing        : Boolean;
         Opportunistic_Training  : Boolean;
      end record;
   Default_DP : constant DP_Link := DP_Link'
     (Receiver_Caps           => DP_Caps'
        (Rev               => 16#00#,
         Max_Link_Rate     => DP_Bandwidth'First,
         Max_Lane_Count    => DP_Lane_Count'First,
         TPS3_Supported    => False,
         Enhanced_Framing  => False,
         No_Aux_Handshake  => False,
         Aux_RD_Interval   => 16#00#),
      Lane_Count              => DP_Lane_Count'First,
      Bandwidth               => DP_Bandwidth'First,
      Enhanced_Framing        => False,
      Opportunistic_Training  => False);

   type Mode_Type is
   record
      Dotclock             : Frequency_Type;
      H_Visible            : Pos16;
      H_Sync_Begin         : Pos16;
      H_Sync_End           : Pos16;
      H_Total              : Pos16;
      V_Visible            : Pos16;
      V_Sync_Begin         : Pos16;
      V_Sync_End           : Pos16;
      V_Total              : Pos16;
      H_Sync_Active_High   : Boolean;
      V_Sync_Active_High   : Boolean;
      BPC                  : BPC_Type;
   end record;

   ----------------------------------------------------------------------------
   -- Constants
   ----------------------------------------------------------------------------

   -- modeline constants
   -- Dotclock is calculated using: Refresh_Rate * H_Total * V_Total

   M2560x1600_60 : constant Mode_Type := Mode_Type'
      (60*(2720*1646), 2560, 2608, 2640, 2720, 1600, 1603, 1609, 1646, True, True, Auto_BPC);

   M2560x1440_60 : constant Mode_Type := Mode_Type'
      (60*(2720*1481), 2560, 2608, 2640, 2720, 1440, 1443, 1448, 1481, True, False, Auto_BPC);

   M1920x1200_60 : constant Mode_Type := Mode_Type'
      (60*(2080*1235), 1920, 1968, 2000, 2080, 1200, 1203, 1209, 1235, False, False, Auto_BPC);

   M1920x1080_60 : constant Mode_Type := Mode_Type'
      (60*(2185*1135), 1920, 2008, 2052, 2185, 1080, 1084, 1089, 1135, False, False, Auto_BPC);

   M1680x1050_60 : constant Mode_Type := Mode_Type'
      (60*(2256*1087), 1680, 1784, 1968, 2256, 1050, 1051, 1054, 1087, False, True, Auto_BPC);

   M1600x1200_60 : constant Mode_Type := Mode_Type'
      (60*(2160*1250), 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, True, True, Auto_BPC);

   M1600x900_60 : constant Mode_Type := Mode_Type'
      (60*(2010*912), 1600, 1664, 1706, 2010, 900, 903, 906, 912, False, False, Auto_BPC);

   M1440x900_60 : constant Mode_Type := Mode_Type'
      (60*(1834*920), 1440, 1488, 1520, 1834, 900, 903, 909, 920, False, False, Auto_BPC);

   M1366x768_60 : constant Mode_Type := Mode_Type'
      (60*(1446*788), 1366, 1414, 1446, 1466, 768, 769, 773, 788, False, False, Auto_BPC);

   M1280x1024_60 : constant Mode_Type := Mode_Type'
      (60*(1712*1063), 1280, 1368, 1496, 1712, 1024, 1027, 1034, 1063, False, True, Auto_BPC);

   M1024x768_60 : constant Mode_Type := Mode_Type'
      (60*(1344*806), 1024, 1048, 1184, 1344, 768, 771, 777, 806, False, False, Auto_BPC);

   Invalid_Mode : constant Mode_Type := Mode_Type'
      (Frequency_Type'First, 1, 1, 1, 1, 1, 1, 1, 1, False, False, Auto_BPC);

end HW.GFX;
