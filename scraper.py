import urllib.request
import json
from PIL import Image
from io import BytesIO

with open("dataset.txt", "w") as f:
    req = urllib.request.Request("https://pokeapi.co/api/v2/item?limit=100", headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req) as response:
        data = json.loads(response.read().decode())
        
    for item in data['results']:
        name = item['name']
        url = f"https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/items/{name}.png"
        try:
            img_req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(img_req) as img_response:
                img = Image.open(BytesIO(img_response.read())).convert("RGBA")
                
                bbox = img.getbbox()
                if bbox:
                    img = img.crop(bbox)
                
                w, h = img.size
                
                min_lum = 255
                max_lum = 0
                for y in range(h):
                    for x in range(w):
                        r, g, b, a = img.getpixel((x, y))
                        if a > 20:
                            lum = (0.299 * r + 0.587 * g + 0.114 * b)
                            if lum < min_lum: min_lum = lum
                            if lum > max_lum: max_lum = lum
                
                lum_range = max_lum - min_lum
                if lum_range == 0: lum_range = 1 
                
                for y in range(h):
                    row = ""
                    for x in range(w):
                        r, g, b, a = img.getpixel((x, y))
                        if a > 20:
                            lum = (0.299 * r + 0.587 * g + 0.114 * b)
                            rel_lum = (lum - min_lum) / lum_range
                            
                            if rel_lum < 0.15:
                                row += "O"
                            elif rel_lum < 0.40:
                                row += "S"
                            elif rel_lum < 0.70:
                                row += "X"
                            elif rel_lum < 0.90:
                                row += "H"
                            else:
                                row += "W"
                        else:
                            row += "."
                    f.write(row + "\n")
                f.write("\n")
                print(f"Added Dynamic Item: {name}")
        except:
            pass