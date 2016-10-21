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

with HW.GFX.I2C;
with HW.GFX.GMA.Config;
with HW.GFX.GMA.Panel;
with HW.GFX.GMA.I2C;
with HW.GFX.GMA.DP_Info;
with HW.GFX.GMA.DP_Aux_Ch;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.Connector_Info is

   function To_DP (Port_Cfg : Port_Config) return DP_Port
   is
   begin
      return
        (if Port_Cfg.Port = DIGI_A then
            DP_A
         else
           (case Port_Cfg.PCH_Port is
               when PCH_DP_B  => DP_B,
               when PCH_DP_C  => DP_C,
               when PCH_DP_D  => DP_D,
               when others    => DP_Port'First));
   end To_DP;

   ----------------------------------------------------------------------------

   procedure Read_EDID
     (Raw_EDID    :    out EDID.Raw_EDID_Data;
      Port_Cfg    : in     Port_Config;
      Success     :    out Boolean)
   is
      Raw_EDID_Length : GFX.I2C.Transfer_Length := Raw_EDID'Length;
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      for I in 1 .. 2 loop
         if Port_Cfg.Display = DP then
            DP_Aux_Ch.I2C_Read
              (Port     => To_DP (Port_Cfg),
               Address  => 16#50#,
               Length   => Raw_EDID_Length,
               Data     => Raw_EDID,
               Success  => Success);
         else
            I2C.I2C_Read
              (Port     => (if Port_Cfg.Display = VGA
                            then Config.Analog_I2C_Port
                            else Port_Cfg.PCH_Port),
               Address  => 16#50#,
               Length   => Raw_EDID_Length,
               Data     => Raw_EDID,
               Success  => Success);
         end if;
         exit when not Success;  -- don't retry if reading itself failed

         pragma Debug (Debug.Put_Buffer ("EDID", Raw_EDID, Raw_EDID_Length));
         Success := EDID.Valid (Raw_EDID);
         exit when Success;
      end loop;
   end Read_EDID;

   ----------------------------------------------------------------------------

   procedure Preferred_Link_Setting
     (Port_Cfg    : in out Port_Config;
      Success     :    out Boolean) is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Display = DP then
         if Port_Cfg.Port = DIGI_A then
            if GMA.Config.Use_PP_VDD_Override then
               Panel.VDD_Override;
            else
               Panel.On;
            end if;
         end if;

         DP_Info.Read_Caps
           (Link     => Port_Cfg.DP,
            Port     => To_DP (Port_Cfg),
            Success  => Success);
         if Success then
            DP_Info.Preferred_Link_Setting
              (Link     => Port_Cfg.DP,
               Mode     => Port_Cfg.Mode,
               Success  => Success);
         end if;
      else
         Success := True;
      end if;
   end Preferred_Link_Setting;

   procedure Next_Link_Setting
     (Port_Cfg : in out Port_Config;
      Success  :    out Boolean)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Display = DP then
         DP_Info.Next_Link_Setting
           (Link     => Port_Cfg.DP,
            Mode     => Port_Cfg.Mode,
            Success  => Success);
      else
         Success := False;
      end if;
   end Next_Link_Setting;

   ----------------------------------------------------------------------------

   function Default_BPC (Port_Cfg : Port_Config) return HW.GFX.BPC_Type
   is
   begin
      return
        (if Port_Cfg.Port = DIGI_A or
            (Port_Cfg.Is_FDI and Port_Cfg.PCH_Port = PCH_LVDS)
         then 6
         else 8);
   end Default_BPC;

end HW.GFX.GMA.Connector_Info;
