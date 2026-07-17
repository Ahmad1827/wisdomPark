import argparse
import os
import sys
import json
import urllib.request
import urllib.error
import textwrap
import base64
from PIL import Image, ImageDraw
from PIL import ImageFont

def render_error_to_image(width, height, title, error_msg, out_path):
    # Pure black solid background
    img = Image.new('RGBA', (width, height), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)
    
    # Maximum neon red caution box
    draw.rectangle([15, 15, width - 15, height - 15], outline=(255, 0, 0, 255), width=5)
    
    font_path = os.path.join("assets", "font.otf")
    if os.path.exists(font_path):
        try:
            title_font = ImageFont.truetype(font_path, 48)
            body_font = ImageFont.truetype(font_path, 28)
        except:
            title_font = ImageFont.load_default()
            body_font = ImageFont.load_default()
    else:
        title_font = ImageFont.load_default()
        body_font = ImageFont.load_default()

    margin_x = 60
    current_y = 80
    
    # Draw a pure white solid banner under the header to force contrast
    draw.rectangle([margin_x - 10, current_y - 5, width - margin_x + 10, current_y + 60], fill=(255, 255, 255, 255))
    # Draw title text in pure pitch black on top of the white banner
    draw.text((margin_x, current_y), f" ERROR: {title.upper()} ", fill=(0, 0, 0, 255), font=title_font)
    
    current_y += 110
    draw.line([margin_x, current_y, width - margin_x, current_y], fill=(255, 0, 0, 255), width=3)
    current_y += 50
    
    # Split the lines out cleanly
    wrapped_lines = textwrap.wrap(error_msg, width=75)
    for line in wrapped_lines:
        # Force the body font to maximum fluorescent white text
        draw.text((margin_x, current_y), line, fill=(255, 255, 255, 255), font=body_font)
        current_y += 45

    img.save(out_path)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--provider", type=str, required=True)
    parser.add_argument("--key", type=str, required=True)
    parser.add_argument("--prompt", type=str, required=True)
    args = parser.parse_args()

    provider = args.provider.lower()
    input_path = "temp_ai_input.png"
    output_path = "temp_ai_output.png"

    try:
        if os.path.exists(output_path):
            os.remove(output_path)
    except:
        pass

    try:
        sketch_img = Image.open(input_path)
        width, height = sketch_img.size
    except:
        width, height = 1024, 1024

    try:
        if provider == "openai":
            url = "https://api.openai.com/v1/images/generations"
            headers = {"Authorization": f"Bearer {args.key}", "Content-Type": "application/json"}
            payload = {"prompt": args.prompt, "model": "dall-e-3", "n": 1, "size": "1024x1024"}
            
        elif provider == "gemini":
            url = f"https://generativelanguage.googleapis.com/v1beta/models/imagen-3.0-generate-001:generateImages?key={args.key}"
            headers = {"Content-Type": "application/json"}
            payload = {
                "prompt": args.prompt,
                "numberOfImages": 1,
                "aspectRatio": "1:1",
                "outputMimeType": "image/jpeg"
            }
            
        elif provider == "claude" or provider == "anthropic":
            raise ValueError("Anthropic Claude models currently only support text and vision analysis. They do not have a public image generation endpoint.")
            
        elif provider == "ollama":
            raise ValueError("Ollama runs local LLMs for text. It does not generate images natively. You need a Stable Diffusion API or ComfyUI bridge for local images.")
            
        elif provider == "openrouter":
            raise ValueError("OpenRouter primarily routes text. To use OpenRouter for images, you must configure a specific image model payload (e.g., google/imagen-3) in the settings.")
            
        else:
            url = f"https://api.{provider}.com/v1/images/generations"
            headers = {"Authorization": f"Bearer {args.key}", "Content-Type": "application/json"}
            payload = {"prompt": args.prompt, "n": 1}

        req = urllib.request.Request(url, data=json.dumps(payload).encode("utf-8"), headers=headers, method="POST")
        
        try:
            with urllib.request.urlopen(req) as response:
                res_data = json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as e:
            error_body = e.read().decode("utf-8")
            try:
                err_json = json.loads(error_body)
                if "error" in err_json and "message" in err_json["error"]:
                    raise ValueError(err_json["error"]["message"])
                else:
                    raise ValueError(error_body)
            except:
                raise ValueError(f"HTTP Error {e.code}: {e.reason} - {error_body}")

        if provider == "gemini":
            if "generatedImages" in res_data and len(res_data["generatedImages"]) > 0:
                b64_img = res_data["generatedImages"][0].get("image", {}).get("imageBytes")
                if not b64_img:
                    raise ValueError("Gemini API connected successfully, but no base64 image data was found in the payload.")
                
                with open(output_path, "wb") as out_file:
                    out_file.write(base64.b64decode(b64_img))
            else:
                raise ValueError("Invalid or empty response structure from Gemini API.")

        else:
            if "data" in res_data and len(res_data["data"]) > 0:
                image_url = res_data["data"][0].get("url")
                if not image_url:
                    raise ValueError("API returned success, but no image URL was found in the data payload.")
                
                img_req = urllib.request.Request(image_url, headers={'User-Agent': 'Mozilla/5.0'})
                with urllib.request.urlopen(img_req) as img_resp, open(output_path, 'wb') as out_file:
                    out_file.write(img_resp.read())
            else:
                raise ValueError("Invalid response structure from API.")

        final_img = Image.open(output_path).convert("RGBA").resize((width, height))
        final_img.save(output_path)
        print(f"Success: Image generated via {provider}.")

    except Exception as e:
        print(f"Generation failed: {str(e)}, falling back to open gateway for testing...")
        try:
            import urllib.parse
            safe_prompt = urllib.parse.quote(f"{args.prompt}, clean pixel art style")
            test_url = f"https://image.pollinations.ai/prompt/{safe_prompt}?width={width}&height={height}&nologo=true"
            
            req = urllib.request.Request(test_url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(req) as resp, open(output_path, 'wb') as out_file:
                out_file.write(resp.read())
            print("Success: Test image downloaded successfully.")
        except Exception as fallback_err:
            print(f"Absolute failure: {fallback_err}")
            render_error_to_image(width, height, "TEST ERROR", str(e), output_path)

if __name__ == "__main__":
    main()