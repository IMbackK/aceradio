# Aceradio

![Screenshot](res/scrot.png)

A C++ Qt graphical user interface for generating music using [acestep.cpp](https://github.com/ServeurpersoCom/acestep.cpp).

## Requirements

- Qt 6 Core, Gui, Widgets, and Multimedia
- CMake 3.14+
- acestep.cpp (bring your own binaries)

## Installing

1. Grab the latest release from https://github.com/IMbackK/aceradio/releases/
2. For now you will have to provide acestep.cpp binaries, these can be optained by following the instructions at https://github.com/ServeurpersoCom/acestep.cpp or the **Build acestep.cpp** section below

## Build acestep.cpp:

```bash
git clone https://github.com/ServeurpersoCom/acestep.cpp.git
cd acestep.cpp
mkdir build && cd build
cmake .. -DGGML_VULKAN=ON  # or other backend
make -j$(nproc)
./models.sh  # Download models (requires ~7.7 GB free space)
```

## Building

### Linux / macOS
```bash
git clone https://github.com/IMbackK/aceradio.git
cd aceradio
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows (Qt6 + CMake)

To build on Windows, ensure you have **CMake** and **Qt6** installed. Run the following commands in **Command Prompt (cmd)** or **PowerShell**:

1. **Configure and Build:**
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

2. **Deploy Dependencies:**
Run windeployqt to copy necessary Qt libraries to the build folder:
```cmd
"C:\Qt\6.x.x\msvc2019_64\bin\windeployqt.exe" --no-translations build\Release\aceradio.exe
```

## Setup Paths:

Go to settings->Ace Step->Model Paths and add the paths to the acestep.cpp binaries the models.
