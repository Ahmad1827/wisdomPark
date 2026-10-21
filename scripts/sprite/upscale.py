"""Brings a frame Claude drew up to a larger size with sharp edges, for an illustration canvas.

Plain scaling either turns every pixel Claude drew into a square or smears it; both look soft
once the artist zooms in. Here the edges are redrawn at the new size instead: where Claude's
colours meet in a hard edge, each new pixel takes one of the colours around it, picked by a
smooth (bicubic) upscale, so edges come out sharp and follow the shape rather than the pixel grid.
In a painted drawing (many colours) where the colours already blend gently, as in soft shading,
the smooth upscale is kept; a drawing with a limited palette keeps every edge sharp, since its
shading steps are meant to be hard.
Edges are anti-aliased by working at twice the size and halving.
"""

import numpy as np
from PIL import Image

SUPERSAMPLE = 2
# How far apart (RGBA, 0..1) the colours around a spot must be for it to count as an edge.
# Below SOFT the smooth upscale is used, above SOFT + RAMP the sharp colour, and in between a mix.
SOFT = 0.10
RAMP = 0.15
# A drawing with at most this many colours is treated as having a palette: no edge is blended
PALETTE_COLORS = 256
# Output rows (at the supersampled size) worked on at once, so a big canvas never needs the
# whole smooth upscale in memory. A multiple of SUPERSAMPLE.
STRIP_ROWS = 256


def upscale(img, size):
    """`img` (any mode) scaled to `size` (width, height) as RGBA with sharp edges."""
    rgba = img.convert("RGBA")
    painted = rgba.getcolors(PALETTE_COLORS) is None
    src = np.asarray(rgba, dtype=np.float32) / 255.0
    h, w = src.shape[:2]
    # Premultiplied, so a transparent pixel's leftover colour never bleeds into an edge
    src[..., :3] *= src[..., 3:4]
    channels = [Image.fromarray(np.ascontiguousarray(src[..., c])) for c in range(4)]
    tw, th = size[0] * SUPERSAMPLE, size[1] * SUPERSAMPLE

    # The four source pixels around each new pixel's centre
    u = (np.arange(tw) + 0.5) * w / tw - 0.5
    v = (np.arange(th) + 0.5) * h / th - 0.5
    x0 = np.clip(np.floor(u).astype(int), 0, w - 1)
    x1 = np.clip(x0 + 1, 0, w - 1)
    y0 = np.clip(np.floor(v).astype(int), 0, h - 1)
    y1 = np.clip(y0 + 1, 0, h - 1)
    cols = np.arange(tw)

    result = Image.new("RGBA", size)
    for top in range(0, th, STRIP_ROWS):
        bottom = min(th, top + STRIP_ROWS)
        # The smooth upscale of just these rows: the box is where they lie on the source
        box = (0, top * h / th, w, bottom * h / th)
        smooth = np.stack([np.asarray(c.resize((tw, bottom - top), Image.BICUBIC, box=box)) for c in channels], -1)
        np.clip(smooth, 0.0, 1.0, out=smooth)
        out = np.empty((bottom - top, tw, 4), np.float32)
        for i, r in enumerate(range(top, bottom)):
            cands = np.stack([src[y0[r], x0], src[y0[r], x1], src[y1[r], x0], src[y1[r], x1]], 0)
            nearest = ((cands - smooth[i][None]) ** 2).sum(-1).argmin(0)
            sharp = cands[nearest, cols]
            if not painted:
                out[i] = sharp
                continue
            spread = np.sqrt(((cands.max(0) - cands.min(0)) ** 2).sum(-1))
            t = np.clip((spread - SOFT) / RAMP, 0.0, 1.0)[:, None]
            t = t * t * (3 - 2 * t)
            out[i] = sharp * t + smooth[i] * (1 - t)
        strip = Image.fromarray((out * 255 + 0.5).astype(np.uint8), "RGBa").reduce(SUPERSAMPLE).convert("RGBA")
        result.paste(strip, (0, top // SUPERSAMPLE))
    return result
