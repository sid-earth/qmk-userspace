// Sid's portrait left screen (32 x 128): focus timer on top, samurai below.
// Pure drawing code (no QMK calls) so it can also be tested on a computer.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sid_left_frames.h"

enum sid_pomo_phase {
    POMO_IDLE = 0,       // circle with 25, no dots
    POMO_FOCUS,          // counting down, 25 small dots (1 min each)
    POMO_FOCUS_PAUSED,   // same, circle drawn dotted
    POMO_BREAK,          // counting down, 5 large dots (1 min each)
    POMO_BREAK_PAUSED,
};

enum sid_sam_state {
    SAM_IDLE = 0,        // not typing: standing, breathing
    SAM_STEADY,          // typing: running
    SAM_FAST,            // fast burst: sword swing + sunrise stars
    SAM_TIRED,           // fast for too long: squiggles, slow down
};

static inline void sid_px(uint32_t *rows, int x, int y) {
    if (x >= 0 && x < 32 && y >= 0 && y < 128) {
        rows[y] |= 1UL << (31 - x);
    }
}

static void sid_draw_number(uint32_t *rows, uint8_t n) {
    uint8_t d[2], len = 0;
    if (n >= 10) d[len++] = n / 10;
    d[len++] = n % 10;
    // 4x7 digits drawn at 2x (8 x 14 px), 2 px apart, centred in the circle
    int x0 = (len == 2) ? 7 : 12;
    int y0 = POMO_Y + 9;
    for (uint8_t k = 0; k < len; k++) {
        for (uint8_t j = 0; j < 7; j++) {
            uint8_t r = pomo_digits[d[k]][j];
            for (uint8_t i = 0; i < 4; i++) {
                if (r & (0x08 >> i)) {
                    int x = x0 + k * 10 + i * 2, y = y0 + j * 2;
                    sid_px(rows, x, y); sid_px(rows, x + 1, y);
                    sid_px(rows, x, y + 1); sid_px(rows, x + 1, y + 1);
                }
            }
        }
    }
}

// rows: the whole screen, one uint32 per row, bit 31 = leftmost pixel.
// minutes_left: number shown in the circle; tick: 100 ms animation counter.
static void sid_left_compose(uint32_t rows[128], uint8_t phase, uint8_t minutes_left,
                             uint8_t sam, uint32_t tick) {
    memset(rows, 0, 128 * sizeof(uint32_t));

    // ---- focus timer ------------------------------------------------------------
    bool paused = phase == POMO_FOCUS_PAUSED || phase == POMO_BREAK_PAUSED;
    for (uint8_t y = 0; y < 32; y++) {
        uint32_t r = pomo_circle[y];
        if (paused) r &= (y & 1) ? 0x55555555UL : 0xAAAAAAAAUL;   // dotted circle
        rows[POMO_Y + y] |= r;
    }
    sid_draw_number(rows, minutes_left);
    if (phase == POMO_FOCUS || phase == POMO_FOCUS_PAUSED) {
        // dots vanish clockwise from 12 o'clock: the first ones go first
        for (uint8_t i = 25 - (minutes_left > 25 ? 25 : minutes_left); i < 25; i++) {
            sid_px(rows, pomo_dots25[i][0], POMO_Y + pomo_dots25[i][1]);
        }
    } else if (phase == POMO_BREAK || phase == POMO_BREAK_PAUSED) {
        for (uint8_t i = 5 - (minutes_left > 5 ? 5 : minutes_left); i < 5; i++) {
            int x = pomo_dots5[i][0], y = POMO_Y + pomo_dots5[i][1];
            sid_px(rows, x, y); sid_px(rows, x + 1, y);
            sid_px(rows, x, y + 1); sid_px(rows, x + 1, y + 1);
        }
    }

    // ---- samurai ------------------------------------------------------------------
    uint8_t f;
    switch (sam) {
        case SAM_STEADY: f = sam_seq_run[tick % sizeof(sam_seq_run)]; break;
        case SAM_FAST:   f = sam_seq_attack[tick % sizeof(sam_seq_attack)]; break;
        case SAM_TIRED:  f = sam_seq_tired[tick % sizeof(sam_seq_tired)]; break;
        default:         f = sam_seq_idle[tick % sizeof(sam_seq_idle)]; break;
    }
    for (uint8_t y = 0; y < SAM_H; y++) rows[SAM_Y + y] |= sam_frames[f][y];
    for (int x = 2; x < 30; x += 2) sid_px(rows, x, GROUND_Y);   // dotted ground

    if (sam == SAM_FAST) {
        const uint32_t *fx = fx_stars[tick % STARS_PERIOD];
        for (uint8_t y = 0; y < STARS_H; y++) rows[STARS_Y0 + y] |= fx[y];
    } else if (sam == SAM_TIRED) {
        const uint32_t *fx = fx_squig[tick % SQUIG_PERIOD];
        for (uint8_t y = 0; y < SQUIG_H; y++) rows[SQUIG_Y0 + y] |= fx[y];
    }
}
