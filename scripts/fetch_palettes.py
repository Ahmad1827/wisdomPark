import json
import urllib.request
import os

PALETTE_SLUGS = [
    "endesga-32",
    "resurrect-64",
    "apollo",
    "pico-8",
    "sweetie-16",
    "slso8",
    "dawnbringer-32",
    "aap-64",
    "zughy-32",
    "vinik-24",
    "kirokaze-gameboy",
    "bubblegum-16",
    "nyx8",
    "edg-32",
    "steam-lords",
    "fantasy-24",
    "curiosities",
    "arcade-cabinet-24",
    "crimso-11",
    "retrocal-8"
]

FALLBACK_PALETTES = [
    {
        "name": "Endesga 32",
        "colors": [
            "#be4a2f", "#d77643", "#ead4aa", "#e4a672", "#b86f56", "#733e39", "#3e2731", "#26171e",
            "#a22633", "#e43b44", "#f77622", "#feae34", "#fee761", "#63c74d", "#3e8948", "#265c42",
            "#193c3e", "#124e89", "#0099db", "#2ce8f5", "#ffffff", "#c0cbdc", "#8b9bb4", "#5a6988",
            "#3a4466", "#262b44", "#181425", "#ff0044", "#68386c", "#b55088", "#f6757a", "#e8b796"
        ]
    },
    {
        "name": "PICO-8",
        "colors": [
            "#000000", "#1D2B53", "#7E2553", "#008751", "#AB5236", "#5F574F", "#C2C3C7", "#FFF1E8",
            "#FF004D", "#FFA300", "#FFEC27", "#00E436", "#29ADFF", "#83769C", "#FF77A8", "#FFCCAA"
        ]
    },
    {
        "name": "Sweetie 16",
        "colors": [
            "#1a1c2c", "#5d275d", "#b13e53", "#ef7d57", "#ffcd75", "#a7f070", "#38b764", "#257179",
            "#29366f", "#3b5dc9", "#41a6f6", "#73eff7", "#f4f4f4", "#94b0c2", "#566c86", "#333c57"
        ]
    },
    {
        "name": "Resurrect 64",
        "colors": [
            "#2e222f", "#3e3546", "#625565", "#966c6c", "#ab947a", "#697b5b", "#526542", "#4f8fba",
            "#73eff7", "#f4f4f4", "#94b0c2", "#566c86", "#333c57", "#e43b44", "#f77622", "#feae34",
            "#fee761", "#63c74d", "#3e8948", "#265c42", "#193c3e", "#124e89", "#0099db", "#2ce8f5",
            "#ffffff", "#c0cbdc", "#8b9bb4", "#5a6988", "#3a4466", "#262b44", "#181425", "#ff0044",
            "#68386c", "#b55088", "#f6757a", "#e8b796", "#c28569", "#a25353", "#742f2f", "#441c1c",
            "#211818", "#ffc825", "#ff8244", "#d34549", "#9b1f48", "#5e153f", "#340c30", "#190826",
            "#0e071b", "#5b3138", "#8e5252", "#ba7563", "#e3a073", "#ffd093", "#e07050", "#a84040"
        ]
    }
]

def fetch_lospec():
    results = []
    headers = {'User-Agent': 'Mozilla/5.0'}
    
    for slug in PALETTE_SLUGS:
        url = f"https://lospec.com/palette-list/{slug}.json"
        try:
            req = urllib.request.Request(url, headers=headers)
            with urllib.request.urlopen(req, timeout=3) as resp:
                data = json.loads(resp.read().decode('utf-8'))
                name = data.get("name", slug.replace("-", " ").title())
                colors = ["#" + c if not c.startswith("#") else c for c in data.get("colors", [])]
                if len(colors) >= 4:
                    results.append({"name": name, "colors": colors})
        except Exception:
            continue

    if not results:
        results = FALLBACK_PALETTES
    else:
        existing_names = {p["name"].lower() for p in results}
        for fb in FALLBACK_PALETTES:
            if fb["name"].lower() not in existing_names:
                results.append(fb)

    os.makedirs("assets", exist_ok=True)
    with open("assets/palettes.json", "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2)

if __name__ == "__main__":
    fetch_lospec()