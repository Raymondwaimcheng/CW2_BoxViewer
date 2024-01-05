#version 460
//Triangle position with values retrieved from main.cpp
layout (location = 0) in vec3 position;
//layout (location = 1) in vec2 texPosition;
layout (location = 1) in vec3 colourVertex;
layout (location = 2) in vec2 texPosition;
//out vec2 texCoordFrag;
uniform mat4 mvpIn;

out vec3 colourFrag;

void main()
{
    //Triangle vertice sent through gl_Position to next stage
    gl_Position = mvpIn * vec4(position.x, position.y, position.z, 1.0);
    //texCoordFrag = texPosition;

    colourFrag = colourVertex;
}