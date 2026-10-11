"""Claude pixel-art bridge.

Reads the canvas (or the selected part of it) from temp_ai_input.png, asks Claude to
finish the drawing (or to carry out a typed request such as "make contour of a cat"),
has Claude look at a render of its own result and correct it, and writes the final
image to temp_ai_output.png. On failure it writes the reason to temp_ai_error.txt and
exits non-zero. temp_ai_status.txt holds a short line on what is happening right now,
which the app shows on its loading screen.

temp_ai_context.json, when the app wrote one, adds what the flat canvas cannot show:
the separate layers of the frame, and the requests made earlier in this project with
what the artist did with each result, so a follow-up such as "bigger" makes sense.
For an animation it lists the neighbouring frames, and it can ask for the next frame
to be drawn instead of an edit to this one. It also says whether Claude should answer with the whole canvas or only with the
pixels it changes, which is quicker and allows larger canvases. It can also hold a colour limit (the only colours new work may use) and a number of
options to make; extra options are written to temp_ai_output_2.png and _3.png. When
several next frames are asked for at once they are drawn one after another, each from
the one before it, and written to the same files in order (up to _4.png).

The job arrives on stdin so neither the API key nor the user's prompt ever
touches a command line:
    line 1: backend  ("cli" = Claude Code login, "api" = Anthropic API key)
    line 2: API key  (empty for the cli backend)
    line 3: the artist's current colour as #rrggbb
    rest:   optional request typed by the user ("make contour of a cat");
            empty means "finish my drawing"
"""
import base64
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
from concurrent.futures import ThreadPoolExecutor

from PIL import Image

INPUT_PATH = "temp_ai_input.png"
OUTPUT_PATH = "temp_ai_output.png"
MAX_OPTIONS = 3
# Next frames drawn in one request. Each is a request of its own plus its checks.
MAX_NEW_FRAMES = 4
ERROR_PATH = "temp_ai_error.txt"
STATUS_PATH = "temp_ai_status.txt"
CONTEXT_PATH = "temp_ai_context.json"

API_MODEL = "claude-opus-5-5"
MAX_CELLS = 128 * 128
# When Claude answers with changes only, the canvas is read but never written back out
# in full, so a larger one is affordable.
MAX_CELLS_CHANGES = 256 * 256
MAX_INPUT_COLORS = 40
CLI_TIMEOUT_SECONDS = 600
# How many times Claude may look at its own result and correct it. Each round is one
# more request, and the loop stops early as soon as Claude calls the result good.
REVIEW_ROUNDS = 2
# Every layer is a second copy of the canvas in the prompt, so only this many pixels of
# layers are spelled out; the rest are listed by name.
LAYER_CELL_BUDGET = 40000

# '.' is transparent; every other symbol is an index into the palette.
SYMBOLS = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

SYSTEM_PROMPT = (
    "You are a pixel-art assistant embedded in a drawing app. "
    "Reply with a single JSON object and nothing else."
)

INSTRUCTIONS = """\
You are helping an artist with a pixel-art drawing inside their drawing app.

The canvas is {width} pixels wide and {height} pixels tall. It is given below as JSON.
{layout}
The character "." is a transparent pixel. Any other character is a palette index using
this alphabet, in order:
{symbols}
So "0" is palette[0], "a" is palette[10], "A" is palette[36].

{canvas_json}

{context}{image_note}
{task}
{answer}"""

# How the canvas itself is laid out, for each of the two ways of answering.
LAYOUT_ROWS = """\
"palette" is a list of hex colours, and "rows" holds one string per pixel row, top to
bottom, one character per pixel."""

LAYOUT_NUMBERED = """\
"palette" is a list of hex colours. Each "yNNN" entry is one pixel row, where NNN is
its y, counting from 0 at the top, with one character per pixel. "tens" and "ones" are
rulers for x: read downwards, the two digits above a pixel give its column, counting
from 0 at the left (on a canvas wider than 100 the hundreds digit is left off)."""

ANSWER_ROWS = """\
Return the resulting canvas as {{"palette": [...], "rows": [...]}}, with "rows" holding
one string per pixel row, top to bottom.

{format_rules}"""

ANSWER_CHANGES = """\
Return only what changes, in this form:
{{"palette": [...], "changes": [{{"y": 12, "x": 5, "pixels": "00a."}}, ...]}}

{change_rules}"""

FORMAT_RULES = """- A program parses your reply and paints it straight onto the canvas, so it must be
  exactly {height} rows of exactly {width} characters each. A row that is one character
  short shifts everything after it, so count carefully.
- Reuse the existing palette where you can. You may append new colours to the end of
  the palette when the work needs them, up to {max_colors} total. Never reorder or
  remove existing palette entries, because the rows refer to them by position.
"""

CHANGE_RULES = """- Each entry in "changes" overwrites one horizontal run of pixels: "pixels" is written
  onto row y starting at column x and going right, one character per pixel, in the same
  symbols as the canvas. A "." there makes that pixel transparent, which is how you
  erase.
- A program applies the changes exactly as written and leaves every other pixel alone,
  so list every pixel that should differ and nothing else. Where changed pixels are
  separated by a few unchanged ones, one run that repeats the unchanged pixels as they
  are is fine.
- Position is what goes wrong most: a run that starts one column off draws the right
  shape in the wrong place. Read x and y off the rulers and the row names rather than
  estimating them, and check the first and last run of each shape against its
  neighbours on the canvas.
- "palette" is the palette you were given, in the same order, with any new colours
  appended, up to {max_colors} total. Never reorder or remove entries, because the
  pixels refer to them by position.
"""

# Second look: Claude sees a render of what it just produced and may correct it.
REVIEW = """You are checking a pixel-art edit before the artist sees it. A first pass produced the
edit from text alone, without seeing how it looks, so mistakes that are obvious to the
eye are common: lines that came out too thick, features out of proportion or in the
wrong place, gaps in an outline, stray pixels, or parts of the drawing changed that
should not have been.

The canvas is {width} pixels wide and {height} pixels tall and is written as JSON.
{layout}
The character "." is a transparent pixel. Any other character is a palette index using
this alphabet, in order:
{symbols}

What the artist asked for:
{task}
The canvas before the edit:
{before_json}

{context}{edit_block}

{image_note}
Compare the two renders and judge the edit the way the artist will when it appears on
their canvas at this size:
- Does it do what was asked, completely, and is the new work recognisable at a glance?
- Is the new work in keeping with the drawing: line weight matching the existing
  lines (one pixel unless the artist draws heavier), sensible proportions, placed where
  it belongs?
- Is everything the request did not concern still exactly as it was before?

If the edit is good as it stands, reply with "verdict": "good" and leave the other
fields empty. Small imperfections that the artist would not notice are not worth a
rewrite.

If it needs fixing, reply with "verdict": "fix" and say in "problems" what is wrong in
a sentence or two. {fix_answer}"""

EDIT_AS_ROWS = """\
The edit that was produced:
{after_json}"""

EDIT_AS_CHANGES = """\
The edit that was produced, written as the changes it made to the canvas above. Each
entry overwrites a run of pixels on row y starting at column x, and "palette" is the
canvas palette with the edit's new colours appended:
{changes_json}"""

FIX_ROWS = """\
Put the corrected canvas in "palette" and "rows". Start from the edit and change only
what fixes the problems.

{format_rules}"""

FIX_CHANGES = """\
Put the correction in "palette" and "changes", in the same form the edit is written
in. Your changes are applied on top of the edit, so list only the pixels that must
differ from the edit as it stands. To undo something the edit did, write the original
pixels back.

{change_rules}"""

# No request typed: plain autocomplete.
TASK_FINISH = """\
The artist is part-way through this drawing and has asked you to finish it. Work out
what they are drawing, then complete it.

- Keep the pixels the artist already placed unless one is clearly a stray mark. The
  result should read as their drawing, completed, rather than a different drawing.
- Continue their style: the same outline weight, shading direction and level of detail.
- Leave the background transparent unless the artist has started filling it in.
"""

# The artist typed what they want, e.g. "make contour of a cat".
TASK_REQUEST = """\
The artist typed this request: "{request}"

Carry it out on the canvas.

- Do what the request asks and nothing more. Pixels the request does not concern stay
  exactly as they are, so the artist can trust that the rest of their work is untouched.
- A request for a contour or outline means line art only: a closed line, one pixel
  wide, tracing the outer edge of the subject, with the inside left as it was
  (transparent on an empty canvas). If the request names a subject ("contour of a
  cat"), draw that subject's outline, large enough to use most of the free space and
  recognisable from its silhouette alone. Add inner lines only where the silhouette
  would not read without them, such as an eye or a limb that overlaps the body. If the
  request names no subject, trace the outline around the drawing already on the
  canvas, on the transparent pixels that border it.
- The artist's currently selected colour is {color}. Use it for anything new unless the
  request names a colour or the existing drawing clearly calls for a different one.
- Leave the background transparent unless the request is about the background.
"""

LAYERS = """\
The drawing is built from {count} layers, and the canvas is what the visible ones look
like put together. Here they are one at a time, bottom layer first. Each has its own
palette and is written as {{"palette", "rows"}} JSON, one string per pixel row:

{layers}

The artist is working on the layer marked "active". Your result is still the whole
canvas, as it should look with every visible layer together: the app compares it with
the canvas and places the pixels you changed on the active layer, or on a new layer
above it. The layers are here so you can tell what belongs to what. When the request
names a layer ("recolour the outline layer"), change only the pixels that layer
contributes. Hidden layers are not part of the canvas, so treat them as reference,
for example a rough sketch the artist wants followed.
"""

HISTORY = """\
Earlier in this session the artist made these requests, oldest first, and this is what
they did with each result:
{turns}
Everything the artist kept is already part of the canvas. This is background for
understanding a follow-up such as "bigger" or "try again"; the only request to carry
out is the current one.
"""

PREVIOUS = """\
The artist discarded your result for the last request. Here it is, so that a follow-up
referring to it makes sense and you do not hand back the same thing (written as
{{"palette", "rows"}} JSON, one string per pixel row):
{previous_json}
"""

COLOR_LIMIT = """\
The artist is keeping this drawing to a fixed set of colours. Everything you draw or
change must use one of these and nothing else:
{colors}
Any of them may be appended to the palette if it is not there yet. The app snaps every
other colour to the nearest one on this list, which rarely looks the way you meant, so
plan shading and highlights around these colours from the start.
"""

OPTION = """\
You are making option {number} of {total}. The artist will see the options next to each
other and keep one. The others are being made separately, so each has to differ in a
way the artist would care about, not by a stray pixel.
{angle}
"""

OPTION_ANGLES = [
    "Option 1 is the most direct reading of the request: what the artist most likely has in mind.",
    "Option 2 takes a different approach to the same request, such as another shape, pose, "
    "placement or proportion than the obvious one, while still doing exactly what was asked.",
    "Option 3 is the boldest: more stylised or more detailed than the obvious reading, "
    "while still doing exactly what was asked.",
]

FRAMES = """\
This drawing is frame {number} of {count} in an animation that plays at {fps} frames a
second. Its neighbouring frames are given here for reference, each with its own palette
and written as {{"palette", "rows"}} JSON, one string per pixel row:

{frames}

"""

FRAMES_CONSISTENT = """\
Whatever you change should hold up when the animation plays: the same subject keeps the
same proportions, colours and line weight from one frame to the next.
"""

# The artist asked for the following frame of the animation rather than an edit.
TASK_NEXT_FRAME = """\
The canvas is one frame of an animation. The artist has asked you to draw the frame
that comes right after it{between}. Your result is that new frame: the same scene a
moment later, as it should look on its own.

{direction}
{sequence}
- Stay on model. Proportions, colours, outline weight and level of detail match this
  frame, so the two read as the same drawing when they are flipped back and forth.
- Move only what moves. At {fps} frames a second one frame is a small step: a moving
  part travels a pixel or a few, not across the canvas. Everything at rest stays where
  it is pixel for pixel, because any stray change shows up as flicker in playback.
- When earlier frames are given, work out the motion from them (what moved, in which
  direction, how far per frame) and take it one step further, including easing if the
  steps are getting smaller or larger.
- Keep the framing. The subject is not recentred or rescaled unless the motion itself
  carries it.
- The artist's currently selected colour is {color}. It is only for something new that
  appears in the frame; what is already drawn keeps its colours.
"""

DIRECTION_GIVEN = 'The artist described what happens: "{request}"'
DIRECTION_OPEN = (
    "The artist gave no direction. Continue the motion the earlier frames show. If there "
    "are no earlier frames, or nothing moves in them, begin a small natural movement that "
    "suits the subject, such as a blink, a breath or a flick of a tail."
)
BETWEEN = (
    " and before the frame that follows it, which is given below, so it has to work as "
    "the in-between of the two"
)
BETWEEN_SEQUENCE = (
    " and before the frame given below as the one after it, which the new frames lead up to"
)
# Several new frames were asked for in one go; each is drawn from the one before it.
SEQUENCE = """
The artist asked for {steps} new frames in a row. They are drawn one at a time and this
is number {step}. {earlier}{pace}
"""
SEQUENCE_EARLIER = "The ones you already drew are the frames right before the canvas. "
SEQUENCE_PACE_BETWEEN = (
    "Together the new frames carry the drawing from where it started to the frame that "
    "follows, in even steps, so this one covers about one part in {parts} of the distance "
    "that is still left."
)
SEQUENCE_PACE_OPEN = (
    "Spread the motion evenly over them: whatever happens should take all {steps} frames "
    "and be complete in the last one, so this frame is one even step of it."
)

RESULT_SCHEMA = {
    "type": "object",
    "properties": {
        "palette": {"type": "array", "items": {"type": "string"}},
        "rows": {"type": "array", "items": {"type": "string"}},
    },
    "required": ["palette", "rows"],
    "additionalProperties": False,
}

CHANGES_ITEMS = {
    "type": "array",
    "items": {
        "type": "object",
        "properties": {"y": {"type": "integer"}, "x": {"type": "integer"}, "pixels": {"type": "string"}},
        "required": ["y", "x", "pixels"],
        "additionalProperties": False,
    },
}

CHANGES_SCHEMA = {
    "type": "object",
    "properties": {"palette": {"type": "array", "items": {"type": "string"}}, "changes": CHANGES_ITEMS},
    "required": ["palette", "changes"],
    "additionalProperties": False,
}

REVIEW_CHANGES_SCHEMA = {
    "type": "object",
    "properties": {
        "verdict": {"type": "string", "enum": ["good", "fix"]},
        "problems": {"type": "string"},
        "palette": {"type": "array", "items": {"type": "string"}},
        "changes": CHANGES_ITEMS,
    },
    "required": ["verdict", "problems", "palette", "changes"],
    "additionalProperties": False,
}

REVIEW_SCHEMA = {
    "type": "object",
    "properties": {
        "verdict": {"type": "string", "enum": ["good", "fix"]},
        "problems": {"type": "string"},
        "palette": {"type": "array", "items": {"type": "string"}},
        "rows": {"type": "array", "items": {"type": "string"}},
    },
    "required": ["verdict", "problems", "palette", "rows"],
    "additionalProperties": False,
}


class BridgeError(Exception):
    pass


class BadReplyError(BridgeError):
    """Claude answered, but not with a usable canvas. Worth one more attempt."""


def encode_canvas(img):
    """Turn an RGBA image into (palette, rows)."""
    width, height = img.size
    px = img.load()
    pixels = [px[x, y] for y in range(height) for x in range(width)]
    opaque = [(r, g, b) for (r, g, b, a) in pixels if a >= 128]

    if len(set(opaque)) > MAX_INPUT_COLORS:
        # Soft brushes and anti-aliasing produce hundreds of near-identical colours;
        # snap them to a small palette so every pixel still gets one symbol.
        quantized = img.convert("RGB").quantize(MAX_INPUT_COLORS, dither=Image.Dither.NONE).convert("RGB")
        qx = quantized.load()
        pixels = [qx[i % width, i // width] + (pixels[i][3],) for i in range(len(pixels))]

    palette = []
    index = {}
    rows = []
    for y in range(height):
        row = []
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            if a < 128:
                row.append(".")
                continue
            color = (r, g, b)
            if color not in index:
                index[color] = len(palette)
                palette.append(color)
            row.append(SYMBOLS[index[color]])
        rows.append("".join(row))
    return ["#%02x%02x%02x" % c for c in palette], rows


def parse_hex(value):
    value = value.strip().lstrip("#")
    if len(value) == 3:
        value = "".join(ch * 2 for ch in value)
    if len(value) != 6:
        raise BadReplyError(f"Claude returned an invalid colour: '{value}'")
    try:
        return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), 255)
    except ValueError:
        raise BadReplyError(f"Claude returned an invalid colour: '{value}'")


def decode_canvas(result, width, height):
    """Turn Claude's {"palette", "rows"} reply back into an RGBA image."""
    if not isinstance(result, dict) or not isinstance(result.get("palette"), list) or not isinstance(result.get("rows"), list):
        raise BadReplyError("Claude's reply was not a palette/rows object.")

    colors = [parse_hex(str(c)) for c in result["palette"]][: len(SYMBOLS)]
    rows = [str(r) for r in result["rows"]]
    if not rows:
        raise BadReplyError("Claude returned an empty canvas.")

    # Off-by-a-few rows are padded or trimmed rather than rejected, but a reply that
    # is mostly the wrong shape would paint garbage, so that one is an error.
    wrong = sum(1 for r in rows if len(r) != width) + abs(len(rows) - height)
    if wrong > max(2, height // 4):
        raise BadReplyError(f"Claude returned a canvas of the wrong size ({wrong} of {height} rows were off). Try again.")

    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    out = img.load()
    for y in range(min(height, len(rows))):
        row = rows[y]
        for x in range(min(width, len(row))):
            idx = SYMBOLS.find(row[x])
            if 0 <= idx < len(colors):
                out[x, y] = colors[idx]
    return img


def extract_json(text):
    """Pull the JSON object out of a reply that may be wrapped in a code fence."""
    start = text.find("{")
    end = text.rfind("}")
    if start < 0 or end <= start:
        raise BadReplyError("Claude did not return a canvas. Reply was: " + text.strip()[:200])
    try:
        return json.loads(text[start : end + 1])
    except json.JSONDecodeError:
        raise BadReplyError("Claude's reply could not be read as JSON. Try again.")


def preview_png(img):
    """Upscaled copy of the canvas so Claude can also look at the drawing."""
    scale = max(1, 512 // max(img.size))
    return img.resize((img.width * scale, img.height * scale), Image.Resampling.NEAREST)


def set_status(text):
    try:
        with open(STATUS_PATH, "w", encoding="utf-8") as f:
            f.write(text)
    except OSError:
        pass


def canvas_json(palette, rows):
    return json.dumps({"palette": palette, "rows": rows}, indent=0)


def format_rules(img):
    return FORMAT_RULES.format(width=img.width, height=img.height, max_colors=len(SYMBOLS))


# Set once in main() from the context file. True means Claude answers with the pixels it
# changes instead of rewriting the whole canvas.
CHANGES_ONLY = False


def change_rules():
    return CHANGE_RULES.format(max_colors=len(SYMBOLS))


def numbered_canvas_json(palette, rows):
    """The canvas with every row named by its y and rulers for x, so pixels can be addressed."""
    width = len(rows[0]) if rows else 0
    lines = ['{"palette": ' + json.dumps(palette) + ",",
             '"tens": "' + "".join(str(x // 10 % 10) for x in range(width)) + '",',
             '"ones": "' + "".join(str(x % 10) for x in range(width)) + '",']
    lines += [f'"y{y:03d}": "{row}"' + ("," if y < len(rows) - 1 else "") for y, row in enumerate(rows)]
    return "\n".join(lines) + "}"


def main_canvas_json(img):
    palette, rows = encode_canvas(img)
    return numbered_canvas_json(palette, rows) if CHANGES_ONLY else canvas_json(palette, rows)


def apply_changes(base, reply, require_some):
    """Paints Claude's {"palette", "changes"} reply onto a copy of `base`."""
    if not isinstance(reply, dict) or not isinstance(reply.get("palette"), list) or not isinstance(reply.get("changes"), list):
        raise BadReplyError("Claude's reply was not a palette/changes object.")
    colors = [parse_hex(str(c)) for c in reply["palette"]][: len(SYMBOLS)]

    out = base.copy()
    px = out.load()
    misplaced = 0
    for change in reply["changes"]:
        try:
            y, x, run = int(change["y"]), int(change["x"]), str(change["pixels"])
        except (KeyError, TypeError, ValueError):
            misplaced += 1
            continue
        if not (0 <= y < out.height and 0 <= x < out.width):
            misplaced += 1
            continue
        for i, symbol in enumerate(run[: out.width - x]):
            if symbol == ".":
                px[x + i, y] = (0, 0, 0, 0)
            else:
                idx = SYMBOLS.find(symbol)
                if 0 <= idx < len(colors):
                    px[x + i, y] = colors[idx]
    # A few runs off the edge are dropped quietly; a reply that mostly misses the canvas
    # means the positions cannot be trusted at all.
    if misplaced > max(2, len(reply["changes"]) // 4):
        raise BadReplyError("Claude's changes did not fit the canvas. Try again.")
    if require_some and out.tobytes() == base.tobytes():
        raise BadReplyError("The assistant made no changes. Try wording the request differently.")
    return out


def diff_as_changes(base, edit):
    """Writes `edit` as (palette, changes) relative to `base`, for showing Claude its own work."""
    palette = encode_canvas(base)[0]
    index = {parse_hex(c)[:3]: i for i, c in enumerate(palette)}
    src, out = base.load(), edit.load()

    def symbol(pixel):
        r, g, b, a = pixel
        if a < 128:
            return "."
        if (r, g, b) not in index:
            if len(palette) >= len(SYMBOLS):
                nearest = min(index, key=lambda c: (c[0] - r) ** 2 + (c[1] - g) ** 2 + (c[2] - b) ** 2)
                return SYMBOLS[index[nearest]]
            index[(r, g, b)] = len(palette)
            palette.append("#%02x%02x%02x" % (r, g, b))
        return SYMBOLS[index[(r, g, b)]]

    changes = []
    for y in range(edit.height):
        x = 0
        while x < edit.width:
            if out[x, y] == src[x, y]:
                x += 1
                continue
            start = x
            run = []
            while x < edit.width and out[x, y] != src[x, y]:
                run.append(symbol(out[x, y]))
                x += 1
            changes.append({"y": y, "x": start, "pixels": "".join(run)})
    return palette, changes


# Set once in main() from the context file: None for an edit, or a dict describing the
# animation when the artist asked for the next frame.
NEXT_FRAME = None


def task_text(hint, color):
    if NEXT_FRAME is not None:
        step, steps, following = NEXT_FRAME["step"], NEXT_FRAME["steps"], NEXT_FRAME["has_following"]
        sequence = ""
        if steps > 1:
            pace = SEQUENCE_PACE_BETWEEN.format(parts=steps - step + 2) if following else SEQUENCE_PACE_OPEN.format(steps=steps)
            sequence = SEQUENCE.format(steps=steps, step=step, earlier=SEQUENCE_EARLIER if step > 1 else "", pace=pace)
        between = (BETWEEN_SEQUENCE if steps > 1 else BETWEEN) if following else ""
        return TASK_NEXT_FRAME.format(
            between=between,
            direction=DIRECTION_GIVEN.format(request=hint) if hint else DIRECTION_OPEN,
            sequence=sequence, fps=NEXT_FRAME["fps"], color=color,
        )
    return TASK_REQUEST.format(request=hint, color=color) if hint else TASK_FINISH


def load_context():
    try:
        with open(CONTEXT_PATH, encoding="utf-8", errors="replace") as f:
            ctx = json.load(f)
    except (OSError, ValueError):
        return {}
    return ctx if isinstance(ctx, dict) else {}


def open_matching(path, size):
    """An image the app saved next to the canvas, or None unless it is the same size."""
    try:
        img = Image.open(str(path)).convert("RGBA")
    except OSError:
        return None
    return img if img.size == size else None


def layers_text(ctx, size):
    layers = [l for l in ctx.get("layers", []) if isinstance(l, dict)]
    if len(layers) < 2:
        return ""  # one layer is the canvas itself
    active = next((i for i, l in enumerate(layers) if l.get("active")), 0)
    budget = LAYER_CELL_BUDGET
    encoded = {}
    # The active layer and its neighbours matter most, so they get the budget first
    for i in sorted(range(len(layers)), key=lambda i: abs(i - active)):
        img = open_matching(layers[i].get("file", ""), size)
        if img is None:
            continue
        if img.getextrema()[3][1] < 128:
            encoded[i] = "empty"
        elif budget >= size[0] * size[1]:
            budget -= size[0] * size[1]
            encoded[i] = canvas_json(*encode_canvas(img))

    lines = []
    for i, layer in enumerate(layers):
        notes = ["active"] if layer.get("active") else []
        notes.append("visible" if layer.get("visible", True) else "hidden")
        opacity = round(float(layer.get("opacity", 1)) * 100)
        if opacity < 100:
            notes.append(f"{opacity}% opacity")
        if layer.get("locked"):
            notes.append("locked")
        body = encoded.get(i, "pixels left out to keep this request small")
        name = str(layer.get("name", "Layer")).replace('"', "'")
        lines.append(f'Layer {i + 1} "{name}" ({", ".join(notes)}):\n{body}')
    return LAYERS.format(count=len(layers), layers="\n\n".join(lines)) + "\n"


def history_text(ctx):
    turns = [t for t in ctx.get("history", []) if isinstance(t, dict)]
    if not turns:
        return ""
    lines = []
    for n, turn in enumerate(turns, 1):
        request = " ".join(str(turn.get("request", "")).split()).replace('"', "'")
        asked = f'"{request}"' if request else "finish my drawing (no request typed)"
        lines.append(f"{n}. {asked} - {turn.get('outcome', 'shown to the artist')}")
    return HISTORY.format(turns="\n".join(lines)) + "\n"


def allowed_colors(ctx, img, color):
    """The colours new work is limited to, or None when the artist set no limit."""
    listed = ctx.get("allowed_colors")
    if not isinstance(listed, list):
        return None
    allowed = []
    # What is already on the canvas and the colour in the artist's hand always count
    for value in [str(c) for c in listed] + encode_canvas(img)[0] + [color]:
        try:
            rgb = parse_hex(value)[:3]
        except BadReplyError:
            continue
        if rgb not in allowed:
            allowed.append(rgb)
    return allowed


def enforce_colors(edit, img, allowed):
    """Snaps any colour outside the limit to the nearest allowed one, on changed pixels only."""
    if not allowed:
        return edit
    allowed_set = set(allowed)
    nearest = {}
    src, out = img.load(), edit.load()
    for y in range(edit.height):
        for x in range(edit.width):
            r, g, b, a = out[x, y]
            if a < 128 or (r, g, b) in allowed_set or out[x, y] == src[x, y]:
                continue
            if (r, g, b) not in nearest:
                nearest[(r, g, b)] = min(allowed, key=lambda c: (c[0] - r) ** 2 + (c[1] - g) ** 2 + (c[2] - b) ** 2)
            out[x, y] = nearest[(r, g, b)] + (255,)
    return edit


def color_limit_text(allowed):
    if not allowed:
        return ""
    return COLOR_LIMIT.format(colors=" ".join("#%02x%02x%02x" % c for c in allowed)) + "\n"


def load_neighbours(ctx, size):
    """The frames the app saved around the canvas: (those before it, oldest first; the one right after, or None)."""
    current = int(ctx.get("frame", 1))
    found = {}
    for frame in ctx.get("frames", []):
        if isinstance(frame, dict):
            img = open_matching(frame.get("file", ""), size)
            if img is not None:
                found[int(frame.get("number", 0))] = img
    return [found[n] for n in sorted(found) if n < current], found.get(current + 1)


def frames_text(ctx, before, after, current):
    """Returns (text, renders) for the frames around frame `current`, nearest first in the budget."""
    candidates = [(current - 1 - i, img) for i, img in enumerate(reversed(before))]
    if after is not None:
        candidates.append((current + 1, after))
    budget = LAYER_CELL_BUDGET
    shown = {}
    for number, img in sorted(candidates, key=lambda c: abs(c[0] - current)):
        if budget < img.width * img.height:
            continue
        budget -= img.width * img.height
        shown[number] = img
    if not shown:
        return "", []

    lines = []
    for number in sorted(shown):
        where = "before this one" if number < current else "after this one"
        lines.append(f"Frame {number} ({where}):\n{canvas_json(*encode_canvas(shown[number]))}")
    text = FRAMES.format(number=current, count=ctx.get("frame_count", len(shown) + 1),
                         fps=ctx.get("fps", 12), frames="\n\n".join(lines))
    if NEXT_FRAME is None:
        text += FRAMES_CONSISTENT + "\n"
    renders = []
    if current - 1 in shown:
        renders.append(("frame_before.png", f"frame {current - 1}, the one before the canvas", shown[current - 1]))
    if current + 1 in shown:
        renders.append(("frame_after.png", f"frame {current + 1}, the one after the canvas", shown[current + 1]))
    return text, renders


def build_context(ctx, size, before, after, current, own_canvas=True):
    """Returns (text for the prompt, extra renders to show alongside the canvas).

    `own_canvas` is False when the canvas is a frame Claude drew earlier in this request,
    which the artist's layers and discarded result say nothing about."""
    frames, renders = frames_text(ctx, before, after, current)
    text = (layers_text(ctx, size) if own_canvas else "") + frames + history_text(ctx)
    previous = open_matching(ctx["previous"], size) if own_canvas and ctx.get("previous") and ctx.get("history") else None
    if previous is not None:
        text += PREVIOUS.format(previous_json=canvas_json(*encode_canvas(previous))) + "\n"
        renders.append(("discarded.png", "your last result, which the artist discarded", previous))
    return text, renders


def image_note(backend, images):
    """Tells Claude where the renders are: files for Claude Code, attachments for the API."""
    if backend == "cli":
        listing = "; ".join(f"{name} is {what}" for name, what, _ in images)
        return ("Enlarged renders are saved in the current directory: " + listing +
                ". Read them first so you can see the drawing.\n")
    listing = "; ".join(f"image {i + 1} is {what}" for i, (_, what, _) in enumerate(images))
    return "Enlarged renders are attached: " + listing + ".\n"


def ask_cli(prompt, images, schema):
    """Claude Code in headless mode: uses the user's own Claude Code login."""
    exe = shutil.which("claude")
    if not exe:
        raise BridgeError("Claude Code is not installed (the 'claude' command was not found). Install it, or switch the Assistant Engine to Claude and enter an access key.")

    # A scratch directory keeps Claude Code away from this project's settings and
    # files; the only things it can read there are the canvas renders.
    with tempfile.TemporaryDirectory(prefix="wisdompark_ai_") as workdir:
        for name, _, img in images:
            preview_png(img).save(os.path.join(workdir, name))
        args = [exe, "-p", "--output-format", "json", "--system-prompt", SYSTEM_PROMPT,
                "--tools", "Read", "--allowedTools", "Read",
                "--strict-mcp-config", "--no-session-persistence"]
        # The schema makes Claude Code validate the reply's shape. A .cmd/.bat launcher
        # would mangle the quotes in it, so only the real executable gets it.
        if not exe.lower().endswith((".cmd", ".bat")):
            args += ["--json-schema", json.dumps(schema)]
        try:
            proc = subprocess.run(
                args,
                input=prompt, capture_output=True, text=True, encoding="utf-8",
                cwd=workdir, timeout=CLI_TIMEOUT_SECONDS,
                # Without this, Claude Code pops a console window over the app on Windows
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
            )
        except subprocess.TimeoutExpired:
            raise BridgeError("Claude Code took too long to answer. Try a smaller area.")

    try:
        envelope = json.loads(proc.stdout)
    except json.JSONDecodeError:
        detail = (proc.stderr or proc.stdout).strip()[:200]
        raise BridgeError("Claude Code failed: " + (detail or f"exit code {proc.returncode}"))
    if envelope.get("is_error") or proc.returncode != 0:
        raise BridgeError("Claude Code failed: " + str(envelope.get("result", f"exit code {proc.returncode}"))[:200])
    if isinstance(envelope.get("structured_output"), dict):
        return envelope["structured_output"]
    return extract_json(str(envelope.get("result", "")))


def ask_api(prompt, images, schema, api_key):
    """Anthropic API: for users who have an API key instead of Claude Code."""
    try:
        import anthropic
    except ImportError:
        raise BridgeError("The Claude engine needs the Anthropic SDK. Run: pip install anthropic")
    if not api_key:
        raise BridgeError("No Claude access key set. Enter one in Settings, or switch the Assistant Engine to Claude Code.")

    content = []
    for _, _, img in images:
        buf = io.BytesIO()
        preview_png(img).save(buf, format="PNG")
        content.append({"type": "image", "source": {"type": "base64", "media_type": "image/png",
                                                    "data": base64.standard_b64encode(buf.getvalue()).decode("ascii")}})
    content.append({"type": "text", "text": prompt})

    client = anthropic.Anthropic(api_key=api_key)
    try:
        # Streamed because a full canvas plus thinking can outlast a plain request.
        # fallbacks="default" lets the API retry on another model if this one declines.
        with client.beta.messages.stream(
            model=API_MODEL,
            max_tokens=32000,
            betas=["server-side-fallback-2026-07-01"],
            fallbacks="default",
            system=SYSTEM_PROMPT,
            output_config={"effort": "medium", "format": {"type": "json_schema", "schema": schema}},
            messages=[{"role": "user", "content": content}],
        ) as stream:
            message = stream.get_final_message()
    except anthropic.AuthenticationError:
        raise BridgeError("The Claude access key was rejected. Check it in Settings.")
    except anthropic.RateLimitError:
        raise BridgeError("Claude API rate limit reached. Wait a moment and try again.")
    except anthropic.APIStatusError as e:
        raise BridgeError(f"Claude API error {e.status_code}: {e.message}"[:200])
    except anthropic.APIConnectionError:
        raise BridgeError("Could not reach the Claude API. Check your internet connection.")

    if message.stop_reason == "refusal":
        raise BridgeError("Claude declined this request.")
    if message.stop_reason == "max_tokens":
        raise BridgeError("The area is too large for Claude to finish in one go. Select a smaller area.")
    text = next((b.text for b in message.content if b.type == "text"), "")
    return extract_json(text)


def ask(backend, api_key, prompt, images, schema):
    if backend == "cli":
        return ask_cli(prompt, images, schema)
    return ask_api(prompt, images, schema, api_key)


def first_pass(backend, api_key, img, hint, color, context, previous):
    images = [("canvas.png", "the canvas", img)] + previous
    if CHANGES_ONLY:
        answer = ANSWER_CHANGES.format(change_rules=change_rules())
    else:
        answer = ANSWER_ROWS.format(format_rules=format_rules(img))
    prompt = INSTRUCTIONS.format(
        width=img.width, height=img.height, symbols=SYMBOLS,
        layout=LAYOUT_NUMBERED if CHANGES_ONLY else LAYOUT_ROWS,
        canvas_json=main_canvas_json(img),
        context=context,
        image_note=image_note(backend, images),
        task=task_text(hint, color),
        answer=answer,
    )
    for attempt in range(2):
        try:
            if CHANGES_ONLY:
                return apply_changes(img, ask(backend, api_key, prompt, images, CHANGES_SCHEMA), require_some=True)
            return decode_canvas(ask(backend, api_key, prompt, images, RESULT_SCHEMA), img.width, img.height)
        except BadReplyError:
            if attempt == 1:
                raise


def review_pass(backend, api_key, img, edit, hint, color, context):
    """Shows Claude a render of its edit. Returns a corrected image, or None if it is fine."""
    images = [("before.png", "the canvas before the edit", img), ("edit.png", "the edit being checked", edit)]
    if CHANGES_ONLY:
        palette, changes = diff_as_changes(img, edit)
        edit_block = EDIT_AS_CHANGES.format(changes_json=json.dumps({"palette": palette, "changes": changes}))
        fix_answer = FIX_CHANGES.format(change_rules=change_rules())
    else:
        edit_block = EDIT_AS_ROWS.format(after_json=canvas_json(*encode_canvas(edit)))
        fix_answer = FIX_ROWS.format(format_rules=format_rules(img))
    prompt = REVIEW.format(
        width=img.width, height=img.height, symbols=SYMBOLS,
        layout=LAYOUT_NUMBERED if CHANGES_ONLY else LAYOUT_ROWS,
        task=task_text(hint, color),
        before_json=main_canvas_json(img),
        context=context,
        edit_block=edit_block,
        image_note=image_note(backend, images),
        fix_answer=fix_answer,
    )
    review = ask(backend, api_key, prompt, images, REVIEW_CHANGES_SCHEMA if CHANGES_ONLY else REVIEW_SCHEMA)
    if not isinstance(review, dict) or review.get("verdict") != "fix":
        print("claude_art: check passed", file=sys.stderr)
        return None
    print("claude_art: fixing - " + str(review.get("problems", "")), file=sys.stderr)
    if CHANGES_ONLY:
        return apply_changes(edit, review, require_some=False)
    return decode_canvas(review, img.width, img.height)


def option_path(number):
    return f"temp_ai_output_{number}.png"


def main():
    for path in [OUTPUT_PATH, ERROR_PATH, STATUS_PATH] + [option_path(n) for n in range(2, max(MAX_OPTIONS, MAX_NEW_FRAMES) + 1)]:
        try:
            os.remove(path)
        except OSError:
            pass

    try:
        # The app writes the job as UTF-8; a piped stdin would otherwise be read in the system codepage
        sys.stdin.reconfigure(encoding="utf-8", errors="replace")
        job = sys.stdin.read().split("\n", 3) + ["", "", "", ""]
        backend, api_key, color = job[0].strip().lower(), job[1].strip(), job[2].strip() or "#000000"
        hint = " ".join(job[3].split()).replace('"', "'")

        try:
            img = Image.open(INPUT_PATH).convert("RGBA")
        except OSError:
            raise BridgeError("Could not read the canvas image.")
        if backend not in ("cli", "api"):
            raise BridgeError(f"Unknown backend '{backend}'.")

        ctx = load_context()
        global NEXT_FRAME, CHANGES_ONLY
        CHANGES_ONLY = ctx.get("output") == "changes"
        if img.width * img.height > (MAX_CELLS_CHANGES if CHANGES_ONLY else MAX_CELLS):
            if CHANGES_ONLY:
                raise BridgeError(f"That is {img.width}x{img.height} pixels; the assistant handles up to 256x256 at a time. Select a smaller area first.")
            raise BridgeError(f"That is {img.width}x{img.height} pixels; with Output set to Whole Canvas the assistant handles up to 128x128. Switch Output to Changes Only, or select a smaller area.")
        current = int(ctx.get("frame", 1))
        before, after = load_neighbours(ctx, img.size)
        new_frames = 1
        if ctx.get("mode") == "next_frame":
            try:
                new_frames = max(1, min(MAX_NEW_FRAMES, int(ctx.get("new_frames", 1))))
            except (TypeError, ValueError):
                pass
            NEXT_FRAME = {"fps": ctx.get("fps", 12), "has_following": after is not None, "step": 1, "steps": new_frames}
        allowed = allowed_colors(ctx, img, color)

        if new_frames > 1:
            # Each frame is drawn from the one before it, so they come one after another
            canvas, drawn = img, 0
            for step in range(1, new_frames + 1):
                NEXT_FRAME["step"] = step
                context, renders = build_context(ctx, img.size, before, after, current, own_canvas=(step == 1))
                context += color_limit_text(allowed)
                set_status(f"Drawing frame {step} of {new_frames}")
                try:
                    edit = enforce_colors(first_pass(backend, api_key, canvas, hint, color, context, renders), canvas, allowed)
                except BridgeError as e:
                    if drawn == 0:
                        raise
                    # The frames already drawn are still worth showing
                    print(f"claude_art: stopped after {drawn} of {new_frames} frames - {e}", file=sys.stderr)
                    break
                for _ in range(REVIEW_ROUNDS):
                    set_status(f"Checking frame {step} of {new_frames}")
                    try:
                        fixed = review_pass(backend, api_key, canvas, edit, hint, color, context)
                    except BridgeError as e:
                        print(f"claude_art: check skipped - {e}", file=sys.stderr)
                        break
                    if fixed is None:
                        break
                    edit = enforce_colors(fixed, canvas, allowed)
                edit.save(OUTPUT_PATH if step == 1 else option_path(step))
                drawn += 1
                before, canvas, current = before + [canvas], edit, current + 1
            return

        context, previous = build_context(ctx, img.size, before, after, current)
        context += color_limit_text(allowed)
        try:
            total = max(1, min(MAX_OPTIONS, int(ctx.get("options", 1))))
        except (TypeError, ValueError):
            total = 1

        def sketch(number):
            extra = OPTION.format(number=number, total=total, angle=OPTION_ANGLES[number - 1]) if total > 1 else ""
            try:
                edit = first_pass(backend, api_key, img, hint, color, context + extra, previous)
            except BridgeError as e:
                return e
            return enforce_colors(edit, img, allowed)

        checked, checked_lock = [], threading.Lock()

        def check(job):
            # There is already a usable result here, so a failed check keeps it rather
            # than failing the request.
            number, edit = job
            extra = OPTION.format(number=number, total=total, angle=OPTION_ANGLES[number - 1]) if total > 1 else ""
            for round_no in range(REVIEW_ROUNDS):
                if total == 1:
                    set_status("Checking how it looks" if round_no == 0 else "Checking the touch-up")
                try:
                    fixed = review_pass(backend, api_key, img, edit, hint, color, context + extra)
                except BridgeError as e:
                    print(f"claude_art: check skipped - {e}", file=sys.stderr)
                    break
                if fixed is None:
                    break
                edit = enforce_colors(fixed, img, allowed)
                if total == 1:
                    edit.save(OUTPUT_PATH)
            if total > 1:
                with checked_lock:
                    checked.append(number)
                    if len(checked) < len(jobs):
                        set_status(f"Checked {len(checked)} of {len(jobs)} options")
            return edit

        if NEXT_FRAME is not None:
            set_status("Drawing the next frame" if total == 1 else f"Drawing {total} takes on the next frame")
        else:
            set_status("Sketching it out" if total == 1 else f"Sketching {total} options")
        with ThreadPoolExecutor(max_workers=total) as pool:
            sketches = list(pool.map(sketch, range(1, total + 1)))
            jobs = [(n, s) for n, s in enumerate(sketches, 1) if not isinstance(s, BridgeError)]
            if not jobs:
                raise sketches[0]
            jobs[0][1].save(OUTPUT_PATH)
            if total > 1:
                set_status("Checking how they look")
            edits = list(pool.map(check, jobs))

        # Two options that came out identical are one option
        distinct = []
        for edit in edits:
            if all(edit.tobytes() != other.tobytes() for other in distinct):
                distinct.append(edit)
        for i, edit in enumerate(distinct):
            edit.save(OUTPUT_PATH if i == 0 else option_path(i + 1))
    except BridgeError as e:
        with open(ERROR_PATH, "w", encoding="utf-8") as f:
            f.write(str(e))
        print(f"claude_art: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
