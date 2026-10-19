# Making a spritesheet

You draw by writing `sprite.py`, a Python script that uses `spritelib.py` (read it for the
full API), then run `python render.py` and look at `preview.png`. Repeat until it is good.
Don't write pixels out by hand except for tiny details (`c.pixels(...)` for eyes, buckles).

## sprite.py contract

```python
from spritelib import Canvas, ramp, mix, part

WIDTH, HEIGHT = 32, 32
ANIMATIONS = {"idle": {"frames": 4, "fps": 6}, "walk": {"frames": 6, "fps": 10}}
PALETTE = [...]          # every colour you use, built from ramps; render.py checks it

def draw(c, anim, frame):
    ...                  # paint one frame on the empty Canvas c
```

## Proportions

Decide them as numbers before drawing, and write them as constants at the top of sprite.py.
- **Default, for anything that walks or fights**: head (with helmet/hair) about 30% of
  the height, torso about 30%, **legs about 40%**. On 32px that is roughly a 9px head,
  9px torso and 11-12px legs. Legs shorter than a third of the height cannot show a
  stride, so the walk turns into a shuffle.
- **Chibi** (big head, ~45% of the height): only when asked for "chibi"/"cute"/"big head".
- Props must not hide the legs: a sword or scabbard on the hip sits on the far side
  (behind the legs, drawn first) or angles away from them, and gets its own outline colour.

## Limbs

Draw arms and legs with `limb(c, start, end, upper, lower, width, color, direction)`:
give it the hip and the foot (or the shoulder and the hand), and it bends the knee/elbow
for you. Animate by moving the feet and hands, not by redrawing boxes. For walks and runs,
`walk_pose(frame, frames, stride)` gives each foot's offset and the hip bob per frame:

```python
p = walk_pose(frame, 6, stride=STRIDE)
hip_y = HIP_Y + p["body_dy"]
limb(c, (HIP_X - 1, hip_y), (HIP_X + p["back"][0], GROUND + p["back"][1]), 5, 6, 3, LEG_DARK, direction=1)
draw_body(c, hip_y)          # torso and head follow the hips
limb(c, (HIP_X + 1, hip_y), (HIP_X + p["front"][0], GROUND + p["front"][1]), 5, 6, 3, LEG, direction=1)
```

Draw the far limbs first and a step darker, then the body, then the near limbs, so the
near leg overlaps the far one. Feet are drawn at the end of each limb (a boot is a small
rect 1-2px longer toward the facing direction).

Build the character from **pose parameters**: write `draw_character(c, body_y, head_y,
left_leg, right_leg, left_arm, right_arm, ...)` once, and have `draw()` look the pose up
from a small table per animation. Every frame then comes from the same code, so the
character stays on model, and a fix in one place fixes every frame.

## Workflow

1. Plan: silhouette, proportions (see Proportions above), palette.
2. Draw idle frame 0 only (set the other animations to 1 frame while you work).
   Render, Read `preview.png`, and judge it at 1x as well as enlarged: is it readable
   as the thing asked for at real size? Fix and re-render until it is.
3. Add the animations as pose tables. Render and check every frame, and that
   neighbouring frames line up (same feet line, same size). preview.png has a
   **motion panel** per animation: every frame's outline laid over the others in its
   own colour, with the ground as a dashed line. In a good walk the leg outlines fan
   out clearly; if they all sit on top of each other, the character is not walking.
4. Read the lint report printed by render.py. Fix every ERROR. Fix WARNs unless they
   are intentional. "lone pixels" on eyes and highlights are fine.
5. Stop when it is good. Typically 4-10 renders. End with a two-line summary.

## Pixel-art rules

- **Palette**: 3-5 step ramps from `ramp(base, n)` (hue-shifted already: cool shadows,
  warm highlights). 12-24 colours total. Use the darkest step of each ramp for its
  outline (`selective_outline`), or one very dark colour (not pure black) for all.
- **One light source**, top-left unless asked otherwise. Shadow on the bottom-right of
  every form; `shade()` / `highlight()` do this per colour.
- **No pillow shading** (dark rim all round, light centre). Shade by form and light.
- **Clean lines**: line segments should step evenly (1-1-1, 2-2-2, 1-2-1-2), no doubled
  corners ("L" shapes inside a 1px line), no lone stray pixels.
- **Clusters**: each colour area should be a readable shape, not scattered noise.
  Dither only on large flat areas, sparingly.
- **Readability first**: big clear silhouette, contrast between the head, body and
  limbs, eyes visible. Leave a 1px margin to the frame edge.
- Feet stand on the same row in every frame (except jumps), usually HEIGHT-2.

## Animation rules

- **idle** (4 frames): breathing. Body and head drop 1px on frames 1-2 and come back.
  Feet never move. Maybe hair or cape lags one frame behind.
- **walk** (6 frames, side or 3/4 view): contact - down - passing - contact (other
  foot) - down - passing. Legs alternate; arms swing opposite to legs; the body is 1px
  lower on the "down" frames and 1px higher on "passing". Stride (the distance between
  the feet at contact) about 1/4 of the height: 8px on a 32px character. Use walk_pose().
- **run**: like walk, but leaning forward, longer stride, both feet off the ground in one frame.
- **attack**: anticipation (wind back) -> fast strike (smear or motion line) -> hold -> recover.
- Keep the volume constant: limbs move, the torso does not grow or shrink.
