with HW.GFX.GMA.Config;

private package HW.GFX.GMA.GMCH is

   GMCH_PORT_PIPE_SELECT_SHIFT : constant := 30;
   GMCH_PORT_PIPE_SELECT_MASK  : constant := 1 * 2 ** 30;
   type GMCH_PORT_PIPE_SELECT_Array is array (Pipe_Index) of Word32;
   GMCH_PORT_PIPE_SELECT       : constant GMCH_PORT_PIPE_SELECT_Array :=
     (Primary   => 0 * 2 ** GMCH_PORT_PIPE_SELECT_SHIFT,
      Secondary => 1 * 2 ** GMCH_PORT_PIPE_SELECT_SHIFT,
      Tertiary  => 0);

end HW.GFX.GMA.GMCH;
