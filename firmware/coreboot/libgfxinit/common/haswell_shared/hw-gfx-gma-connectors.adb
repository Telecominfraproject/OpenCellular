--
-- Copyright (C) 2015-2017 secunet Security Networks AG
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

with HW.GFX.GMA.Config;
with HW.GFX.GMA.Panel;
with HW.GFX.GMA.Connectors.DDI;

with HW.Debug;
with GNAT.Source_Info;

package body HW.GFX.GMA.Connectors is

   procedure Post_Reset_Off
   is
   begin
      DDI.Post_Reset_Off;
   end Post_Reset_Off;

   procedure Initialize
   is
   begin
      DDI.Initialize;
   end Initialize;

   procedure Pre_On
     (Pipe     : in     Pipe_Index;
      Port_Cfg : in     Port_Config;
      PLL_Hint : in     Word32;
      Success  :    out Boolean)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));
      if Port_Cfg.Port in Digital_Port then
         DDI.Pre_On (Port_Cfg, PLL_Hint, Success);
      else
         Success := False; -- Should not happen
      end if;
   end Pre_On;

   procedure Post_On
     (Pipe     : in     Pipe_Index;
      Port_Cfg : in     Port_Config;
      PLL_Hint : in     Word32;
      Success  :    out Boolean)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Port in Digital_Port then
         DDI.Post_On (Port_Cfg);

         if Port_Cfg.Port = DIGI_A then
            Panel.Backlight_On;
         end if;
         Success := True;
      else
         Success := False; -- Should not happen
      end if;
   end Post_On;

   ----------------------------------------------------------------------------

   procedure Pre_Off (Port_Cfg : Port_Config)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));

      if Port_Cfg.Port = DIGI_A then
         Panel.Backlight_Off;
         Panel.Off;
      end if;
   end Pre_Off;

   procedure Post_Off (Port_Cfg : Port_Config)
   is
   begin
      pragma Debug (Debug.Put_Line (GNAT.Source_Info.Enclosing_Entity));
      if Port_Cfg.Port in Digital_Port then
         DDI.Off (Port_Cfg.Port);
      end if;
   end Post_Off;

   ----------------------------------------------------------------------------

   procedure Pre_All_Off
   is
   begin
      Panel.Backlight_Off;
      Panel.Off;
   end Pre_All_Off;

   procedure Post_All_Off
   is
   begin
      for Port in Digital_Port range DIGI_A .. Config.Last_Digital_Port loop
         DDI.Off (Port);
      end loop;
   end Post_All_Off;

end HW.GFX.GMA.Connectors;
