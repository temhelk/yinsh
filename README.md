## [Play on itch.io](https://temhelk.itch.io/yinsh)
[![image](https://github.com/user-attachments/assets/182d1756-bd9f-42c9-b9d4-d68c8bb4b1c8)](https://temhelk.itch.io/yinsh)

## Description
Rules of Yinsh: https://www.gipf.com/yinsh/rules/rules.html

Yinsh board game written with [raylib](https://github.com/raysan5/raylib) and [ImGui](https://github.com/ocornut/imgui) that allows you to play against AI or another player locally

Uses [yngine](https://github.com/temhelk/yngine) as an engine for AI

## Compilation
The game can be compiled for Linux, Windows, and Web (WASM).

The project uses submodules to download dependencies so use recursive flag when cloning it: `git clone --recursive https://github.com/temhelk/yinsh`

You can also refer to github actions for building scripts

Linux (gcc, clang):
- Install neccessary build tools like cmake, make and the compiler
- Clone this repository `git clone --recursive https://github.com/temhelk/yinsh`
- cd into it `cd yinsh`
- Configure cmake `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release`
- Build the game `cmake --build build-release --parallel`
- The resulting binary should be available at `./build-release/yinsh-gui/Yinsh-gui`

Windows (mingw64 or clang with MSYS2, msvc is not supported yet):
- Install msys2 and packages for the appropriate environment like cmake, make (or ninja) and the compiler
- Example for CLANG64: `pacman -S git`; `pacman -S mingw-w64-clang-x86_64-cmake`; `pacman -S mingw-w64-clang-x86_64-clang`; `pacman -S mingw-w64-clang-x86_64-ninja`
- Clone this repository `git clone --recursive https://github.com/temhelk/yinsh`
- cd into it `cd yinsh`
- Configure cmake `cmake -GNinja -S . -B build-release -DCMAKE_BUILD_TYPE=Release`
- Build the game `cmake --build build-release --parallel`
- The resulting binary should be available at `./build-release/yinsh-gui/Yinsh-gui.exe`

Web, on linux (emscripten):
- For now, you need to build the native app as well, so it generates `yngine/tables.hpp` in the process
- `emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DPLATFORM=Web`
- `cmake --build build-web --parallel`
- Now you can run it locally with `emrun build-web/yinsh-gui/Yinsh-gui.html`
- Or you can host it with some web server, thought you need to configure cross-origin headers for SharedArrayBuffer to work
- Or you can zip it for itch.io with:
  - `cd build-web/yinsh-gui`
  - `cp Yinsh-gui.html index.html`
  - `zip yinsh-web.zip index.html Yinsh-gui.js Yinsh-gui.wasm`
  - You also need to enable "SharedArrayBuffer support" on itch.io game page
- Firefox seems to run it almost twice as fast compared to Chrome in terms of AI performance
