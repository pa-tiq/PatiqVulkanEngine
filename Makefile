include .env

CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# create list of all spv files and set as dependency
# patsubst pattern, replacement, text
# finds whitespace-separated words in text that match pattern and replaces them with replacement.
vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

TARGET = first_app.out
$(TARGET): $(vertObjFiles) $(fragObjFiles)
$(TARGET): *.cpp *.hpp
	g++ $(CFLAGS) -o $(TARGET) *.cpp $(LDFLAGS)

# make shader targets
# $@ is the name of the target being generated
# $< is the first prerequisite (usually a source file)
# each spv file depends on itself without the spv extension
%.spv: %
	${GLSLC_PATH} $< -o $@

.PHONY: test clean

test: first_app.out
	./first_app.out

clean:
	rm -f first_app.out
	rm -f *.spv