"""Has Claude Code draw a spritesheet by writing and rendering a sprite script.

Prototype entry point (the app will call it the same way later):

    python scripts/sprite/claude_sprite.py --out projects/sprites/knight \
        --size 32x32 --anim idle:4:6 --anim walk:6:10 \
        "a small knight with a blue cape and a sword on his back"

A follow-up on the same --out folder continues the same Claude session, so
"make the cape red" only changes what it says:

    python scripts/sprite/claude_sprite.py --out projects/sprites/knight "make the cape red"

The folder keeps sprite.py (Claude's drawing code), sheet.png, sheet.json and
preview.png. status.txt holds one line on what Claude is doing right now.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
SUPPORT_FILES = ("spritelib.py", "render.py", "GUIDE.md")
SESSION_FILE = "session.txt"
STATUS_FILE = "status.txt"
TIMEOUT_SECONDS = 1200

SYSTEM_PROMPT = """You are a pixel artist working inside a sprite tool. You draw by writing
sprite.py with the drawing library in this folder, rendering it with `python render.py`,
and looking at preview.png. Follow GUIDE.md. Work only in this folder. When the sheet is
good, stop and reply with a short summary of what you drew and any compromise you made."""


def set_status(path, text):
    try:
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
    except OSError:
        pass
    print(text, flush=True)


def copy_support_files(workdir):
    for name in SUPPORT_FILES:
        shutil.copy(os.path.join(HERE, name), os.path.join(workdir, name))


class ClaudeMissing(Exception):
    pass


def run_claude(workdir, prompt, system_prompt, status, model="", effort="high", timeout=TIMEOUT_SECONDS):
    """Runs Claude Code headless in `workdir`, continuing the session saved there if any.

    Claude may only read, write and edit files in `workdir` and run `python render.py`.
    `status(text)` is called with a short line whenever Claude starts on something.
    Returns (result event or None, number of renders, stderr text).
    """
    exe = shutil.which("claude")
    if not exe:
        raise ClaudeMissing("Claude Code is not installed (the 'claude' command was not found).")
    session_path = os.path.join(workdir, SESSION_FILE)
    session = open(session_path).read().strip() if os.path.exists(session_path) else ""

    cmd = [exe, "-p", "--output-format", "stream-json", "--verbose",
           "--append-system-prompt", system_prompt,
           "--tools", "Read,Write,Edit,Bash",
           "--allowedTools", "Read", "Edit(./**)", "Write(./**)", "Bash(python render.py)",
           "--permission-mode", "dontAsk",
           "--strict-mcp-config", "--effort", effort]
    if model:
        cmd += ["--model", model]
    if session:
        cmd += ["--resume", session]

    status("Starting Claude")
    proc = subprocess.Popen(cmd, cwd=workdir, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True, encoding="utf-8",
                            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    proc.stdin.write(prompt)
    proc.stdin.close()
    # stderr is drained on its own thread so a chatty Claude Code cannot fill the pipe and stall
    err_chunks = []
    drain = threading.Thread(target=lambda: err_chunks.append(proc.stderr.read()), daemon=True)
    drain.start()

    result, renders = None, 0
    for line in proc.stdout:
        try:
            event = json.loads(line)
        except json.JSONDecodeError:
            continue
        if event.get("type") == "system" and event.get("session_id"):
            with open(session_path, "w") as f:
                f.write(event["session_id"])
        elif event.get("type") == "assistant":
            for block in event.get("message", {}).get("content", []):
                if block.get("type") == "tool_use":
                    if block.get("name") == "Bash":
                        renders += 1
                    status(describe_tool(block) + (f" (render {renders})" if block.get("name") == "Bash" else ""))
        elif event.get("type") == "result":
            result = event
    try:
        proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
    drain.join(timeout=5)
    return result, renders, "".join(err_chunks)


def describe_tool(block):
    name, inp = block.get("name"), block.get("input", {})
    if name == "Bash":
        return "Rendering the sheet"
    if name == "Read":
        target = os.path.basename(inp.get("file_path", ""))
        return "Looking at the result" if target.endswith(".png") else f"Reading {target}"
    if name in ("Write", "Edit"):
        return "Drawing (editing sprite.py)"
    return name


def first_prompt(args):
    w, h = args.size.lower().split("x")
    anims = {}
    for spec in args.anim:
        name, frames, fps = (spec.split(":") + ["8"])[:3]
        anims[name] = {"frames": int(frames), "fps": int(fps)}
    return (f"Make a spritesheet: {args.request}\n\n"
            f"Frame size: {w}x{h}. Animations: {json.dumps(anims)}.\n"
            + (f"Use only this palette: {args.palette}\n" if args.palette else "")
            + "Read GUIDE.md and spritelib.py first, then follow the workflow.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("request")
    ap.add_argument("--out", required=True)
    ap.add_argument("--size", default="32x32")
    ap.add_argument("--anim", action="append", default=[])
    ap.add_argument("--palette", default="")
    ap.add_argument("--model", default="")
    ap.add_argument("--effort", default="high")
    args = ap.parse_args()

    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    copy_support_files(out)

    session_path = os.path.join(out, SESSION_FILE)
    session = open(session_path).read().strip() if os.path.exists(session_path) else ""
    if session:
        prompt = (f"Change request: {args.request}\n"
                  "Edit sprite.py, re-render and check every frame as before.")
    else:
        if not args.anim:
            args.anim = ["idle:4:6", "walk:6:10"]
        prompt = first_prompt(args)

    status_path = os.path.join(out, STATUS_FILE)
    result, renders, stderr = run_claude(out, prompt, SYSTEM_PROMPT, lambda t: set_status(status_path, t),
                                         model=args.model, effort=args.effort)
    if not result or result.get("is_error"):
        detail = (result or {}).get("result") or stderr[:500]
        set_status(status_path, "Failed")
        sys.exit(f"Claude Code failed: {detail}")
    set_status(status_path, "Done")
    print("\n" + str(result.get("result", "")).strip())
    # total_cost_usd is only an API-price estimate; on a Claude subscription nothing is
    # billed and the run counts toward the plan's usage limits instead, so it isn't shown.
    usage = result.get("usage", {})
    tokens = usage.get("input_tokens", 0) + usage.get("output_tokens", 0)
    print(f"\n{renders} renders, {result.get('num_turns', 0)} turns, ~{tokens // 1000}k new tokens, "
          f"{result.get('duration_ms', 0) / 1000:.0f}s. Output in {out}")


if __name__ == "__main__":
    main()
