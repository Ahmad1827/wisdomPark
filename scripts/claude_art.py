"""Claude pixel-art bridge.

Reads the canvas from temp_ai_input.png, asks Claude to finish the drawing (or to
carry out a typed request such as "make contour of a cat") and writes the result
to temp_ai_output.png. On failure it writes the reason to
temp_ai_error.txt and exits non-zero.

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

API_MODEL = "claude-opus-5-5"
MAX_CELLS = 128 * 128
MAX_INPUT_COLORS = 40
CLI_TIMEOUT_SECONDS = 600

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

{image_note}
{task}
Return the resulting canvas in exactly the same JSON shape: {{"palette": [...], "rows": [...]}}.

- A program parses your reply and paints it straight onto the canvas, so it must be
  exactly {height} rows of exactly {width} characters each. A row that is one character
  short shifts everything after it, so count carefully.
- Reuse the existing palette where you can. You may append new colours to the end of
  the palette when the work needs them, up to {max_colors} total. Never reorder or
  remove existing palette entries, because the rows refer to them by position.
"""

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

RESULT_SCHEMA = {
    "type": "object",
    "properties": {
        "palette": {"type": "array", "items": {"type": "string"}},
        "rows": {"type": "array", "items": {"type": "string"}},
    },
    "required": ["palette", "rows"],
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


def build_prompt(img, palette, rows, hint, color, image_note):
    task = TASK_REQUEST.format(request=hint, color=color) if hint else TASK_FINISH
    return INSTRUCTIONS.format(
        width=img.width,
        height=img.height,
        symbols=SYMBOLS,
        canvas_json=json.dumps({"palette": palette, "rows": rows}, indent=0),
        image_note=image_note,
        task=task,
        max_colors=len(SYMBOLS),
    )


def run_cli(img, palette, rows, hint, color):
    """Claude Code in headless mode: uses the user's own Claude Code login."""
    exe = shutil.which("claude")
    if not exe:
        raise BridgeError("Claude Code is not installed (the 'claude' command was not found). Install it, or switch the AI provider to Claude and enter an API key.")

    # A scratch directory keeps Claude Code away from this project's settings and
    # files; the only thing it can read there is the canvas preview.
    with tempfile.TemporaryDirectory(prefix="wisdompark_ai_") as workdir:
        preview_png(img).save(os.path.join(workdir, "canvas.png"))
        prompt = build_prompt(
            img, palette, rows, hint, color,
            "An enlarged render of the same canvas is saved as canvas.png in the current "
            "directory. Read it first so you can see the drawing.\n",
        )
        args = [exe, "-p", "--output-format", "json", "--system-prompt", SYSTEM_PROMPT,
                "--tools", "Read", "--allowedTools", "Read",
                "--strict-mcp-config", "--no-session-persistence"]
        # The schema makes Claude Code validate the reply's shape. A .cmd/.bat launcher
        # would mangle the quotes in it, so only the real executable gets it.
        if not exe.lower().endswith((".cmd", ".bat")):
            args += ["--json-schema", json.dumps(RESULT_SCHEMA)]
        try:
            proc = subprocess.run(
                args,
                input=prompt, capture_output=True, text=True, encoding="utf-8",
                cwd=workdir, timeout=CLI_TIMEOUT_SECONDS,
                # Without this, Claude Code pops a console window over the app on Windows
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
            )
        except subprocess.TimeoutExpired:
            raise BridgeError("Claude Code took too long to answer. Try a smaller canvas.")

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


def run_api(img, palette, rows, hint, color, api_key):
    """Anthropic API: for users who have an API key instead of Claude Code."""
    try:
        import anthropic
    except ImportError:
        raise BridgeError("The Claude API provider needs the Anthropic SDK. Run: pip install anthropic")
    if not api_key:
        raise BridgeError("No Claude API key set. Enter one in Settings, or switch the AI provider to Claude Code.")

    buf = io.BytesIO()
    preview_png(img).save(buf, format="PNG")
    prompt = build_prompt(img, palette, rows, hint, color, "The attached image is an enlarged render of the same canvas.\n")

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
            output_config={"effort": "medium", "format": {"type": "json_schema", "schema": RESULT_SCHEMA}},
            messages=[{
                "role": "user",
                "content": [
                    {"type": "image", "source": {"type": "base64", "media_type": "image/png",
                                                 "data": base64.standard_b64encode(buf.getvalue()).decode("ascii")}},
                    {"type": "text", "text": prompt},
                ],
            }],
        ) as stream:
            message = stream.get_final_message()
    except anthropic.AuthenticationError:
        raise BridgeError("The Claude API key was rejected. Check it in Settings.")
    except anthropic.RateLimitError:
        raise BridgeError("Claude API rate limit reached. Wait a moment and try again.")
    except anthropic.APIStatusError as e:
        raise BridgeError(f"Claude API error {e.status_code}: {e.message}"[:200])
    except anthropic.APIConnectionError:
        raise BridgeError("Could not reach the Claude API. Check your internet connection.")

    if message.stop_reason == "refusal":
        raise BridgeError("Claude declined this request.")
    if message.stop_reason == "max_tokens":
        raise BridgeError("The canvas is too large for Claude to finish in one go. Try a smaller canvas.")
    text = next((b.text for b in message.content if b.type == "text"), "")
    return extract_json(text)


def main():
    for path in (OUTPUT_PATH, ERROR_PATH):
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
            raise BridgeError(f"Claude works on canvases up to {MAX_CELLS} pixels (128x128). This one is {img.width}x{img.height}.")

        palette, rows = encode_canvas(img)
        if backend not in ("cli", "api"):
            raise BridgeError(f"Unknown backend '{backend}'.")

        for attempt in range(2):
            try:
                if backend == "cli":
                    result = run_cli(img, palette, rows, hint, color)
                else:
                    result = run_api(img, palette, rows, hint, color, api_key)
                decode_canvas(result, img.width, img.height).save(OUTPUT_PATH)
                break
            except BadReplyError:
                if attempt == 1:
                    raise
    except BridgeError as e:
        with open(ERROR_PATH, "w", encoding="utf-8") as f:
            f.write(str(e))
        print(f"claude_art: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
