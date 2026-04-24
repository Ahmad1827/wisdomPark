# Wisdom Park

Welcome to **Wisdom Park**, a gamified 2D drawing and frame-by-frame animation software built with C++ and SFML. 

Unlike traditional digital art software with rigid, boring menus, Wisdom Park brings the creative process to life through a "Theme Park Tycoon" inspired user interface. Your tools are interactive stalls on the grass, and your canvas is the sky above. 

## Features

* **Gamified Tycoon UI:** Interact with park elements (stalls, bulldozers, mascots) to select your tools instead of clicking standard UI buttons.
* **Frame-by-Frame Animation:**
    * Add and navigate through infinite frames.
    * **Onion Skinning:** See a faint overlay of your previous frame to guide your next drawing.
    * **Playback:** Real-time animation playback at 12 FPS.
* **Dynamic Brush System:** Smooth stroke interpolation with variable brush size controlled via the mouse wheel.
* **Interactive AI Mascot:** A dedicated "AI Helper" living in the park, designed to assist, critique, and guide the user's artistic process (currently in Alpha).
* **Export System:** Save your frames locally as `.png` files.

## Controls

### Canvas & Tools
* **Left Click (Hold):** Draw on the canvas / Interact with park buildings.
* **Mouse Scroll Wheel:** Increase or decrease brush size.
* **Keyboard Shortcuts (Colors):** `1` (Black), `2` (Red), `3` (Green), `4` (Blue).
* **E / Bulldozer:** Eraser tool (makes pixels transparent).
* **C:** Clear the current frame entirely.

### Animation & Playback
* **Right Arrow:** Go to the next frame (creates a new one if it doesn't exist).
* **Left Arrow:** Go to the previous frame.
* **Spacebar (Hold):** Play the animation at 12 FPS.

## Built With
* **Language:** C++
* **Graphics API:** SFML 2.6.1 (Simple and Fast Multimedia Library)
* **IDE:** Visual Studio 2022 (x64)

## Getting Started

Ensure you have SFML 2.6.1 (64-bit) set up in your environment. 

1. Clone the repository:
   `git clone https://github.com/Ahmad1827/wisdomPark.git`

2. Move the required SFML `.dll` files into the same directory as the `.vcxproj` file:
   * `sfml-graphics-2.dll`
   * `sfml-window-2.dll`
   * `sfml-system-2.dll`

3. Open `wisdomPark.sln` in Visual Studio 2022, set the configuration to **Release / x64**, and run.

## 🗺️ Roadmap
- Implement text rendering (TTF) for the AI Mascot's speech bubbles.
- Add advanced animation layers.
- Connect the AI Mascot to an external AI API for real-time creative suggestions.
- Expand the park with more interactive rides and tools.