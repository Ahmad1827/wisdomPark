"""Writes view.html next to sheet.png/sheet.json: the animations playing at their real
speed, enlarged with hard pixel edges (image viewers smooth a scaled sprite, which makes
pixel art look blurry), plus the whole sheet.

    python scripts/sprite/viewer.py <sprite folder> [more folders...]

Several folders are shown one under the other, for comparing versions.
"""

import base64
import json
import os
import sys

PAGE = """<!doctype html>
<html><head><meta charset="utf-8"><title>Sprite viewer</title>
<style>
  body { background: #24242c; color: #ddd; font: 14px sans-serif; margin: 16px; }
  h2 { font-size: 15px; margin: 24px 0 8px; }
  .row { display: flex; gap: 24px; flex-wrap: wrap; align-items: flex-end; }
  figure { margin: 0; text-align: center; }
  canvas, img { image-rendering: pixelated; image-rendering: crisp-edges;
                background: repeating-conic-gradient(#3a3a46 0 25%, #30303a 0 50%) 0 0 / 16px 16px; }
  .controls { margin: 8px 0 16px; }
  button { background: #3a3a46; color: #ddd; border: 1px solid #555; padding: 4px 10px; cursor: pointer; }
  button.on { background: #5a5aa0; }
</style></head><body>
<div class="controls">Zoom:
  <button data-z="4">4x</button><button data-z="6">6x</button><button data-z="8" class="on">8x</button><button data-z="12">12x</button>
</div>
<div id="root"></div>
<script>
const SPRITES = __SPRITES__;
let zoom = 8;
const players = [];
function build() {
  const root = document.getElementById('root');
  root.innerHTML = '';
  players.length = 0;
  for (const s of SPRITES) {
    const h = document.createElement('h2'); h.textContent = s.name; root.appendChild(h);
    const row = document.createElement('div'); row.className = 'row'; root.appendChild(row);
    const img = new Image(); img.src = s.png;
    const fw = s.meta.meta.frameSize.w, fh = s.meta.meta.frameSize.h;
    for (const tag of s.meta.meta.frameTags) {
      const names = Object.keys(s.meta.frames).slice(tag.from, tag.to + 1);
      for (const scale of [1, zoom]) {
        const fig = document.createElement('figure');
        const c = document.createElement('canvas'); c.width = fw * scale; c.height = fh * scale;
        const cap = document.createElement('figcaption'); cap.textContent = tag.name + ' ' + scale + 'x';
        fig.append(c, cap); row.appendChild(fig);
        players.push({ c, img, names, frames: s.meta.frames, scale, i: 0, t: 0 });
      }
    }
    const fig = document.createElement('figure');
    const sheet = new Image(); sheet.src = s.png;
    sheet.onload = () => { sheet.width = sheet.naturalWidth * Math.max(2, zoom / 2); };
    const cap = document.createElement('figcaption'); cap.textContent = 'sheet';
    fig.append(sheet, cap); row.appendChild(fig);
  }
}
let last = performance.now();
function tick(now) {
  const dt = now - last; last = now;
  for (const p of players) {
    if (!p.img.complete) continue;
    p.t += dt;
    const f = p.frames[p.names[p.i]];
    if (p.t >= f.duration) { p.t = 0; p.i = (p.i + 1) % p.names.length; }
    const r = p.frames[p.names[p.i]].frame;
    const g = p.c.getContext('2d');
    g.imageSmoothingEnabled = false;
    g.clearRect(0, 0, p.c.width, p.c.height);
    g.drawImage(p.img, r.x, r.y, r.w, r.h, 0, 0, r.w * p.scale, r.h * p.scale);
  }
  requestAnimationFrame(tick);
}
document.querySelectorAll('button[data-z]').forEach(b => b.onclick = () => {
  document.querySelectorAll('button[data-z]').forEach(x => x.classList.remove('on'));
  b.classList.add('on'); zoom = +b.dataset.z; build();
});
build(); requestAnimationFrame(tick);
</script></body></html>
"""


def load(folder):
    with open(os.path.join(folder, "sheet.png"), "rb") as f:
        png = "data:image/png;base64," + base64.b64encode(f.read()).decode("ascii")
    with open(os.path.join(folder, "sheet.json"), encoding="utf-8") as f:
        meta = json.load(f)
    return {"name": os.path.basename(os.path.normpath(folder)), "png": png, "meta": meta}


def main():
    folders = sys.argv[1:] or ["."]
    sprites = [load(f) for f in folders]
    out = os.path.join(folders[-1], "view.html")
    with open(out, "w", encoding="utf-8") as f:
        f.write(PAGE.replace("__SPRITES__", json.dumps(sprites)))
    print(out)


if __name__ == "__main__":
    main()
