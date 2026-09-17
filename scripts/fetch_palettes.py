import urllib.request
import json
import os

VIBE_PALETTES = [
    ("Sunset Warmth (Desert 32)", "endesga-32"),
    ("Neon Cyberpunk (Apollo 32)", "apollo"),
    ("Gothic Dungeon (Resurrect 64)", "resurrect-64"),
    ("Pastel Fantasy (Sweetie 16)", "sweetie-16"),
    ("Forest & Woodland (Nature 16)", "slso8"),
    ("Retro 8-Bit (Pico-8)", "pico-8"),
    ("Deep Space (Cosmic 8)", "nyx8"),
    ("Noir & Brass (Steampunk 16)", "steam-lords"),
    ("Cozy Autumn (Amber 24)", "curiosities"),
    ("Candy Pop (Bubblegum 16)", "bubblegum-16"),
    ("Toxic Wasteland (Acid 11)", "crimso-11"),
    ("Retro LCD (GameBoy 4)", "kirokaze-gameboy")
]

FALLBACKS = [
    {
        "name": "Sunset Warmth (Desert 32)",
        "colors": [
            "#be4a2f", "#d77643", "#ead4aa", "#e4a672", "#b86f56", "#733e39", "#3e2731", "#26171e",
            "#a22633", "#e43b44", "#f77622", "#feae34", "#fee761", "#63c74d", "#3e8948", "#265c42",
            "#193c3e", "#124e89", "#0099db", "#2ce8f5", "#ffffff", "#c0cbdc", "#8b9bb4", "#5a6988",
            "#3a4466", "#262b44", "#181425", "#ff0044", "#68386c", "#b55088", "#f6757a", "#e8b796"
        ]
    },
    {
        "name": "Pastel Fantasy (Sweetie 16)",
        "colors": [
            "#1a1c2c", "#5d275d", "#b13e53", "#ef7d57", "#ffcd75", "#a7f070", "#38b764", "#257179",
            "#29366f", "#3b5dc9", "#41a6f6", "#73eff7", "#f4f4f4", "#94b0c2", "#566c86", "#333c57"
        ]
    },
    {
        "name": "Retro 8-Bit (Pico-8)",
        "colors": [
            "#000000", "#1D2B53", "#7E2553", "#008751", "#AB5236", "#5F574F", "#C2C3C7", "#FFF1E8",
            "#FF004D", "#FFA300", "#FFEC27", "#00E436", "#29ADFF", "#83769C", "#FF77A8", "#FFCCAA"
        ]
    }
]

def fetch_palettes():
    results = []
    headers = {'User-Agent': 'Mozilla/5.0'}

    for vibe_name, slug in VIBE_PALETTES:
        url = f"https://lospec.com/palette-list/{slug}.json"
        try:
            req = urllib.request.Request(url, headers=headers)
            with urllib.request.urlopen(req, timeout=3) as resp:
                data = json.loads(resp.read().decode('utf-8'))
                colors = ["#" + c.lstrip("#") for c in data.get("colors", [])]
                if len(colors) >= 4:
                    results.append({"name": vibe_name, "colors": colors})
        except Exception:
            continue

    if not results:
        results = FALLBACKS

    os.makedirs("assets", exist_ok=True)
    with open("assets/palettes.txt", "w", encoding="utf-8") as f:
        for p in results:
            f.write(p["name"] + "\n")
            f.write(" ".join(p["colors"]) + "\n")

if __name__ == "__main__":
    fetch_palettes()