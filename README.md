# Wisdom Park

Welcome to **Wisdom Park**, a gamified 2D drawing and frame-by-frame animation software built with C++ and SFML. 

Unlike traditional digital art software with rigid, boring menus, Wisdom Park brings the creative process to life through a "Theme Park Tycoon" inspired user interface. Your tools are interactive stalls on the grass, and your canvas is the sky above. 

## Features

* **Gamified Tycoon UI:** Interact with park elements (palette vignettes, ferris wheels, desk tools) to select your tools and colors.
* **Frame-by-Frame Animation:**
    * Add and navigate through infinite frames.
    * **Onion Skinning:** See a faint overlay of your previous frame to guide your next drawing.
    * **Playback:** Real-time animation playback at 12 FPS.
* **Generative AI Mascot:** A dedicated AI Helper living in the park that generates procedural pixel art directly onto your canvas. It calculates dynamic blueprints (swords, trees, potions) and renders them pixel-by-pixel with automated shading and outlining.
* **Custom Machine Learning Pipeline:** Train the AI on your own art! Draw an object, save it to the dataset, and the AI will build a probability heatmap to mutate and generate new variations of your style.
* **Web Scraping Integration:** Includes a Python ingestion script to automatically scrape thousands of sprites from the web, resize them, and compile them into training data for the C++ application.
* **Robust Drawing Tools:** Smooth stroke interpolation, dynamic brush sizing, undo/redo history, and a middle-click eyedropper.

## Controls

### Canvas & Tools
* **Left Click (Hold):** Draw on the canvas / Interact with UI palettes and the AI Mascot.
* **Middle Click:** Eyedropper tool (pick a color from the canvas).
* **Mouse Scroll Wheel:** Increase or decrease brush size.
* **1 / B:** Equip Black Brush.
* **C:** Clear the current frame entirely.
* **Z:** Undo.
* **Y:** Redo.

### Animation & File Management
* **Right Arrow:** Go to the next frame (creates a new one if it doesn't exist).
* **Left Arrow:** Go to the previous frame.
* **Spacebar:** Toggle playback of the animation at 12 FPS.
* **S:** Save the current frame as `export.png`.
* **E:** Export *all* frames as individual `.png` files.
* **R:** Reset the entire animation sequence.

### AI Training & Generation
* **Click the AI Mascot:** Triggers the AI to generate a complex piece of pixel art on your canvas based on its current dataset.
* **T:** Train the AI. Saves whatever is currently drawn inside the canvas area to `dataset.txt` and recalculates the AI's probability heatmap.
* **Backspace:** Removes the last saved template from `dataset.txt` (useful if you make a mistake while training).

## The AI Dataset Pipeline

Wisdom Park uses a custom probability-based generative model. The AI reads from a `dataset.txt` file located in the working directory. 

To mass-train the AI on external images instead of drawing them by hand, use the included Python script:
1. Ensure Python and Pillow are installed (`pip install Pillow`).
2. Run `python scraper.py` from your project directory.
3. The script will download assets, convert them into 48x48 ASCII matrices, and generate a massive `dataset.txt` file.
4. Launch the C++ app. The AI will automatically load the templates and start generating shapes based on the ingested data.

## Built With
* **Language:** C++17 / Python 3
* **Graphics API:** SFML 2.6.1 (Simple and Fast Multimedia Library)
* **IDE:** Visual Studio 2022 (x64)

## Getting Started

Ensure you have SFML 2.6.1 (64-bit) set up in your environment. 

1. Clone the repository:
   `git clone https://github.com/Ahmad1827/wisdomPark.git`
2. Move the required SFML `.dll` files into the same directory as your executable:
   * `sfml-graphics-2.dll`
   * `sfml-window-2.dll`
   * `sfml-system-2.dll`
3. Open `wisdomPark.sln` in Visual Studio 2022.
4. **Crucial:** Go to Project Properties -> C/C++ -> Language -> Set **C++ Language Standard** to **ISO C++17 Standard (/std:c++17)**.
5. Set the configuration to **Release / x64**, and run.

## 🗺️ Roadmap
- Implement text rendering (TTF) for the AI Mascot's speech bubbles to provide active feedback.
- Add advanced animation layers and timeline UI.
- Expand the generative AI to support larger grid sizes and multi-color palette learning.
- Introduce an in-engine Sprite Sheet slicer tool to fully bypass Python for local files.