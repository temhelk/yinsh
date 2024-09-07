## [Play on itch.io](https://temhelk.itch.io/yinsh)
[![image](https://github.com/user-attachments/assets/182d1756-bd9f-42c9-b9d4-d68c8bb4b1c8)](https://temhelk.itch.io/yinsh)

## Description
Rules of Yinsh: https://www.gipf.com/yinsh/rules/rules.html

Yinsh board game written with [raylib](https://github.com/raysan5/raylib) that allows you to play against AI or another player locally

Uses [yngine](https://github.com/temhelk/yngine) as an engine for AI

## Compilation
The game can be compiled for Linux, Windows, and Web (WASM).

The project uses submodules to download dependencies so use recursive flag when cloning it: `git clone --recursive https://github.com/temhelk/yinsh`

Linux (gcc, clang):
- Install neccessary build tools like cmake, make and the compiler
- Configure cmake with `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release`
- Build the game with `cmake --build build-release --parallel`
- The resulting binary should be available at `./build-release/yinsh-gui/Yinsh-gui`

Windows (mingw64 or clang with MSYS2, msvc is not supported yet):
- Install msys2 and packages for the appropriate environment like cmake, make (or ninja) and the compiler (@TODO: write this step-by-step)
- @todo: complete this
