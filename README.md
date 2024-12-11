### [PatiqVulkanEngine](https://github.com/pa-tiq/PatiqVulkanEngine)

This is a beginner Vulkan Game Engine. Made following [this playlist](https://www.youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR) by Brendan Galea.

Follow [this tutorial](https://vulkan-tutorial.com/Development_environment#page_Linux) to create your development environment. Commands:

```bash
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers vulkan-utility-libraries-dev spirv-tools libglfw3-dev libglm-dev libxxf86vm-dev libxi-dev glslc

whereis glslc
```

### Directory Structure
```
.
├── external/          # External dependencies
│   └── tinyobjloader/ # OBJ file loader library
├── include/           # Header files
├── models/            # 3D model files (.obj) 
├── shaders/           # GLSL shader source files and 
│   └── compiled/      # Compiled SPIR-V shader files (.spv)
└── src/               # Source code files
    ├── pve/           # Core engine components and utilities
    ├── controllers/   # Input and game control systems
    └── systems/       # Rendering and other engine systems
```

This project uses [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader/blob/release/tiny_obj_loader.h) for loading `.obj` files. The header file is already inside the `tinyobjloader/` folder.

You'll also need to create a `models/` folder and put `.obj` files there. [Here](https://drive.google.com/drive/folders/1Rr7UiVsbbmYocNqhYAruHGQ25Da_Jd4Z?usp=drive_link) are my files.

Your `.env` file should have the path for GLSLC and tinyobjloader:

```
GLSLC_PATH = /usr/bin/glslc
TINYOBJLOADER_PATH = external/tinyobjloader
```

To compile the shaders to obtain the .spv files and make and run the app use the following command:

```bash
make test
```
