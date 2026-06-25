# Fluid Simulation

Real-time fluid simulation on the GPU using WebGPU. Just something pretty to look at.

![screenshot](assets/screenshot.png)

## Features

- Real-time Navier-Stokes fluid simulation
- Interactive brush: inject dye, paint velocity, draw obstacles
- Random splat generator
- Import images as dye, velocity field, or obstacle map
- Save/load simulation state
- Screenshot to clipboard or file
- Multiple render modes: dye, velocity, pressure, divergence, curl
- Python scripting engine with per-script isolated namespaces and tick callbacks

## Building

Requires CMake 3.28+, a C++23 compiler, and ZLIB.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./FluidSimulation
```

By default uses the Dawn WebGPU backend. To use wgpu-native instead:

```bash
cmake -B build -DWEBGPU_BACKEND=WGPU
```

All other dependencies (GLFW, ImGui, sigslot, nfd, stb, etc.) are fetched automatically via CMake FetchContent.

Python scripting is enabled automatically if Python 3.10+ development headers are found. To disable, build without Python headers installed.

## Controls

| Input | Action |
|---|---|
| Left drag | Apply brush |
| Right drag | Erase obstacles |
| Space | Pause / resume |
| D / V / P / G / C | Switch render mode |
| F12 | Screenshot to clipboard |
| Alt | Toggle menu bar |
| → (paused) | Step one frame |

## License

[GPL-3.0](LICENSE)