#version 450

layout (location = 0) in vec3 fragColor;

// this is the output variable.
// the "layyout" qualifier takes a location value.
// a fragment shader is capable of outputting to multiple different locations.
// the "out" qualifier specifies that this variable will be used as an output.
layout (location = 0) out vec4 outColor;

void main() {
    // RGB and alpha, each value from 0 to 1
    outColor = vec4(fragColor, 1.0);
}