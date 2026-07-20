# Wisdom Park Studio

**Wisdom Park** is a high-performance, custom-built 2D animation and pixel art studio written in C++ and powered by SFML. Designed for professional workflows, it bridges the gap between traditional frame-by-frame animation, retro pixel-perfect art, and modern AI-assisted generation.

---

## Core Features

### Canvas & Drawing Engine
* **Dynamic Canvas Resolutions:** Create projects ranging from 16x16 pixel art sprites to 4K HD illustrations, with accurate memory allocation to ensure optimal performance.
* **Dual Rendering Modes:**
  * *Normal Mode:* Smooth, anti-aliased brush strokes.
  * *Pixel Art Mode:* Hard-edged rendering with zero blurring, 1:1 grid snapping, and visual tile-wrapping support for seamless textures.
* **Pixel Perfect Algorithm:** Real-time stroke cleanup algorithm that removes L-shaped corner pixels for flawless retro line art.
* **Advanced Selection (Lasso):** Draw custom selection bounds, drag, flip horizontally/vertically, duplicate, crop, and perform free-transform (scaling) on selected pixel data.
* **Flood Fill:** Queue-based contiguous or global color replacement with adjustable tolerance.

### Animation & Timeline
* **Frame-by-Frame Timeline:** Add, duplicate, delete, and rearrange frames with real-time playback and adjustable FPS.
* **Advanced Onion Skinning:** View previous and next frames simultaneously. Fully configurable frame counts and opacity fading.
* **Audio Sync:** Built-in audio panel to scan and playback audio files perfectly synced to your animation timeline.

### Layer Management
* **Infinite Layers:** Add, duplicate, delete, and merge (Down / Visible) layers.
* **Layer Properties:** Visibility toggles, edit-locking, opacity control, and Blend Modes (Normal, Multiply, Additive, Screen, Overlay).
* **Persistent Layers:** Lock a layer (e.g., a static background) so it remains persistent across all animation frames without needing to copy memory.
* **Image Importing:** Dynamically import PNG/JPG reference images directly into the active layer with automatic bounding-box scaling.

### AI & Generative Tools
* **Async AI Generation:** Highlight a selection, enter a prompt via the integrated terminal or AI Panel, and asynchronously generate new art or modify existing pixels.
* **Context-Aware Themes:** Generate assets based on specific architectural parameters (Structure, Clutter, Custom, Wave Function Collapse).
* **Review Modal:** Preview AI-generated results in a pop-up modal before committing or rejecting the changes to your canvas.

### ✋ Hardware Input Integration
* **Hand Tracking Camera Controls:** Built-in UDP socket listener that receives real-time coordinate data from an external Python/OpenCV hand-tracking script.
* **Gesture Actions:** Control the mouse, left/right click via finger pinches, and zoom the canvas using a two-finger vertical/horizontal pinch-and-drag.

### Project Management (.wpk)
* **Custom File Format:** Saves projects as `.wpk` directories containing JSON metadata, serialized timeline/layer data, and individual layer PNGs.
* **Project Browser:** Visual start menu with thumbnails, resolution stats, last-modified dates, and safe deletion.
* **Exporting:** Flatten frames and export them as standard PNG files or full sprite sheets.

---

## Tech Stack & Dependencies

* **Language:** C++17
* **Graphics & Windowing:** SFML 2.6.x (Simple and Fast Multimedia Library)
* **Data Parsing:** `nlohmann::json` (for project metadata and API handling)
* **Networking:** SFML Network (UDP Sockets for hand tracker communication)

---

## Building and Compiling (Linux/WSL2)

Ensure you have a C++17 compatible compiler, CMake, and SFML installed on your system.

```bash
sudo apt update
sudo apt install build-essential cmake libsfml-dev

cd wisdom_park

mkdir build
cd build
cmake ..
make

./WisdomPark
```

*(Note for Windows users: You can build using Visual Studio/MSVC or MinGW, provided SFML 2.6.x is correctly linked in your CMake configuration).*

---

## Default Keybinds

Wisdom Park features a fully customizable Keybind Manager. You can rebind these at any time via the Settings or by pressing `ESC` in the workspace.

* **B** - Brush Tool
* **P** - Pencil Tool
* **E** - Eraser Tool
* **F** - Fill Tool
* **M** - Select (Lasso) Tool
* **Space** - Play/Pause Animation
* **Left/Right Arrows** - Previous/Next Frame
* **Ctrl + Z** - Undo
* **Ctrl + Y** - Redo
* **Middle/Right Mouse Button** - Pan Canvas
* **Scroll Wheel** - Zoom Canvas

---

## Credits
**Lead Developer & Architect:** Ahmad Arnaoute (AtodDev)  
**Education:** Universitatea Politehnica Bucuresti, Facultatea de Automatica si Calculatoare 
**License:** Commercial