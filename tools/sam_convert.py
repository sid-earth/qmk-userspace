"""1-bit conversions of the CC0 'Samurai Animated' sprites (1x, 40x29)."""
import sys, glob
from PIL import Image, ImageDraw
SRC, OUT = sys.argv[1], sys.argv[2]

def lum(p): return 0.299*p[0]+0.587*p[1]+0.114*p[2]

def conv(path, mode):
    a = Image.open(path).convert('RGBA'); w, h = a.size
    o = Image.new('1', (w, h), 0); px = a.load(); q = o.load()
    for y in range(h):
        for x in range(w):
            r, g, b, al = px[x, y]
            if al < 128: continue
            L = lum((r, g, b))
            if mode == 'A':   q[x, y] = 1 if L > 60 else 0          # lit fill, black outline becomes the gap
            if mode == 'B':   q[x, y] = 1 if L > 140 else 0         # only the light parts (hat, shirt, blade)
            if mode == 'C':   q[x, y] = 1 if L <= 60 else 0         # outline only (black line art lit)
    return o

if __name__ == '__main__':
    frames = ['idle_0', 'run_1', 'attack_5', 'attack_7', 'x_1', 'x_3']
    SC = 10
    sheet = Image.new('RGB', (len(frames)*(40*SC+10)+10, 3*(29*SC+24)+10), (30, 30, 34))
    d = ImageDraw.Draw(sheet)
    for r, m in enumerate('ABC'):
        for i, f in enumerate(frames):
            im = conv(f'{SRC}/{f}.png', m).convert('L').point(lambda v: 235 if v else 8)
            x, y = 10+i*(40*SC+10), 10+r*(29*SC+24)
            sheet.paste(im.resize((40*SC, 29*SC), Image.NEAREST).convert('RGB'), (x, y+14))
            d.text((x, y), f'{m}: {f}', fill=(225, 225, 225))
    sheet.save(f'{OUT}/sam_1bit_variants.png')
