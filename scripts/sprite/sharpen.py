"""Sharpens drawings Claude already put on an illustration canvas.

Older drawings were placed at the canvas's own resolution, so they blur when the artist zooms in,
and the ones Claude drew shrunk are made of square blocks besides. This brings each image up to
`detail` times its size with upscale.py. For a block image it first finds the block size and
where the blocks line up, and shrinks it back to the pixels Claude drew, so the blocks are not
kept as they are.

    python sharpen.py <dir> [detail]

reads every <dir>/in/NAME.png and writes <dir>/out/NAME.png at `detail` (default 2) times its
size. With a detail of 1 only block images are written. Prints how many were written.
"""

import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from upscale import upscale  # noqa: E402

MAX_BLOCK = 64
# Share of colour changes that must fall on the block grid: a few strokes painted over the
# blocks by hand are let through
ON_GRID = 0.97
# Fewer colour changes than this along an axis cannot show a grid
MIN_EDGES = 12


def edge_counts(px, axis):
    """How many colour changes fall between each pixel and the one before it along `axis`."""
    changed = np.any(np.diff(px, axis=axis) != 0, axis=-1)
    counts = changed.sum(axis=1 - axis)
    # counts[i] is the change between pixel i and i + 1, so the edge sits at i + 1
    return np.concatenate([[0], counts])


def grid_fits(counts, size, offset):
    total = counts.sum()
    positions = np.arange(len(counts))
    return counts[(positions - offset) % size == 0].sum() >= ON_GRID * total


def find_grid(px):
    """(block size, x offset, y offset), or None when the image is not made of blocks."""
    cx, cy = edge_counts(px, 1), edge_counts(px, 0)
    if cx.sum() < MIN_EDGES and cy.sum() < MIN_EDGES:
        return None
    # The largest size that fits: a grid of 6 also fits sizes 2 and 3
    for size in range(MAX_BLOCK, 1, -1):
        ox = next((o for o in range(size) if grid_fits(cx, size, o)), None)
        if ox is None:
            continue
        oy = next((o for o in range(size) if grid_fits(cy, size, o)), None)
        if oy is not None:
            return size, ox, oy
    return None


def sharpen(img, detail):
    """`img` at `detail` times its size with sharp edges, or None when there is nothing to do."""
    px = np.asarray(img.convert("RGBA"))
    h, w = px.shape[:2]
    grid = find_grid(px)
    if grid is None:
        return upscale(img, (w * detail, h * detail)) if detail > 1 else None
    size, ox, oy = grid
    # Blocks cut off at the edges are filled out with their own colour, so every block is whole
    left, top = (size - ox) % size, (size - oy) % size
    right = (-(w + left)) % size
    bottom = (-(h + top)) % size
    padded = np.pad(px, ((top, bottom), (left, right), (0, 0)), mode="edge")
    # Each block's middle pixel is the pixel Claude drew
    small = padded[size // 2::size, size // 2::size]
    step = size * detail
    big = upscale(Image.fromarray(small, "RGBA"), (small.shape[1] * step, small.shape[0] * step))
    return big.crop((left * detail, top * detail, (left + w) * detail, (top + h) * detail))


def main():
    root = sys.argv[1]
    detail = int(sys.argv[2]) if len(sys.argv) > 2 else 2
    src, dst = os.path.join(root, "in"), os.path.join(root, "out")
    os.makedirs(dst, exist_ok=True)
    done = 0
    for name in sorted(os.listdir(src)):
        if not name.endswith(".png"):
            continue
        out = sharpen(Image.open(os.path.join(src, name)), detail)
        if out is not None:
            out.save(os.path.join(dst, name))
            done += 1
    print(done)


if __name__ == "__main__":
    main()
