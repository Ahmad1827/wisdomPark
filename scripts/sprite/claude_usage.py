"""Asks Claude Code where the plan's usage limits stand, for the chat panel's header.

Claude Code reports them only while it runs, so this sends it the smallest request there is
(Haiku, no tools, a one-line system prompt) and keeps what it reports. Writes:
    chat_usage_probe.json   {"five_hour": {"utilization", "resetsAt"}, "seven_day": {...}}
and exits non-zero when Claude Code is missing or reported nothing.
"""

import json
import shutil
import subprocess
import sys

OUT_PATH = "chat_usage_probe.json"


def main():
    exe = shutil.which("claude")
    if not exe:
        sys.exit(1)
    cmd = [exe, "-p", "--output-format", "stream-json", "--verbose", "--model", "claude-haiku-5-5",
           "--system-prompt", "Reply with the single word ok.", "--tools", "", "--strict-mcp-config",
           "--no-session-persistence"]
    proc = subprocess.run(cmd, input="ok", capture_output=True, text=True, encoding="utf-8", timeout=120,
                          creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    windows = None
    for line in proc.stdout.splitlines():
        try:
            event = json.loads(line)
        except json.JSONDecodeError:
            continue
        if event.get("type") == "rate_limit_event":
            windows = event.get("rate_limit_info", {}).get("unifiedWindows") or windows
    if not windows:
        sys.exit(1)
    with open(OUT_PATH, "w", encoding="utf-8") as f:
        json.dump(windows, f)


if __name__ == "__main__":
    main()
