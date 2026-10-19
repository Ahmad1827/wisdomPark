"""Small pixel-art drawing library for Claude's sprite scripts.

A sprite script (sprite.py) draws every frame of a spritesheet with these calls instead
of listing pixels, so a change such as "longer legs" is one number, and every frame of
an animation comes from the same code and stays on model.

Coordinates are integer pixels, (0, 0) is the top-left, x grows right and y grows down.
Colours are "#rrggbb" strings (or None for transparent). Nothing is anti-aliased: every
pixel is either fully opaque or fully transparent.
"""

import colorsys
import math

from PIL import Image


# ---------------------------------------------------------------- colours

def hex_to_rgb(value):
    value = value.lstrip("#")
    return tuple(int(value[i:i + 2], 16) for i in (0, 2, 4))


def rgb_to_hex(rgb):
    return "#%02x%02x%02x" % tuple(max(0, min(255, round(c))) for c in rgb)


def ramp(base, steps=4, hue_shift=12, light_range=0.42, sat_shift=0.10):
    """A hue-shifted colour ramp from shadow to highlight with `base` in the middle.

    Shadows lean toward blue/purple and lose lightness, highlights lean toward yellow
    and gain it, which reads far better than plain darker/lighter copies of one hue.
    Returns `steps` colours, darkest first.
    """
    r, g, b = (c / 255 for c in hex_to_rgb(base))
    h, l, s = colorsys.rgb_to_hls(r, g, b)
    out = []
    mid = (steps - 1) / 2
    for i in range(steps):
        k = (i - mid) / max(mid, 1)          # -1 shadow ... +1 highlight
        # Rotate toward 60deg (yellow) for highlights and toward 240deg (blue) for shadows.
        target = 60 / 360 if k > 0 else 240 / 360
        dh = ((target - h + 0.5) % 1.0) - 0.5
        nh = (h + math.copysign(min(abs(dh), hue_shift / 360 * abs(k)), dh)) % 1.0
        nl = max(0.04, min(0.96, l + k * light_range / 2))
        ns = max(0.0, min(1.0, s + (sat_shift if k < 0 else -sat_shift) * abs(k)))
        out.append(rgb_to_hex(c * 255 for c in colorsys.hls_to_rgb(nh, nl, ns)))
    return out


def mix(a, b, t=0.5):
    ra, rb = hex_to_rgb(a), hex_to_rgb(b)
    return rgb_to_hex(x + (y - x) * t for x, y in zip(ra, rb))


# ---------------------------------------------------------------- canvas

def line_points(x0, y0, x1, y1):
    """The pixels of a Bresenham line from (x0, y0) to (x1, y1), ends included."""
    x0, y0, x1, y1 = (int(round(v)) for v in (x0, y0, x1, y1))
    dx, dy = abs(x1 - x0), -abs(y1 - y0)
    sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
    err = dx + dy
    while True:
        yield x0, y0
        if x0 == x1 and y0 == y1:
            return
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy



class Canvas:
    """A grid of colours (None = transparent)."""

    def __init__(self, width, height):
        self.w, self.h = width, height
        self.px = [[None] * width for _ in range(height)]

    # -- single pixels
    def inside(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h

    def get(self, x, y):
        return self.px[y][x] if self.inside(x, y) else None

    def set(self, x, y, color):
        x, y = int(round(x)), int(round(y))
        if self.inside(x, y):
            self.px[y][x] = color

    def clear(self):
        for row in self.px:
            row[:] = [None] * self.w

    # -- shapes
    def line(self, x0, y0, x1, y1, color):
        """Bresenham line: one pixel thick, no doubled corners."""
        for x, y in line_points(x0, y0, x1, y1):
            self.set(x, y, color)

    def polyline(self, points, color, closed=False):
        pts = list(points) + ([points[0]] if closed else [])
        for (a, b), (c, d) in zip(pts, pts[1:]):
            self.line(a, b, c, d, color)

    def rect(self, x, y, w, h, color, fill=True):
        """Rectangle with top-left (x, y) and size w x h."""
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                if fill or yy in (y, y + h - 1) or xx in (x, x + w - 1):
                    self.set(xx, yy, color)

    def ellipse(self, cx, cy, rx, ry, color, fill=True):
        """Ellipse centred on (cx, cy). Half-integer centres give even-width shapes,
        e.g. ellipse(7.5, 7.5, 4, 4) covers x 4..11."""
        for y in range(int(math.floor(cy - ry)) - 1, int(math.ceil(cy + ry)) + 2):
            for x in range(int(math.floor(cx - rx)) - 1, int(math.ceil(cx + rx)) + 2):
                d = ((x - cx) / (rx + 0.35)) ** 2 + ((y - cy) / (ry + 0.35)) ** 2
                if d <= 1.0:
                    if fill:
                        self.set(x, y, color)
                    else:
                        # Edge pixel: inside, but a 4-neighbour is outside.
                        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                            if ((nx - cx) / (rx + 0.35)) ** 2 + ((ny - cy) / (ry + 0.35)) ** 2 > 1.0:
                                self.set(x, y, color)
                                break

    def polygon(self, points, color):
        """Filled polygon (even-odd scanline), edges included."""
        ys = [p[1] for p in points]
        for y in range(int(math.floor(min(ys))), int(math.ceil(max(ys))) + 1):
            xs = []
            for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1]):
                if (y0 <= y < y1) or (y1 <= y < y0):
                    xs.append(x0 + (y - y0) * (x1 - x0) / (y1 - y0))
            xs.sort()
            for a, b in zip(xs[::2], xs[1::2]):
                for x in range(int(math.ceil(a - 0.5)), int(math.floor(b + 0.5)) + 1):
                    self.set(x, y, color)
        self.polyline(points, color, closed=True)

    def fill(self, x, y, color):
        """Flood fill (4-connected) from (x, y)."""
        target = self.get(x, y)
        if not self.inside(x, y) or target == color:
            return
        stack = [(x, y)]
        while stack:
            px, py = stack.pop()
            if self.inside(px, py) and self.px[py][px] == target:
                self.px[py][px] = color
                stack += [(px + 1, py), (px - 1, py), (px, py + 1), (px, py - 1)]

    def recolor(self, old, new, region=None):
        """Replace one colour with another, optionally only inside (x, y, w, h)."""
        x0, y0, w, h = region or (0, 0, self.w, self.h)
        for y in range(y0, y0 + h):
            for x in range(x0, x0 + w):
                if self.get(x, y) == old:
                    self.set(x, y, new)

    def pixels(self, x, y, rows, colors):
        """Paint a small hand-made pattern. `rows` is a list of strings; each character
        is a key into `colors` and '.' or ' ' is skipped. Good for eyes, buckles, logos:

            c.pixels(10, 6, ["ab", "bb"], {"a": WHITE, "b": BLACK})
        """
        for dy, row in enumerate(rows):
            for dx, ch in enumerate(row):
                if ch not in ". ":
                    self.set(x + dx, y + dy, colors[ch])

    # -- whole-sprite effects
    def outline(self, color, inner=False, diagonal=False):
        """Outline the drawn shape. Outer (default) draws around it; inner=True
        repaints its own edge pixels. diagonal=True also counts corner neighbours,
        which gives a thicker, rounder line."""
        nbrs = [(1, 0), (-1, 0), (0, 1), (0, -1)]
        if diagonal:
            nbrs += [(1, 1), (1, -1), (-1, 1), (-1, -1)]
        marks = []
        for y in range(self.h):
            for x in range(self.w):
                filled = self.px[y][x] is not None
                if inner and filled and any(self.get(x + dx, y + dy) is None for dx, dy in nbrs):
                    marks.append((x, y))
                if not inner and not filled and any(self.get(x + dx, y + dy) is not None for dx, dy in nbrs):
                    marks.append((x, y))
        for x, y in marks:
            self.px[y][x] = color

    def selective_outline(self, dark_for, default=None):
        """Outer outline whose colour follows what it touches: `dark_for` maps a fill
        colour to its outline colour (usually the darkest step of that colour's ramp).
        Softer than one black line around everything."""
        marks = []
        for y in range(self.h):
            for x in range(self.w):
                if self.px[y][x] is not None:
                    continue
                for dx, dy in ((0, 1), (1, 0), (-1, 0), (0, -1)):
                    c = self.get(x + dx, y + dy)
                    if c is not None:
                        marks.append((x, y, dark_for.get(c, default or c)))
                        break
        for x, y, c in marks:
            self.px[y][x] = c

    def shade(self, base, shadow, light=(-1, -1), region=None):
        """Darken pixels of colour `base` that face away from the light: a pixel whose
        neighbour in the shadow direction is empty or another colour becomes `shadow`.
        light=(-1, -1) is light from the top-left."""
        sx, sy = -light[0], -light[1]
        x0, y0, w, h = region or (0, 0, self.w, self.h)
        marks = []
        for y in range(y0, y0 + h):
            for x in range(x0, x0 + w):
                if self.get(x, y) != base:
                    continue
                if (sx and self.get(x + sx, y) != base) or (sy and self.get(x, y + sy) != base):
                    marks.append((x, y))
        for x, y in marks:
            self.px[y][x] = shadow

    def highlight(self, base, light_color, light=(-1, -1), region=None):
        """The mirror of shade(): pixels of `base` on the lit edge become `light_color`."""
        self.shade(base, light_color, light=(-light[0], -light[1]), region=region)

    def dither(self, x, y, w, h, color, pattern="checker"):
        """Paint `color` over every other pixel of a region that is already drawn.
        pattern: "checker" (50%) or "sparse" (25%)."""
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                if self.get(xx, yy) is None:
                    continue
                if pattern == "checker" and (xx + yy) % 2 == 0:
                    self.set(xx, yy, color)
                elif pattern == "sparse" and xx % 2 == 0 and yy % 2 == 0:
                    self.set(xx, yy, color)

    def mirror_x(self, from_left=True):
        """Copy the left half onto the right half (or the reverse): symmetric fronts."""
        for y in range(self.h):
            for x in range(self.w // 2):
                a, b = x, self.w - 1 - x
                if from_left:
                    self.px[y][b] = self.px[y][a]
                else:
                    self.px[y][a] = self.px[y][b]

    # -- reusable parts
    def stamp(self, other, x, y, flip_x=False, flip_y=False):
        """Draw `other` (a Canvas, e.g. a part drawn once) with its top-left at (x, y).
        Transparent pixels of `other` leave this canvas untouched."""
        for oy in range(other.h):
            for ox in range(other.w):
                c = other.px[other.h - 1 - oy if flip_y else oy][other.w - 1 - ox if flip_x else ox]
                if c is not None:
                    self.set(x + ox, y + oy, c)

    def copy(self):
        c = Canvas(self.w, self.h)
        c.px = [row[:] for row in self.px]
        return c

    def shifted(self, dx, dy):
        c = Canvas(self.w, self.h)
        c.stamp(self, dx, dy)
        return c

    def to_image(self):
        img = Image.new("RGBA", (self.w, self.h), (0, 0, 0, 0))
        data = [(0, 0, 0, 0) if c is None else hex_to_rgb(c) + (255,) for row in self.px for c in row]
        img.putdata(data)
        return img


def thick_line(c, x0, y0, x1, y1, width, color):
    """A line `width` pixels thick (square brush), e.g. for limbs, swords, staffs."""
    lo = -(width // 2)
    for x, y in line_points(x0, y0, x1, y1):
        for dy in range(lo, lo + width):
            for dx in range(lo, lo + width):
                c.set(x + dx, y + dy, color)


def bend(start, end, upper, lower, direction=1):
    """Two-bone joint solver: where the knee (or elbow) goes for a limb from `start`
    (hip/shoulder) to `end` (foot/hand) with segment lengths `upper` and `lower`.
    direction=+1 bends the joint toward +x (a knee of a character facing right),
    -1 toward -x (an elbow pointing back). If `end` is out of reach the limb goes straight."""
    sx, sy = start
    ex, ey = end
    dx, dy = ex - sx, ey - sy
    dist = math.hypot(dx, dy) or 1e-6
    if dist >= upper + lower:
        k = upper / (upper + lower)
        return (round(sx + dx * k), round(sy + dy * k))
    a = (upper * upper - lower * lower + dist * dist) / (2 * dist)
    h = math.sqrt(max(upper * upper - a * a, 0))
    mx, my = sx + dx * a / dist, sy + dy * a / dist
    # Two solutions; pick the one on the requested side.
    px, py = -dy / dist * h, dx / dist * h
    if (px > 0) != (direction > 0):
        px, py = -px, -py
    return (round(mx + px), round(my + py))


def limb(c, start, end, upper, lower, width, color, direction=1, joint_color=None):
    """Draw a two-segment limb (thigh+shin, upper arm+forearm) from `start` to `end`.
    Returns the joint position so a knee pad or elbow can be drawn on it."""
    j = bend(start, end, upper, lower, direction)
    thick_line(c, start[0], start[1], j[0], j[1], width, color)
    thick_line(c, j[0], j[1], end[0], end[1], width, color)
    if joint_color:
        c.set(j[0], j[1], joint_color)
    return j


# One walk cycle as 8 key poses for a character facing right. Each entry:
# (front foot dx, front foot lift, back foot dx, back foot lift, body dy), where dx is
# in units of half a stride from under the hip, lift is in pixels off the ground, and
# body dy is how far the hips drop (+) or rise (-). The "front" foot is the one that
# starts forward; the other foot does the same half a cycle later.
_WALK_KEYS = [
    (+1.0, 0, -1.0, 0, 0),    # contact: legs spread, both feet down
    (+0.5, 0, -0.6, 1, 1),    # down: weight lands, back foot peels off, hips lowest
    (0.0, 0, -0.1, 2, 0),     # passing: back foot swings under the body, lifted
    (-0.5, 0, +0.5, 1, -1),   # up: pushing off, hips highest
]


def walk_pose(frame, frames, stride, lift=2):
    """Foot and hip offsets for frame `frame` of a `frames`-long walk loop.

    Returns dict(front=(dx, dy), back=(dx, dy), body_dy) in pixels, relative to each foot's
    rest position under its hip (dy is negative when the foot is lifted). `stride` is the
    distance between the feet at contact; about a quarter of the character's height
    reads well. Draw the back leg first (darker), then the body, then the front leg.
    """
    keys = _WALK_KEYS + [(b, bl, f, fl, d) for f, fl, b, bl, d in _WALK_KEYS]  # second half: feet swap
    t = (frame / frames) * len(keys)
    i, f = int(t) % len(keys), t - int(t)
    a, b = keys[i], keys[(i + 1) % len(keys)]
    v = [x + (y - x) * f for x, y in zip(a, b)]
    half = stride / 2
    scale = lift / 2
    return {"front": (round(v[0] * half), -round(v[1] * scale)),
            "back": (round(v[2] * half), -round(v[3] * scale)),
            "body_dy": round(v[4])}


def load_png(path):
    """Load a PNG (e.g. a frame the artist drew, canvas/frame_000.png) as a Canvas, so
    it can be stamped and drawn on. Pixels under half opacity count as transparent."""
    img = Image.open(path).convert("RGBA")
    c = Canvas(img.width, img.height)
    data = img.tobytes()
    for y in range(img.height):
        for x in range(img.width):
            i = (y * img.width + x) * 4
            r, g, b, a = data[i], data[i + 1], data[i + 2], data[i + 3]
            if a >= 128:
                c.px[y][x] = "#%02x%02x%02x" % (r, g, b)
    return c


def part(width, height, draw):
    """Make a reusable part: `draw(c)` paints a fresh width x height Canvas.

        arm = part(3, 7, lambda c: c.rect(0, 0, 3, 7, SKIN[2]))
        canvas.stamp(arm, 10, 12)
    """
    c = Canvas(width, height)
    draw(c)
    return c
