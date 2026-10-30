"""Pomodoro icon for the portrait left screen: a fixed 32x40 box (one of three
stacked icon slots). A circle with the minutes inside; when running, dots around
the circle count the time down and vanish clockwise from 12 o'clock.

Variant A: 5 dots, each worth 5 minutes.
Variant B: 25 tiny dots, each worth 1 minute (a dotted ring that erodes).
"""
import math
import sys
from PIL import Image, ImageDraw

OUT = sys.argv[1]
BW, BH = 32, 40                   # the icon's fixed box
CX, CY = 15.5, 19.5               # circle centre inside the box
R_CIRCLE = 9                      # the circle
R_DOTS = 13.5                     # ring the dots sit on

D57 = {
    "0": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    "3": ["11111", "00010", "00100", "00010", "00001", "10001", "01110"],
    "4": ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    "5": ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    "6": ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    "7": ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    "8": ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    "9": ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],
}


def digits(d, s, cx, cy):
    w = len(s) * 5 + (len(s) - 1)
    x0, y0 = round(cx - w / 2), round(cy - 3.5)
    for k, ch in enumerate(s):
        for j, r in enumerate(D57[ch]):
            for i, c in enumerate(r):
                if c == "1":
                    d.point((x0 + k * 6 + i, y0 + j), fill=1)


def icon(minutes_left, running, variant="A"):
    im = Image.new("1", (BW, BH), 0)
    d = ImageDraw.Draw(im)
    d.ellipse([CX - R_CIRCLE, CY - R_CIRCLE, CX + R_CIRCLE, CY + R_CIRCLE], outline=1)
    digits(d, str(minutes_left), CX, CY)
    if running:
        n, unit = (5, 5) if variant == "A" else (25, 1)
        left = math.ceil(minutes_left / unit)            # dots still standing
        # dots vanish clockwise from 12 o'clock: the first ones to go are the earliest
        for i in range(n):
            if i < n - left:
                continue
            a = -math.pi / 2 + 2 * math.pi * i / n
            x, y = CX + R_DOTS * math.cos(a), CY + R_DOTS * math.sin(a)
            if variant == "A":
                d.rectangle([round(x - 1), round(y - 1), round(x), round(y)], fill=1)   # 2x2 dot
            else:
                d.point((round(x), round(y)), fill=1)
    return im


def slot_screen(pomo):
    """The whole left screen: pomodoro in slot 1, two placeholder slots below."""
    im = Image.new("1", (32, 128), 0)
    d = ImageDraw.Draw(im)
    im.paste(pomo, (0, 2))
    for k in (1, 2):                                      # placeholders for the other icons
        y0 = 2 + k * 42
        for x in range(4, 28, 3):
            d.point((x, y0 + 4), fill=1)
            d.point((x, y0 + 36), fill=1)
        for y in range(y0 + 4, y0 + 37, 3):
            d.point((4, y), fill=1)
            d.point((27, y), fill=1)
    return im


states = [("Idle", 25, False), ("Start", 25, True), ("24 min", 24, True), ("17 min", 17, True),
          ("8 min", 8, True), ("3 min", 3, True)]
SC, pad = 6, 18
rows = [("A: 5 dots x 5 min", "A"), ("B: 25 dots x 1 min", "B")]
sheet = Image.new("RGB", (len(states) * (BW * SC + pad) + pad, len(rows) * (BH * SC + 50) + 20), (30, 30, 34))
sd = ImageDraw.Draw(sheet)
for r, (rlabel, var) in enumerate(rows):
    y = 10 + r * (BH * SC + 50)
    sd.text((pad, y), rlabel, fill=(225, 225, 225))
    for i, (label, m, run) in enumerate(states):
        x = pad + i * (BW * SC + pad)
        im = icon(m, run, var).convert("L").point(lambda v: 235 if v else 8)
        sheet.paste(im.resize((BW * SC, BH * SC), Image.NEAREST).convert("RGB"), (x, y + 14))
        sd.text((x, y + 18 + BH * SC), label, fill=(200, 200, 200))
sheet.save(f"{OUT}/pomo_icon.png")

# in context: the full left screen with two empty slots, at 4x
ctx = Image.new("RGB", (3 * (32 * 4 + 24) + 24, 128 * 4 + 40), (30, 30, 34))
cd = ImageDraw.Draw(ctx)
for i, (label, m, run, var) in enumerate([("Idle", 25, False, "A"), ("17 min (A)", 17, True, "A"), ("17 min (B)", 17, True, "B")]):
    x = 24 + i * (32 * 4 + 24)
    ctx.paste(slot_screen(icon(m, run, var)).convert("L").point(lambda v: 235 if v else 8)
              .resize((32 * 4, 128 * 4), Image.NEAREST).convert("RGB"), (x, 10))
    cd.text((x, 128 * 4 + 18), label, fill=(225, 225, 225))
ctx.save(f"{OUT}/pomo_in_context.png")
print("ok")
