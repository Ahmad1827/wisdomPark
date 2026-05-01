import urllib.request
import json
import os
import socket
from PIL import Image
from io import BytesIO

socket.setdefaulttimeout(5)

existing_data = []
if os.path.exists("dataset.json"):
    try:
        with open("dataset.json", "r") as f:
            existing_data = json.load(f)
    except:
        pass

existing_names = {item['name'] for item in existing_data}
new_items = []
targets = []

try:
    req_pkmn = urllib.request.Request("https://pokeapi.co/api/v2/pokemon?limit=2000", headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req_pkmn) as response:
        data = json.loads(response.read().decode())
        for item in data['results']:
            pkmn_id = item['url'].split('/')[-2]
            targets.append({
                "name": f"pokemon_{pkmn_id}_{item['name']}",
                "url": f"https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/{pkmn_id}.png",
                "category": "structure",
                "scale": 1.5
            })
except:
    pass

try:
    req_items = urllib.request.Request("https://pokeapi.co/api/v2/item?limit=2000", headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req_items) as response:
        data = json.loads(response.read().decode())
        for item in data['results']:
            targets.append({
                "name": item['name'],
                "url": f"https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/items/{item['name']}.png",
                "category": "clutter",
                "scale": 0.5
            })
except:
    pass

for target in targets:
    name = target["name"]
    if name in existing_names:
        print(f"Skipped duplicate: {name}")
        continue
        
    try:
        img_req = urllib.request.Request(target["url"], headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(img_req) as img_response:
            img_data = img_response.read()
            img = Image.open(BytesIO(img_data)).convert("RGBA")
            bbox = img.getbbox()
            if bbox: img = img.crop(bbox)
            
            w, h = img.size
            min_lum, max_lum = 255, 0
            for y in range(h):
                for x in range(w):
                    r, g, b, a = img.getpixel((x, y))
                    if a > 20:
                        lum = (0.299 * r + 0.587 * g + 0.114 * b)
                        min_lum = min(min_lum, lum)
                        max_lum = max(max_lum, lum)
            
            lum_range = max(1, max_lum - min_lum)
            pixels = []
            for y in range(h):
                row = ""
                for x in range(w):
                    r, g, b, a = img.getpixel((x, y))
                    if a > 20:
                        lum = (0.299 * r + 0.587 * g + 0.114 * b)
                        rel_lum = (lum - min_lum) / lum_range
                        if rel_lum < 0.15: row += "O"
                        elif rel_lum < 0.40: row += "S"
                        elif rel_lum < 0.70: row += "X"
                        elif rel_lum < 0.90: row += "H"
                        else: row += "W"
                    else: row += "."
                pixels.append(row)
            
            new_items.append({
                "name": name,
                "category": target["category"],
                "scale": target["scale"],
                "width": w,
                "height": h,
                "pixels": pixels
            })
            print(f"Added new item: {name}")
            
            if len(new_items) % 50 == 0:
                with open("dataset.json", "w") as f:
                    json.dump(existing_data + new_items, f, indent=2)
                    
    except socket.timeout:
        print(f"TIMEOUT: {name} took too long to respond.")
    except Exception as e:
        print(f"ERROR on {name}: {e}")

final_data = existing_data + new_items
with open("dataset.json", "w") as f:
    json.dump(final_data, f, indent=2)