#version 460
//Colour value to send to next stage
out vec4 FragColor;

//in vec2 texCoordFrag;
//uniform sampler2D textureIn;

in vec3 colourFrag;

void main()
{
    //RGBA values
    //FragColor = texture(textureIn, texCoordFrag);

    //Setting of colour coordinates to colour map
    FragColor = vec4(colourFrag, 1.0f);
}