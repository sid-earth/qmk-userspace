#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "quantum.h"

// Whether THIS half currently has a working pointing device — used by the
// peripheral info RPC and the runtime ball detection (HK_SPLIT_DETECT_POINTING).
#include "pointing_device.h"
static inline bool hk_local_pointing_present(void) {
    return pointing_device_get_status() == POINTING_DEVICE_STATUS_SUCCESS;
}

typedef enum {
    POINTER_KIND_NONE,
    POINTER_KIND_PIMORONI_TRACKBALL,
    POINTER_KIND_TRACKPOINT,
    POINTER_KIND_CIRQUE35,
    POINTER_KIND_CIRQUE40,
    POINTER_KIND_TPS43,
    POINTER_KIND_TPS65,
    POINTER_KIND_PMW3360,
} hk_pointer_kind;

typedef enum {
    SCROLL_LOCK_OFF,
    SCROLL_LOCK_HORIZONTAL,
    SCROLL_LOCK_VERTICAL,
} hk_scroll_lock;

typedef enum {
    CURSOR_MODE_DEFAULT,
    CURSOR_MODE_SNIPING,
} hk_cursor_mode;

typedef struct PACKED {
    hk_pointer_kind pointer_kind : 4;
    hk_cursor_mode cursor_mode : 2;
    hk_scroll_lock scroll_lock : 2;
    bool drag_scroll : 1;
    bool scroll_direction_inverted : 1;
    float pointer_default_sensitivity;
    float pointer_sniping_sensitivity;
    uint8_t pointer_scroll_throttle;
} hk_pointer_state_t;

#define HK_OLED_MAX_PRESSING_KEYCODES 6

typedef struct PACKED {
    // Per-half bongocat toggle: _main is the master's OLED, _peripheral the other
    // half's. HK_BONGO_TOGGLE flips _main; shift+HK_BONGO_TOGGLE flips _peripheral
    // (the same shift-targets-the-peripheral convention as the pointing config).
    bool           show_bongo_main;
    bool           show_bongo_peripheral;
    uint8_t        sid_scene;       // Sid: screensaver scene picked with a key
    uint8_t        pomo_phase;      // Sid: focus timer phase (sid_left_draw.h)
    uint8_t        pomo_left;       // Sid: minutes shown in the timer circle
    bool           auto_mouse;      // Sid: mouse layer was switched on by the trackball
    uint16_t       last_kc;
    keypos_t       last_pos;
    report_mouse_t last_mouse;

    // Buffer to indicate pressing keys.
    char pressing_keys[HK_OLED_MAX_PRESSING_KEYCODES + 1];
} hk_display_state_t;

typedef struct PACKED {
    bool init : 1;
    bool dirty : 1;

    bool is_main_side : 1;
    bool setting_default_sensitivity : 1;
    bool setting_sniping_sensitivity : 1;
    bool setting_scroll_throttle : 1;

    // Which half's pointer state the unshifted config keycodes and the OLED
    // pointer panel act on: main (the master's) normally, peripheral when the
    // master half has no pointing device. See hk_active_side_peripheral in
    // holykeebs.c for when that happens; set on the master, split-synced.
    bool active_is_peripheral : 1;

    hk_pointer_state_t main;
    hk_pointer_state_t peripheral;

    hk_display_state_t display;
} hk_state_t;

#ifdef HK_SPLIT_SYNC_STATE
    _Static_assert(sizeof(hk_state_t) <= RPC_M2S_BUFFER_SIZE, "State too big to fit in split sync buffer size.");
#endif

extern hk_state_t g_hk_state;

// The pointer state the unshifted config keycodes and the OLED pointer panel
// refer to (see hk_state_t.active_is_peripheral).
static inline const hk_pointer_state_t* hk_active_pointer_state(void) {
    return g_hk_state.active_is_peripheral ? &g_hk_state.peripheral : &g_hk_state.main;
}
