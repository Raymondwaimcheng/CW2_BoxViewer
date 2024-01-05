#include <iostream>
//GLAD
#include "GLAD/glad.h"
//No GLEW With GLAD
//#include <GL/glew.h>
//#include "Main.h"
//#include "LoadShaders.h"

#include "glm/glm/ext/vector_float3.hpp"
#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "FastNoiseLite.h"
//Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
//LEARNOPENGL
#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>

#include "Main.h"

/*#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/
#define RENDER_DISTANCE 128 //Render width of map
#define MAP_SIZE RENDER_DISTANCE * RENDER_DISTANCE //Size of map in x & z space

const int squaresRow = RENDER_DISTANCE - 1; //Amount of chunks across one dimension
const int trianglesPerSquare = 2; //Two triangles per square to form a 1x1 chunk
const int trianglesGrid = squaresRow * squaresRow * trianglesPerSquare; //Amount of triangles on map

using namespace std;
using namespace glm;

mat4 model;
mat4 view;
mat4 projection;

vec3 cameraPosition = vec3(0.0f, 0.0f, 3.0f); //Relative position within world space
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f); //The direction of travel
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f); //Up position within world space

float cameraYaw = -90.0f; //Camera sideways rotation
float cameraPitch = 0.0f; //Camera vertical rotation
bool mouseFirstEntry = true; //Determines if first entry of mouse into window
float cameraLastXPos = 800.0f / 2.0f;  //Positions of camera from given last frame
float cameraLastYPos = 600.0f / 2.0f;

float deltaTime = 0.0f; //Time change
float lastFrame = 0.0f; //Last value of time change

//VAO vertex attribute positions in correspondence to vertex attribute type
enum VAO_IDs { Triangles, Indices, Colours, Textures, NumVAOs = 2 };
//VAOs
GLuint VAOs[NumVAOs];

//Buffer types
enum Buffer_IDs { ArrayBuffer, NumBuffers = 4 };
//Buffer objects
GLuint Buffers[NumBuffers];
GLuint program;

int main()
{
    int windowWidth = 1280;
    int windowHeight = 720;
    //Initialisation of GLFW
    glfwInit();
    //Initialisation of 'GLFWwindow' object
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Lab5", NULL, NULL);

    //Checks if window has been successfully instantiated
    if (window == NULL)
    {
        cout << "GLFW Window did not instantiate\n";
        glfwTerminate();
        return -1;
    }

    //Sets cursor to automatically bind to window & hides cursor pointer
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //Binds OpenGL to window
    glfwMakeContextCurrent(window);
    //glewInit();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "GLAD failed to initialise\n";
        return -1;
    }

    //Loading of shaders
    Shader program("shaders/vertexShader.vert", "shaders/fragmentShader.frag");
    Model Rock("media/rock/Rock07-Base.obj");
    program.use();

    /*ShaderInfo shaders[] =
    {
        { GL_VERTEX_SHADER, "shaders/vertexShader.vert" },
        { GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag" },
        { GL_NONE, NULL }
    };

    program = LoadShaders(shaders);
    glUseProgram(program);*/

    glViewport(0, 0, windowWidth, windowHeight);
    //Sets the framebuffer_size_callback() function as the callback for the window resizing event
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    /*float vertices[] = {
        //Positions             //Textures
        0.5f, 0.5f, 0.0f,       1.0f, 1.0f, //top right
        0.5f, -0.5f, 0.0f,      1.0f, 0.0f, //bottom right
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, //bottom left
        -0.5f, 0.5f, 0.0f,      0.0f, 1.0f  //top left
    };

    unsigned int indices[] = {
        0, 1, 3, //first triangle
        1, 2, 3 //second triangle
    };*/

    //Assigning perlin noise type for map
    FastNoiseLite TerrainNoise;
    //Setting noise type to Perlin
    TerrainNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    //Sets the noise scale
    TerrainNoise.SetFrequency(0.05f);
    //Generates a random seed between integers 0 & 100
    int terrainSeed = rand() % 100;
    //Sets seed for noise
    TerrainNoise.SetSeed(terrainSeed);

    //Biome noise
    FastNoiseLite BiomeNoise;
    BiomeNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    BiomeNoise.SetFrequency(0.05f);
    int biomeSeed = rand() % 100;
    TerrainNoise.SetSeed(biomeSeed);

    //Generation of height map vertices
    GLfloat terrainVertices[MAP_SIZE][6];

    //Positions to start drawing from
    float drawingStartPosition = 1.0f;
    float columnVerticesOffset = drawingStartPosition;
    float rowVerticesOffset = drawingStartPosition;

    int rowIndex = 0;
    int i = 0;
    //Using x & y nested for loop in order to apply noise 2-dimensionally
    for (int y = 0; y < RENDER_DISTANCE; y++)
    {
        for (int x = 0; x < RENDER_DISTANCE; x++)
        {
            //Setting of height from 2D noise value at respective x & y coordinate
            terrainVertices[i][1] = TerrainNoise.GetNoise((float)x, (float)y);

            //Retrieval of biome to set
            float biomeValue = BiomeNoise.GetNoise((float)x, (float)y);

            if (biomeValue <= -0.75f) //Plains
            {
                terrainVertices[i][3] = 1.0f;
                terrainVertices[i][4] = 1.0f;
                terrainVertices[i][5] = 0.5f;
            }
            else //Desert
            {
                terrainVertices[i][3] = 0.0f;
                terrainVertices[i][4] = 0.75f;
                terrainVertices[i][5] = 0.25f;
                
            }

            i++;
        }
    }

    cout << "Start here";
    for (int i = 0; i < MAP_SIZE; i++)
    {
        //Generation of x & z vertices for horizontal plane
        terrainVertices[i][0] = columnVerticesOffset;
        //terrainVertices[i][1] = 0.0f;
        terrainVertices[i][2] = rowVerticesOffset;

        //Colour
        //terrainVertices[i][3] = 0.0f;
        //terrainVertices[i][4] = 0.75f;
        //terrainVertices[i][5] = 0.25f;

        if (terrainVertices[i][1] >= (4.0f / 8.0f)) {
            //snow
            terrainVertices[i][3] = 1.0f;
            terrainVertices[i][4] = 1.0f;
            terrainVertices[i][5] = 1.0f;
        }

        if (terrainVertices[i][1] <= (0.025f)) {
            //sea
            terrainVertices[i][3] = 0.0f;
            terrainVertices[i][4] = 0.0f;
            terrainVertices[i][5] = 1.0f;
        }

        //Shifts x position across for next triangle along grid
        columnVerticesOffset = columnVerticesOffset + -0.0625f;

        //Indexing of each chunk within row
        rowIndex++;
        //True when all triangles of the current row have been generated
        if (rowIndex == RENDER_DISTANCE)
        {
            //Resets for next row of triangles
            rowIndex = 0;
            //Resets x position for next row of triangles
            columnVerticesOffset = drawingStartPosition;
            //Shifts y position
            rowVerticesOffset = rowVerticesOffset + -0.0625f;
        }
    }
    cout << "Start2 here";
    //Generation of height map indices
    GLuint terrainIndices[trianglesGrid][3];

    //Positions to start mapping indices from
    int columnIndicesOffset = 0;
    int rowIndicesOffset = 0;

    //Generation of map indices in the form of chunks (1x1 right angle triangle squares)
    rowIndex = 0;
    for (int i = 0; i < trianglesGrid - 1; i += 2)
    {
        terrainIndices[i][0] = columnIndicesOffset + rowIndicesOffset; //top left
        terrainIndices[i][2] = RENDER_DISTANCE + columnIndicesOffset + rowIndicesOffset; //bottom left
        terrainIndices[i][1] = 1 + columnIndicesOffset + rowIndicesOffset; //top right

        terrainIndices[i + 1][0] = 1 + columnIndicesOffset + rowIndicesOffset; //top right
        terrainIndices[i + 1][2] = RENDER_DISTANCE + columnIndicesOffset + rowIndicesOffset; //bottom left
        terrainIndices[i + 1][1] = 1 + RENDER_DISTANCE + columnIndicesOffset + rowIndicesOffset; //bottom right

        //Shifts x position across for next chunk along grid
        columnIndicesOffset = columnIndicesOffset + 1;

        //Indexing of each chunk within row
        rowIndex++;

        //True when all chunks of the current row have been generated
        if (rowIndex == squaresRow)
        {
            //Resets for next row of chunks
            rowIndex = 0;
            //Resets x position for next row of chunks
            columnIndicesOffset = 0;
            //Shifts y position
            rowIndicesOffset = rowIndicesOffset + RENDER_DISTANCE;
        }
    }
    cout << "Start3 here";
    //Sets index of VAO
    glGenVertexArrays(NumVAOs, VAOs);
    //Binds VAO to a buffer
    glBindVertexArray(VAOs[0]);
    //Sets indexes of all required buffer objects
    glGenBuffers(NumBuffers, Buffers);

    //Binds Vertex object to array buffer
    glBindBuffer(GL_ARRAY_BUFFER, Buffers[Triangles]);
    //Allocates buffer memory for the vertices of the 'Trianlges' buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(terrainVertices), terrainVertices, GL_STATIC_DRAW);

    //Binds Vertex object to elements array buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[Indices]);
    //Allocates buffer memory for the vertices of the 'Indices' buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(terrainIndices), terrainIndices, GL_STATIC_DRAW);


    //Allocates vertex attribute memory for vertex shader
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    //Index of vertex attribute for vertex shader
    //glEnableVertexAttribArray(0);
    //Textures
    //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    //glEnableVertexAttribArray(1);

    //Allocation & indexing of vertex attribute memory for vertex shader
    //Positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //Colours
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Unbinding
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /*//Textures to generate
    glGenTextures(NumBuffers, Buffers);

    //Binding texture to type 2D texture
    glBindTexture(GL_TEXTURE_2D, Buffers[Textures]);

    //Selects x axis (S) of texture bound to GL_TEXTURE_2D & sets to repeat beyond normalised coordinates
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //Selects y axis (T) equivalently
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //Parameters that will be sent & set based on retrieved texture
    int width, height, colourChannels;
    //Retrieves texture data
    unsigned char* data = stbi_load("media/woodPlanks.jpg", &width, &height, &colourChannels, 0);

    if (data) //If retrieval successful
    {
        //Generation of texture from retrieved texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        //Automatically generates all required mipmaps on bound texture
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else //If retrieval unsuccessful
    {
        cout << "Failed to load texture.\n";
        return -1;
    }

    //Clears retrieved texture from memory
    stbi_image_free(data);*/
    cout << "Render Start here";
    //MVP
    model = mat4(1.0f);
    model = scale(model, vec3(2.0f, 2.0f, 2.0f));
    model = rotate(model, radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
    model = translate(model, vec3(0.0f, -2.f, -1.5f));
    //view = lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
    projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    //Render loop
    while (glfwWindowShouldClose(window) == false)
    {
        //cout << "loop start" << endl;
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //Input
        ProcessUserInput(window);

        //Rendering
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //Colour to display on cleared window
        glClear(GL_COLOR_BUFFER_BIT); //Clears the colour buffer
        //glEnable(GL_CULL_FACE);

        //Transform
        //Instantiation of matrix
        //model = mat4(1.0f);
        //Downscale by 50%
        //model = scale(model, vec3(2.0f, 2.0f, 2.0f));
        //Rotate by 90 degrees on Z axis
        //model = rotate(model, (float)glfwGetTime(), vec3(1.0f, 0.0f, 0.0f));
        //Rotation to look down
        //model = rotate(model, radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
        //Movement to position further back
        //model = translate(model, vec3(0.0f, -2.f, -1.5f));
        //View matrix
        view = lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
        //Projection matrix
        //projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

        //cout << "Problem1 start" << endl;
        // Model-view-projection matrix uniform for vertex shader
        //mat4 mvp = projection * view * model;
        //int mvpLoc = glGetUniformLocation(program, "mvpIn");
        //glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, value_ptr(mvp));
        //program.setMat4("mvpIn", mvp);
        SetMatrices(program);

        //cout << "Problem2 start" << endl;
        glBindTexture(GL_TEXTURE_2D, Buffers[Textures]);
        glBindVertexArray(VAOs[0]); //Bind buffer object to render; VAOs[0]
        glDrawElements(GL_TRIANGLES, MAP_SIZE * 32, GL_UNSIGNED_INT, 0);

        //model = scale(model, vec3(0.025f, 0.025f, 0.025f));
        //SetMatrices(program);
        //Rock.Draw(program);

        //cout << "Problem3 start" << endl;
        //Refreshing
        glfwSwapBuffers(window); //Swaps the colour buffer
        glfwPollEvents(); //Queries all GLFW events

    }

    //Safely terminates GLFW
    glfwTerminate();
    return 0;

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    //Resizes window based on contemporary width & height values
    glViewport(0, 0, width, height);
}

void ProcessUserInput(GLFWwindow* WindowIn)
{
    //Closes window on 'exit' key press
    if (glfwGetKey(WindowIn, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(WindowIn, true);
    }

    //Extent to which to move in one instance
    const float movementSpeed = 1.0f * deltaTime;
    //Closes window on 'exit' key press
    if (glfwGetKey(WindowIn, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(WindowIn, true);
    }
    if (glfwGetKey(WindowIn, GLFW_KEY_W) == GLFW_PRESS)
    {
        cout << "W was pressed" << endl;
        cameraPosition += movementSpeed * cameraFront;
    }
    if (glfwGetKey(WindowIn, GLFW_KEY_S) == GLFW_PRESS)
    {
        cout << "S was pressed" << endl;
        cameraPosition -= movementSpeed * cameraFront;
    }
    if (glfwGetKey(WindowIn, GLFW_KEY_A) == GLFW_PRESS)
    {
        cout << "A was pressed" << endl;
        cameraPosition -= normalize(cross(cameraFront, cameraUp)) * movementSpeed;
    }
    if (glfwGetKey(WindowIn, GLFW_KEY_D) == GLFW_PRESS)
    {
        cout << "D was pressed" << endl;
        cameraPosition += normalize(cross(cameraFront, cameraUp)) * movementSpeed;
    }

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    //Initially no last positions, so sets last positions to current positions
    if (mouseFirstEntry)
    {
        cameraLastXPos = (float)xpos;
        cameraLastYPos = (float)ypos;
        mouseFirstEntry = false;
    }

    //Sets values for change in position since last frame to current frame
    float xOffset = (float)xpos - cameraLastXPos;
    float yOffset = cameraLastYPos - (float)ypos;

    //Sets last positions to current positions for next frame
    cameraLastXPos = (float)xpos;
    cameraLastYPos = (float)ypos;

    //Moderates the change in position based on sensitivity value
    const float sensitivity = 0.025f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    //Adjusts yaw & pitch values against changes in positions
    cameraYaw += xOffset;
    cameraPitch += yOffset;

    //Prevents turning up & down beyond 90 degrees to look backwards
    if (cameraPitch > 89.0f)
    {
        cameraPitch = 89.0f;
    }
    else if (cameraPitch < -89.0f)
    {
        cameraPitch = -89.0f;
    }

    //Modification of direction vector based on mouse turning
    vec3 direction;
    direction.x = cos(radians(cameraYaw)) * cos(radians(cameraPitch));
    direction.y = sin(radians(cameraPitch));
    direction.z = sin(radians(cameraYaw)) * cos(radians(cameraPitch));
    cameraFront = normalize(direction);
}

void SetMatrices(Shader& ShaderProgramIn) {
    mat4 mvp = projection * view * model;
    ShaderProgramIn.setMat4("mvpIn", mvp);
}