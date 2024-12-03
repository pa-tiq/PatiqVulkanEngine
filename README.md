### [PatiqVulkanEngine](https://github.com/pa-tiq/PatiqVulkanEngine)

This is a beginner Vulkan Game Engine. Made following [this playlist](https://www.youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR) by Brendan Galea.

Follow [this](https://vulkan-tutorial.com/Development_environment#page_Linux) tutorial to create your development environment. Commands:

```bash
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers vulkan-utility-libraries-dev spirv-tools libglfw3-dev libglm-dev libxxf86vm-dev libxi-dev glslc
whereis glslc
```

This project uses [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader/blob/release/tiny_obj_loader.h) for loading .obj files. The header file is already inside the folder with the same name.

Your .env file should have the path for GLSLC and tinyobjloader:

```
GLSLC_PATH = /usr/bin/glslc
TINYOBJLOADER_PATH = tinyobjloader
```

Compile the shaders to obtain the .spv files, make and run the app with:

```bash
make test
```
