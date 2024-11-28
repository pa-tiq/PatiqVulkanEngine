include .env

# specify compiler options
# -std=c++17: Use the C++17 standard.
# -O2: Optimize for speed without excessive compile time.
CFLAGS = -std=c++17 -O2

# LDFLAGS: Specifies linker options.
# -lglfw: Links the GLFW library (for windowing and OpenGL/Vulkan integration).
# -lvulkan: Links the Vulkan API library.
# -ldl, -lpthread, etc. link additional required system libraries for threading, dynamic loading, and X11 support.
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# find ./shaders -type f -name "*.vert": Finds all .vert (vertex shader) files in the shaders directory.
vertSources = $(shell find ./shaders -type f -name "*.vert")

# find ./shaders -type f -name "*.frag": Finds all .frag (fragment shader) files in the shaders directory.
fragSources = $(shell find ./shaders -type f -name "*.frag")

# $(patsubst pattern, replacement, text): finds whitespace-separated words in text that match pattern and replaces them with replacement.
# Converts source shader file names (e.g., shader.vert) into output object file names (shader.vert.spv), which are compiled SPIR-V binaries.
# Then, create list of all spv files and set as dependency
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

# name of the final executable
TARGET = first_app.out

# Dependencies:
# $(vertObjFiles) and $(fragObjFiles): Compiled shader SPIR-V files
# *.cpp *.hpp: All C++ source and header files in the current directory.
# Recipe:
# g++ $(CFLAGS) -o $(TARGET) *.cpp $(LDFLAGS): Compiles all .cpp files and links them with the specified libraries to produce the executable.
$(TARGET): $(vertObjFiles) $(fragObjFiles)
$(TARGET): *.cpp *.hpp
	g++ $(CFLAGS) -o $(TARGET) *.cpp $(LDFLAGS)

# make shader targets
# This is a pattern rule for compiling shaders:
# %: Matches prerequisites with the same base name (e.g., shader.vert → shader.vert.spv).
# %.spv: Matches targets ending in .spv.
# Recipe:
# $(GLSLC_PATH): Calls the GLSL compiler specified in the .env file.
# $< is the first prerequisite (usually a source file, like shader.vert)
# $@ is the name of the target being generated (like shader.vert.spv)
# each spv file depends on itself without the spv extension
%.spv: %
	${GLSLC_PATH} $< -o $@

# Phony Targets
# Declares test and clean as phony targets, meaning they are not actual files and should always execute
.PHONY: test clean

# Test Target
# Ensures first_app.out is built and then runs it
test: first_app.out
	./first_app.out

# Clean Target
# Removes the executable (first_app.out) and all compiled shader files (*.spv).
clean:
	rm -f first_app.out
	rm -f *.spv