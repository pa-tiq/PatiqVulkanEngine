This is a beginner Vulkan Game Engine. Made following [this playlist](https://www.youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR) by Brendan Galea. 

Follow [this](https://vulkan-tutorial.com/Development_environment#page_Linux) tutorial to create your development environment.

Your .env file should have GLSLC's path, for example:
```
GLSLC_PATH = /usr/local/bin/glslc
```

Compile the shaders to obtain the .spv files and make and run the app with:
```bash
make test
``` 