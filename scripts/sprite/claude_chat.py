"""Chat bridge between the app and Claude Code: one message from the artist in, a reply and
the frames Claude drew out.

The app sends a JSON job on stdin:
    {"dir": "projects/Knight.wpk/assistant", "width": 32, "height": 32, "fps": 12,
     "current_frame": 0, "frame_count": 3, "message": "now make him walk"}
and before that writes the timeline into <dir>/canvas/:
    frame_NNN.png   each frame as it looks (all visible layers)
    claude_NNN.png  each frame's "Claude" layer, when the project has one

Each project folder keeps one Claude Code session, so a message continues the
conversation, and sprite.py (Claude's drawing code) lives there too. Claude's frames
become timeline frames in order. Only frames whose render changed this turn are handed
back, so a frame the artist painted over by hand stays as it is unless Claude redraws it.

Writes:
    <dir>/reply.json    {"reply": text, "changed": [frame indices], "total": frames drawn}
    <dir>/frames/       every frame Claude's sprite.py draws (frames/NNN.png)
    <dir>/chat.jsonl    the conversation, one {"role", "text"} per line, for the chat panel
    temp_ai_status.txt  what Claude is doing right now (the app's loading screen shows it)
    temp_ai_error.txt   why it failed, when it exits non-zero
"""

import json
import os
import shutil
import subprocess
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from claude_sprite import ClaudeMissing, copy_support_files, run_claude, set_status  # noqa: E402

STATUS_PATH = "temp_ai_status.txt"
ERROR_PATH = "temp_ai_error.txt"

SYSTEM_PROMPT = """You are the pixel-art assistant inside wisdomPark, a sprite editor, talking with
the artist in a chat panel next to their canvas. You draw by writing sprite.py with the drawing
library in this folder (spritelib.py), rendering it with `python render.py`, and looking at
preview.png. Follow GUIDE.md. The frames sprite.py draws go onto the artist's timeline.

Not every message needs drawing: answer questions and discuss ideas without rendering.
Your final reply is shown in the chat: keep it short (one to four sentences), plain text, no
headings or lists. Say what you drew or changed and anything the artist should check.
Work only in this folder."""


def norm_bytes(path):
    """RGBA bytes with every pixel under half opacity made fully transparent, so two
    images that look the same compare equal."""
    img = Image.open(path).convert("RGBA")
    data = bytearray(img.tobytes())
    for i in range(3, len(data), 4):
        if data[i] < 128:
            data[i - 3:i + 1] = b"\0\0\0\0"
        else:
            data[i] = 255
    return img.size, bytes(data)


def same_image(a, b):
    return os.path.exists(a) and os.path.exists(b) and norm_bytes(a) == norm_bytes(b)


def is_blank(path):
    return not os.path.exists(path) or Image.open(path).convert("RGBA").getchannel("A").getextrema()[1] < 128


def log_chat(workdir, role, text):
    with open(os.path.join(workdir, "chat.jsonl"), "a", encoding="utf-8") as f:
        f.write(json.dumps({"role": role, "text": text}) + "\n")


def describe_canvas(job, workdir):
    """The part of the prompt that tells Claude what the timeline looks like right now."""
    w, h, n = job["width"], job["height"], job["frame_count"]
    canvas = os.path.join(workdir, "canvas")
    lines = [f"Canvas: {w}x{h} pixels. Timeline: {n} frame{'s' if n != 1 else ''} at {job.get('fps', 12)} fps; "
             f"the artist is looking at frame {job.get('current_frame', 0) + 1}."]
    drawn = [i for i in range(n) if not is_blank(os.path.join(canvas, "frame_%03d.png" % i))]
    if drawn:
        lines.append("The timeline as it looks now (all visible layers) is in canvas/frame_000.png to "
                     f"canvas/frame_{n - 1:03d}.png; frames with something drawn: {', '.join(str(i + 1) for i in drawn)}. "
                     "Read the ones that matter for this request.")
    else:
        lines.append("Nothing is drawn on the timeline yet.")

    edited = []
    for i in range(n):
        mine = os.path.join(canvas, "claude_%03d.png" % i)
        last = os.path.join(workdir, "last", "%03d.png" % i)
        if os.path.exists(mine) and os.path.exists(last) and not same_image(mine, last):
            edited.append(i)
    if edited:
        lines.append("The artist painted over your drawing by hand since your last turn on frame"
                     f"{'s' if len(edited) > 1 else ''} {', '.join(str(i + 1) for i in edited)}; your layer as it is now "
                     "is canvas/claude_NNN.png. Keep their changes (load_png() loads a frame so sprite.py can build "
                     "on it) unless they ask otherwise.")

    lines.append(
        f"How your work reaches the app: sprite.py's frames, all animations in order, are timeline frames 1, 2, 3 "
        f"and so on, so WIDTH, HEIGHT must be {w}, {h}. They go onto a layer called \"Claude\"; the artist's own "
        "layers are never touched. Only frames whose render changes are sent, so frames you leave alone stay as "
        "the artist has them. More frames in sprite.py adds frames at the end of the timeline; frames are never "
        "deleted, so tell the artist if some should go. render.py ends with TIMELINE lines saying which frames "
        "differ from the timeline: check that only the frames you meant to change are listed.")
    if not os.path.exists(os.path.join(workdir, "sprite.py")):
        lines.append("There is no sprite.py yet. Read GUIDE.md and spritelib.py first. When the artist asks for a "
                     "single sprite, give ANIMATIONS one animation with one frame, and add frames or animations when "
                     "they ask for them. If they want you to work from what they drew, load it with load_png().")
    return "\n".join(lines)


def render(workdir):
    proc = subprocess.run([sys.executable, "render.py"], cwd=workdir, capture_output=True, text=True,
                          encoding="utf-8", creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    if proc.returncode != 0:
        raise RuntimeError("sprite.py does not run: " + proc.stdout.strip()[-300:])


def changed_frames(workdir, frame_count):
    """Indices of the frames to hand to the app: new ones past the end of the timeline,
    and ones whose render differs from what Claude drew last turn (or, the first time,
    from what is on the Claude layer)."""
    frames_dir = os.path.join(workdir, "frames")
    total = len([f for f in os.listdir(frames_dir) if f.endswith(".png")])
    changed = []
    for i in range(total):
        new = os.path.join(frames_dir, "%03d.png" % i)
        last = os.path.join(workdir, "last", "%03d.png" % i)
        mine = os.path.join(workdir, "canvas", "claude_%03d.png" % i)
        if i >= frame_count:
            changed.append(i)
        elif os.path.exists(last):
            if not same_image(new, last):
                changed.append(i)
        elif os.path.exists(mine):
            if not same_image(new, mine):
                changed.append(i)
        elif not is_blank(new):
            changed.append(i)
    return changed, total


def fail(text, workdir=None):
    set_status(STATUS_PATH, "Failed")
    with open(ERROR_PATH, "w", encoding="utf-8") as f:
        f.write(text)
    if workdir:
        log_chat(workdir, "error", text)
    sys.exit(1)


def main():
    for path in (ERROR_PATH,):
        if os.path.exists(path):
            os.remove(path)
    try:
        job = json.loads(sys.stdin.read())
        workdir = os.path.abspath(job["dir"])
        message = job["message"].strip()
        int(job["width"]), int(job["height"]), int(job["frame_count"])
    except (ValueError, KeyError, TypeError) as e:
        fail(f"The chat request from the app was malformed ({e}).")
    if not message:
        fail("Type a message first.")

    os.makedirs(workdir, exist_ok=True)
    copy_support_files(workdir)
    for stale in ("reply.json",):
        if os.path.exists(os.path.join(workdir, stale)):
            os.remove(os.path.join(workdir, stale))
    log_chat(workdir, "artist", message)

    prompt = f"The artist says: {message}\n\n{describe_canvas(job, workdir)}"
    try:
        result, renders, stderr = run_claude(workdir, prompt, SYSTEM_PROMPT, lambda t: set_status(STATUS_PATH, t),
                                             model=job.get("model", ""), effort=job.get("effort", "high"))
    except ClaudeMissing as e:
        fail(str(e), workdir)
    if not result or result.get("is_error"):
        detail = str((result or {}).get("result") or stderr.strip() or "no answer")[:300]
        fail("Claude Code failed: " + detail, workdir)
    reply = str(result.get("result", "")).strip() or "Done."

    changed, total = [], 0
    if os.path.exists(os.path.join(workdir, "sprite.py")):
        set_status(STATUS_PATH, "Bringing the frames over")
        try:
            render(workdir)
        except RuntimeError as e:
            fail(str(e), workdir)
        size = Image.open(os.path.join(workdir, "frames", "000.png")).size
        if size != (int(job["width"]), int(job["height"])):
            fail(f"Claude drew {size[0]}x{size[1]} frames but the canvas is {job['width']}x{job['height']}. "
                 "Ask it to match the canvas size.", workdir)
        changed, total = changed_frames(workdir, int(job["frame_count"]))
        # What Claude drew this turn is what the next turn compares against
        last = os.path.join(workdir, "last")
        shutil.rmtree(last, ignore_errors=True)
        shutil.copytree(os.path.join(workdir, "frames"), last)

    with open(os.path.join(workdir, "reply.json"), "w", encoding="utf-8") as f:
        json.dump({"reply": reply, "changed": changed, "total": total}, f)
    log_chat(workdir, "claude", reply)
    set_status(STATUS_PATH, "Done")


if __name__ == "__main__":
    main()
