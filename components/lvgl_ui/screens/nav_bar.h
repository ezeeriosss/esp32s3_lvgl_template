#ifndef __NAV_BAR_H__
#define __NAV_BAR_H__

#include "screens.h"

/**
 * Persistent bottom navigation bar (Home / System / Settings).
 *
 * Built once on lv_layer_top(), so it stays above every screen and is never
 * recreated on screen changes. It uses the bottom 36 px that the Squareline
 * screens leave free. Hidden on the splash screen.
 */

#define NAV_BAR_HEIGHT  36

/** Create the bar. Call once from lvgl_ui_start(). LVGL context only. */
void nav_bar_create(void);

/** Highlight the item of the active screen; hide the bar on the splash.
 *  Called by the screen engine on every transition. LVGL context only. */
void nav_bar_set_active(ui_screens_t screen);

#endif
