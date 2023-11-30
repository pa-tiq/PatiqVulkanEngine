#version 450

// This file is the vertex shader.

// As input, the vertex shader will get each vertex as input from the Assembler stage, 
// and then needs to output a position.

vec2 positions[3] = vec2[] (
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
// the gl_Position is a 4-dimensional vector that maps to the output frame buffer image.
// the top left corner is (-1,-1) and the bottom right corner is (1,1). The center is (0,0).

// the 1st and 2nd parameters are the X and Y coordinates of the object.
// gl_VertexIndex contains the index of the current vertex for each time the main function is run
// the 3rd parameter is the Z value (0 to 1) (0 is the front-most layer).
// in subsequent graphics pipeline stages, th gl_Position vector is turned into a normalized
// coordinate by dividing the whole vector by its last component.
// the 4th parameter is what the vector will be divided by.
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}