import argparse
import os
import sys
import base64
import json
import http.client
from PIL import Image, ImageDraw

def main():
    # 1. FORCE THE DELAY IMMEDIATELY BEFORE ANYTHING ELSE RUNS
    import time
    print("Forcing 5 second delay for UI loading test...")
    time.sleep(5)

    parser = argparse.ArgumentParser()
    parser.add_argument("--provider", type=str, required=True)
    parser.add_argument("--key", type=str, required=True)
    parser.add_argument("--prompt", type=str, required=True)
    args = parser.parse_args()

    provider = args.provider.lower()
    input_path = "temp_ai_input.png"
    output_path = "temp_ai_output.png"

    # 2. SAFE WRAP FILE REMOVAL SO IT CANNOT CRASH THE SCRIPT
    try:
        if os.path.exists(output_path):
            os.remove(output_path)
    except Exception as e:
        print(f"Warning: Could not clear old output file: {e}")

    try:
        sketch_img = Image.open(input_path)
        width, height = sketch_img.size

        # Shrink image to prevent network payload choke
        max_preview_dim = 512
        preview_img = sketch_img.copy()
        preview_img.thumbnail((max_preview_dim, max_preview_dim))
        temp_thumb_path = "temp_ai_thumb.jpg"
        preview_img.convert("RGB").save(temp_thumb_path, "JPEG", quality=80)

        with open(temp_thumb_path, "rb") as f:
            image_b64 = base64.b64encode(f.read()).decode("utf-8")
        if os.path.exists(temp_thumb_path):
            os.remove(temp_thumb_path)

        # ROUTING MECHANISM FOR MULTIPLE PROVIDERS
        if "openai" in provider:
            conn = http.client.HTTPSConnection("api.openai.com", timeout=30)
            headers = {"Content-Type": "application/json", "Authorization": f"Bearer {args.key}"}
            url = "/v1/chat/completions"
            payload = {
                "model": "gpt-4o-mini",
                "messages": [{
                    "role": "user",
                    "content": [
                        {"type": "text", "text": f"Convert this sketch to vector coordinate lines matching prompt: {args.prompt}. Return ONLY a JSON list like: {{'lines': [[[x1,y1],[x2,y2]]]}}"},
                        {"type": "image_url", "image_url": {"url": f"data:image/jpeg;base64,{image_b64}"}}
                    ]
                }],
                "response_format": {"type": "json_object"}
            }
        elif "ollama" in provider:
            # Local offline fallback generation route
            conn = http.client.HTTPConnection("localhost", 11434, timeout=30)
            headers = {"Content-Type": "application/json"}
            url = "/api/chat"
            payload = {
                "model": "llava",
                "messages": [{
                    "role": "user",
                    "content": f"Transform this image according to: {args.prompt}. Output coordinate JSON: {{'lines': []}}",
                    "images": [image_b64]
                }],
                "stream": False
            }
        else:
            # Default to Gemini REST routing layout
            conn = http.client.HTTPSConnection("generativelanguage.googleapis.com", timeout=30)
            headers = {"Content-Type": "application/json"}
            url = f"/v1beta/models/gemini-2.5-flash:generateContent?key={args.key}"
            payload = {
                "contents": [{"parts": [
                    {"inlineData": {"mimeType": "image/jpeg", "data": image_b64}},
                    {"text": f"Transform this sketch to coordinate lines matching prompt: {args.prompt}. Return ONLY JSON structure: {{ 'lines': [ [ [x1,y1], [x2,y2] ] ] }}"}
                ]}],
                "generationConfig": {"responseMimeType": "application/json"}
            }

        print(f"Sending generation request package to {provider}...")
        conn.request("POST", url, body=json.dumps(payload), headers=headers)
        response = conn.getresponse()
        data = response.read().decode("utf-8")
        conn.close()

        # Parse text out and build vector canvas lines
        res_json = json.loads(data)
        text_out = ""
        if "openai" in provider:
            text_out = res_json["choices"][0]["message"]["content"]
        elif "ollama" in provider:
            text_out = res_json["message"]["content"]
        else:
            text_out = res_json["candidates"][0]["content"]["parts"][0]["text"]

        line_data = json.loads(text_out)
        output_image = Image.new('RGBA', (width, height), (255, 255, 255, 255))
        draw = ImageDraw.Draw(output_image)
        for line in line_data.get("lines", []):
            if len(line) < 2: continue
            flat_points = [coord for pt in line for coord in pt]
            draw.line(flat_points, fill=(15, 15, 20, 255), width=2)
        output_image.save(output_path)
        print("Success: temp_ai_output.png dropped successfully.")

    except Exception as e:
        print(f"Generation channel failed: {str(e)}")
        # FIX: Explicitly qualify PIL.Image to avoid local variable scoping panics
        import PIL.ImageOps
        import PIL.Image
        
        sketch = PIL.Image.open(input_path)
        inverted = PIL.ImageOps.invert(sketch.convert('RGB'))
        inverted.save(output_path)
        print("Success: Local fallback frame dropped.")

if __name__ == "__main__":
    main()