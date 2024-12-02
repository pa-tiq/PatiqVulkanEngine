#version 450

// This file is the vertex shader.

// As input, the vertex shader will get each vertex as input from the Assembler stage, 
// and then needs to output a position.

// "in" signifies this variable takes its value from a vertex buffer
// "layout(location)" sets the storage of where this variable value will come from
// this is how we connect the attribute description to the variable we mean to reference in the shader
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
    mat4 transform;
    vec3 color;
} push;

void main() {
// the gl_Position is a 4-dimensional vector that maps to the output frame buffer image.
// the top left corner is (-1,-1) and the bottom right corner is (1,1). The center is (0,0).

// the 1st and 2nd parameters are the X and Y coordinates of the object.
// gl_VertexIndex contains the index of the current vertex for each time the main function is run
// the 3rd parameter is the Z value (0 to 1) (0 is the front-most layer).
// in subsequent graphics pipeline stages, th gl_Position vector is turned into a normalized
// coordinate by dividing the whole vector by its last component.
// the 4th parameter is what the vector will be divided by.
// gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

// the position attribute will automatically be set with the value from the vertex buffer
    //gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0);
    gl_Position = vec4(push.transform * vec4(position, 1.0));
    fragColor = color;
}