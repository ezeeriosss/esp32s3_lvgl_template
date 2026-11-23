#ifndef __SCR_SETTINGS_H__
#define __SCR_SETTINGS_H__

/**
 * Settings screen controller (ui_scrSettings).
 *
 * Convention for all screen controllers:
 *   prepare()  — called once at startup; register persistent event callbacks
 *                on widgets that should survive screen transitions.
 *   init()     — called each time this screen becomes active; restore state,
 *                start animations, refresh dynamic labels.
 *   uninit()   — called when navigating away; stop timers, free temporaries.
 *   step()     — called every ~20 ms while active; poll sensors, drive
 *                incremental animations, handle encoder input.
 *
 * All four run inside the LVGL UI timer, so lv_* calls are allowed in them.
 */
void scr_settings_prepare(void);
void scr_settings_init(void);
void scr_settings_uninit(void);
void scr_settings_step(void);

#endif
