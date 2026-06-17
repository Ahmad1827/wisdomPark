import sys
import requests

if len(sys.argv) < 4:
    sys.exit(1)

provider = sys.argv[1]
api_key = sys.argv[2]
prompt = sys.argv[3]

def request_groq(prompt, key):
    url = "https://api.groq.com/openai/v1/chat/completions"
    headers = {"Authorization": f"Bearer {key}", "Content-Type": "application/json"}
    payload = {
        "model": "llama-3.3-70b-versatile",
        "messages": [{"role": "user", "content": f"You are a 2D pixel art asset generator. Blueprint for: {prompt}. Output only raw text rows using M, X, W, and . characters. Ensure rect grid."}]
    }
    response = requests.post(url, json=payload, headers=headers)
    return response.json()['choices'][0]['message']['content']

def request_openai(prompt, key):
    url = "https://api.openai.com/v1/chat/completions"
    headers = {"Authorization": f"Bearer {key}", "Content-Type": "application/json"}
    payload = {
        "model": "gpt-4o",
        "messages": [{"role": "user", "content": f"You are a 2D pixel art asset generator. Blueprint for: {prompt}. Output only raw text rows using M, X, W, and . characters. Ensure rect grid."}]
    }
    response = requests.post(url, json=payload, headers=headers)
    return response.json()['choices'][0]['message']['content']

def request_anthropic(prompt, key):
    url = "https://api.anthropic.com/v1/messages"
    headers = {"x-api-key": key, "anthropic-version": "2023-06-01", "Content-Type": "application/json"}
    payload = {
        "model": "claude-3-opus-20240229",
        "max_tokens": 1000,
        "messages": [{"role": "user", "content": f"You are a 2D pixel art asset generator. Blueprint for: {prompt}. Output only raw text rows using M, X, W, and . characters. Ensure rect grid."}]
    }
    response = requests.post(url, json=payload, headers=headers)
    return response.json()['content'][0]['text']

try:
    raw_text = ""
    if provider.lower() == "groq": raw_text = request_groq(prompt, api_key)
    elif provider.lower() == "openai": raw_text = request_openai(prompt, api_key)
    elif provider.lower() == "anthropic": raw_text = request_anthropic(prompt, api_key)
    else: sys.exit(1)

    clean_blueprint = []
    for line in raw_text.split('\n'):
        line = line.replace("`", "").strip()
        if line and not line.startswith(("here", "blueprint", "size", "width", "height")):
            clean_blueprint.append(line)
            
    if clean_blueprint:
        max_width = max(len(line) for line in clean_blueprint)
        perfect_grid = [line.ljust(max_width, '.') for line in clean_blueprint]
        with open("temp_blueprint.txt", "w", encoding="utf-8") as f:
            f.write('\n'.join(perfect_grid))
except Exception:
    sys.exit(1)