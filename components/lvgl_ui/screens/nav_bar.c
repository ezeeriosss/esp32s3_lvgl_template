#include "nav_bar.h"
#include "screens.h"
#include "esp_log.h"

static const char *TAG = "nav_bar";

#define NAV_BAR_ANIM_MS   250   /* same animation as the swipe gestures */
#define NAV_ITEM_WIDTH    80

typedef struct {
    ui_screens_t  screen;
    const char   *symbol;
} nav_item_def_t;

static const nav_item_def_t s_item_defs[] = {
    { SCREEN_HOME,     LV_SYMBOL_HOME     },
    { SCREEN_SYSTEM,   LV_SYMBOL_LIST     },   /* Sistema */
    { SCREEN_SETTINGS, LV_SYMBOL_SETTINGS },   /* Ajustes */
};

#define NAV_ITEM_COUNT (sizeof(s_item_defs) / sizeof(s_item_defs[0]))

static lv_obj_t *s_bar = NULL;
static lv_obj_t *s_item_labels[NAV_ITEM_COUNT];

/* ------------------------------------------------------------------ */

/* Tap on an item: go to its screen, sliding in the direction of travel. */
static void on_item_clicked(lv_event_t *e)
{
    ui_screens_t target  = (ui_screens_t)(uintptr_t)lv_event_get_user_data(e);
    ui_screens_t current = ui_screens_get_current();
    if (target == current) return;
    if (lv_disp_get_default()->scr_to_load != NULL) return;   /* load animation running */

    lv_scr_load_anim_t anim = (target > current) ? LV_SCR_LOAD_ANIM_MOVE_LEFT
                                                 : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    ESP_LOGD(TAG, "item -> %d", (int)target);
    ui_screens_navigate_anim(target, anim, NAV_BAR_ANIM_MS);
}

/* ------------------------------------------------------------------ */

void nav_bar_create(void)
{
    if (s_bar != NULL) return;

    s_bar = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_bar);
    lv_obj_set_size(s_bar, LV_PCT(100), NAV_BAR_HEIGHT);
    lv_obj_align(s_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_bar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_60, 0);
    lv_obj_set_flex_flow(s_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_CLICKABLE);

    for (size_t i = 0; i < NAV_ITEM_COUNT; i++) {
        /* Transparent full-height touch target with the symbol centered */
        lv_obj_t *item = lv_obj_create(s_bar);
        lv_obj_remove_style_all(item);
        lv_obj_set_size(item, NAV_ITEM_WIDTH, NAV_BAR_HEIGHT);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(item, on_item_clicked, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)s_item_defs[i].screen);

        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text_static(label, s_item_defs[i].symbol);
        lv_obj_center(label);
        s_item_labels[i] = label;
    }

    lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);    /* the app starts on the splash */
}

void nav_bar_set_active(ui_screens_t screen)
{
    if (s_bar == NULL) return;

    if (screen == SCREEN_SPLASH || screen >= SCREEN_MAX) {
        lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_HIDDEN);

    for (size_t i = 0; i < NAV_ITEM_COUNT; i++) {
        lv_color_t color = (s_item_defs[i].screen == screen)
                         ? lv_theme_get_color_primary(s_item_labels[i])
                         : lv_palette_main(LV_PALETTE_GREY);
        lv_obj_set_style_text_color(s_item_labels[i], color, 0);
    }
}
