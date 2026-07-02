# Wisdom Park

Wisdom Park is a custom 2D animation and pixel-art engine built for creating and managing multi-layered animation projects.

## Overview

Designed with a focus on systems architecture and efficient asset control, Wisdom Park provides a tailored environment for digital artists and animators. It bypasses generic UI constraints by offering a dedicated workspace for frame-by-frame drawing, custom color management, and robust file serialization.

## Key Features

* **Core Animation & Canvas Engine**
  * Frame-by-frame timeline controls with variable FPS playback.
  * Multi-layer compositing supporting visibility, locking, opacity scaling, and custom blend modes (Multiply, Additive, Screen).
  * Layer persistence for static backgrounds across animation frames.
  * Adjustable onion skinning (previous/next frame rendering).

* **Advanced Color Palette System**
  * Live HSV color picker with RGB and HEX input synchronization.
  * Automatic mathematical color harmony generation (Complementary, Analogous, Triadic, Monochromatic).
  * Persistent custom swatches and recent color history caching.
  * Native eyedropper tool mapped directly to canvas pixel data.

* **Project Management (.wpk)**
  * Custom directory-based `.wpk` (Wisdom Park) project file structure.
  * Automatically serializes layer metadata, frame properties, and thumbnails to disk.
  * Integrated Win32 API hooks for native Windows Save, Open, and Folder Selection dialogs.

* **Export Pipeline**
  * **Single Frame:** Export the current active frame as a flattened PNG.
  * **Image Sequence:** Dump all timeline frames into a designated directory as sequential PNGs.
  * **Sprite Sheets:** Generate a unified sprite sheet with customizable column wrapping.
  * **Auto-Crop:** Alpha-channel scanning to automatically crop empty transparent space around sprites during export.

## Tech Stack

* **Language:** C++
* **Graphics Framework:** SFML (Simple and Fast Multimedia Library)
* **OS Integration:** Win32 API (`windows.h`, `commdlg.h`, `shlobj.h`)
* **Standard Library:** `<filesystem>`, `<fstream>`, `<chrono>`

## Architecture 

The engine is modularized into dedicated core managers:
* `Canvas.cpp` / `Timeline.cpp`: Manages the underlying 2D grid, render textures, layer arrays, and frame transitions.
* `ProjectManager.cpp`: Handles the serialization and deserialization of the `.wpk` project states to the local disk.
* `ExportManager.cpp`: Responsible for flattening frame stacks, calculating bounding boxes for auto-cropping, and building sprite sheets.
* `ColorManager.cpp`: Handles mathematical conversions between HSV/RGB color spaces to drive UI components and harmony logic.
* `UIManager.cpp`: The central hub bridging SFML window events to the various tool panels (LayerPanel, ColorPalettePanel, RightProperties, BottomTimeline).

## Author

Ahmad Arnaoute