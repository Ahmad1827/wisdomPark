import urllib.request
import json
import os
import sys

def get_slug(input_str):
    clean = input_str.strip().rstrip("/")
    if "palette-list/" in clean:
        clean = clean.split("palette-list/")[-1]
    elif "/" in clean:
        clean = clean.split("/")[-1]
    return clean

def fetch_single(slug_or_url):
    slug = get_slug(slug_or_url)
    url = f"https://lospec.com/palette-list/{slug}.json"
    headers = {'User-Agent': 'Mozilla/5.0'}
    req = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=5) as resp:
            data = json.loads(resp.read().decode('utf-8'))
            name = data.get("name", slug.replace("-", " ").title())
            colors = ["#" + c.lstrip("#") for c in data.get("colors", [])]
            if len(colors) >= 4:
                existing = {}
                filepath = "assets/palettes.txt"
                if os.path.exists(filepath):
                    with open(filepath, "r", encoding="utf-8") as f:
                        lines = [line.strip() for line in f if line.strip()]
                        for i in range(0, len(lines), 2):
                            if i + 1 < len(lines):
                                existing[lines[i].lower()] = (lines[i], lines[i + 1])

                existing[name.lower()] = (name, " ".join(colors))

                os.makedirs("assets", exist_ok=True)
                with open(filepath, "w", encoding="utf-8") as f:
                    for orig_name, color_str in existing.values():
                        f.write(orig_name + "\n")
                        f.write(color_str + "\n")
                return True
    except Exception:
        return False
    return False

if __name__ == "__main__":
    if len(sys.argv) > 1:
        fetch_single(sys.argv[1])