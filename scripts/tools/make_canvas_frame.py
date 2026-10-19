"""Draws the frame round the canvas in pixel art, to sit in the app's dusk theme-park scene.

    python scripts/tools/make_canvas_frame.py --style marquee                  writes the pieces to assets/textures/canvas_frame/
    python scripts/tools/make_canvas_frame.py --style neon --out DIR --preview out.png

Styles:
    walnut   a classic picture frame in dark walnut: rounded outer lip, grained face, inner bevel
    dusk     the same shape in wood stained deep plum, to sit with the park at dusk
    ebony    near-black ebony with a fine grain and a thin pewter fillet next to the canvas
    neon     a dark steel bezel with cut corners and a magenta neon tube, cyan lights at the corners
    marquee  a plum fairground sign: cut corners, studded rim, a row of warm bulbs all round
    slate    a quiet dark frame with a soft violet rim light and a dithered surface

Each frame is described as a profile across its width, shaded for the light from the top left; the sides
mitre into the corners (cut at 45 degrees for the styles that have cut corners). Pieces (design pixels; the app scales them up with hard
edges):
    top.png, bottom.png        TILE x WIDTH, repeat along x
    left.png, right.png        WIDTH x TILE, repeat along y
    corner_tl/tr/bl/br.png     CORNER x CORNER, the frame's outer corner at the piece's outer corner
    bulb.png                   (marquee) one light, placed by the app evenly along the middle of each side
"""

import argparse
import math
import os

from PIL import Image

WIDTH = 16
CORNER = 22
TILE = 48        # one repeat along a side; long, so the wood grain does not visibly repeat

LIGHT = {"top": 1.0, "left": 0.7, "diag": 0.9, "bottom": -1.0, "right": -0.7}


def pick(ramp, level):
    return ramp[max(0, min(len(ramp) - 1, int(round(level))))]


def blend(a, b, f):
    return tuple(int(round(a[i] + (b[i] - a[i]) * f)) for i in range(3))


# ---------------------------------------------------------------------------------------------- styles

class Neon:
    cut = 7
    STEEL = [(10, 8, 20), (20, 16, 36), (32, 26, 54), (46, 38, 74), (64, 54, 98), (92, 80, 130)]
    TUBE = (255, 92, 214)
    GLOW = (186, 52, 160)
    CORE = (255, 214, 246)
    OUTLINE = (6, 4, 12)

    def colour(self, t, side, u, x, y):
        L = LIGHT[side]
        S = self.STEEL
        if t == 0:
            return self.OUTLINE
        if t <= 2:                                          # outer rim, catching the light
            return pick(S, 3.4 + L * 1.6 - (t - 1) * 0.8)
        if t <= 5:                                          # the body: brushed steel
            brushed = 0.4 if (u * 7 + t * 3) % 11 == 0 else 0
            return pick(S, 2.2 + L * 0.4 + brushed)
        if t == 6:
            return pick(S, 1.0)
        if t == 7:
            return self.GLOW
        if t == 8:
            return self.CORE if (u // 2) % 9 else self.TUBE   # a few dim spots, like a real tube
        if t == 9:
            return self.TUBE
        if t == 10:
            return self.GLOW
        if t == 11:
            return pick(S, 1.0)
        if t <= 13:                                         # inner step, in shadow
            return pick(S, 1.8 - L * 0.6 - (t - 12) * 0.6)
        if t == 14:
            return self.OUTLINE
        return blend(self.GLOW, (12, 6, 16), 0.65)          # the tube's glow spilling onto the lip

    def corner_light(self, img, which):
        """A small cyan lamp in each cut corner."""
        cx = cy = 4.6
        if which in ("tr", "br"):
            cx = img.width - 1 - cx
        if which in ("bl", "br"):
            cy = img.height - 1 - cy
        for y in range(img.height):
            for x in range(img.width):
                d = math.hypot(x - cx, y - cy)
                if d <= 1.0:
                    img.putpixel((x, y), (220, 255, 255, 255))
                elif d <= 1.9:
                    img.putpixel((x, y), (70, 220, 255, 255))
                elif d <= 2.6 and img.getpixel((x, y))[3]:
                    img.putpixel((x, y), (24, 70, 110, 255))


class Marquee:
    cut = 7
    PLUM = [(14, 6, 20), (30, 14, 40), (48, 24, 62), (68, 36, 86), (92, 52, 112), (124, 78, 140)]
    TRIM = [(60, 24, 30), (120, 52, 44), (196, 104, 60), (240, 160, 84), (255, 210, 140)]
    OUTLINE = (10, 4, 14)

    def colour(self, t, side, u, x, y):
        L = LIGHT[side]
        P, T = self.PLUM, self.TRIM
        if t == 0:
            return self.OUTLINE
        if t <= 2:                                          # copper trim round the outside
            return pick(T, 2.4 + L * 1.4 - (t - 1) * 1.0)
        if t == 3:
            return self.OUTLINE
        if t <= 11:                                         # the painted board the bulbs sit on
            v = (t - 4) / 7
            stripe = 0.5 if (u + t) % 4 == 0 and 0.15 < v < 0.85 else 0
            return pick(P, 2.6 + L * 0.5 * (0.5 - v) + stripe)
        if t == 12:
            return self.OUTLINE
        if t <= 13:                                         # inner copper trim
            return pick(T, 2.0 + L * 0.8)
        if t == 14:
            return self.OUTLINE
        return (16, 8, 18)

    def bulb(self):
        img = Image.new("RGBA", (7, 7), (0, 0, 0, 0))
        for y in range(7):
            for x in range(7):
                d = math.hypot(x - 3, y - 3)
                if d <= 1.2:
                    c = (255, 250, 214, 255) if x <= 3 and y <= 3 else (255, 226, 140, 255)
                elif d <= 2.2:
                    c = (255, 182, 72, 255)
                elif d <= 3.2:
                    c = (172, 92, 40, 255)
                else:
                    continue
                img.putpixel((x, y), c)
        return img

    def corner_light(self, img, which):
        """A copper stud in each cut corner."""
        cx = cy = 4.6
        if which in ("tr", "br"):
            cx = img.width - 1 - cx
        if which in ("bl", "br"):
            cy = img.height - 1 - cy
        for y in range(img.height):
            for x in range(img.width):
                d = math.hypot(x - cx, y - cy)
                if d <= 1.0:
                    img.putpixel((x, y), (255, 214, 150, 255))
                elif d <= 2.0:
                    img.putpixel((x, y), pick(self.TRIM, 2.2 - (x - cx + y - cy) * 0.5) + (255,))
                elif d <= 2.6 and img.getpixel((x, y))[3]:
                    img.putpixel((x, y), self.OUTLINE + (255,))


class Slate:
    cut = 7
    SLATE = [(8, 8, 16), (18, 16, 30), (28, 26, 44), (40, 36, 60), (56, 50, 80), (78, 70, 108)]
    RIM = (150, 110, 210)
    OUTLINE = (4, 4, 10)

    def colour(self, t, side, u, x, y):
        L = LIGHT[side]
        S = self.SLATE
        if t == 0:
            return self.OUTLINE
        if t == 1:                                          # violet rim light on the lit edges
            return self.RIM if L > 0 else pick(S, 1.4)
        if t <= 11:
            dither = 0.5 if (x + y) % 2 == 0 and t in (3, 6, 9) else 0
            return pick(S, 2.4 + L * 0.5 * (1 if t < 6 else -1) + dither)
        if t == 12:
            return pick(S, 4.4 if L < 0 else 1.0)           # inner bevel, lit on the far sides
        if t == 13:
            return self.OUTLINE
        return (14, 12, 22)

    def corner_light(self, img, which):
        pass


def wood_grain(u, t, seed=0.0):
    """Wood grain running along the side: bands across the width that wander along it, plus open pores.
    Tileable along u with period TILE. Returns a shade offset, about -1..1."""
    w = 2 * math.pi * u / TILE
    wander = 1.3 * math.sin(w + seed) + 0.5 * math.sin(3 * w + 2 * seed) + 0.25 * math.sin(7 * w + seed * 3)
    band = math.sin((t + wander) * 1.9)
    pores = -0.8 if (u * 7 + t * 13 + int(seed * 10)) % 23 == 0 else 0
    return (0.55 if band > 0.55 else -0.5 if band < -0.6 else 0.0) + pores


class Wood:
    """A classic moulding: a rounded outer lip, a wide grained face sloping gently down to the canvas,
    an inner bevel that faces the canvas (so it is lit on the far sides), and a dark lip."""
    cut = 0
    OUTLINE = (10, 6, 6)

    def colour(self, t, side, u, x, y):
        L = LIGHT[side]
        R = self.RAMP
        if t == 0:
            return self.OUTLINE
        if t <= 3:                                          # the rounded outer lip
            slope = (2.0 - t) / 1.0
            return pick(R, 3.0 + slope * L * 1.5 + wood_grain(u, t, 1.0) * 0.5)
        if t == 4:                                          # the shadow line under the lip
            return pick(R, 0.9)
        if t <= 11:                                         # the face, sloping down towards the canvas
            v = (t - 5) / 6
            return pick(R, 2.6 + L * 0.5 - v * 0.7 + wood_grain(u, t, 2.0) * 0.8)
        if t <= 13:                                         # the inner bevel, facing the canvas
            return pick(R, 2.4 - L * 1.2 + (t - 12) * 0.3)
        return self.inner(t, side, u)

    def inner(self, t, side, u):
        return (14, 8, 8) if t == 14 else (22, 14, 14)

    def corner_light(self, img, which):
        pass


class Walnut(Wood):
    RAMP = [(22, 12, 10), (44, 26, 18), (68, 42, 28), (94, 60, 38), (124, 82, 52), (156, 108, 70), (190, 140, 96)]


class Dusk(Wood):
    RAMP = [(16, 8, 18), (32, 16, 36), (52, 28, 58), (74, 42, 80), (98, 58, 104), (126, 80, 128), (160, 108, 156)]


class Ebony(Wood):
    RAMP = [(6, 5, 8), (14, 12, 18), (24, 21, 30), (36, 32, 44), (50, 45, 60), (68, 62, 80), (92, 86, 106)]
    PEWTER = [(40, 42, 52), (78, 82, 96), (128, 134, 150), (176, 182, 196)]

    def inner(self, t, side, u):
        L = LIGHT[side]
        if t == 14:                                         # a thin pewter fillet next to the canvas
            return pick(self.PEWTER, 1.6 - L * 1.4)
        return (10, 8, 12)


STYLES = {"walnut": Walnut, "dusk": Dusk, "ebony": Ebony, "neon": Neon, "marquee": Marquee, "slate": Slate}


# ---------------------------------------------------------------------------------------------- pieces

def side_and_depth(x, y, w, h, cut):
    """The nearest outer edge of a framed rectangle (outer size w x h), with its corners cut at 45 degrees
    when cut > 0, and how deep in from it the pixel lies. None outside the cut corners."""
    d = {"top": y, "left": x, "bottom": h - 1 - y, "right": w - 1 - x}
    for (a, b) in (("top", "left"), ("top", "right"), ("bottom", "left"), ("bottom", "right")):
        if cut <= 0:
            break
        diag = d[a] + d[b] - cut
        if diag < 0:
            return None, None
        if diag < min(d[a], d[b]):
            lit = "diag" if (a, b) == ("top", "left") else ("bottom" if (a, b) == ("bottom", "right") else
                                                            ("top" if a == "top" else "left"))
            if (a, b) == ("bottom", "left"):
                lit = "left"
            if (a, b) == ("top", "right"):
                lit = "top"
            return lit, diag
    side = min(d, key=lambda k: d[k])
    return side, d[side]


def make_piece(style, w, h, ox, oy, big=400):
    """A w x h window of a big framed rectangle, at (ox, oy) in it."""
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    for y in range(h):
        for x in range(w):
            gx, gy = ox + x, oy + y
            side, t = side_and_depth(gx, gy, big, big, style.cut)
            if side is None or t >= WIDTH:
                continue
            u = gx - WIDTH if side in ("top", "bottom", "diag") else gy - WIDTH
            c = style.colour(t, side, u, gx, gy)
            # the mitre joint where two sides meet shows as a fine dark line
            dx = gx if gx < big // 2 else big - 1 - gx
            dy = gy if gy < big // 2 else big - 1 - gy
            if style.cut == 0 and dx == dy and t > 0:
                c = blend(c, (0, 0, 0), 0.35)
            img.putpixel((x, y), c + (255,))
    return img


def make_all(style, big=400):
    # The sides are cut where their pattern lines up with the corners' (u = 0 at the canvas edge)
    m = WIDTH + TILE * ((big // 2 - WIDTH) // TILE)
    pieces = {
        "top": make_piece(style, TILE, WIDTH, m, 0, big),
        "bottom": make_piece(style, TILE, WIDTH, m, big - WIDTH, big),
        "left": make_piece(style, WIDTH, TILE, 0, m, big),
        "right": make_piece(style, WIDTH, TILE, big - WIDTH, m, big),
    }
    for which, (ox, oy) in {"tl": (0, 0), "tr": (big - CORNER, 0), "bl": (0, big - CORNER), "br": (big - CORNER, big - CORNER)}.items():
        img = make_piece(style, CORNER, CORNER, ox, oy, big)
        style.corner_light(img, which)
        pieces["corner_" + which] = img
    if hasattr(style, "bulb"):
        pieces["bulb"] = style.bulb()
    return pieces


BULB_SPACING = 14   # design pixels between bulbs, about; the app spreads them evenly along each side


def preview(pieces, background, out_path, cw=210, ch=118, zoom=3):
    """The framed sample canvas over the app's own background, as it will look in the workspace."""
    W = WIDTH
    bw, bh = cw + 2 * W + 80, ch + 2 * W + 50
    bg = Image.open(background).convert("RGBA").resize((bw * 2, int(bw * 2 * 1536 / 2752)), Image.LANCZOS)
    out = bg.crop((bg.width // 2 - bw // 2, bg.height - bh, bg.width // 2 + bw - bw // 2, bg.height))
    ox, oy = (bw - cw) // 2, (bh - ch) // 2
    for y in range(ch):
        for x in range(cw):
            out.putpixel((ox + x, oy + y), (246, 246, 248, 255) if ((x // 6 + y // 6) % 2) else (214, 214, 220, 255))

    def run(img, x0, y0, length, horizontal):
        for k in range(0, length, TILE):
            n = min(TILE, length - k)
            piece = img.crop((0, 0, n, img.height)) if horizontal else img.crop((0, 0, img.width, n))
            out.alpha_composite(piece, (x0 + k, y0) if horizontal else (x0, y0 + k))

    run(pieces["top"], ox, oy - W, cw, True)
    run(pieces["bottom"], ox, oy + ch, cw, True)
    run(pieces["left"], ox - W, oy, ch, False)
    run(pieces["right"], ox + cw, oy, ch, False)
    out.alpha_composite(pieces["corner_tl"], (ox - W, oy - W))
    out.alpha_composite(pieces["corner_tr"], (ox + cw + W - CORNER, oy - W))
    out.alpha_composite(pieces["corner_bl"], (ox - W, oy + ch + W - CORNER))
    out.alpha_composite(pieces["corner_br"], (ox + cw + W - CORNER, oy + ch + W - CORNER))
    if "bulb" in pieces:
        b = pieces["bulb"]
        mid = W // 2 - 1
        for length, place in ((cw, lambda p: (ox + p, oy - mid - 1)), (cw, lambda p: (ox + p, oy + ch + mid - 1)),
                              (ch, lambda p: (ox - mid - 1, oy + p)), (ch, lambda p: (ox + cw + mid - 1, oy + p))):
            n = max(1, round(length / BULB_SPACING))
            for i in range(n):
                p = int((i + 0.5) * length / n)
                x, y = place(p)
                out.alpha_composite(b, (x - b.width // 2 + 1, y - b.height // 2 + 1))
    out.resize((out.width * zoom, out.height * zoom), Image.NEAREST).save(out_path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--style", choices=sorted(STYLES), default="walnut")
    ap.add_argument("--out", default=os.path.join("assets", "textures", "canvas_frame"))
    ap.add_argument("--preview", metavar="PNG", help="also write a framed sample canvas over the app's background")
    args = ap.parse_args()
    pieces = make_all(STYLES[args.style]())
    os.makedirs(args.out, exist_ok=True)
    for old in os.listdir(args.out):
        if old.endswith(".png"):
            os.remove(os.path.join(args.out, old))
    for name, img in pieces.items():
        img.save(os.path.join(args.out, name + ".png"))
    if args.preview:
        preview(pieces, os.path.join("assets", "landofwisdompark2.jfif"), args.preview)


if __name__ == "__main__":
    main()
