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
                        if a > 20:
                            luminance = (0.299 * r + 0.587 * g + 0.114 * b)
                            if luminance < 40:
                                row += "O"
                            elif luminance < 100:
                                row += "S"
                            elif luminance < 180:
                                row += "X"
                            else:
                                row += "H"
                        else:
                            row += "."
                    f.write(row + "\n")
                f.write("\n")
                print(f"Added Pokemon: {i}")
        except:
            pass