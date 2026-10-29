// Sid's Keyball61+ OLED screens.
//
//   Left screen  : layer legend + held modifiers (sid_oled_render_legend)
//   Right screen : big layer number + modifier icons, portrait
//   (sid_oled_render_icons) Screensaver  : a mug's steam writes names on the
//   left; scenes on the right
//
// Screensaver triggers:
//   - 30 s without input             -> left: Sid, Jen, Elliot in turn; right:
//   cats
//   - Shift + Bongo key              -> same, straight away
//   - SID_MOTO / SID_PINES /
//     SID_WINDOW / SID_RIDE keys     -> left: Sid only; right: that scene
//   Any later input ends it; after 5 min without input both screens go dark.

#include "quantum.h"
#include "users/holykeebs/holykeebs.h"
#include "users/holykeebs/pointing.h"

// ---- Left screen: layer legend (128x32: 4 lines x 21 characters) -----------
#define OLED_COLS 21

static const char *const legend[][4] = {
    {"0 BASE", "MO1: SYM   MO2: FN", "MO1+MO2: ADJUST", "Esc/'=Ctl Enter=Cmd"},
    {"1 SYM", "Top: !@#$%  ^&*()", "Home: {([<=  ->])}", "NM,./:|`+_=> Bs=Del"},
    {"2 FN/NAV", "Num row: F1-F6 F7-F12", "HJKL: <-  v  ^  ->",
     "S=Snip UIO=V+ V- Mute"},
    {"3 ADJUST", "` key = BOOT", "';=Sat  RSft/=Val", "Bksp=RGB off Ent=on"},
};
#define LEGEND_LAYERS (sizeof(legend) / sizeof(legend[0]))

// Write s, then pad with spaces to width so older, longer text is erased.
static void write_padded(const char *s, uint8_t width) {
  uint8_t n = 0;
  while (s[n] != '\0' && n < width) {
    oled_write_char(s[n], false);
    n++;
  }
  while (n < width) {
    oled_write_char(' ', false);
    n++;
  }
}

void sid_oled_render_legend(void) {
  uint8_t layer = get_highest_layer(layer_state | default_layer_state);
  uint8_t mods = get_mods() | get_oneshot_mods();

  // Line 0: layer title + modifiers G A C S (Cmd, Opt, Ctrl, Shift)
  oled_set_cursor(0, 0);
  write_padded(layer < LEGEND_LAYERS ? legend[layer][0] : "? LAYER", 17);
  oled_write_char((mods & MOD_MASK_GUI) ? 'G' : '-', false);
  oled_write_char((mods & MOD_MASK_ALT) ? 'A' : '-', false);
  oled_write_char((mods & MOD_MASK_CTRL) ? 'C' : '-', false);
  oled_write_char((mods & MOD_MASK_SHIFT) ? 'S' : '-', false);

  // Lines 1-3: legend for the active layer
  for (uint8_t line = 1; line < 4; line++) {
    oled_set_cursor(0, line);
    write_padded(layer < LEGEND_LAYERS ? legend[layer][line] : "", OLED_COLS);
  }
}

// ---- Right screen: portrait icons (32x128) ----------------------------------
// Top 32 px: layer number tile. Then Control, Option, Shift, Command (24 px
// each). Held modifier / non-base layer = filled tile with the symbol cut out.
#define ICON_W 18
#define DIGIT_W 15
#define DIGIT_H 21

static const uint32_t icon_mod[4][ICON_W] = {
    // Control
    {0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00780, 0x00FC0, 0x00CC0,
     0x01860, 0x03870, 0x07038, 0x06018, 0x0C00C, 0x1C00E, 0x08004, 0x00000,
     0x00000, 0x00000},
    // Option
    {0x00000, 0x00000, 0x00000, 0x00000, 0x1FCFE, 0x1FEFE, 0x00600, 0x00700,
     0x00380, 0x00180, 0x001C0, 0x000E0, 0x00060, 0x0007E, 0x0003E, 0x00000,
     0x00000, 0x00000},
    // Shift
    {0x00000, 0x00300, 0x00780, 0x00CC0, 0x01860, 0x01830, 0x03018, 0x0600C,
     0x0C006, 0x1F83F, 0x01830, 0x01830, 0x01830, 0x01830, 0x01830, 0x01830,
     0x01FF0, 0x00000},
    // Command
    {0x0E01C, 0x1F03E, 0x31863, 0x31863, 0x31863, 0x1FFFE, 0x0FFFC, 0x01860,
     0x01860, 0x01860, 0x01860, 0x0FFFC, 0x1FFFE, 0x31863, 0x31863, 0x31863,
     0x1F03E, 0x0E01C},
};
static const uint8_t mod_mask[4] = {MOD_MASK_CTRL, MOD_MASK_ALT, MOD_MASK_SHIFT,
                                    MOD_MASK_GUI};

static const uint32_t icon_digit[4][DIGIT_H] = {
    {0x0FF8, 0x0FF8, 0x0FF8, 0x7007, 0x7007, 0x7007, 0x7007,
     0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007, 0x7007,
     0x7007, 0x7007, 0x7007, 0x7007, 0x0FF8, 0x0FF8, 0x0FF8},
    {0x01C0, 0x01C0, 0x01C0, 0x0FC0, 0x0FC0, 0x0FC0, 0x01C0,
     0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0,
     0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x0FF8, 0x0FF8, 0x0FF8},
    {0x0FF8, 0x0FF8, 0x0FF8, 0x7007, 0x7007, 0x7007, 0x0007,
     0x0007, 0x0007, 0x0038, 0x0038, 0x0038, 0x01C0, 0x01C0,
     0x01C0, 0x0E00, 0x0E00, 0x0E00, 0x7FFF, 0x7FFF, 0x7FFF},
    {0x7FFF, 0x7FFF, 0x7FFF, 0x0038, 0x0038, 0x0038, 0x01C0,
     0x01C0, 0x01C0, 0x0038, 0x0038, 0x0038, 0x0007, 0x0007,
     0x0007, 0x7007, 0x7007, 0x7007, 0x0FF8, 0x0FF8, 0x0FF8},
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
static void draw_cell(const uint32_t *rows, uint8_t w, uint8_t h, uint8_t x0,
                      uint8_t y0, uint8_t x1, uint8_t y1, bool selected) {
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

void sid_oled_render_icons(void) {
  static uint8_t last_layer = 0xFF, last_mods = 0xFF;
  static uint32_t last_draw = 0;
  uint8_t layer = get_highest_layer(layer_state | default_layer_state);
  uint8_t mods = get_mods() | get_oneshot_mods();

  // Redraw only on change (plus once a second, in case the screen was cleared).
  if (layer == last_layer && mods == last_mods &&
      timer_elapsed32(last_draw) < 1000) {
    return;
  }
  last_layer = layer;
  last_mods = mods;
  last_draw = timer_read32();

  if (layer < 4) {
    draw_cell(icon_digit[layer], DIGIT_W, DIGIT_H, 2, 2, 29, 29, layer != 0);
  } else {
    fill_tile(2, 2, 29, 29, true);
  }
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t y0 = 32 + i * 24;
    draw_cell(icon_mod[i], ICON_W, ICON_W, 3, y0 + 1, 28, y0 + 22,
              (mods & mod_mask[i]) != 0);
  }
}

// ---- Screensaver
// --------------------------------------------------------------
#include "users/holykeebs/cats_frames.h" // right default: two cats, fish + 0s
#include "users/holykeebs/moto_frames.h" // right: top-down bike on a night road
#include "users/holykeebs/pines_frames.h" // right: night sky over pines
#include "users/holykeebs/ride_frames.h" // right: Sid's bike riding (landscape)
#include "users/holykeebs/sid_saver_frames.h" // left default: steam writes Sid, Jen, Elliot (128x32 raw)
#include "users/holykeebs/sidonly_frames.h" // left with a scene key: steam writes Sid only
#include "users/holykeebs/window_frames.h" // right: rainy window with a plant

#define SID_SAVER_IDLE_MS 120000UL // saver starts after 2 min without input
#define SID_OLED_OFF_MS 300000UL   // both screens go dark after 5 min

// The ride scene is landscape, drawn on the portrait-rotated right screen
// through a 90-degree remap. If it shows upside down, set this to 1.
#define SID_RIDE_FLIP 0

// Scene keys: they appear in Vial's User tab after the 18 holykeebs keys.
enum sid_keycodes {
  SID_MOTO = QK_KB_0 + 18,
  SID_PINES,
  SID_WINDOW,
  SID_RIDE,
};

enum sid_scene {
  SCENE_CATS = 0, // default
  SCENE_MOTO,
  SCENE_PINES,
  SCENE_WINDOW,
  SCENE_RIDE,
};

// Holykeebs calls this for every key before its own handling.
bool process_record_keymap(uint16_t keycode, keyrecord_t *record) {
  if (keycode >= SID_MOTO && keycode <= SID_RIDE) {
    if (record->event.pressed && is_keyboard_master()) {
      g_hk_state.display.sid_scene = SCENE_MOTO + (keycode - SID_MOTO);
      g_hk_state.dirty = true; // tell the other half
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
  static uint8_t pos = 0;
  static uint32_t last = 0;
  uint16_t ms, count;
  switch (scene) {
  case SCENE_MOTO:
    ms = MOTO_FRAME_MS;
    count = MOTO_FRAME_COUNT;
    break;
  case SCENE_PINES:
    ms = PINES_FRAME_MS;
    count = PINES_FRAME_COUNT;
    break;
  case SCENE_WINDOW:
    ms = WINDOW_FRAME_MS;
    count = WINDOW_FRAME_COUNT;
    break;
  case SCENE_RIDE:
    ms = RIDE_FRAME_MS;
    count = RIDE_FRAME_COUNT;
    break;
  default:
    ms = CATS_FRAME_MS;
    count = CATS_FRAME_COUNT;
    break;
  }
  if (restart) {
    pos = 0;
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
  case SCENE_MOTO:
    blit_portrait(moto_frames[pos]);
    break;
  case SCENE_PINES:
    blit_portrait(pines_frames[pos]);
    break;
  case SCENE_WINDOW:
    blit_portrait(window_frames[pos]);
    break;
  case SCENE_RIDE:
    blit_landscape(ride_frames[pos]);
    break;
  default:
    blit_portrait(cats_frames[pos]);
    break;
  }
  pos = (pos + 1) % count;
}

// Left screen during the screensaver: names by default, Sid only with a scene
// key.
static void left_saver_render(bool sid_only, bool restart) {
  static uint16_t pos = 0;
  static uint32_t last = 0;
  static bool last_mode = false;
  if (restart || sid_only != last_mode) {
    pos = 0;
    last = 0;
    last_mode = sid_only;
  }
  if (timer_elapsed32(last) <
      (sid_only ? SIDONLY_FRAME_MS : SID_SAVER_FRAME_MS)) {
    return;
  }
  last = timer_read32();
  if (sid_only) {
    oled_write_raw((const char *)sidonly_frames[sidonly_seq[pos]], 512);
    pos = (pos + 1) % SIDONLY_SEQ_LEN;
  } else {
    oled_write_raw((const char *)sid_saver_frames[sid_saver_seq[pos]], 512);
    pos = (pos + 1) % SID_SAVER_SEQ_LEN;
  }
}

// A saver started by a key (Shift + Bongo, or a scene key) runs until the next
// input.
static bool saver_requested(void) {
  static bool was_on = false;
  static uint32_t on_at = 0;
  bool on = g_hk_state.display.show_bongo_peripheral ||
            g_hk_state.display.sid_scene != SCENE_CATS;
  if (on && !was_on) {
    on_at = timer_read32();
  }
  was_on = on;
  if (on && is_keyboard_master() &&
      last_input_activity_elapsed() + 500 < timer_elapsed32(on_at)) {
    g_hk_state.display.show_bongo_peripheral =
        false; // new input after the request
    g_hk_state.display.sid_scene = SCENE_CATS;
    g_hk_state.dirty = true; // tell the other half
    was_on = on = false;
  }
  return on;
}

// Returns true while the screensaver or sleep owns this screen.
bool sid_oled_idle_task(void) {
  static bool taken = false;
  static uint8_t last_scene = 0xFF;
  uint32_t idle = last_input_activity_elapsed();
  bool force = saver_requested();

  if (idle > SID_OLED_OFF_MS) { // long break: both screens dark
    oled_off();
    taken = true;
    return true;
  }
  if (force || idle > SID_SAVER_IDLE_MS) {
    oled_on(); // keep the screen awake while the saver plays
    if (is_keyboard_left()) {
      left_saver_render(g_hk_state.display.sid_scene != SCENE_CATS, !taken);
    } else {
      uint8_t scene = g_hk_state.display.sid_scene;
      bool restart = !taken || scene != last_scene;
      if (restart) {
        oled_clear();
      }
      last_scene = scene;
      right_scene_render(scene, restart);
    }
    taken = true;
    return true;
  }
  if (taken) { // back from a break: wipe and let the normal screens redraw
    taken = false;
    last_scene = 0xFF;
    oled_clear();
    oled_on();
  }
  return false;
}
