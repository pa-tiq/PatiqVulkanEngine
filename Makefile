include .env

# specify compiler options
# -std=c++17: Use the C++17 standard.
# -O2: Optimize for speed without excessive compile time.
CFLAGS = -std=c++17 -O2 -I${TINYOBJLOADER_PATH} -Iinclude

# LDFLAGS: Specifies linker options.
# -lglfw: Links the GLFW library (for windowing and OpenGL/Vulkan integration).
# -lvulkan: Links the Vulkan API library.
# -ldl, -lpthread, etc. link additional required system libraries for threading, dynamic loading, and X11 support.
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# Find all source files
SRCS := $(shell find src -name '*.cpp')
# Convert source paths to object file paths under build/
OBJS := $(SRCS:src/%.cpp=build/%.o)

# Find shader files
VERT_SHADERS := $(shell find shaders -type f -name "*.vert")
FRAG_SHADERS := $(shell find shaders -type f -name "*.frag")
SHADER_BINS := $(patsubst shaders/%.vert,shaders/compiled/%.vert.spv,$(VERT_SHADERS)) \
               $(patsubst shaders/%.frag,shaders/compiled/%.frag.spv,$(FRAG_SHADERS))

TARGET = build/first_app.out

# Create build directory
$(shell mkdir -p build)
$(shell mkdir -p shaders/compiled)

# Create subdirectories for object files
$(shell mkdir -p $(sort $(dir $(OBJS))))

# Main target
$(TARGET): $(SHADER_BINS) $(OBJS)
	g++ $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files
build/%.o: src/%.cpp
	g++ $(CFLAGS) -c $< -o $@

# Compile shaders
shaders/compiled/%.spv: shaders/%
	${GLSLC_PATH} $< -o $@


.PHONY: clean test

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf shaders/compiled/
	rm -rf build/