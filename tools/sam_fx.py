"""Outline samurai (style C) + effects: 'sunrise' stars for fast typing, smoke for slow down."""
import sys, math
from PIL import Image, ImageDraw
SRC, OUT, POMO, CONV = sys.argv[1:5]
g = {"__name__": "x"}; exec(open(CONV).read().split("if __name__")[0], g); conv = g["conv"]
sys.argv = ["x", OUT]; src = open(POMO).read(); src = src[:src.index("def slot_screen")]
p = {"__name__": "x"}; exec(src, p)
pomo = p["icon"](17, True, "B").crop((0, 4, 32, 36))
F = lambda n: conv(f"{SRC}/{n}.png", "C")
DX, SY = 2, 88
GROUND = SY + 27
OX, OY = 16, SY + 9            # just above the hat's peak

def star(px, x, y, big):
    pts = [(0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)] if big else [(0, 0)]
    for dx, dy in pts:
        if 0 <= x + dx < 32 and 41 <= y + dy < 128:
            px[x + dx, y + dy] = 1

def sunrise(px, t):
    """Stars fan out upward from the hat like rays of a rising sun."""
    angles = [-90 + a for a in (-60, -30, 0, 30, 60)]
    for k, a in enumerate(angles):
      for j in (0, 17):                         # two stars in flight per ray
        r = 5 + ((t * 2 + k * 7 + j) % 34)      # outward travel, staggered per ray
        x = round(OX + r * math.cos(math.radians(a)))
        y = round(OY + r * math.sin(math.radians(a)))
        star(px, x, y, big=r < 24)              # sparkle near, a single dot as it fades out

def squiggle_pt(a, dist):
    """A point on a snake-like path heading out at angle a (the wiggle is always sideways)."""
    wob = 1.6 * math.sin(dist * 0.85)
    return (round(OX + dist * math.cos(math.radians(a)) + wob),
            round(OY + dist * math.sin(math.radians(a))))

def smoke(d, t):
    """Wavy lines shoot out of the hat in a few directions, then pop into a dot."""
    px = d.im
    P, START, END, TAIL = 20, 5, 22, 6        # one cycle = 2 s
    for k, a in enumerate((-125, -90, -55)):
        ph = (t + k * 7) % P
        if ph < 10:                            # the squiggle travels outward
            head = START + (END - START) * (ph + 1) / 10
            tail = max(START, head - TAIL)
            dist = tail
            while dist <= head:
                x, y = squiggle_pt(a, dist)
                if 0 <= x < 32 and 42 <= y < 128: d.point((x, y), fill=1)
                dist += 0.4
        elif ph < 13:                          # pop: a small burst, then a single dot
            x, y = squiggle_pt(a, END + 1)
            pts = [(0, 0), (1, 1), (-1, -1), (1, -1), (-1, 1)] if ph == 10 else [(0, 0)]
            for dx, dy in pts:
                if 0 <= x + dx < 32 and 42 <= y + dy < 128: d.point((x + dx, y + dy), fill=1)

STATES = [("Not typing: idle", [("idle_0", 6), ("idle_1", 6), ("idle_2", 6), ("idle_3", 6)], None),
          ("Steady: run", [(f"run_{i}", 1) for i in range(6)], None),
          ("50+ wpm: attack + sunrise", [(f"attack_{i}", 1) for i in range(8)] + [("attack_7", 4)], "stars"),
          ("Too long: squiggles, slow down", [("idle_0", 8), ("idle_1", 8), ("idle_2", 8), ("idle_3", 8)], "smoke")]

def screen(frame, fx, t):
    im = Image.new("1", (32, 128), 0); d = ImageDraw.Draw(im); px = im.load()
    im.paste(pomo, (0, 8))
    spr = F(frame); im.paste(spr, (DX, SY), spr)
    for x in range(2, 30, 2): d.point((x, GROUND), fill=1)
    if fx == "stars": sunrise(px, t)
    if fx == "smoke": smoke(d, t)
    return im

def expand(seq): return [f for f, n in seq for _ in range(n)]
seqs = [expand(s) for _, s, _ in STATES]
SC, pad = 4, 24
W = len(STATES) * (32 * SC + pad) + pad
def sheet(t, key=False):
    sh = Image.new("RGB", (W, 128 * SC + 44), (30, 30, 34)); dd = ImageDraw.Draw(sh)
    for i, ((label, _, fx), sq) in enumerate(zip(STATES, seqs)):
        x = pad + i * (32 * SC + pad)
        fr = ["idle_0", "run_1", "attack_5", "idle_0"][i] if key else sq[t % len(sq)]
        sc = screen(fr, fx, t).convert("L").point(lambda v: 235 if v else 8)
        sh.paste(sc.resize((32 * SC, 128 * SC), Image.NEAREST).convert("RGB"), (x, 10))
        dd.text((x, 128 * SC + 20), label, fill=(225, 225, 225))
    return sh
frames = [sheet(t) for t in range(96)]
frames[0].save(f"{OUT}/samurai_fx.gif", save_all=True, append_images=frames[1:], duration=100, loop=0)
sheet(9, key=True).save(f"{OUT}/samurai_fx.png"); print("ok")
