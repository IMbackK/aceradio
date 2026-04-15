# Aceradio

![Screenshot](res/scrot.png)

A C++ Qt graphical user interface for generating music using [acestep.cpp](https://github.com/ServeurpersoCom/acestep.cpp).
Provides endless playlist style music generation on ROCm, Vulkan or CUDA.

## Requirements

- Qt 6 Core, Gui, Widgets, and Multimedia
- CMake 3.14+
- acestep.cpp (bring your own binaries)
- ROCm, cuda or vulkan headers and shader compiler

## Installing

Grab the latest release from https://github.com/IMbackK/aceradio/releases/

## Models

grab the models from [here](https://huggingface.co/Serveurperso/ACE-Step-1.5-GGUF/tree/main)

## Building

### Linux / macOS

1. clone the repo and create a build directory
	* `git clone https://github.com/IMbackK/aceradio.git && cd aceradio`
2. create a build directory and enter it
	* `mkdir build && cd build`
3. configure the project
	* `cmake -DGGML_HIP=on ..` for ROCm support.
	* `cmake -DGGML_CUDA=on ..` for CUDA support.
	* `cmake -DGGML_VULKAN=on ..` for Vulkan support.
4. build the project
	`make -j$(nproc)`

### Windows (Qt6 + CMake)

To build on Windows run the following commands in *Command Prompt (cmd)* or *PowerShell*:

1. Configure and Build:
```cmd
git clone https://github.com/IMbackK/aceradio.git
cd aceradio
mkdir build
cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"

# Use --parallel to build using all CPU cores
# Or use -j N (e.g., -j 4) to limit the number of threads
cmake --build build --config Release --parallel
```
Note: Replace C:/Qt/6.x.x/msvc2019_64 with your actual Qt installation path.

2. Deploy Dependencies:
Run windeployqt to copy necessary Qt libraries to the build folder:
```cmd
"C:\Qt\6.x.x\msvc2019_64\bin\windeployqt.exe" --no-translations build\Release\aceradio.exe
```

## Setup Paths:

Go to settings->Ace Step->Model Paths and add the paths to the AceStep models.

## License

Aceradio and all its files, unless otherwise noted, are licensed to you under the GPL-3.0-or-later license
