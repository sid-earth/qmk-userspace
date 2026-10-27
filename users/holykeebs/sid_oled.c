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
