# Wisdom Park Studio

A magical, AI-assisted pixel art and animation studio built with modern C++17 and SFML. 

**Wisdom Park Studio** combines a robust drawing and animation environment (inspired by FlipaClip and Aseprite) with an abstraction layer for modern AI integration. It features a completely local, BYOK (Bring Your Own Key) architecture, allowing users to leverage OpenAI, Gemini, Anthropic, or OpenRouter models to assist their creative workflow—all framed within a cozy, fantasy "Wisdom Park" workbench theme.

---

## 🎨 Overview

This application goes beyond standard pixel editors by introducing intelligent workflow assistance without taking away the user's creative control. 

Whether you are drawing frame-by-frame sprites, creating procedural tilemaps with Wave Function Collapse (WFC), or asking an AI to generate structural base templates, Wisdom Park Studio handles it locally on your machine. The visual identity of the application avoids the sterile, flat-gray look of standard developer tools, placing your canvas directly on a magical workbench within Wisdom Park.

---

## ✨ Features

### Drawing & Pixel Art
* **Essential Toolset:** Pencil, Eraser, Bucket Fill, and Selection tools.
* **Brush Controls:** Dynamic brush sizing (mouse wheel support) and palette management.
* **Pixel Perfect Mode:** Hardware acceleration with sharp, non-antialiased rendering.

### Animation System
* **Timeline Management:** Full frame control (add, delete, duplicate, navigate).
* **Onion Skinning:** Configurable overlay of previous/next frames with adjustable opacity to assist in smooth tweening.
* **Playback Control:** Real-time playback loop with adjustable Frames Per Second (FPS).
* **Layer Support:** Dedicated layer management per frame, supporting visibility toggles and alpha blending.

### AI Integration (BYOK)
* **Bring Your Own Key:** The application uses a secure, local configuration system. You provide your own API keys. No hidden subscriptions.
* **Multi-Provider Support:** Seamlessly switch between **OpenAI**, **Google Gemini**, **Anthropic Claude**, and **OpenRouter**.
* **AI Assistants:** Generate base outlines, clean up sketches, suggest shading logic, and create color palettes. 

### Procedural Generation
* **WFC Generation:** Build procedural architecture and buildings using Wave Function Collapse.
* **Environment Builders:** Generate dynamic terrain patches, trees, and clutter based on thematic keywords.

---

## 📸 Screenshots

*(Replace these placeholders with actual screenshots of your application)*

| Welcome Screen | Main Workspace | AI Generation |
| :---: | :---: | :---: |
| `[Screenshot of Welcome Screen]` | `[Screenshot of Workbench Canvas]` | `[Screenshot of AI Modal]` |

---

## 🏗️ Project Structure

The codebase is strictly modular, separating the rendering pipeline from UI state management and AI bridging.

    WisdomPark/
    ├── src/
    │   ├── main.cpp                  # Entry point
    │   ├── Application.cpp / .h      # Main state machine and event loop
    │   │
    │   ├── core/                     # Foundational Systems
    │   │   ├── SettingsManager.h     # Handles local BYOK config and encryption
    │   │   ├── Canvas.cpp / .h       # Manages Layers, Onion Skinning, RenderTextures
    │   │   └── Timeline.cpp / .h     # Manages Frames, Tweening, and playback
    │   │
    │   ├── ui/                       # Modern Interface Components
    │   │   ├── UIManager.cpp / .h    # Orchestrates panels and passes clicks
    │   │   ├── LeftToolbar.cpp / .h  # Drawing and AI tool selection
    │   │   ├── TopMenuBar.cpp / .h   # Application-level controls
    │   │   ├── RightProperties.cpp / .h # Context-sensitive settings
    │   │   ├── BottomTimeline.cpp / .h  # Animation playback controls
    │   │   └── Screens/              
    │   │       ├── WelcomeScreen.cpp / .h   
    │   │       └── AISettingsModal.cpp / .h 
    │   │
    │   └── ai/                       # AI Abstraction Layer
    │       ├── AIProvider.cpp / .h   # Abstract Base Classes for LLM routing
    │       ├── LocalPythonBridge.cpp / .h # System execution layer
    │       └── AIHelper.cpp / .h     # Parses generated grids to the SFML Canvas
    │
    ├── assets/                       # UI Icons, Sprites, Fonts
    ├── scripts/                      
    │   └── brain.py                  # Multi-provider BYOK Python bridge
    └── dataset.json                  # Local offline sprite templates


---

## 🚀 Building the Project

### Prerequisites
* **C++ Compiler:** Requires C++17 support (MSVC, GCC, or Clang).
* **SFML 2.6+:** Simple and Fast Multimedia Library.
* **Python 3.8+:** Required for the `brain.py` local API bridge.
* **Python Packages:** `requests` and `pillow`.

### Python Setup
Before running the application, ensure the required Python packages are installed:
    
    pip install requests pillow

### Windows Build (Visual Studio)
1. Download and extract **SFML 2.6.x** for your specific compiler version.
2. Open `WisdomPark.sln` in Visual Studio.
3. Right-click the project -> **Properties**.
4. Set **C/C++ -> General -> Additional Include Directories** to `[Path_To_SFML]\include`.
5. Set **Linker -> General -> Additional Library Directories** to `[Path_To_SFML]\lib`.
6. Set **Linker -> Input -> Additional Dependencies** to `sfml-graphics.lib; sfml-window.lib; sfml-system.lib;`.
7. Ensure the `.dll` files from SFML are copied to your build output directory.
8. Build and Run.

### Linux Build (CMake / Make)
*(Assuming SFML is installed via package manager: `sudo apt-get install libsfml-dev`)*

    mkdir build
    cd build
    cmake ..
    make
    ./WisdomPark

---

## ⌨️ Controls & Hotkeys

While the UI is fully mouse-driven, standard animation hotkeys are supported for workflow efficiency:

| Action | Shortcut |
| :--- | :--- |
| **Next Frame** | `Right Arrow` |
| **Previous Frame** | `Left Arrow` |
| **First Frame** | `Home` |
| **Last Frame** | `End` |
| **Play / Pause** | `Spacebar` |
| **New Frame** | `Ctrl` + `N` |
| **Duplicate Frame** | `Ctrl` + `D` |
| **Delete Frame** | `Delete` |
| **Undo** | `Z` |
| **Redo** | `Y` |
| **Brush Color: Black**| `B` |
| **Save State** | `C` |
| **Exit** | `Escape` |

*(Note: Mouse Wheel scrolls to adjust brush size dynamically).*

---

## 🧠 Configuring AI (BYOK)

Wisdom Park Studio uses a **Bring Your Own Key** model. Your API keys are never transmitted to our servers; they remain completely local to your machine.

1. Launch the application.
2. From the Welcome Screen, click **Configure AI Providers** (or access it via the Top Menu Bar during painting).
3. Type the name of your preferred provider (`openai`, `gemini`, `anthropic`, or `openrouter`).
4. Paste your secure API key.
5. Click **Save**. The AI functionality on the Left Toolbar will immediately unlock.

*Your configuration is saved locally to `config.txt`.*

---

## 💾 Saving & Exporting Projects

* **Export:** Press `S` to export the current active frame as a `.png`.
* **Mass Export:** Press `E` to dump the entire frame timeline as sequenced `.png` files (`frame_0.png`, `frame_1.png`, etc.) for external compilation.
* *(Native project `.wpark` save states are currently in development).*

---

## 🗺️ Roadmap

* [ ] Implement Native `.wpark` binary project saving (preserving layer states and timeline data).
* [ ] Hardware-accelerated brush stroke interpolation.
* [ ] Expanded AI tooling (Palette swapping via Local Color Analysis).
* [ ] MP4 / GIF export integration via FFmpeg.
* [ ] Drag-and-drop reference image support.

---

## 🤝 Contributing

Contributions are welcome! If you would like to expand the AI toolset, add a new UI panel, or optimize the rendering pipeline, please fork the repository and submit a pull request. 

Ensure all new C++ code adheres to the existing architectural separation (Core, UI, AI).

---

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

---

## 🙏 Credits

* Built with the incredible [SFML (Simple and Fast Multimedia Library)](https://www.sfml-dev.org/).
* AI generation powered dynamically by OpenAI, Anthropic, Google, and Groq.
* Wisdom Park assets, themes, and design philosophy by Ahmad Arnaoute.