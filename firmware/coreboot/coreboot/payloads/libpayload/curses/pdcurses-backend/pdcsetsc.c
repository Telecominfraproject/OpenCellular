/* Public Domain Curses */
/* This file is BSD licensed, Copyright 2011 secunet AG */

#include <libpayload.h>
#include "lppdc.h"

int PDC_curs_set(int visibility)
{
    int ret_vis;

    PDC_LOG(("PDC_curs_set() - called: visibility=%d\n", visibility));

    ret_vis = SP->visibility;
    SP->visibility = visibility;

#if IS_ENABLED(CONFIG_LP_SERIAL_CONSOLE)
    if (curses_flags & F_ENABLE_SERIAL) {
        serial_cursor_enable(visibility);
    }
#endif
#if IS_ENABLED(CONFIG_LP_VIDEO_CONSOLE)
    if (curses_flags & F_ENABLE_CONSOLE) {
        video_console_cursor_enable(visibility);
    }
#endif

    return ret_vis;
}

int PDC_set_blink(bool blinkon)
{
	if (pdc_color_started)
		COLORS = 16;

	return ERR;
}
