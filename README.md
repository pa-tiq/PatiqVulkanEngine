### [PatiqVulkanEngine](https://github.com/pa-tiq/PatiqVulkanEngine)

This is a beginner Vulkan Game Engine. Made following [this playlist](https://www.youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR) by Brendan Galea. 

Follow [this](https://vulkan-tutorial.com/Development_environment#page_Linux) tutorial to create your development environment. Commands:
```bash
sudo apt install vulkan-tools
sudo apt install libvulkan-dev
sudo apt install vulkan-validationlayers
sudo apt install vulkan-validationlayers-dev
sudo apt install vulkan-utility-libraries-dev spirv-tools
sudo apt install libglfw3-dev
sudo apt install libglm-dev
sudo apt install libxxf86vm-dev libxi-dev
sudo apt install glslc
whereis glslc
```

Your .env file should have GLSLC's path, for example:
```
GLSLC_PATH = /usr/bin/glslc
```

Compile the shaders to obtain the .spv files, make and run the app with:
```bash
make test
``` 