"""Brings a frame Claude drew at a reduced size up to the full size of an illustration canvas.

Plain block scaling turns every pixel Claude drew into a square that looks soft and blocky once
the artist zooms in. Here the edges are redrawn at full size instead: where Claude's colours
meet in a hard edge, each new pixel takes one of the colours around it, picked by a smooth
(bicubic) upscale, so edges come out sharp and follow the shape rather than the pixel grid.
Where the colours already blend gently, as in painted shading, the smooth upscale is kept.
Edges are anti-aliased by working at twice the size and halving.
"""

import numpy as np
from PIL import Image

SUPERSAMPLE = 2
# How far apart (RGBA, 0..1) the colours around a spot must be for it to count as an edge.
# Below SOFT the smooth upscale is used, above SOFT + RAMP the sharp colour, and in between a mix.
SOFT = 0.10
RAMP = 0.15


def upscale(img, size):
    """`img` (any mode) scaled to `size` (width, height) as RGBA with sharp edges."""
    src = np.asarray(img.convert("RGBA"), dtype=np.float32) / 255.0
    h, w = src.shape[:2]
    # Premultiplied, so a transparent pixel's leftover colour never bleeds into an edge
    src[..., :3] *= src[..., 3:4]
    tw, th = size[0] * SUPERSAMPLE, size[1] * SUPERSAMPLE
    smooth = np.stack([np.asarray(Image.fromarray(np.ascontiguousarray(src[..., c])).resize((tw, th), Image.BICUBIC))
                       for c in range(4)], -1)
    np.clip(smooth, 0.0, 1.0, out=smooth)

    # The four source pixels around each new pixel's centre
    u = (np.arange(tw) + 0.5) * w / tw - 0.5
    v = (np.arange(th) + 0.5) * h / th - 0.5
    x0 = np.clip(np.floor(u).astype(int), 0, w - 1)
    x1 = np.clip(x0 + 1, 0, w - 1)
    y0 = np.clip(np.floor(v).astype(int), 0, h - 1)
    y1 = np.clip(y0 + 1, 0, h - 1)
    cols = np.arange(tw)

    out = np.empty((th, tw, 4), np.float32)
    # A row at a time keeps memory small on a full HD canvas
    for r in range(th):
        cands = np.stack([src[y0[r], x0], src[y0[r], x1], src[y1[r], x0], src[y1[r], x1]], 0)
        nearest = ((cands - smooth[r][None]) ** 2).sum(-1).argmin(0)
        sharp = cands[nearest, cols]
        spread = np.sqrt(((cands.max(0) - cands.min(0)) ** 2).sum(-1))
        t = np.clip((spread - SOFT) / RAMP, 0.0, 1.0)[:, None]
        t = t * t * (3 - 2 * t)
        out[r] = sharp * t + smooth[r] * (1 - t)

    big = Image.fromarray((out * 255 + 0.5).astype(np.uint8), "RGBa")
    return big.reduce(SUPERSAMPLE).convert("RGBA")
