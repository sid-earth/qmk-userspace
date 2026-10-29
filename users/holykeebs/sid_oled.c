#include "quantum.h"
#include "users/holykeebs/holykeebs.h"

// Layer legend for the Keyball61+ OLED on the half without USB.
// Screen is 128x32: 4 lines x 21 characters.
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

// Replaces the Keyball logo (see keyball61plus.c, made weak).
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
