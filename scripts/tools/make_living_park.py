"""Builds the living park: the app's background with the parts that move taken out, the sprites that move,
and where everything goes.

    python scripts/tools/make_living_park.py [--preview DIR]

Reads assets/landofwisdompark2.jfif (left as it is) and writes, in assets/park/:
    background.png    the scene at 1920x1080 with the Ferris wheel, the monorail train, the people and the
                      glowing atom painted into it taken out, the stretches of coaster track the wheel and
                      the atom used to hide painted in, and a garden with a knight's statue where the atom stood
    wheel_front.png   the wheel's legs, drawn over the turning wheel so it turns behind them
    sprites.png       every moving thing, in one sheet
    layout.json       where things are: the coaster and monorail tracks, the lamps, signs and bulbs, the
                      wheel, the lake, the skyline windows, the balloon seller's balloons

All positions are in the app's 1920x1080 coordinates. --preview writes check images of each step.
"""

import argparse
import json
import math
import os
import random

import cv2
import numpy as np
from PIL import Image, ImageDraw

SRC = os.path.join("assets", "landofwisdompark2.jfif")
OUT = os.path.join("assets", "park")
W, H = 1920, 1080


def is_magenta(a):
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    return (r > 140) & (b > 150) & (g < 120) & (r - g > 60)


def dist_to_segment(px, py, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    t = np.clip(((px - ax) * dx + (py - ay) * dy) / float(dx * dx + dy * dy), 0, 1)
    return np.hypot(px - (ax + t * dx), py - (ay + t * dy))


# ---------------------------------------------------------------------------------------------- curves

def catmull_rom(points, samples_per_segment=12):
    """A smooth curve through the points."""
    pts = [points[0]] + list(points) + [points[-1]]
    out = []
    for i in range(1, len(pts) - 2):
        p0, p1, p2, p3 = (np.array(p, float) for p in pts[i - 1:i + 3])
        for k in range(samples_per_segment):
            t = k / samples_per_segment
            out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t
                              + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t))
    out.append(np.array(points[-1], float))
    return out


def resample(points, step):
    """Points every `step` pixels along the curve."""
    pts = [np.array(p, float) for p in points]
    out = [pts[0]]
    carry = 0.0
    for a, b in zip(pts, pts[1:]):
        seg = np.linalg.norm(b - a)
        if seg < 1e-9:
            continue
        d = step - carry
        while d <= seg:
            out.append(a + (b - a) * (d / seg))
            d += step
        carry = seg - (d - step)
    return out


def snap_to_rail(img, pts, skip, reach=5):
    """Moves each point onto the nearest bright rail pixel along the curve's normal, then smooths, so the
    cars ride the painted rail exactly. Points where skip(p) is true are left where the curve puts them."""
    a = np.asarray(img).astype(int)
    rail = is_magenta(a)
    out = []
    for i, p in enumerate(pts):
        if skip(p):
            out.append(p)
            continue
        q = pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]
        n = np.array([-q[1], q[0]]) / (np.linalg.norm(q) + 1e-6)
        best = None
        for k in range(-reach, reach + 1):
            x, y = (p + n * k).round().astype(int)
            if 0 <= x < a.shape[1] and 0 <= y < a.shape[0] and rail[y, x]:
                if best is None or abs(k) < abs(best):
                    best = k
        out.append(p + n * best if best is not None else p)
    sm = []
    for i in range(len(out)):
        win = out[max(0, i - 3):i + 4]
        sm.append(sum(win) / len(win))
    return sm


# ---------------------------------------------------------------------------------------------- the coaster

# From its start on the left, behind the atom and the Ferris wheel, over the hill on the right, down and
# round the vertical loop
COASTER_WAYPOINTS = [
    (424, 322), (445, 316), (475, 316), (500, 328), (540, 364), (570, 405), (600, 436), (635, 451), (680, 455),
    (720, 445), (750, 400), (780, 340), (800, 310), (820, 304), (840, 311), (860, 340), (878, 368), (895, 392),
    (920, 404), (965, 412), (1010, 418), (1060, 421), (1110, 412),
    (1160, 392), (1210, 368), (1260, 350), (1310, 342),
    (1365, 345), (1400, 367), (1440, 380), (1462, 384), (1500, 372), (1540, 345), (1575, 318), (1600, 302),
    (1625, 293), (1650, 300), (1668, 318), (1673, 342), (1665, 375), (1640, 402), (1600, 422), (1560, 428),
    (1520, 424), (1488, 410), (1465, 385), (1450, 350), (1446, 310), (1455, 272), (1475, 250), (1495, 246),
    (1515, 255), (1530, 285), (1532, 320), (1522, 355), (1505, 383),
]
# The vertical loop starts at this waypoint; before it the track only rises, falls and turns flat
LOOP_FROM = (1465, 385)
# The flat turn on the right, where the track runs away from us and comes back: from the top of the right
# hill, round its far side, to the bottom
TURN = ((1640, 296), (1585, 426))
BEHIND_WHEEL = (1085, 1372)
BEHIND_ATOM = (905, 1050)


def distance_along(path, point, last=True):
    """How far along the path (in pixels) it comes by the point: the last time, or the first."""
    along, found = 0.0, None
    for i, p in enumerate(path):
        if i:
            along += math.dist(path[i - 1], p)
        if math.dist(p, point) < 4.0 and (last or found is None):
            found = along
    return round(found or 0.0, 1)


def loop_start(path):
    """How far along the coaster (in pixels) the vertical loop begins: the last time the path comes by
    LOOP_FROM (it passes close by earlier too, on its way to the hill)."""
    along, last = 0.0, 0.0
    for i, p in enumerate(path):
        if i:
            along += math.dist(path[i - 1], p)
        if math.dist(p, LOOP_FROM) < 4.0:
            last = along
    return round(last, 1)


def coaster_path(img):
    pts = resample(catmull_rom(COASTER_WAYPOINTS), 2.0)
    pts = snap_to_rail(img, pts, lambda p: (BEHIND_WHEEL[0] <= p[0] <= BEHIND_WHEEL[1] and p[1] < 430)
                       or (BEHIND_ATOM[0] <= p[0] <= BEHIND_ATOM[1]), reach=8)
    return [(float(p[0]), float(p[1])) for p in pts]


RAIL = (196, 84, 196)
RAIL_DARK = (96, 40, 126)
POST = (82, 48, 118)


def paint_track_behind_wheel(bg, path):
    """The stretch of coaster the wheel hid: posts down to the ground, a dark lower rail, a bright upper one."""
    img = Image.fromarray(bg)
    d = ImageDraw.Draw(img)
    run = [p for p in path if BEHIND_WHEEL[0] - 30 <= p[0] <= BEHIND_WHEEL[1] + 6 and p[1] < 430]
    for i, (x, y) in enumerate(run):
        if i % 14 == 0:
            d.rectangle([x - 1, y + 4, x + 1, 432], fill=POST)
    d.line([(x, y + 4) for x, y in run], fill=RAIL_DARK, width=3)
    d.line([(x, y) for x, y in run], fill=RAIL_DARK, width=3)
    d.line([(x, y) for x, y in run], fill=RAIL, width=1)
    return np.asarray(img).copy()


# ---------------------------------------------------------------------------------------------- the wheel

WHEEL = {"cx": 1237, "cy": 283, "rim": 140, "gondolas": 12, "spokes": 16, "turn_seconds": 40}
LEGS = [((1237, 296), (1163, 492)), ((1237, 296), (1330, 488))]
# The wheel's own gondolas that rest on the platform go with the rest of the wheel
WHEEL_EXTRA = [(1180, 418, 1205, 444), (1218, 428, 1252, 458), (1268, 418, 1300, 444)]
# Behind the top of the wheel: mountains and the city down to where the dark trees begin, and how far left
# the stretch of skyline is that fills it
SKYLINE_BOTTOM = 222
SKY_SHIFT = 340


def take_out_wheel(img):
    """Removes the wheel from the background. Returns (background, legs layer). Behind the wheel is dark sky
    and trees; a large median of the picture there has no thin lines left in it, so the hole is filled from it."""
    h, w = img.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy = WHEEL["cx"], WHEEL["cy"]
    disc = (np.hypot(xx - cx, yy - cy) <= WHEEL["rim"] + 22) & (yy < 432)
    for (x0, y0, x1, y1) in WHEEL_EXTRA:
        disc[y0:y1, x0:x1] = True

    legs_band = np.zeros_like(disc)
    for a, b in LEGS:
        legs_band |= dist_to_segment(xx, yy, a, b) <= 8
    legs = legs_band & (img.sum(axis=2) > 170) & ~is_magenta(img)

    hole = disc & ~legs
    # The median is taken with the wheel already roughed out (its bright pixels darkened to the sky), so
    # what is left of the hub and gondolas does not bleed in
    rough = img.copy()
    rough[hole & (img.sum(axis=2) > 200)] = (12, 12, 56)
    median = cv2.medianBlur(rough, 41)
    fill = cv2.GaussianBlur(median.astype(np.float32), (0, 0), 2.0)
    rng = np.random.default_rng(3)
    fill += rng.normal(0, 1.5, fill.shape).astype(np.float32)
    out = img.copy()
    out[hole] = np.clip(fill[hole], 0, 255).astype(np.uint8)

    # Above the treeline the wheel stood in front of the mountains and the city; the median would darken
    # them, so the skyline is carried on from just left of the wheel instead, blended in at the seams
    sky_rows = yy < SKYLINE_BOTTOM
    src_x = np.clip(xx - SKY_SHIFT, 0, w - 1)
    borrowed = img[yy, src_x].astype(np.float32)
    seam = np.clip((SKYLINE_BOTTOM - yy) / 10.0, 0, 1)[..., None]
    sky_fill = borrowed * seam + fill * (1 - seam)
    sel = hole & sky_rows
    out[sel] = np.clip(sky_fill[sel], 0, 255).astype(np.uint8)

    front = np.zeros((h, w, 4), np.uint8)
    front[..., :3] = img
    front[..., 3] = np.where(legs & (yy >= 290), 255, 0)
    return out, front


# ---------------------------------------------------------------------------------------------- the atom

# The glowing atom in the middle of the park is taken out and a garden with a knight's statue put in its place
ATOM_BOX = (826, 360, 1124, 668)
ATOM_SPHERES = [(977, 386), (905, 418), (1049, 423), (977, 448), (905, 482), (1049, 482), (849, 494), (1106, 494),
                (977, 516), (905, 537), (1050, 532), (977, 578)]
# The posts it stands on: (x, top, bottom)
ATOM_POSTS = [(848, 494, 575), (1107, 494, 575), (905, 537, 628), (1050, 532, 628), (977, 578, 686)]
# Below this the plaza is lit by its lamps, not by the atom
PLAZA_TOP = 604
# Where the middle of the statue's foot stands: the middle of the garden
STATUE_AT = (977, 612)


def atom_area(original):
    """The atom itself (spheres, rods, posts) and, around it, as far as its glow reaches."""
    x0, y0, x1, y1 = ATOM_BOX
    a = original.astype(int)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    solid = np.zeros(original.shape[:2], np.uint8)
    solid[y0:y1, x0:x1] = ((r > 120) & (r - b > 25) & (g > 60))[y0:y1, x0:x1]
    for cx, cy in ATOM_SPHERES:
        cv2.circle(solid, (cx, cy), 31, 1, -1)
    for px, top, bottom in ATOM_POSTS:
        cv2.rectangle(solid, (px - 7, top), (px + 7, bottom), 1, -1)
    solid = cv2.dilate(solid, np.ones((5, 5), np.uint8))
    return solid, cv2.dilate(solid, np.ones((61, 61), np.uint8))


def take_out_atom(bg, original):
    """Takes the atom out. Its glow is light added over the scene, so that is taken back off, leaving the
    trees, sky and lawn under it; the spheres, rods and posts themselves are filled in from round them."""
    solid, halo = atom_area(original)

    # The night scene is blue: red is under half the blue. What red there is over that is the atom's light,
    # whose colour is (1, 0.58, 0.13); above the plaza it all comes off
    f = bg.astype(np.float32)
    light = np.clip((f[..., 0] - 0.45 * f[..., 2]) / 0.94, 0, None)
    yy = np.arange(f.shape[0], dtype=np.float32)[:, None]
    light *= halo * np.clip((PLAZA_TOP + 16 - yy) / 40.0, 0, 1)
    clean = np.clip(f - light[..., None] * np.array([1.0, 0.58, 0.13], np.float32), 0, 255).astype(np.uint8)

    # On the plaza only the posts and the lowest sphere go; the rest there is lit ground
    holes = solid.copy()
    holes[PLAZA_TOP:, :] = 0
    for px, top, bottom in ATOM_POSTS:
        cv2.rectangle(holes, (px - 7, top), (px + 7, bottom), 1, -1)
    cv2.circle(holes, ATOM_SPHERES[-1], 31, 1, -1)
    # and the shadow of its middle post on the plaza
    cv2.ellipse(holes, (977, 678), (30, 22), 0, 0, 360, 1, -1)
    filled = cv2.inpaint(clean, holes, 4, cv2.INPAINT_TELEA)
    # Filled in on the scene's own pixel grid, three screen pixels to one
    small = cv2.resize(filled, (W // 3, H // 3), interpolation=cv2.INTER_AREA)
    blocky = cv2.resize(small, (W, H), interpolation=cv2.INTER_NEAREST)
    out = bg.copy()
    near = halo.astype(bool)
    out[near] = clean[near]
    out[holes.astype(bool)] = blocky[holes.astype(bool)]
    return out


def paint_track_behind_atom(bg, path):
    """The stretch of coaster the atom hid, like the one behind the wheel."""
    img = Image.fromarray(bg)
    d = ImageDraw.Draw(img)
    run = [p for p in path if 846 <= p[0] <= 1100]
    for i, (x, y) in enumerate(run):
        if i % 14 == 7:
            d.rectangle([x - 1, y + 4, x + 1, 470], fill=POST)
    d.line([(x, y + 4) for x, y in run], fill=RAIL_DARK, width=3)
    d.line([(x, y) for x, y in run], fill=RAIL_DARK, width=3)
    d.line([(x, y) for x, y in run], fill=RAIL, width=1)
    return np.asarray(img).copy()


SKY = (9, 9, 57)
TREE_MASS = (8, 29, 58)
TREE_TUFT = (13, 44, 64)
# A round tree: outline, shade under the crown, the crown, its lit top, the trunk
TREE = {"o": (6, 16, 38), "d": (11, 30, 58), "m": (16, 52, 66), "l": (24, 76, 78), "t": (62, 32, 82)}
# The trees behind the statue: (x, foot y, crown radius), in screen pixels
GARDEN_TREES = [(948, 540, 16), (1006, 536, 18), (912, 562, 24), (1044, 564, 23), (866, 588, 30), (1088, 584, 28)]


# Bushes round the statue and in front of it: (x, foot y, half width, half height), screen pixels
GARDEN_BUSHES = [
    (930, 592, 12, 8), (1024, 592, 12, 8), (897, 600, 15, 10), (1057, 600, 15, 10),
    (868, 614, 15, 10), (1086, 614, 15, 10), (905, 632, 16, 10), (1049, 632, 16, 10),
    (935, 642, 10, 7), (1019, 642, 10, 7),
]
# A round stone planter with a bush in it, where the paths meet in front of the garden: (x, foot y)
PLANTER = (977, 692)
# A bush, in the colours of the scene's hedges: outline, the dark under it, its leaves, their lit tops
BUSH = {"o": (6, 10, 40), "d": (10, 26, 58), "m": (18, 56, 70), "l": (32, 84, 82), "r": (48, 108, 94)}


def tree_line_top(x):
    """How high the dark trees behind the garden reach at x: a row of round crowns, higher on the left
    where the trees beside it are taller."""
    base = 418 - 22 * max(0.0, (900 - x) / 40.0)
    bumps = 7 * math.sin(x / 13.0) + 4 * math.sin(x / 5.3 + 1.0)
    return base - abs(bumps)


def round_tree(radius):
    """A round tree like the scene's own, on its three-pixel grid: a dark crown lit from above, a trunk."""
    r = max(3, radius // 3)
    w, h = 2 * r + 3, 2 * r + 5
    grid = [["." for _ in range(w)] for _ in range(h)]
    cx, cy = w // 2, r + 1
    for y in range(h):
        for x in range(w):
            d = math.hypot(x - cx, (y - cy) * 1.1)
            if d <= r:
                lit = math.hypot(x - cx + r * 0.25, y - cy + r * 0.35)
                grid[y][x] = "l" if lit < r * 0.35 else ("m" if lit < r * 0.8 and y <= cy + 1 else "d")
            elif d <= r + 1:
                grid[y][x] = "o"
    for y in range(cy + r - 1, h):
        if grid[y][cx] in ".od":
            grid[y][cx] = "t"
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = im.load()
    for y in range(h):
        for x in range(w):
            if grid[y][x] in TREE:
                px[x, y] = TREE[grid[y][x]] + (255,)
    return im.resize((w * 3, h * 3), Image.NEAREST)


def round_bush(half_w, half_h, seed):
    """A bush like the scene's hedges: a rounded body with a few soft lumps along its top, each column lit
    at the top and going dark towards the ground, with a thin dark edge. Drawn two screen pixels to one so
    the lumps come out round."""
    rng = random.Random(seed)
    rx, ry = max(4, half_w // 2), max(3, half_h // 2)
    w, h = 2 * rx + 3, 2 * ry + 3
    cx, base = w / 2 - 0.5, h - 2
    n = max(2, round(rx / 3))
    r = ry * 0.62
    lumps = [(cx - rx + r + (2 * rx - 2 * r) * k / (n - 1), 1 + r + rng.uniform(0, 1.2), r + rng.uniform(-0.3, 0.5))
             for k in range(n)]
    filled = [[False] * w for _ in range(h)]
    for y in range(h - 1):
        for x in range(w):
            body = ((x - cx) / (rx + 0.3)) ** 2 + ((y - (base - ry * 0.65)) / (ry * 0.75)) ** 2 <= 1.0
            lump = any(math.hypot(x - lx, y - ly) <= lr for lx, ly, lr in lumps)
            filled[y][x] = body or lump
    grid = [["." for _ in range(w)] for _ in range(h)]
    for x in range(w):
        col = [y for y in range(h) if filled[y][x]]
        if not col:
            continue
        top, bottom = col[0], col[-1]
        for y in col:
            depth = (y - top) / max(1.0, bottom - top)
            grid[y][x] = "r" if depth < 0.12 and x < cx else "l" if depth < 0.32 else "m" if depth < 0.62 else "d"
    out = [row[:] for row in grid]
    for y in range(h):
        for x in range(w):
            if grid[y][x] == "." and any(0 <= x + dx < w and 0 <= y + dy < h and grid[y + dy][x + dx] != "."
                                         for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = "o"
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = im.load()
    for y in range(h):
        for x in range(w):
            if out[y][x] in BUSH:
                px[x, y] = BUSH[out[y][x]] + (255,)
    return im.resize((w * 2, h * 2), Image.NEAREST)


# The old man who sells balloons, sitting by his cart behind the planter. He is drawn finer than the scene,
# two screen pixels to one, so his face shows. o outline, C his cap, c its brim, H his white hair, F his
# face, f its shade, E his eyes, B his beard, b its shade, S his red scarf, J his jacket, j its shade,
# A the hand holding a balloon's string, P his trousers, p their shade, K his shoes, W the stool
OLD_MAN = [
    "....oooo....",
    "...oCCCCo...",
    "..oCCCCCCo..",
    "..occccccco.",
    ".oHFFFFFFHo.",
    ".oHFEFFEFHo.",
    ".oHFFFfFFHo.",
    "..oBBffBBo..",
    "..obBBBBbo..",
    ".ooJSbbSJoo.",
    "oJJJJSSJJJJo",
    "oJjJJJJJJjJo",
    "oAjJJJJJJjFo",
    "ooPPPPPPPPoo",
    ".oPPPPPPPPo.",
    ".oPpoWWoPpo.",
    ".oPpoWWoPpo.",
    ".oKKo..oKKo.",
    ".oooo..oooo.",
]
# His cart: a box painted in the park's pink and teal, on two wheels, with the pole for the balloons.
# t its top, k the pink panels, q the teal stripe, n the shade under it, R the wheels, h their hubs
CART = [
    "........oo........",
    "........oo........",
    "........oo........",
    "........oo........",
    "........oo........",
    "oooooooooooooooooo",
    "otttttttttttttttto",
    "okkkkkqqqqqqkkkkko",
    "okkkkkqqqqqqkkkkko",
    "oqqqqqqqqqqqqqqqqo",
    "okkkkkqqqqqqkkkkko",
    "onnnnnnnnnnnnnnnno",
    "oooooooooooooooooo",
    "..oRRo......oRRo..",
    ".oRhhRo....oRhhRo.",
    "..oRRo......oRRo..",
    "...oo........oo...",
]
VENDOR = {
    "o": (22, 14, 30), "C": (110, 72, 50), "c": (72, 46, 34), "H": (224, 220, 232), "F": (226, 172, 132),
    "f": (184, 124, 96), "E": (40, 26, 30), "B": (236, 232, 242), "b": (172, 166, 188), "S": (186, 58, 72),
    "J": (92, 108, 80), "j": (60, 72, 54), "A": (226, 172, 132), "P": (66, 60, 86), "p": (46, 42, 62),
    "K": (44, 30, 28), "W": (122, 80, 54),
    "t": (238, 150, 214), "k": (204, 84, 176), "q": (70, 196, 210), "n": (104, 40, 100), "R": (40, 30, 48),
    "h": (150, 140, 170),
}
BALLOONS = [(244, 92, 170), (84, 210, 236), (250, 206, 96), (170, 120, 240), (255, 140, 96), (120, 230, 150)]
# The balloons the app draws, filled in when the stand is put up
BALLOON_STRINGS = []
# Where the man sits and where his cart stands: the middle of their feet
VENDOR_AT = (960, 676)
CART_AT = (1012, 684)


def grid_image(rows, colours, scale):
    im = Image.new("RGBA", (len(rows[0]), len(rows)), (0, 0, 0, 0))
    px = im.load()
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c in colours:
                px[x, y] = colours[c] + (255,)
    return im.resize((im.width * scale, im.height * scale), Image.NEAREST)


def balloon(d, cx, cy, colour):
    """One balloon, two screen pixels to one like the man: round, a shine at the top left, a knot below."""
    dark = tuple(int(c * 0.62) for c in colour)
    light = tuple(min(255, int(c + (255 - c) * 0.55)) for c in colour)
    pts = [(-1, -3), (0, -3), (1, -3), (-2, -2), (-1, -2), (0, -2), (1, -2), (2, -2), (-2, -1), (-1, -1), (0, -1),
           (1, -1), (2, -1), (-2, 0), (-1, 0), (0, 0), (1, 0), (2, 0), (-1, 1), (0, 1), (1, 1), (0, 2)]
    ring = {(-2, -3), (2, -3), (-3, -2), (3, -2), (-3, -1), (3, -1), (-3, 0), (3, 0), (-2, 1), (2, 1), (-1, 2),
            (1, 2), (-1, -4), (0, -4), (1, -4), (0, 3)}
    for x, y in ring:
        d.rectangle([cx + 2 * x, cy + 2 * y, cx + 2 * x + 1, cy + 2 * y + 1], fill=(22, 14, 30))
    for x, y in pts:
        c = dark if (x >= 1 and y >= 0) or y >= 1 else colour
        d.rectangle([cx + 2 * x, cy + 2 * y, cx + 2 * x + 1, cy + 2 * y + 1], fill=c)
    for x, y in ((-1, -2), (-1, -1)):
        d.rectangle([cx + 2 * x, cy + 2 * y, cx + 2 * x + 1, cy + 2 * y + 1], fill=light)


def balloon_sprite(colour):
    """A balloon on its own, for the app to move: its knot, where the string ties on, is at the middle of
    its bottom edge."""
    im = Image.new("RGBA", (14, 16), (0, 0, 0, 0))
    balloon(ImageDraw.Draw(im), 6, 8, colour)
    return im


def put_up_balloon_stand(img):
    """The cart and the old man on his stool beside it. Their balloons are not painted in: the app draws
    them swaying on their strings, from where this says they are tied (the top of the cart's pole and the
    man's hand) and where each floats at rest. Returns those, back to front."""
    cart = grid_image(CART, VENDOR, 2)
    cx, cy = CART_AT
    left, top = cx - cart.width // 2, cy - cart.height
    pole = (left + 17, top)
    bunch = [(-14, -30), (-3, -36), (8, -31), (17, -24), (-8, -20), (4, -22), (-18, -18)]
    balloons = []
    for i, (bx, by) in enumerate(sorted(bunch, key=lambda b: b[1])):
        balloons.append({"sprite": f"balloon{i % len(BALLOONS)}", "tied": list(pole),
                         "knot": [pole[0] + bx + 1, pole[1] + by + 8]})
    img.alpha_composite(cart, (left, top))
    man = grid_image(OLD_MAN, VENDOR, 2)
    mx, my = VENDOR_AT
    mleft, mtop = mx - man.width // 2, my - man.height
    hand = (mleft + 3, mtop + 25)
    balloons.append({"sprite": "balloon0", "tied": list(hand), "knot": [hand[0] - 5, hand[1] - 34]})
    img.alpha_composite(man, (mleft, mtop))
    BALLOON_STRINGS[:] = balloons


def plant_garden(bg, original):
    """Where the atom stood: the sky and a row of dark trees behind the garden, faded in at the edges of
    where the atom and its glow were, and into the lawn below."""
    img = Image.fromarray(bg)
    d = ImageDraw.Draw(img)
    for x in range(780, 1170, 3):
        top = int(tree_line_top(x)) // 3 * 3
        d.rectangle([x, 330, x + 2, top - 1], fill=SKY)
        d.rectangle([x, top, x + 2, 560], fill=TREE_MASS)
        d.rectangle([x, top, x + 2, top + 2], fill=TREE_TUFT)
    painted = np.asarray(img).astype(np.float32)
    solid, halo = atom_area(original)
    # All of it where the atom was, fading out over the glow round it
    weight = cv2.GaussianBlur(cv2.dilate(solid, np.ones((31, 31), np.uint8)).astype(np.float32), (0, 0), 12)
    yy = np.arange(H, dtype=np.float32)[:, None]
    weight *= np.clip((548 - yy) / 40.0, 0, 1)
    weight = np.clip(weight * 1.4, 0, 1)[..., None]
    return (painted * weight + bg.astype(np.float32) * (1 - weight)).astype(np.uint8)


# The pedestal: t the top of a ledge, c its front, u the shadow under the cap, m the shaft, n its shaded
# side, g the brass plaque, x the plaque's lettering
PEDESTAL = [
    "oooooooooooooooooooooo",
    "otttttttttttttttttttto",
    "occcccccccccccccccccco",
    "ouuuuuuuuuuuuuuuuuuuuo",
    "..ommmmmmmmmmmmmmmno..",
    "..ommmgggggggggmmmno..",
    "..ommmgxxxxxxxgmmmno..",
    "..ommmgggggggggmmmno..",
    "..ommmmmmmmmmmmmmmno..",
    "..ommmmmmmmmmmmmmmno..",
    ".otttttttttttttttttto.",
    ".occcccccccccccccccco.",
    "oooooooooooooooooooooo",
]
# Stone for the knight: l lit from the left, s, d in shade, k the dark of the visor; the pedestal's own
STONE = {
    "o": (20, 14, 34), "l": (140, 134, 174), "s": (100, 94, 138), "d": (66, 60, 100), "k": (28, 22, 44),
    "t": (96, 84, 130), "c": (70, 60, 102), "u": (40, 32, 62), "m": (78, 68, 110), "n": (54, 44, 80),
    "g": (190, 146, 74), "x": (120, 86, 44),
}
# The paved round the statue stands on
PAVING, PAVING_EDGE = (74, 58, 96), (46, 34, 66)


def knight():
    """A knight in armour, standing guard: a spear upright in his right hand, a shield on his left arm with
    a chevron on it, a plume on his helm. Drawn on the scene's grid, one cell to three screen pixels, lit
    from the left, with a dark outline round it all."""
    w, h = 23, 38
    g = [["." for _ in range(w)] for _ in range(h)]

    def put(x, y, c):
        if 0 <= x < w and 0 <= y < h:
            g[y][x] = c

    def rect(x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                put(x, y, c)

    def shaded(x0, y0, x1, y1):
        """A block of armour: its left column lit, its right column in shade."""
        rect(x0, y0, x1, y1, "s")
        rect(x0, y0, x0, y1, "l")
        rect(x1, y0, x1, y1, "d")

    # The spear, its head a leaf at the top
    rect(4, 5, 4, 36, "d")
    put(4, 1, "l")
    rect(3, 2, 5, 3, "l")
    put(5, 3, "s")
    put(4, 4, "s")
    rect(3, 5, 5, 5, "s")
    # The plume, sweeping back off the top of the helm
    for x, y in ((11, 1), (12, 1), (12, 2), (13, 2), (13, 3), (14, 3), (14, 4)):
        put(x, y, "s")
    put(11, 2, "l")
    put(12, 3, "l")
    # The great helm, its visor slit and breaths
    shaded(9, 3, 14, 10)
    put(9, 3, ".")
    put(14, 3, ".")
    rect(10, 6, 13, 6, "k")
    put(13, 8, "k")
    put(13, 9, "k")
    rect(11, 4, 11, 9, "l")
    # Gorget, shoulders, breastplate with its ridge, belt, the plates over the hips
    rect(10, 11, 13, 11, "d")
    shaded(6, 12, 17, 13)
    shaded(8, 14, 15, 20)
    rect(11, 14, 11, 19, "l")
    rect(8, 21, 15, 21, "d")
    shaded(7, 22, 16, 24)
    # Legs, knees, feet
    shaded(8, 25, 10, 34)
    shaded(13, 25, 15, 34)
    put(9, 29, "l")
    put(14, 29, "l")
    shaded(7, 35, 10, 36)
    shaded(13, 35, 16, 36)
    # His right arm down to the spear, the gauntlet round the shaft
    shaded(6, 13, 7, 17)
    shaded(5, 17, 6, 20)
    rect(3, 19, 5, 21, "s")
    put(3, 19, "l")
    # The shield, a heater with a chevron, in front of his left side
    rows = {14: (14, 21), 15: (14, 21), 16: (14, 21), 17: (14, 21), 18: (14, 21), 19: (14, 21), 20: (14, 21),
            21: (14, 21), 22: (15, 20), 23: (15, 20), 24: (16, 19), 25: (16, 19), 26: (17, 18)}
    for y, (x0, x1) in rows.items():
        rect(x0, y, x1, y, "s")
        put(x0, y, "l")
        put(x1, y, "d")
    rect(14, 14, 21, 14, "l")
    for i, y in enumerate(range(17, 21)):
        put(17 - i, y, "l")
        put(18 + i, y, "l")
    # The outline: every empty cell next to the figure
    out = [row[:] for row in g]
    for y in range(h):
        for x in range(w):
            if g[y][x] == "." and any(0 <= x + dx < w and 0 <= y + dy < h and g[y + dy][x + dx] != "."
                                      for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = "o"
    return ["".join(row) for row in out]


def statue():
    """The knight on his pedestal, at three screen pixels to one like the rest of the scene."""
    figure = knight()
    w = max(len(PEDESTAL[0]), len(figure[0]))
    rows = [r.center(w, ".") for r in figure] + [r.center(w, ".") for r in PEDESTAL]
    # The figure's last row is its outline under the feet; the pedestal's cap takes its place
    rows = rows[:len(figure) - 1] + rows[len(figure):]
    im = Image.new("RGBA", (w, len(rows)), (0, 0, 0, 0))
    px = im.load()
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c in STONE:
                px[x, y] = STONE[c] + (255,)
    return im.resize((w * 3, len(rows) * 3), Image.NEAREST)


def put_up_statue(bg):
    """Round trees in the garden, in the middle of it the statue on a paved round with a soft shadow at its
    foot, bushes round it, and in front the old man selling balloons behind a planter."""
    img = Image.fromarray(bg).convert("RGBA")
    for x, foot, radius in GARDEN_TREES:
        tree = round_tree(radius)
        img.alpha_composite(tree, (x - tree.width // 2, foot - tree.height))
    sx, sy = STATUE_AT
    d = ImageDraw.Draw(img)
    d.ellipse([sx - 57, sy - 17, sx + 57, sy + 14], fill=PAVING_EDGE)
    d.ellipse([sx - 54, sy - 15, sx + 54, sy + 11], fill=PAVING)
    shadow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).ellipse([sx - 40, sy - 8, sx + 46, sy + 6], fill=(10, 6, 20, 130))
    img = Image.alpha_composite(img, shadow)

    def at_foot(im, x, foot):
        return lambda: img.alpha_composite(im, (x - im.width // 2, foot - im.height))

    def planter():
        px_, py_ = PLANTER
        d = ImageDraw.Draw(img)
        d.ellipse([px_ - 24, py_ - 13, px_ + 24, py_ + 6], fill=PAVING_EDGE)
        d.rectangle([px_ - 24, py_ - 4, px_ + 24, py_ - 3], fill=PAVING_EDGE)
        d.ellipse([px_ - 21, py_ - 14, px_ + 21, py_ - 1], fill=PAVING)
        d.ellipse([px_ - 18, py_ - 12, px_ + 18, py_ - 3], fill=(30, 22, 30))
        bush = round_bush(20, 8, 99)
        img.alpha_composite(bush, (px_ - bush.width // 2, py_ - 4 - bush.height))

    # Back to front: whatever stands further forward is drawn over what is behind it
    things = [(sy, at_foot(statue(), sx, sy)), (VENDOR_AT[1], lambda: put_up_balloon_stand(img)), (PLANTER[1], planter)]
    things += [(foot, at_foot(round_bush(hw, hh, i), x, foot)) for i, (x, foot, hw, hh) in enumerate(GARDEN_BUSHES)]
    for _, draw in sorted(things, key=lambda t: t[0]):
        draw()
    return np.asarray(img.convert("RGB")).copy()


# ---------------------------------------------------------------------------------------------- the monorail

TRAIN_BOX = (1414, 506, 1641, 540)
TRAIN_CARS = [(1418, 1456), (1456, 1492), (1492, 1541), (1541, 1570), (1570, 1637)]
TRAIN_TOP, TRAIN_BOTTOM = 509, 541
# The top of the rail, from where it leaves the screen on the right along the far side, round the left end
# and back along the near side to the right
MONORAIL_WAYPOINTS = [
    (1960, 538), (1800, 538), (1640, 539), (1480, 540), (1410, 543), (1350, 552), (1305, 568), (1275, 595),
    (1262, 630), (1265, 670), (1282, 708), (1318, 740), (1370, 765), (1440, 786), (1530, 797), (1650, 801),
    (1800, 803), (1960, 804),
]


def take_out_train(img):
    """Removes the parked train: each row is blended between the scene just left and right of it, which
    carries the rail and the dark trees behind straight through."""
    x0, y0, x1, y1 = TRAIN_BOX
    out = img.copy().astype(np.float32)
    for y in range(y0, y1):
        left = img[y, x0 - 3:x0].astype(np.float32).mean(axis=0)
        right = img[y, x1:x1 + 3].astype(np.float32).mean(axis=0)
        for x in range(x0, x1):
            t = (x - x0) / float(x1 - x0)
            out[y, x] = left * (1 - t) + right * t
    rng = np.random.default_rng(5)
    out[y0:y1, x0:x1] += rng.normal(0, 1.5, (y1 - y0, x1 - x0, 3)).astype(np.float32)
    return np.clip(out, 0, 255).astype(np.uint8)


# ---------------------------------------------------------------------------------------------- the people

# Everyone painted into the scene, with their shadow: (x0, y0, x1, y1, how). "patch" covers them with the
# ground beside them; "counter" with the same shop front a little to the side; "iso" and "iso_up" with the
# shop front further along its slanted wall (down to the right, or up to the right)
PAINTED_PEOPLE = [
    (341, 559, 358, 597, "patch"), (312, 582, 330, 622, "patch"), (375, 635, 393, 681, "patch"),
    (597, 534, 613, 570, "patch"), (637, 530, 655, 570, "patch"), (609, 652, 627, 690, "patch"),
    (431, 769, 452, 815, "patch"), (532, 757, 552, 810, "patch"), (555, 734, 573, 778, "patch"),
    (629, 757, 652, 805, "patch"), (444, 845, 463, 890, "patch"), (655, 863, 677, 913, "patch"),
    (968, 767, 1000, 813, "patch"), (1049, 805, 1068, 855, "patch"), (1004, 845, 1023, 893, "patch"),
    (394, 934, 416, 983, "patch"), (934, 937, 976, 986, "patch"),
    (1074, 802, 1093, 855, "iso"), (116, 699, 131, 749, "iso"), (720, 722, 738, 766, "counter"),
    (822, 727, 840, 771, "counter"),
]


def take_out_people(img):
    """Covers every painted person with a patch from beside them: the candidate patch whose edges match the
    surroundings best. When none fits, the spot is filled in from around it."""
    out = img.copy()
    boxes = [b[:4] for b in PAINTED_PEOPLE]

    def overlaps(x0, y0, x1, y1):
        return any(x0 < bx1 and bx0 < x1 and y0 < by1 and by0 < y1 for bx0, by0, bx1, by1 in boxes)

    fill_mask = np.zeros(img.shape[:2], np.uint8)
    for x0, y0, x1, y1, how in PAINTED_PEOPLE:
        w, h = x1 - x0, y1 - y0
        if how == "patch":
            offsets = [(dx * (w + gap), dy) for dx in (-1, 1) for gap in (2, 6, 12, 20, 32, 46) for dy in (-14, -6, 0, 6, 14)]
        elif how == "counter":
            offsets = [(dx * (w + gap), 0) for dx in (-1, 1) for gap in range(1, 30)]
        else:
            slope = 0.5 if how == "iso" else -0.5
            offsets = [(dx, int(round(dx * slope + e))) for dx in list(range(-60, -w)) + list(range(w + 1, 60)) for e in (-1, 0, 1)]
        ring = 3
        dest = img[y0 - ring:y1 + ring, x0 - ring:x1 + ring].astype(int)
        best, best_cost = None, None
        for ox, oy in offsets:
            sx0, sy0 = x0 + ox, y0 + oy
            if sx0 - ring < 0 or sx0 + w + ring > img.shape[1] or sy0 - ring < 0 or sy0 + h + ring > img.shape[0]:
                continue
            if overlaps(sx0 - ring, sy0 - ring, sx0 + w + ring, sy0 + h + ring):
                continue
            src = img[sy0 - ring:sy0 + h + ring, sx0 - ring:sx0 + w + ring].astype(int)
            border = np.ones(src.shape[:2], bool)
            border[ring:-ring, ring:-ring] = False
            cost = np.abs(src - dest)[border].sum()
            if best_cost is None or cost < best_cost:
                best, best_cost = (sx0, sy0), cost
        if best is None:
            fill_mask[y0:y1, x0:x1] = 255
            continue
        sx0, sy0 = best
        patch = img[sy0:sy0 + h, sx0:sx0 + w].astype(np.float32)
        # soften the patch's edge into what is around it
        alpha = np.ones((h, w), np.float32)
        for k in range(2):
            v = (k + 1) / 3.0
            alpha[k, :] = np.minimum(alpha[k, :], v)
            alpha[-1 - k, :] = np.minimum(alpha[-1 - k, :], v)
            alpha[:, k] = np.minimum(alpha[:, k], v)
            alpha[:, -1 - k] = np.minimum(alpha[:, -1 - k], v)
        region = out[y0:y1, x0:x1].astype(np.float32)
        out[y0:y1, x0:x1] = (patch * alpha[..., None] + region * (1 - alpha[..., None])).astype(np.uint8)
    if fill_mask.any():
        out = cv2.inpaint(out, fill_mask, 4, cv2.INPAINT_TELEA)
    return out


# ---------------------------------------------------------------------------------------------- lights

def find_lamps(img):
    """Street lamps: small, very bright warm spots on the ground (not in the sky, not where the atom was)."""
    a = img.astype(int)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    hot = ((r > 235) & (g > 200) & (b < 190)).astype(np.uint8)
    n, lab, stats, cent = cv2.connectedComponentsWithStats(hot)
    out = []
    x0, y0, x1, y1 = ATOM_BOX[0], ATOM_BOX[1], ATOM_BOX[2], PLAZA_TOP + 30
    for i in range(1, n):
        x, y, w, h, area = stats[i]
        cx, cy = cent[i]
        if 3 <= area <= 200 and w <= 20 and h <= 20 and cy > 470 and not (x0 <= cx <= x1 and y0 <= cy <= y1):
            out.append([round(float(cx), 1), round(float(cy), 1)])
    return out


# Neon signs that flicker now and then: (x0, y0, x1, y1, glow colour)
SIGNS = [
    (80, 604, 152, 656, (255, 190, 110)),      # CHEESE
    (262, 498, 302, 552, (255, 120, 210)),     # CHEESE, upright
    (704, 683, 760, 707, (255, 214, 120)),     # SNACK
    (788, 686, 850, 710, (255, 214, 120)),     # SNACK
    (1064, 748, 1118, 792, (255, 150, 90)),    # SNACK by the lake path
]


def carousel_bulbs():
    """Bulbs round the carousel's canopy, along its scalloped rim."""
    out = []
    for i in range(16):
        t = i / 15.0
        x = 404 + (574 - 404) * t
        y = 558 + 34 * (1 - (2 * t - 1) ** 2)
        out.append([round(x, 1), round(y, 1)])
    return out


LAKE = [
    [(1292, 606), (1420, 574), (1560, 572), (1700, 576), (1920, 580), (1920, 790), (1660, 792), (1500, 786),
     (1390, 764), (1310, 718), (1288, 660)],
    [(1380, 822), (1600, 818), (1920, 822), (1920, 980), (1740, 1000), (1500, 990), (1420, 930)],
]


def skyline_windows(img, count=110, seed=7):
    """Spots on the dark city buildings along the horizon where a window can light up."""
    a = img.astype(int)
    rng = random.Random(seed)
    out = []
    tries = 0
    while len(out) < count and tries < 20000:
        tries += 1
        x = rng.randrange(0, W)
        y = rng.randrange(150, 212)
        if 1040 <= x <= 1440:
            continue                       # the wheel is in front there
        p = a[y, x]
        above = a[max(0, y - 10), x]
        building = 30 < p.sum() < 150 and p[2] >= p[0]
        if building and above.sum() < 260:
            out.append([x, y])
    return out


# ---------------------------------------------------------------------------------------------- sprites

def coaster_car():
    """A coaster car facing right, with two riders, 10x7."""
    im = Image.new("RGBA", (10, 7), (0, 0, 0, 0))
    px = im.load()
    body, stripe, edge, wheel = (214, 70, 168), (80, 220, 230), (110, 30, 96), (30, 20, 34)
    for x, c in ((2, (40, 26, 30)), (3, (255, 214, 180)), (6, (92, 56, 34)), (7, (230, 176, 136))):
        px[x, 0] = c + (255,)
        px[x, 1] = c + (255,)
    for x in range(10):
        px[x, 2] = body + (255,)
        px[x, 3] = stripe + (255,) if 0 < x < 9 else body + (255,)
        px[x, 4] = edge + (255,)
    px[9, 2] = (255, 150, 220, 255)
    for x in (1, 8):
        px[x, 5] = wheel + (255,)
        px[x, 6] = wheel + (255,)
    return im


def gondola(roof, body, glass):
    """A Ferris wheel gondola hanging from its pin, 9x9."""
    im = Image.new("RGBA", (9, 9), (0, 0, 0, 0))
    px = im.load()
    out = (20, 14, 30)
    px[4, 0] = (150, 140, 170, 255)
    for x in range(1, 8):
        px[x, 1] = roof + (255,)
    for x in range(0, 9):
        px[x, 2] = roof + (255,)
    for y in (3, 4, 5):
        px[0, y] = body + (255,)
        px[8, y] = body + (255,)
        for x in range(1, 8):
            px[x, y] = (glass if 2 <= x <= 6 and y < 5 else body) + (255,)
    for x in range(0, 9):
        px[x, 6] = body + (255,)
    for x in range(1, 8):
        px[x, 7] = out + (255,)
    return im


def glow(size=64):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    px = im.load()
    c = (size - 1) / 2
    for y in range(size):
        for x in range(size):
            d = math.hypot(x - c, y - c) / c
            if d < 1:
                px[x, y] = (255, 255, 255, int(255 * (1 - d) ** 2.2))
    return im


def sparkle():
    im = Image.new("RGBA", (5, 5), (0, 0, 0, 0))
    px = im.load()
    for (x, y, a) in ((2, 2, 255), (1, 2, 160), (3, 2, 160), (2, 1, 160), (2, 3, 160), (0, 2, 60), (4, 2, 60), (2, 0, 60), (2, 4, 60)):
        px[x, y] = (255, 255, 255, a)
    return im


def bird(frame):
    im = Image.new("RGBA", (7, 3), (0, 0, 0, 0))
    px = im.load()
    c = (24, 14, 34, 255)
    shape = [(0, 0), (1, 1), (2, 1), (3, 2), (4, 1), (5, 1), (6, 0)] if frame == 0 else [(0, 2), (1, 1), (2, 1), (3, 2), (4, 1), (5, 1), (6, 2)]
    for x, y in shape:
        px[x, y] = c
    return im


def dot():
    im = Image.new("RGBA", (1, 1), (255, 255, 255, 255))
    return im


def crop_to_content(rgba):
    """The part of a full-scene layer that has anything in it, and where that part goes."""
    ys, xs = np.where(rgba[..., 3] > 0)
    x0, y0, x1, y1 = int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1
    return rgba[y0:y1, x0:x1].copy(), [x0, y0]


class Atlas:
    """Packs sprites in rows into one sheet and remembers where each went."""

    def __init__(self, width=512):
        self.width = width
        self.items = []

    def add(self, name, img):
        self.items.append((name, img))

    def build(self):
        x = y = row_h = 0
        rects = {}
        for name, img in self.items:
            w, h = img.size
            if x + w + 1 > self.width:
                x, y, row_h = 0, y + row_h + 1, 0
            rects[name] = [x, y, w, h]
            x += w + 1
            row_h = max(row_h, h)
        sheet = Image.new("RGBA", (self.width, y + row_h + 1), (0, 0, 0, 0))
        for name, img in self.items:
            sheet.paste(img, tuple(rects[name][:2]))
        return sheet, rects


# ---------------------------------------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", metavar="DIR")
    args = ap.parse_args()
    original = np.asarray(Image.open(SRC).convert("RGB").resize((W, H), Image.LANCZOS)).copy()

    coaster = coaster_path(Image.fromarray(original))
    bg, legs = take_out_wheel(original)
    bg = paint_track_behind_wheel(bg, coaster)
    bg = take_out_train(bg)
    bg = take_out_people(bg)
    bg = take_out_atom(bg, original)
    bg = plant_garden(bg, original)
    bg = paint_track_behind_atom(bg, coaster)
    bg = put_up_statue(bg)

    # The front layers are kept to the area they cover, so drawing them is not a whole extra screen
    legs, legs_at = crop_to_content(legs)

    atlas = Atlas()
    atlas.add("coaster_car", coaster_car())
    atlas.add("gondola_teal", gondola((60, 200, 210), (40, 110, 150), (140, 240, 220)))
    atlas.add("gondola_pink", gondola((214, 84, 196), (112, 42, 124), (246, 176, 234)))
    atlas.add("glow", glow())
    atlas.add("sparkle", sparkle())
    atlas.add("bird0", bird(0))
    atlas.add("bird1", bird(1))
    atlas.add("dot", dot())
    for i, colour in enumerate(BALLOONS):
        atlas.add(f"balloon{i}", balloon_sprite(colour))
    for i, (cx0, cx1) in enumerate(TRAIN_CARS):
        car = Image.fromarray(original[TRAIN_TOP:TRAIN_BOTTOM, cx0:cx1]).convert("RGBA")
        # What looks the same as the scene with the train taken out is the scene, not the car
        arr = np.asarray(car).copy()
        behind = bg[TRAIN_TOP:TRAIN_BOTTOM, cx0:cx1].astype(int)
        same = np.abs(arr[..., :3].astype(int) - behind).sum(axis=2) < 46
        arr[..., 3] = np.where(same, 0, 255)
        atlas.add(f"train{i}", Image.fromarray(arr))
    sheet, rects = atlas.build()

    layout = {
        "size": [W, H],
        "atlas": "sprites.png",
        "sprites": rects,
        "background": "background.png",
        "wheel": dict(WHEEL, front="wheel_front.png", front_at=legs_at),
        "coaster": {"points": [[round(x, 1), round(y, 1)] for x, y in coaster], "cars": 4,
                    "loop_from": loop_start(coaster),
                    "turn": [distance_along(coaster, TURN[0], last=False), distance_along(coaster, TURN[1], last=False)]},
        "monorail": {"points": [[round(float(x), 1), round(float(y), 1)] for x, y in resample(catmull_rom(MONORAIL_WAYPOINTS), 3.0)],
                     "cars": [f"train{i}" for i in range(len(TRAIN_CARS))]},
        "lamps": find_lamps(original),
        "signs": [list(s[:4]) + [list(s[4])] for s in SIGNS],
        "carousel": carousel_bulbs(),
        "lake": [[list(p) for p in poly] for poly in LAKE],
        "windows": skyline_windows(original),
        "sky": [120, 30, 1800, 150],
        "balloons": BALLOON_STRINGS,
    }

    os.makedirs(OUT, exist_ok=True)
    Image.fromarray(bg).save(os.path.join(OUT, "background.png"))
    Image.fromarray(legs).save(os.path.join(OUT, "wheel_front.png"))
    sheet.save(os.path.join(OUT, "sprites.png"))
    with open(os.path.join(OUT, "layout.json"), "w") as f:
        json.dump(layout, f, separators=(",", ":"))

    if args.preview:
        os.makedirs(args.preview, exist_ok=True)
        b = Image.fromarray(bg)
        b.crop((1040, 100, 1440, 520)).resize((800, 840), Image.NEAREST).save(os.path.join(args.preview, "wheel_removed.png"))
        b.crop((1380, 480, 1700, 580)).resize((960, 300), Image.NEAREST).save(os.path.join(args.preview, "train_removed.png"))
        b.crop((80, 520, 1120, 1000)).save(os.path.join(args.preview, "people_removed.png"))
        b.crop((780, 330, 1180, 720)).resize((800, 780), Image.NEAREST).save(os.path.join(args.preview, "statue.png"))
        sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(os.path.join(args.preview, "sprites_x3.png"))
        net = b.copy()
        d = ImageDraw.Draw(net)
        for p in layout["lamps"]:
            d.ellipse([p[0] - 5, p[1] - 5, p[0] + 5, p[1] + 5], outline=(0, 255, 0))
        for p in layout["carousel"]:
            d.point(tuple(p), fill=(0, 255, 255))
        for s in layout["signs"]:
            d.rectangle(s[:4], outline=(255, 0, 255))
        for poly in layout["lake"]:
            d.polygon([tuple(p) for p in poly], outline=(0, 200, 255))
        for p in layout["windows"]:
            d.point(tuple(p), fill=(255, 255, 255))
        d.line([tuple(p) for p in layout["coaster"]["points"]], fill=(255, 128, 0), width=1)
        d.line([tuple(p) for p in layout["monorail"]["points"]], fill=(0, 255, 128), width=1)
        net.resize((1440, 810)).save(os.path.join(args.preview, "layout_check.png"))


if __name__ == "__main__":
    main()
