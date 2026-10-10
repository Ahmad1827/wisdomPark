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

from PIL import Image

INPUT_PATH = "temp_ai_input.png"
OUTPUT_PATH = "temp_ai_output.png"
ERROR_PATH = "temp_ai_error.txt"
STATUS_PATH = "temp_ai_status.txt"
CONTEXT_PATH = "temp_ai_context.json"

API_MODEL = "claude-opus-5-5"
MAX_CELLS = 128 * 128
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

The canvas is {width} pixels wide and {height} pixels tall. It is given below as JSON:
"palette" is a list of hex colours, and "rows" holds one string per pixel row, top to
bottom, one character per pixel. The character "." is a transparent pixel. Any other
character is a palette index using this alphabet, in order:
{symbols}
So "0" is palette[0], "a" is palette[10], "A" is palette[36].

{canvas_json}

{context}{image_note}
{task}
Return the resulting canvas in exactly the same JSON shape: {{"palette": [...], "rows": [...]}}.

{format_rules}"""

FORMAT_RULES = """- A program parses your reply and paints it straight onto the canvas, so it must be
  exactly {height} rows of exactly {width} characters each. A row that is one character
  short shifts everything after it, so count carefully.
- Reuse the existing palette where you can. You may append new colours to the end of
  the palette when the work needs them, up to {max_colors} total. Never reorder or
  remove existing palette entries, because the rows refer to them by position.
"""

# Second look: Claude sees a render of what it just produced and may correct it.
REVIEW = """You are checking a pixel-art edit before the artist sees it. A first pass produced the
edit from text alone, without seeing how it looks, so mistakes that are obvious to the
eye are common: lines that came out too thick, features out of proportion or in the
wrong place, gaps in an outline, stray pixels, or parts of the drawing changed that
should not have been.

The canvas is {width} pixels wide and {height} pixels tall. Canvases are written as
JSON: "palette" is a list of hex colours, and "rows" holds one string per pixel row,
top to bottom, one character per pixel. The character "." is a transparent pixel. Any
other character is a palette index using this alphabet, in order:
{symbols}

What the artist asked for:
{task}
The canvas before the edit:
{before_json}

{context}The edit that was produced:
{after_json}

{image_note}
Compare the two renders and judge the edit the way the artist will when it appears on
their canvas at this size:
- Does it do what was asked, completely, and is the new work recognisable at a glance?
- Is the new work in keeping with the drawing: line weight matching the existing
  lines (one pixel unless the artist draws heavier), sensible proportions, placed where
  it belongs?
- Is everything the request did not concern still exactly as it was before?

If the edit is good as it stands, reply with "verdict": "good" and leave "problems",
"palette" and "rows" empty. Small imperfections that the artist would not notice are
not worth a rewrite.

If it needs fixing, reply with "verdict": "fix", say in "problems" what is wrong in a
sentence or two, and put the corrected canvas in "palette" and "rows". Start from the
edit and change only what fixes the problems.

{format_rules}"""

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
palette and is written in the same format as the canvas:

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
referring to it makes sense and you do not hand back the same thing:
{previous_json}
"""

RESULT_SCHEMA = {
    "type": "object",
    "properties": {
        "palette": {"type": "array", "items": {"type": "string"}},
        "rows": {"type": "array", "items": {"type": "string"}},
    },
    "required": ["palette", "rows"],
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


def task_text(hint, color):
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


def build_context(ctx, size):
    """Returns (text for the prompt, the discarded last result or None)."""
    text = layers_text(ctx, size) + history_text(ctx)
    previous = open_matching(ctx["previous"], size) if ctx.get("previous") and ctx.get("history") else None
    if previous is not None:
        text += PREVIOUS.format(previous_json=canvas_json(*encode_canvas(previous))) + "\n"
    return text, previous


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
    palette, rows = encode_canvas(img)
    images = [("canvas.png", "the canvas", img)]
    if previous is not None:
        images.append(("discarded.png", "your last result, which the artist discarded", previous))
    prompt = INSTRUCTIONS.format(
        width=img.width, height=img.height, symbols=SYMBOLS,
        canvas_json=canvas_json(palette, rows),
        context=context,
        image_note=image_note(backend, images),
        task=task_text(hint, color),
        format_rules=format_rules(img),
    )
    for attempt in range(2):
        try:
            return decode_canvas(ask(backend, api_key, prompt, images, RESULT_SCHEMA), img.width, img.height)
        except BadReplyError:
            if attempt == 1:
                raise


def review_pass(backend, api_key, img, edit, hint, color, context):
    """Shows Claude a render of its edit. Returns a corrected image, or None if it is fine."""
    images = [("before.png", "the canvas before the edit", img), ("edit.png", "the edit being checked", edit)]
    prompt = REVIEW.format(
        width=img.width, height=img.height, symbols=SYMBOLS,
        task=task_text(hint, color),
        before_json=canvas_json(*encode_canvas(img)),
        context=context,
        after_json=canvas_json(*encode_canvas(edit)),
        image_note=image_note(backend, images),
        format_rules=format_rules(img),
    )
    review = ask(backend, api_key, prompt, images, REVIEW_SCHEMA)
    if not isinstance(review, dict) or review.get("verdict") != "fix":
        print("claude_art: check passed", file=sys.stderr)
        return None
    print("claude_art: fixing - " + str(review.get("problems", "")), file=sys.stderr)
    return decode_canvas(review, img.width, img.height)


def main():
    for path in (OUTPUT_PATH, ERROR_PATH, STATUS_PATH):
        try:
            os.remove(path)
        except OSError:
            pass

    try:
        job = sys.stdin.read().split("\n", 3) + ["", "", "", ""]
        backend, api_key, color = job[0].strip().lower(), job[1].strip(), job[2].strip() or "#000000"
        hint = " ".join(job[3].split()).replace('"', "'")

        try:
            img = Image.open(INPUT_PATH).convert("RGBA")
        except OSError:
            raise BridgeError("Could not read the canvas image.")
        if img.width * img.height > MAX_CELLS:
            raise BridgeError(f"That is {img.width}x{img.height} pixels; the assistant handles up to 128x128 at a time. Select a smaller area first.")
        if backend not in ("cli", "api"):
            raise BridgeError(f"Unknown backend '{backend}'.")

        context, previous = build_context(load_context(), img.size)

        set_status("Sketching it out")
        edit = first_pass(backend, api_key, img, hint, color, context, previous)
        edit.save(OUTPUT_PATH)

        # From here on there is always a usable result, so a failed check keeps it
        # rather than failing the whole request.
        for round_no in range(REVIEW_ROUNDS):
            set_status("Checking how it looks" if round_no == 0 else "Checking the touch-up")
            try:
                fixed = review_pass(backend, api_key, img, edit, hint, color, context)
            except BridgeError as e:
                print(f"claude_art: check skipped - {e}", file=sys.stderr)
                break
            if fixed is None:
                break
            edit = fixed
            edit.save(OUTPUT_PATH)
    except BridgeError as e:
        with open(ERROR_PATH, "w", encoding="utf-8") as f:
            f.write(str(e))
        print(f"claude_art: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
