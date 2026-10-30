// Sid's Keyball61+ OLED screens (both portrait, 32 x 128).
//
//   Left screen  : focus timer on top, a samurai below who reacts to typing speed
//   Right screen : layer number + modifier icons; bottom row shows drag-scroll and snipe;
//                  a dot on the layer tile when the trackball switched the mouse layer on
//   Screensaver  : a mug's steam writes names on the left; scenes on the right
//
// Focus key (SID_POMO, Vial "Focus"):
//   tap  = start / pause / resume      hold (0.6 s) = reset to 25
//   25 min focus (25 small dots) -> 5 min break (5 large dots) -> back to 25.
//   While a focus or break runs, the left screen stays on (no saver, no sleep).
//
// Samurai (from typing speed, WPM):
//   below SID_WPM_STEADY      -> standing still
//   steady typing             -> running
//   SID_WPM_FAST and over     -> sword swing, stars fan out like a sunrise
//   fast for SID_FAST_TOO_LONG_MS -> squiggles: slow down (until SID_TIRED_REST_MS calmer)
//
// Screensaver triggers:
//   - 2 min without input            -> left: Sid, Jen, Elliot in turn; right: cats
//   - Shift + Bongo key              -> same, straight away
//   - SID_MOTO / SID_PINES /
//     SID_WINDOW / SID_RIDE keys     -> left: Sid only; right: that scene
//   Any later input ends it; after 5 min without input both screens go dark.

#include "quantum.h"
#include "users/holykeebs/holykeebs.h"
#include "users/holykeebs/pointing.h"
#include "users/holykeebs/sid_left_draw.h"

// ---- Settings ------------------------------------------------------------------
#define SID_POMO_FOCUS_MIN    25
#define SID_POMO_BREAK_MIN    5
#define SID_POMO_HOLD_MS      600      // hold the Focus key this long to reset
#define SID_WPM_STEADY        15       // below this: standing still
#define SID_WPM_FAST          50       // at or above this: sword + stars
#define SID_FAST_TOO_LONG_MS  120000UL // fast this long without easing off: squiggles
#define SID_TIRED_REST_MS     30000UL  // squiggles stop after this long below SID_WPM_FAST

#ifdef HK_SPLIT_SYNC_STATE
_Static_assert(sizeof(g_hk_state) <= RPC_M2S_BUFFER_SIZE, "g_hk_state no longer fits the split sync buffer");
#endif

// ---- Focus timer (runs on the USB half, shown on the left) -----------------------
static uint32_t pomo_remaining_ms = SID_POMO_FOCUS_MIN * 60000UL;
static uint32_t pomo_last         = 0;
static uint16_t pomo_pressed_at   = 0;

static void pomo_publish(uint8_t phase) {
    uint8_t left = (phase == POMO_IDLE) ? SID_POMO_FOCUS_MIN : (uint8_t)((pomo_remaining_ms + 59999UL) / 60000UL);
    if (g_hk_state.display.pomo_phase != phase || g_hk_state.display.pomo_left != left) {
        g_hk_state.display.pomo_phase = phase;
        g_hk_state.display.pomo_left  = left;
        g_hk_state.dirty              = true;  // tell the other half
    }
}

// Was the mouse layer switched on by the trackball (not by holding its key)?
// Known only on the USB half, so it is shared with the other half.
static void auto_mouse_task(void) {
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
    bool on = is_auto_mouse_active() && layer_state_is(get_auto_mouse_layer());
#else
    bool on = false;
#endif
    if (g_hk_state.display.auto_mouse != on) {
        g_hk_state.display.auto_mouse = on;
        g_hk_state.dirty              = true;
    }
}

static void pomo_task(void) {
    if (!is_keyboard_master()) {
        return;
    }
    auto_mouse_task();
    uint32_t now = timer_read32();
    uint32_t dt  = TIMER_DIFF_32(now, pomo_last);
    pomo_last    = now;
    uint8_t phase = g_hk_state.display.pomo_phase;
    if (phase == POMO_FOCUS || phase == POMO_BREAK) {
        if (dt >= pomo_remaining_ms) {
            if (phase == POMO_FOCUS) {           // focus done: break starts by itself
                phase             = POMO_BREAK;
                pomo_remaining_ms = SID_POMO_BREAK_MIN * 60000UL;
            } else {                             // break done: back to 25, waiting
                phase             = POMO_IDLE;
                pomo_remaining_ms = SID_POMO_FOCUS_MIN * 60000UL;
            }
        } else {
            pomo_remaining_ms -= dt;
        }
    }
    pomo_publish(phase);
}

static void pomo_key(bool pressed) {
    if (pressed) {
        pomo_pressed_at = timer_read();
        return;
    }
    uint8_t phase = g_hk_state.display.pomo_phase;
    if (timer_elapsed(pomo_pressed_at) >= SID_POMO_HOLD_MS) {
        phase             = POMO_IDLE;  // hold: reset
        pomo_remaining_ms = SID_POMO_FOCUS_MIN * 60000UL;
    } else {
        switch (phase) {
            case POMO_IDLE:
                pomo_remaining_ms = SID_POMO_FOCUS_MIN * 60000UL;
                phase             = POMO_FOCUS;
                break;
            case POMO_FOCUS:        phase = POMO_FOCUS_PAUSED; break;
            case POMO_FOCUS_PAUSED: phase = POMO_FOCUS;        break;
            case POMO_BREAK:        phase = POMO_BREAK_PAUSED; break;
            default:                phase = POMO_BREAK;        break;
        }
    }
    pomo_last = timer_read32();
    pomo_publish(phase);
}

static bool pomo_active(void) {
    return g_hk_state.display.pomo_phase != POMO_IDLE;
}

// ---- Samurai mood, from typing speed (computed where the left screen is) ---------
static uint8_t sam_state(void) {
    static bool     fast = false, tired = false;
    static uint32_t fast_since = 0, calm_since = 0;
#ifdef WPM_ENABLE
    uint8_t wpm = get_current_wpm();
#else
    uint8_t wpm = 0;
#endif
    if (wpm >= SID_WPM_FAST) {
        if (!fast) {
            fast       = true;
            fast_since = timer_read32();
        }
        if (timer_elapsed32(fast_since) >= SID_FAST_TOO_LONG_MS) {
            tired = true;
        }
    } else {
        if (fast) {
            fast       = false;
            calm_since = timer_read32();
        }
        if (tired && timer_elapsed32(calm_since) >= SID_TIRED_REST_MS) {
            tired = false;
        }
    }
    if (tired) return SAM_TIRED;
    if (fast) return SAM_FAST;
    return wpm >= SID_WPM_STEADY ? SAM_STEADY : SAM_IDLE;
}

// ---- Left screen ------------------------------------------------------------------
void sid_oled_render_left(void) {
    static uint32_t last_tick = 0xFFFFFFFF;
    static uint16_t last_key  = 0xFFFF;
    uint32_t tick = timer_read32() / 100;             // 100 ms animation steps
    uint8_t  sam  = sam_state();
    uint16_t key  = (uint16_t)((g_hk_state.display.pomo_phase << 10) | (g_hk_state.display.pomo_left << 2) | sam);
    if (tick == last_tick && key == last_key) {
        return;
    }
    last_tick = tick;
    last_key  = key;

    uint32_t rows[128];
    sid_left_compose(rows, g_hk_state.display.pomo_phase, g_hk_state.display.pomo_left, sam, tick);
    for (uint8_t y = 0; y < 128; y++) {
        uint32_t row = rows[y];
        for (uint8_t x = 0; x < 32; x++) {
            oled_write_pixel(x, y, (row >> (31 - x)) & 1);
        }
    }
}

// ---- Right screen: portrait icons (32x128) ----------------------------------
// Top 32 px: layer number tile. Then Control, Option, Shift, Command (24 px each).
// Held modifier / non-base layer = filled tile with the symbol cut out.
#define ICON_W 18
#define DIGIT_W 15
#define DIGIT_H 21

static const uint32_t icon_mod[4][ICON_W] = {
    // Control
    {0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00780, 0x00FC0, 0x00CC0, 0x01860,
     0x03870, 0x07038, 0x06018, 0x0C00C, 0x1C00E, 0x08004, 0x00000, 0x00000, 0x00000},
    // Option
    {0x00000, 0x00000, 0x00000, 0x00000, 0x1FCFE, 0x1FEFE, 0x00600, 0x00700, 0x00380,
     0x00180, 0x001C0, 0x000E0, 0x00060, 0x0007E, 0x0003E, 0x00000, 0x00000, 0x00000},
    // Shift
    {0x00000, 0x00300, 0x00780, 0x00CC0, 0x01860, 0x01830, 0x03018, 0x0600C, 0x0C006,
     0x1F83F, 0x01830, 0x01830, 0x01830, 0x01830, 0x01830, 0x01830, 0x01FF0, 0x00000},
    // Command
    {0x0E01C, 0x1F03E, 0x31863, 0x31863, 0x31863, 0x1FFFE, 0x0FFFC, 0x01860, 0x01860,
     0x01860, 0x01860, 0x0FFFC, 0x1FFFE, 0x31863, 0x31863, 0x31863, 0x1F03E, 0x0E01C},
};
static const uint8_t mod_mask[4] = {MOD_MASK_CTRL, MOD_MASK_ALT, MOD_MASK_SHIFT, MOD_MASK_GUI};

static const uint32_t icon_digit[4][DIGIT_H] = {
    {0x0FF8, 0x0FF8, 0x0FF8, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007,
     0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x0FF8, 0x0FF8, 0x0FF8},
    {0x01C0, 0x01C0, 0x01C0, 0x0FC0, 0x0FC0, 0x0FC0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0,
     0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x0FF8, 0x0FF8, 0x0FF8},
    {0x0FF8, 0x0FF8, 0x0FF8, 0x7007, 0x7007, 0x7007, 0x0007, 0x0007, 0x0007, 0x0038, 0x0038,
     0x0038, 0x01C0, 0x01C0, 0x01C0, 0x0E00, 0x0E00, 0x0E00, 0x7FFF, 0x7FFF, 0x7FFF},
    {0x7FFF, 0x7FFF, 0x7FFF, 0x0038, 0x0038, 0x0038, 0x01C0, 0x01C0, 0x01C0, 0x0038, 0x0038,
     0x0038, 0x0007, 0x0007, 0x0007, 0x7007, 0x7007, 0x7007, 0x0FF8, 0x0FF8, 0x0FF8},
};

// Fill a tile (on) or clear it (off); corners are trimmed to look rounded.
static void fill_tile(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on) {
    for (uint8_t y = y0; y <= y1; y++) {
        for (uint8_t x = x0; x <= x1; x++) {
            bool corner = (x - x0) + (y - y0) < 2 || (x1 - x) + (y - y0) < 2 ||
                          (x - x0) + (y1 - y) < 2 || (x1 - x) + (y1 - y) < 2;
            oled_write_pixel(x, y, on && !corner);
        }
    }
}

// Draw a glyph centred in the tile; on a filled tile its pixels are cut out.
static void draw_cell(const uint32_t *rows, uint8_t w, uint8_t h,
                      uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool selected) {
    fill_tile(x0, y0, x1, y1, selected);
    uint8_t gx = x0 + ((x1 - x0 + 1) - w) / 2;
    uint8_t gy = y0 + ((y1 - y0 + 1) - h) / 2;
    for (uint8_t y = 0; y < h; y++) {
        for (uint8_t x = 0; x < w; x++) {
            if (rows[y] & (1UL << (w - 1 - x))) {
                oled_write_pixel(gx + x, gy + y, !selected);
            }
        }
    }
}

// Like draw_cell, but centres the glyph by its visible rows, so glyphs with
// different amounts of blank space still look evenly spaced in equal boxes.
static void draw_centred(const uint32_t *rows, uint8_t w, uint8_t h,
                         uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool selected) {
    uint8_t top = 0, bot = h - 1;
    while (top < bot && rows[top] == 0) top++;
    while (bot > top && rows[bot] == 0) bot--;
    uint8_t n = bot - top + 1;
    fill_tile(x0, y0, x1, y1, selected);
    uint8_t gx = x0 + ((x1 - x0 + 1) - w) / 2;
    uint8_t gy = y0 + ((y1 - y0 + 1) - n + 1) / 2;
    for (uint8_t y = 0; y < n; y++) {
        for (uint8_t x = 0; x < w; x++) {
            if (rows[top + y] & (1UL << (w - 1 - x))) {
                oled_write_pixel(gx + x, gy + y, !selected);
            }
        }
    }
}

// Status row: drag-scroll (up/down arrows) and snipe (crosshair), 7 x 7.
static const uint32_t icon_drag[7]  = {0x08, 0x1C, 0x2A, 0x08, 0x2A, 0x1C, 0x08};
static const uint32_t icon_snipe[7] = {0x1C, 0x2A, 0x41, 0x77, 0x41, 0x2A, 0x1C};

void sid_oled_render_icons(void) {
    static uint8_t  last_layer = 0xFF, last_mods = 0xFF, last_flags = 0xFF;
    static uint32_t last_draw  = 0;
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    uint8_t mods  = get_mods() | get_oneshot_mods();
    bool    autom = g_hk_state.display.auto_mouse;
    // either trackball counts (the keys act on whichever half has the active ball)
    bool drag  = g_hk_state.main.drag_scroll || g_hk_state.peripheral.drag_scroll;
    bool snipe = g_hk_state.main.cursor_mode == CURSOR_MODE_SNIPING || g_hk_state.peripheral.cursor_mode == CURSOR_MODE_SNIPING;
    uint8_t flags = (autom ? 1 : 0) | (drag ? 2 : 0) | (snipe ? 4 : 0);

    // Redraw only on change (plus once a second, in case the screen was cleared).
    if (layer == last_layer && mods == last_mods && flags == last_flags && timer_elapsed32(last_draw) < 1000) {
        return;
    }
    last_layer = layer;
    last_mods  = mods;
    last_flags = flags;
    last_draw  = timer_read32();

    // Layer number: box x 2-29, y 1-26
    if (layer < 4) {
        draw_centred(icon_digit[layer], DIGIT_W, DIGIT_H, 2, 1, 29, 26, layer != 0);
    } else {
        fill_tile(2, 1, 29, 26, true);
    }
    if (autom && layer != 0) {
        // Mouse layer switched on by the trackball: a small round dot cut out of
        // the tile's top-right corner (4x4, 2 px in from the edges).
        static const uint8_t dot[4] = {0x6, 0xF, 0xF, 0x6};
        for (uint8_t y = 0; y < 4; y++) {
            for (uint8_t x = 0; x < 4; x++) {
                if (dot[y] & (0x8 >> x)) oled_write_pixel(24 + x, 3 + y, false);
            }
        }
    }
    // Control, Option, Shift, Command: equal boxes, 20 px tall, 2 px apart
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t y0 = 29 + i * 22;
        draw_centred(icon_mod[i], ICON_W, ICON_W, 3, y0, 28, y0 + 19, (mods & mod_mask[i]) != 0);
    }
    // Status row (y 118-127): drag-scroll left, snipe right
    if (drag) {
        draw_cell(icon_drag, 7, 7, 3, 118, 14, 127, true);
    } else {
        fill_tile(3, 118, 14, 127, false);
    }
    if (snipe) {
        draw_cell(icon_snipe, 7, 7, 17, 118, 28, 127, true);
    } else {
        fill_tile(17, 118, 28, 127, false);
    }
}

// ---- Screensaver --------------------------------------------------------------
#include "users/holykeebs/sid_saver_frames.h"  // left default: steam writes Sid, Jen, Elliot (128x32 raw)
#include "users/holykeebs/sidonly_frames.h"    // left with a scene key: steam writes Sid only
#include "users/holykeebs/cats_frames.h"       // right default: two cats, fish + 0s
#include "users/holykeebs/moto_frames.h"       // right: top-down bike on a night road
#include "users/holykeebs/pines_frames.h"      // right: night sky over pines
#include "users/holykeebs/window_frames.h"     // right: rainy window with a plant
#include "users/holykeebs/ride_frames.h"       // right: Sid's bike riding (landscape)

#define SID_SAVER_IDLE_MS 120000UL  // saver starts after 2 min without input
#define SID_OLED_OFF_MS   300000UL  // both screens go dark after 5 min

// The ride scene is landscape, drawn on the portrait-rotated right screen
// through a 90-degree remap. If it shows upside down, set this to 1.
#define SID_RIDE_FLIP 0

// Scene keys: they appear in Vial's User tab after the 18 holykeebs keys.
enum sid_keycodes {
    SID_MOTO = QK_KB_0 + 18,
    SID_PINES,
    SID_WINDOW,
    SID_RIDE,
    SID_POMO,  // "Focus": tap start/pause, hold to reset
};

enum sid_scene {
    SCENE_CATS = 0,  // default
    SCENE_MOTO,
    SCENE_PINES,
    SCENE_WINDOW,
    SCENE_RIDE,
};

// Holykeebs calls this for every key before its own handling.
bool process_record_keymap(uint16_t keycode, keyrecord_t *record) {
    if (keycode == SID_POMO) {
        if (is_keyboard_master()) {
            pomo_key(record->event.pressed);
        }
        return false;
    }
    if (keycode >= SID_MOTO && keycode <= SID_RIDE) {
        if (record->event.pressed && is_keyboard_master()) {
            g_hk_state.display.sid_scene = SCENE_MOTO + (keycode - SID_MOTO);
            g_hk_state.dirty             = true;  // tell the other half
        }
        return false;
    }
    return true;
}

// Draw one portrait frame (one uint32_t per row, bit 31 = left pixel).
static void blit_portrait(const uint32_t *rows) {
    for (uint8_t y = 0; y < 128; y++) {
        uint32_t row = rows[y];
        for (uint8_t x = 0; x < 32; x++) {
            oled_write_pixel(x, y, (row >> (31 - x)) & 1);
        }
    }
}

// Draw one landscape frame (32 rows x 4 words) onto the portrait screen.
static void blit_landscape(const uint32_t rows[32][4]) {
    for (uint8_t y = 0; y < 32; y++) {
        for (uint8_t x = 0; x < 128; x++) {
            bool on = (rows[y][x >> 5] >> (31 - (x & 31))) & 1;
#if SID_RIDE_FLIP
            oled_write_pixel(31 - y, x, on);
#else
            oled_write_pixel(y, 127 - x, on);
#endif
        }
    }
}

// Right screen during the screensaver: play the chosen scene.
static void right_scene_render(uint8_t scene, bool restart) {
    static uint8_t  pos  = 0;
    static uint32_t last = 0;
    uint16_t        ms, count;
    switch (scene) {
        case SCENE_MOTO:   ms = MOTO_FRAME_MS;   count = MOTO_FRAME_COUNT;   break;
        case SCENE_PINES:  ms = PINES_FRAME_MS;  count = PINES_FRAME_COUNT;  break;
        case SCENE_WINDOW: ms = WINDOW_FRAME_MS; count = WINDOW_FRAME_COUNT; break;
        case SCENE_RIDE:   ms = RIDE_FRAME_MS;   count = RIDE_FRAME_COUNT;   break;
        default:           ms = CATS_FRAME_MS;   count = CATS_FRAME_COUNT;   break;
    }
    if (restart) {
        pos  = 0;
        last = 0;
    }
    if (timer_elapsed32(last) < ms) {
        return;
    }
    last = timer_read32();
    if (pos >= count) {
        pos = 0;
    }
    switch (scene) {
        case SCENE_MOTO:   blit_portrait(moto_frames[pos]);   break;
        case SCENE_PINES:  blit_portrait(pines_frames[pos]);  break;
        case SCENE_WINDOW: blit_portrait(window_frames[pos]); break;
        case SCENE_RIDE:   blit_landscape(ride_frames[pos]);  break;
        default:           blit_portrait(cats_frames[pos]);   break;
    }
    pos = (pos + 1) % count;
}

// The name frames were made for the left screen in landscape (raw SSD1306 pages).
// Now that it runs in portrait, remap them so they look exactly as before.
static void blit_raw_landscape(const uint8_t *frame) {
    for (uint8_t y = 0; y < 32; y++) {
        for (uint8_t x = 0; x < 128; x++) {
            oled_write_pixel(y, 127 - x, (frame[(y >> 3) * 128 + x] >> (y & 7)) & 1);
        }
    }
}

// Left screen during the screensaver: names by default, Sid only with a scene key.
static void left_saver_render(bool sid_only, bool restart) {
    static uint16_t pos       = 0;
    static uint32_t last      = 0;
    static bool     last_mode = false;
    if (restart || sid_only != last_mode) {
        pos       = 0;
        last      = 0;
        last_mode = sid_only;
    }
    if (timer_elapsed32(last) < (sid_only ? SIDONLY_FRAME_MS : SID_SAVER_FRAME_MS)) {
        return;
    }
    last = timer_read32();
    if (sid_only) {
        blit_raw_landscape(sidonly_frames[sidonly_seq[pos]]);
        pos = (pos + 1) % SIDONLY_SEQ_LEN;
    } else {
        blit_raw_landscape(sid_saver_frames[sid_saver_seq[pos]]);
        pos = (pos + 1) % SID_SAVER_SEQ_LEN;
    }
}

// A saver started by a key (Shift + Bongo, or a scene key) runs until the next input.
static bool saver_requested(void) {
    static bool     was_on = false;
    static uint32_t on_at  = 0;
    bool on = g_hk_state.display.show_bongo_peripheral || g_hk_state.display.sid_scene != SCENE_CATS;
    if (on && !was_on) {
        on_at = timer_read32();
    }
    was_on = on;
    if (on && is_keyboard_master() && last_input_activity_elapsed() + 500 < timer_elapsed32(on_at)) {
        g_hk_state.display.show_bongo_peripheral = false;  // new input after the request
        g_hk_state.display.sid_scene             = SCENE_CATS;
        g_hk_state.dirty                         = true;   // tell the other half
        was_on = on = false;
    }
    return on;
}

// Returns true while the screensaver or sleep owns this screen.
bool sid_oled_idle_task(void) {
    static bool    taken      = false;
    static uint8_t last_scene = 0xFF;
    pomo_task();
    uint32_t idle  = last_input_activity_elapsed();
    bool     force = saver_requested();
    bool     keep  = is_keyboard_left() && pomo_active();  // timer running: left stays on

    if (idle > SID_OLED_OFF_MS && !keep) {  // long break: screens go dark
        oled_off();
        taken = true;
        return true;
    }
    if (force || (idle > SID_SAVER_IDLE_MS && !keep)) {
        oled_on();  // keep the screen awake while the saver plays
        if (is_keyboard_left()) {
            left_saver_render(g_hk_state.display.sid_scene != SCENE_CATS, !taken);
        } else {
            uint8_t scene   = g_hk_state.display.sid_scene;
            bool    restart = !taken || scene != last_scene;
            if (restart) {
                oled_clear();
            }
            last_scene = scene;
            right_scene_render(scene, restart);
        }
        taken = true;
        return true;
    }
    if (taken) {  // back from a break: wipe and let the normal screens redraw
        taken      = false;
        last_scene = 0xFF;
        oled_clear();
        oled_on();
    }
    return false;
}
