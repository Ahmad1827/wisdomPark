import urllib.request
from PIL import Image
from io import BytesIO

with open("dataset.txt", "w") as f:
    for i in range(1, 152):
        url = f"https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/{i}.png"
        try:
            req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(req) as response:
                img = Image.open(BytesIO(response.read())).convert("RGBA")
                img = img.resize((48, 48), Image.Resampling.NEAREST)
                for y in range(48):
                    row = ""
                    for x in range(48):
                        r, g, b, a = img.getpixel((x, y))
                        if a > 20 and (r < 250 or g < 250 or b < 250):
                            row += "X"
                        else:
                            row += "."
                    f.write(row + "\n")
                f.write("\n")
        except:
            pass