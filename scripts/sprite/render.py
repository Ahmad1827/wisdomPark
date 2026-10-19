"""Renders sprite.py (in the current directory) into a spritesheet and checks it.

sprite.py defines:
    WIDTH, HEIGHT = 32, 32                      # one frame
    ANIMATIONS = {"idle": {"frames": 4, "fps": 6}, "walk": {"frames": 6, "fps": 10}}
    PALETTE = ["#1a1c2c", ...]                  # optional: every colour used must be in it
    def draw(c, anim, frame): ...               # paint frame `frame` of `anim` on Canvas c

Writes:
    sheet.png      one row per animation, one column per frame, 1:1 pixels
    sheet.json     frame rectangles, durations and animation tags (Aseprite-style)
    preview.png    the sheet enlarged with a pixel grid, plus 1x and 2x copies, for looking at
    frames/NNN.png every frame on its own, all animations one after another (the app's timeline order)
and prints a lint report. Exit code 1 means sprite.py itself failed.
"""

import json
import os
import sys
import traceback

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())
from spritelib import Canvas  # noqa: E402

ZOOM = 8
MAX_ORPHANS_LISTED = 6


def load_sprite():
    import importlib.util
    spec = importlib.util.spec_from_file_location("sprite", os.path.join(os.getcwd(), "sprite.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    for name in ("WIDTH", "HEIGHT", "ANIMATIONS", "draw"):
        if not hasattr(mod, name):
            raise AttributeError(f"sprite.py must define {name}")
    return mod


def render_all(mod):
    frames = {}
    for anim, spec in mod.ANIMATIONS.items():
        frames[anim] = []
        for i in range(int(spec["frames"])):
            c = Canvas(mod.WIDTH, mod.HEIGHT)
            mod.draw(c, anim, i)
            frames[anim].append(c)
    return frames


def build_sheet(mod, frames):
    w, h = mod.WIDTH, mod.HEIGHT
    cols = max(len(f) for f in frames.values())
    sheet = Image.new("RGBA", (cols * w, len(frames) * h), (0, 0, 0, 0))
    meta_frames, tags, index = {}, [], 0
    for row, (anim, canvases) in enumerate(frames.items()):
        fps = float(mod.ANIMATIONS[anim].get("fps", 8))
        tags.append({"name": anim, "from": index, "to": index + len(canvases) - 1, "direction": "forward"})
        for col, c in enumerate(canvases):
            sheet.paste(c.to_image(), (col * w, row * h))
            meta_frames[f"{anim}_{col}"] = {
                "frame": {"x": col * w, "y": row * h, "w": w, "h": h},
                "duration": round(1000 / fps),
            }
            index += 1
    meta = {"frames": meta_frames,
            "meta": {"image": "sheet.png", "size": {"w": sheet.width, "h": sheet.height},
                     "frameSize": {"w": w, "h": h}, "frameTags": tags}}
    return sheet, meta


def build_preview(mod, sheet, frames):
    """Enlarged sheet on a checkerboard with a faint pixel grid, frame borders and
    labels, then the sheet at 2x and 1x underneath so it can be judged at real size."""
    w, h = mod.WIDTH, mod.HEIGHT
    label_w = 60
    big = sheet.resize((sheet.width * ZOOM, sheet.height * ZOOM), Image.NEAREST)
    small2 = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
    pad = 12
    motion = [motion_panel(anim, canvases) for anim, canvases in frames.items() if len(canvases) > 1]
    motion_w = sum(m.width + pad for m in motion)
    low_w = small2.width + sheet.width + pad * 2 + motion_w
    W = label_w + max(big.width, low_w) + pad
    H = big.height + pad * 3 + max([small2.height] + [m.height for m in motion]) + 14
    out = Image.new("RGBA", (W, H), (40, 40, 48, 255))
    d = ImageDraw.Draw(out)

    # checkerboard behind the enlarged sheet so transparency is visible
    for y in range(0, big.height, ZOOM):
        for x in range(0, big.width, ZOOM):
            shade = (92, 92, 104, 255) if ((x // ZOOM // 2 + y // ZOOM // 2) % 2) else (78, 78, 90, 255)
            d.rectangle([label_w + x, y, label_w + x + ZOOM - 1, y + ZOOM - 1], fill=shade)
    out.alpha_composite(big, (label_w, 0))
    for x in range(0, big.width + 1, ZOOM):
        strong = (x // ZOOM) % w == 0
        d.line([label_w + x, 0, label_w + x, big.height], fill=(255, 255, 255, 150 if strong else 28), width=1)
    for y in range(0, big.height + 1, ZOOM):
        strong = (y // ZOOM) % h == 0
        d.line([label_w, y, label_w + big.width, y], fill=(255, 255, 255, 150 if strong else 28), width=1)
    for row, (anim, canvases) in enumerate(frames.items()):
        d.text((4, row * h * ZOOM + 4), anim, fill=(255, 255, 255, 255))
        for col in range(len(canvases)):
            d.text((label_w + col * w * ZOOM + 3, row * h * ZOOM + 2), str(col), fill=(255, 230, 120, 255))

    y0 = big.height + pad
    d.text((4, y0), "2x / 1x", fill=(255, 255, 255, 255))
    out.alpha_composite(small2, (label_w, y0))
    out.alpha_composite(sheet, (label_w + small2.width + pad, y0))
    x = label_w + small2.width + sheet.width + pad * 2
    for m in motion:
        out.alpha_composite(m, (x, y0))
        x += m.width + pad
    return out


MOTION_ZOOM = 6
MOTION_COLORS = [(255, 90, 90), (255, 200, 60), (120, 230, 90), (60, 200, 255), (160, 120, 255), (255, 120, 220),
                 (255, 255, 255), (255, 150, 60)]


def motion_panel(anim, canvases):
    """Every frame of one animation laid over the others, one colour per frame, so how
    far the limbs actually travel (and whether anything drifts) is visible at a glance.
    The dashed line is the lowest drawn row of frame 0: the ground."""
    w, h = canvases[0].w, canvases[0].h
    Z = MOTION_ZOOM
    panel = Image.new("RGBA", (w * Z, h * Z + 14), (24, 24, 30, 255))
    for i, c in enumerate(canvases):
        r, g, b = MOTION_COLORS[i % len(MOTION_COLORS)]
        layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        # Outline only: filled silhouettes would blend into one blob.
        edge = [[p is not None and any(c.get(x + dx, y + dy) is None for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                 for x, p in enumerate(row)] for y, row in enumerate(c.px)]
        layer.putdata([(r, g, b, 230) if e else (0, 0, 0, 0) for row in edge for e in row])
        panel.alpha_composite(layer.resize((w * Z, h * Z), Image.NEAREST), (0, 14))
    d = ImageDraw.Draw(panel)
    ground = max((y for y in range(h) for x in range(w) if canvases[0].px[y][x] is not None), default=h - 1)
    gy = 14 + ground * Z + Z
    for x in range(0, w * Z, 6):
        d.line([x, gy, x + 2, gy], fill=(255, 255, 255, 160))
    d.text((2, 1), anim + " motion", fill=(255, 255, 255, 255))
    for i in range(len(canvases)):
        d.rectangle([w * Z - 8 * (len(canvases) - i), 3, w * Z - 8 * (len(canvases) - i) + 5, 8],
                    fill=MOTION_COLORS[i % len(MOTION_COLORS)])
    return panel


def write_frames(frames):
    os.makedirs("frames", exist_ok=True)
    for old in os.listdir("frames"):
        if old.endswith(".png"):
            os.remove(os.path.join("frames", old))
    n = 0
    for canvases in frames.values():
        for c in canvases:
            c.to_image().save(os.path.join("frames", "%03d.png" % n))
            n += 1


def timeline_changes(frames):
    """In the app's chat, last/ holds the frames as they were last sent to the timeline.
    Says which frames this render would change, so a rewrite of sprite.py cannot quietly
    alter a frame the artist asked to keep."""
    if not os.path.isdir("last"):
        return []
    notes, n = [], 0
    for canvases in frames.values():
        for c in canvases:
            path = os.path.join("last", "%03d.png" % n)
            if os.path.exists(path):
                old = Image.open(path).convert("RGBA")
                new = c.to_image()
                a, b = old.tobytes(), new.tobytes()
                diff = sum(1 for i in range(0, len(a), 4)
                           if (a[i + 3] >= 128) != (b[i + 3] >= 128) or (a[i + 3] >= 128 and a[i:i + 3] != b[i:i + 3]))
                if diff:
                    notes.append(f"TIMELINE frame {n + 1}: {diff} pixels differ from what is on the timeline now")
            else:
                notes.append(f"TIMELINE frame {n + 1}: new frame")
            n += 1
    if not notes:
        notes.append("TIMELINE no frame differs from what is on the timeline now")
    return notes


def mask_of(c):
    return {(x, y) for y in range(c.h) for x in range(c.w) if c.px[y][x] is not None}


def lint(mod, frames):
    notes = []
    palette = {p.lower() for p in getattr(mod, "PALETTE", [])}
    used = set()
    for anim, canvases in frames.items():
        masks = [mask_of(c) for c in canvases]
        for i, (c, m) in enumerate(zip(canvases, masks)):
            tag = f"{anim}[{i}]"
            if not m:
                notes.append(f"ERROR {tag}: frame is empty")
                continue
            colors = {c.px[y][x].lower() for x, y in m}
            used |= colors
            if palette and colors - palette:
                notes.append(f"ERROR {tag}: colours not in PALETTE: {sorted(colors - palette)}")
            xs, ys = [p[0] for p in m], [p[1] for p in m]
            if min(xs) == 0 or min(ys) == 0 or max(xs) == c.w - 1 or max(ys) == c.h - 1:
                notes.append(f"WARN  {tag}: touches the frame edge (bbox x{min(xs)}-{max(xs)} y{min(ys)}-{max(ys)}), may be clipped")
            orphans = []
            for x, y in m:
                col = c.px[y][x]
                if not any(c.get(x + dx, y + dy) == col
                           for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy):
                    orphans.append((x, y))
            if len(orphans) > 3:
                listed = ", ".join(f"({x},{y})" for x, y in sorted(orphans)[:MAX_ORPHANS_LISTED])
                notes.append(f"CHECK {tag}: {len(orphans)} lone pixels (no same-colour neighbour) e.g. {listed}"
                             " - fine for eyes/sparkles, noise otherwise")
        # consistency between neighbouring frames (looping, so last -> first too)
        n = len(masks)
        for i in range(n if n > 2 else n - 1):
            a, b = masks[i], masks[(i + 1) % n]
            if not a or not b:
                continue
            iou = len(a & b) / len(a | b)
            ha = max(p[1] for p in a) - min(p[1] for p in a)
            hb = max(p[1] for p in b) - min(p[1] for p in b)
            if iou < 0.6:
                notes.append(f"WARN  {anim}[{i}]->[{(i + 1) % n}]: silhouettes overlap only {iou:.0%}, big jump between frames")
            if abs(ha - hb) > 2 and anim.startswith("idle"):
                notes.append(f"WARN  {anim}[{i}]->[{(i + 1) % n}]: height changes {ha + 1}->{hb + 1}px, idle should barely move")
        if anim.startswith(("walk", "run")) and all(masks):
            # Compare the lowest rows (the feet) between frames: in a real walk most of
            # those pixels change from frame to frame; in a shuffle most stay put.
            floor = max(p[1] for m in masks for p in m)
            feet = [{p for p in m if p[1] >= floor - 3} for m in masks]
            still = len(set.intersection(*feet)) / max(len(set.union(*feet)), 1)
            if still > 0.25:
                notes.append(f"WARN  {anim}: the feet barely travel ({still:.0%} of the bottom 4 rows is the same "
                             "in every frame); the stride should open and close clearly, see the motion panel")
        areas = [len(m) for m in masks if m]
        if areas and max(areas) > 1.35 * min(areas):
            notes.append(f"WARN  {anim}: pixel area varies {min(areas)}-{max(areas)} between frames, character may be changing size")
    notes.insert(0, f"INFO  {len(used)} colours used in total" + (f" (PALETTE has {len(palette)})" if palette else ""))
    if len(used) > 32:
        notes.append(f"WARN  {len(used)} colours is a lot for pixel art; 8-24 usually reads better")
    return notes


def main():
    try:
        mod = load_sprite()
        frames = render_all(mod)
    except Exception:
        print("sprite.py failed:\n" + traceback.format_exc(limit=4))
        sys.exit(1)
    sheet, meta = build_sheet(mod, frames)
    sheet.save("sheet.png")
    with open("sheet.json", "w") as f:
        json.dump(meta, f, indent=1)
    build_preview(mod, sheet, frames).save("preview.png")
    write_frames(frames)
    counts = ", ".join(f"{a} x{len(f)}" for a, f in frames.items())
    print(f"Rendered {mod.WIDTH}x{mod.HEIGHT} frames: {counts}. Wrote sheet.png, sheet.json, preview.png")
    print("\n".join(lint(mod, frames)))
    print("\n".join(timeline_changes(frames)))


if __name__ == "__main__":
    main()
