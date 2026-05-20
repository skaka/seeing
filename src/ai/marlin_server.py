import os
import sys
import torch
import uvicorn
import base64
import requests
import av
from io import BytesIO
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from transformers import Qwen2VLForConditionalGeneration, AutoProcessor
from qwen_vl_utils import process_vision_info

app = FastAPI(title="Marlin Gateway VLM Server")

# Global variables for local model (only loaded if local VLM is selected)
model = None
processor = None
model_loaded = False
model_error = None

class CaptionRequest(BaseModel):
    video_path: str

class FindRequest(BaseModel):
    video_path: str
    query: str

def get_config_path():
    home = os.path.expanduser("~")
    paths = [
        # Inside docker container (if run as root)
        "/root/.config/SeeingTeam/Seeing.conf",
        # Native Linux
        os.path.join(home, ".config/SeeingTeam/Seeing.conf"),
        # Native macOS
        os.path.join(home, "Library/Preferences/com.SeeingTeam.Seeing.plist"),
        os.path.join(home, "Library/Preferences/SeeingTeam/Seeing.conf"),
        # Native Windows
        os.path.join(os.environ.get("APPDATA", ""), "SeeingTeam/Seeing.ini"),
        os.path.join(os.environ.get("APPDATA", ""), "SeeingTeam/Seeing.conf"),
    ]
    for p in paths:
        if p and os.path.exists(p):
            return p
    return "/root/.config/SeeingTeam/Seeing.conf"

def get_setting_value(key_name, default=""):
    config_path = get_config_path()
    if not os.path.exists(config_path):
        return default
    try:
        with open(config_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if "=" in line:
                    k, v = line.split("=", 1)
                    k = k.strip().replace("\\", "/") # normalize key path
                    if k == key_name or k == f"ai/{key_name}" or k == f"ai\\{key_name}":
                        return v.strip()
    except Exception as e:
        print(f"Error reading config key {key_name}: {e}")
    return default

def check_local_model_exists():
    model_path = get_setting_value("vlm_local_path", "/app/model_weights")
    if not model_path or not os.path.exists(model_path):
        return False
    try:
        files = os.listdir(model_path)
        has_config = "config.json" in files
        has_weights = any(f.endswith(".safetensors") or f.endswith(".bin") or f.startswith("model.safetensors") for f in files)
        return has_config and has_weights
    except Exception:
        return False

@app.on_event("startup")
def load_local_model_if_needed():
    global model, processor, model_loaded, model_error
    vlm_type = get_setting_value("vlm_engine_type", "marlin")
    if vlm_type != "marlin":
        print(f"Marlin VLM Server: VLM Engine is set to '{vlm_type}'. Local Qwen2-VL will be bypassed.")
        return
        
    if not check_local_model_exists():
        model_path = get_setting_value("vlm_local_path", "/app/model_weights")
        model_error = f"Local model weights not found at {model_path}. Please switch VLM engine or select a valid local folder."
        print(f"Marlin VLM Server: {model_error}")
        return

    try:
        model_path = get_setting_value("vlm_local_path", "/app/model_weights")
        print(f"Marlin VLM Server: Loading local model from {model_path}...")
        device = "cuda" if torch.cuda.is_available() else "cpu"
        dtype = torch.bfloat16 if torch.cuda.is_available() else torch.float32
        
        model = Qwen2VLForConditionalGeneration.from_pretrained(
            model_path,
            torch_dtype=dtype,
            device_map=device
        )
        processor = AutoProcessor.from_pretrained(model_path)
        model_loaded = True
        model_error = None
        print(f"Model loaded successfully on {device} from {model_path}!")
    except Exception as e:
        model_error = str(e)
        print(f"Error loading model: {e}", file=sys.stderr)

def ensure_local_model_loaded():
    global model, processor, model_loaded, model_error
    if model_loaded:
        return True
    if not check_local_model_exists():
        model_path = get_setting_value("vlm_local_path", "/app/model_weights")
        model_error = f"Local model weights not found at {model_path}. Please configure OpenAI/Gemini or select a valid folder."
        return False
    try:
        model_path = get_setting_value("vlm_local_path", "/app/model_weights")
        print(f"Marlin VLM Server: Loading local model on demand from {model_path}...")
        device = "cuda" if torch.cuda.is_available() else "cpu"
        dtype = torch.bfloat16 if torch.cuda.is_available() else torch.float32
        model = Qwen2VLForConditionalGeneration.from_pretrained(model_path, torch_dtype=dtype, device_map=device)
        processor = AutoProcessor.from_pretrained(model_path)
        model_loaded = True
        model_error = None
        return True
    except Exception as e:
        model_error = str(e)
        return False

def encode_image_base64(image_path):
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def extract_video_frames_base64(video_path, max_frames=5):
    container = av.open(video_path)
    stream = container.streams.video[0]
    total_frames = stream.frames
    if total_frames <= 0:
        total_frames = int(stream.duration * stream.time_base * 30)
        if total_frames <= 0:
            total_frames = 300 # fallback
            
    indices = [int(i * total_frames / max_frames) for i in range(max_frames)]
    frames = []
    frame_count = 0
    
    for frame in container.decode(video=0):
        if frame_count in indices:
            img = frame.to_image()
            buffered = BytesIO()
            img.save(buffered, format="JPEG")
            img_str = base64.b64encode(buffered.getvalue()).decode("utf-8")
            frames.append(img_str)
        frame_count += 1
        if len(frames) >= max_frames:
            break
            
    return frames

def query_openai_vlm(file_path, is_image, prompt_text):
    api_key = get_setting_value("openai_key")
    model_name = get_setting_value("openai_model", "gpt-4o")
    if not api_key:
        raise Exception("OpenAI API key is missing. Please set it in Settings.")
        
    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {api_key}"
    }
    
    content = [{"type": "text", "text": prompt_text}]
    if is_image:
        base64_image = encode_image_base64(file_path)
        content.append({
            "type": "image_url",
            "image_url": {"url": f"data:image/jpeg;base64,{base64_image}"}
        })
    else:
        frames = extract_video_frames_base64(file_path)
        for frame in frames:
            content.append({
                "type": "image_url",
                "image_url": {"url": f"data:image/jpeg;base64,{frame}"}
            })
            
    payload = {
        "model": model_name,
        "messages": [{"role": "user", "content": content}],
        "max_tokens": 512,
        "temperature": 0.2
    }
    
    res = requests.post("https://api.openai.com/v1/chat/completions", headers=headers, json=payload)
    if res.status_code == 200:
        return res.json()["choices"][0]["message"]["content"]
    else:
        raise Exception(f"OpenAI API error ({res.status_code}): {res.text}")

def query_gemini_vlm(file_path, is_image, prompt_text):
    api_key = get_setting_value("gemini_key")
    model_name = get_setting_value("gemini_model", "gemini-2.5-flash")
    if not api_key:
        raise Exception("Gemini API key is missing. Please set it in Settings.")
        
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{model_name}:generateContent?key={api_key}"
    headers = {"Content-Type": "application/json"}
    
    parts = [{"text": prompt_text}]
    if is_image:
        base64_image = encode_image_base64(file_path)
        parts.append({
            "inlineData": {
                "mimeType": "image/jpeg",
                "data": base64_image
            }
        })
    else:
        frames = extract_video_frames_base64(file_path)
        for frame in frames:
            parts.append({
                "inlineData": {
                    "mimeType": "image/jpeg",
                    "data": frame
                }
            })
            
    payload = {
        "contents": [{"parts": parts}],
        "generationConfig": {"temperature": 0.2}
    }
    
    res = requests.post(url, headers=headers, json=payload)
    if res.status_code == 200:
        return res.json()["candidates"][0]["content"]["parts"][0]["text"]
    else:
        raise Exception(f"Gemini API error ({res.status_code}): {res.text}")

@app.get("/health")
def health():
    vlm_type = get_setting_value("vlm_engine_type", "marlin")
    local_found = check_local_model_exists()
    return {
        "status": "ready",
        "mode": vlm_type,
        "local_model_found": local_found,
        "local_model_path": "/app/model_weights"
    }

@app.post("/caption")
def caption(req: CaptionRequest):
    if not os.path.exists(req.video_path):
        raise HTTPException(status_code=404, detail=f"File not found: {req.video_path}")
        
    vlm_type = get_setting_value("vlm_engine_type", "marlin")
    is_image = req.video_path.lower().endswith(('.png', '.jpg', '.jpeg', '.gif', '.webp', '.bmp'))
    
    try:
        if vlm_type == "dummy":
            name = os.path.basename(req.video_path)
            mock_desc = f"🎬 [Mock VLM Analysis] Simulated description of asset '{name}'. Features color charts and visual test pattern geometries."
            return {"status": "success", "result": mock_desc}
            
        elif vlm_type == "openai":
            prompt = "Describe this image in detail." if is_image else "Describe this video sequence in detail with chronological event updates."
            desc = query_openai_vlm(req.video_path, is_image, prompt)
            return {"status": "success", "result": desc}
            
        elif vlm_type == "gemini":
            prompt = "Describe this image in detail." if is_image else "Describe this video sequence in detail with chronological event updates."
            desc = query_gemini_vlm(req.video_path, is_image, prompt)
            return {"status": "success", "result": desc}
            
        else: # "marlin" (Local Qwen2-VL)
            if not ensure_local_model_loaded():
                raise HTTPException(status_code=503, detail=f"Local model could not be loaded: {model_error}")
                
            print(f"Processing local caption for: {req.video_path}")
            device = "cuda" if torch.cuda.is_available() else "cpu"

            if is_image:
                content = [
                    {"type": "image", "image": req.video_path},
                    {"type": "text", "text": "Describe this image in detail."}
                ]
            else:
                content = [
                    {"type": "video", "video": req.video_path, "fps": 1.0},
                    {"type": "text", "text": "Describe this video in detail, listing different events and their timestamps in seconds."}
                ]

            messages = [{"role": "user", "content": content}]
            text = processor.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
            image_inputs, video_inputs = process_vision_info(messages)
            
            inputs = processor(
                text=[text],
                images=image_inputs,
                videos=video_inputs,
                padding=True,
                return_tensors="pt"
            )
            inputs = inputs.to(device)
            
            with torch.no_grad():
                generated_ids = model.generate(**inputs, max_new_tokens=256)
                
            generated_ids_trimmed = [
                out_ids[len(in_ids):] for in_ids, out_ids in zip(inputs.input_ids, generated_ids)
            ]
            output_text = processor.batch_decode(
                generated_ids_trimmed, skip_special_tokens=True, clean_up_tokenization_spaces=False
            )[0]

            return {"status": "success", "result": output_text}
            
    except Exception as e:
        print(f"Caption failed: {e}", file=sys.stderr)
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/find")
def find(req: FindRequest):
    if not os.path.exists(req.video_path):
        raise HTTPException(status_code=404, detail=f"Video file not found: {req.video_path}")
        
    vlm_type = get_setting_value("vlm_engine_type", "marlin")
    prompt = f"Find the start and end timestamps in seconds for the event: '{req.query}'. Respond strictly in this format: <start_seconds - end_seconds>. Example: <12.5 - 18.2>."
    
    try:
        if vlm_type == "dummy":
            return {"status": "success", "result": "<5.0 - 15.0>"}
            
        elif vlm_type == "openai":
            res_text = query_openai_vlm(req.video_path, False, prompt)
            return {"status": "success", "result": res_text}
            
        elif vlm_type == "gemini":
            res_text = query_gemini_vlm(req.video_path, False, prompt)
            return {"status": "success", "result": res_text}
            
        else: # "marlin" (Local Qwen2-VL)
            if not ensure_local_model_loaded():
                raise HTTPException(status_code=503, detail=f"Local model could not be loaded: {model_error}")
                
            print(f"Finding query '{req.query}' in video locally: {req.video_path}")
            device = "cuda" if torch.cuda.is_available() else "cpu"
            
            content = [
                {"type": "video", "video": req.video_path, "fps": 1.0},
                {"type": "text", "text": prompt}
            ]

            messages = [{"role": "user", "content": content}]
            text = processor.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
            image_inputs, video_inputs = process_vision_info(messages)
            
            inputs = processor(
                text=[text],
                images=image_inputs,
                videos=video_inputs,
                padding=True,
                return_tensors="pt"
            )
            inputs = inputs.to(device)
            
            with torch.no_grad():
                generated_ids = model.generate(**inputs, max_new_tokens=64)
                
            generated_ids_trimmed = [
                out_ids[len(in_ids):] for in_ids, out_ids in zip(inputs.input_ids, generated_ids)
            ]
            output_text = processor.batch_decode(
                generated_ids_trimmed, skip_special_tokens=True, clean_up_tokenization_spaces=False
            )[0]

            return {"status": "success", "result": output_text}
            
    except Exception as e:
        print(f"Find failed: {e}", file=sys.stderr)
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=8012)
