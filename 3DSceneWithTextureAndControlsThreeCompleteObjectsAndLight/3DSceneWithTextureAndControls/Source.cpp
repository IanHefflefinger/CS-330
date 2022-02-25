#include <iostream>             // cout, cerr
#include <cstdlib>              // EXIT_FAILURE
#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// FIXME: include directions on how to use the program

// Unnamed namespace
namespace
{
    // to convert degrees to radians
    float pi = 3.14159265359;
    float toRadians = (pi / 180); // multiply by this to get radians

    const char* const WINDOW_TITLE = "3D scene w/texture, controls, and lighting"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao[6];         // Handle for the vertex array object
        GLuint vbo[6];         // Handle for the vertex buffer object
        GLuint nVertices[6];    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh gMesh;

    // Texture(s) (include all textures here)
    GLuint tableTextureId, penTextureId, watchStrapTextureId, watchBodyTextureId, watchFaceTextureId, appleTextureId; // FIXME: remove appleTextureId
    glm::vec2 gUVScale(1.0f, 1.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader program
    GLuint gProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 1.0f, 7.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool isPerspective = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Light: color, position, object, and scale
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f); // FIXME: slightly yellow
    glm::vec3 gLightPosition(8.0f, 6.0f, 2.0f); // above table and slightly to the right
    glm::vec3 gLightScale(0.5f); // change me as needed

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
    layout(location = 1) in vec3 normal; // Normal data from Vertex Attrib Pointer 1
    layout(location = 2) in vec2 textureCoordinate;  // Texture data from Vertex Attrib Pointer 2

    // normals, fragments, and texture coordinates
    out vec3 vertexNormal; // outgoing normals
    out vec3 vertexFragmentPos; // outgoing fragment positions
    out vec2 vertexTextureCoordinate; // outgoing texture coordinates

    //Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate;
    }
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // Incoming noramls
    in vec3 vertexFragmentPos; // Incoming fragment position
    in vec2 vertexTextureCoordinate; // Incoming texture coordinates

    out vec4 fragmentColor; // Ooutgoing object color to the GPU

    // Uniform / Global variables for light color, light position, and camera/view position
    //uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    uniform sampler2D uTexture; // Useful when working with multiple textures
    uniform vec2 uvScale;

    void main()
    {
        /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

        //Calculate Ambient lighting*/
        float ambientStrength = 0.5f; // Set ambient or global lighting strength FIXME: adjust if too dark or bright
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting*/
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting*/
        float specularIntensity = 0.8f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    }
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

    void main()
    {
        fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    }
);


// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture(s)
    const char* tableTexFilename = "../resources/textures/woodTableTexture.png"; 
    const char* penTexFilename = "../resources/textures/blueTexture1.png";
    const char* watchStrapTexFilename = "../resources/textures/watchStrapTexture.png";
    const char* watchBodyTexFilename = "../resources/textures/watchBodyTexture.png";
    const char* watchFaceTexFilename = "../resources/textures/watchFaceTexture.png";
    const char* appleTexFilename = "../resources/textures/appleTexture.png"; 



    if (!UCreateTexture(tableTexFilename, tableTextureId))
    {
        cout << "Failed to load texture " << tableTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(penTexFilename, penTextureId))
    {
        cout << "Failed to load texture " << penTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(watchStrapTexFilename, watchStrapTextureId))
    {
        cout << "Failed to load texture " << watchStrapTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(watchBodyTexFilename, watchBodyTextureId))
    {
        cout << "Failed to load texture " << watchBodyTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(watchFaceTexFilename, watchFaceTextureId))
    {
        cout << "Failed to load texture " << watchFaceTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(appleTexFilename, appleTextureId))
    {
        cout << "Failed to load texture " << appleTexFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture(s)
    UDestroyTexture(tableTextureId);
    UDestroyTexture(penTextureId);
    UDestroyTexture(watchStrapTextureId);
    UDestroyTexture(watchBodyTextureId);
    UDestroyTexture(watchFaceTextureId);
    UDestroyTexture(appleTextureId); 


    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraOffset = cameraSpeed * gDeltaTime;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    // add up and down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    // toggle normal and orthographic projection when user presses "P"
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        isPerspective = !isPerspective;

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset * 0.015, yoffset * 0.015); 
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Function called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // slight grey keeps the color of the watch body from blending in
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a projection THEN create perspective or orthographic view
    glm::mat4 projection;
    // NOW check if 'isPerspective' and THEN set the perspective
    if (isPerspective) {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        float scale = 90;
        projection = glm::ortho((800.0f / scale), -(800.0f / scale), -(600.0f / scale), (600.0f / scale), -2.5f, 6.5f);
    }
        
    // glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);


    // Table (plane that objects sit on)
    glm::mat4 scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 model = translation * rotation * scale;

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    GLint keyLightColorLoc = glGetUniformLocation(gProgramId, "keyLightColor"); // FIXME: might need to remove or add to

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[0]);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tableTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);


    // Pen (for writing) shaped object
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    rotation = glm::rotate(-60.0f * (toRadians), glm::vec3(0.0f, 1.0f, 0.0f)); 
    translation = glm::translate(glm::vec3(-1.5f, 0.0f, 1.7f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[1]);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, penTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[1]);


    // Watch strap 
    scale = glm::scale(glm::vec3(1.5f, 1.0f, 1.0f));
    rotation = glm::rotate(15.0f * (toRadians), glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-1.0f, 0.0f, -2.3f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[2]);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, watchStrapTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[2]);


    // Watch body
    scale = glm::scale(glm::vec3(0.6f, 0.4f, 0.7f));
    rotation = glm::rotate(15.0f * (toRadians), glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-1.0f, 0.0f, -2.3f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[3]);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, watchBodyTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[3]);


    // Watch face
    scale = glm::scale(glm::vec3(0.5f, 0.4f, 0.6f));
    rotation = glm::rotate(15.0f * (toRadians), glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-1.0f, 0.01f, -2.3f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[4]);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, watchFaceTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[4]);


    // Apple body
    scale = glm::scale(glm::vec3(0.9f, 1.2f, 1.0f));
    rotation = glm::rotate(18.0f * (toRadians), glm::vec3(1.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(1.4f, 0.75f, 0.2f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[5]);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, appleTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[5]);


    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]); // FIXME: might be better to use a specific object(s) as the light source(s)
    
    




    // KEEP DOWN HERE (out of the way of the code upstairs)
    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Vertex data
    GLfloat tableVerts[] = { 
        // Vertex Positions    // Normals // Texture 
        -5.0f, -0.1f,  5.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f,
         5.0f, -0.1f, -5.0f, 0.0f, 1.0f,  0.0f, 1.0f, 1.0f,
        -5.0f, -0.1f, -5.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,

        -5.0f, -0.1f,  5.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f,
         5.0f, -0.1f, -5.0f, 0.0f, 1.0f,  0.0f, 1.0f, 1.0f,
         5.0f, -0.1f,  5.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };
    GLfloat penVerts[] = { 
        // Vertex Positions    // Normals // Texture 
        // BODY OF PEN
         // back 
         -2.0f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
          2.0f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         -2.0f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

          2.0f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         -2.0f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
          2.0f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

          // front 
          -2.0f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
           2.0f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
          -2.0f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

           2.0f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
          -2.0f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
           2.0f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

           // left 
          -2.0f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
          -2.0f,  0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
          -2.0f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

          -2.0f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
          -2.0f, -0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
          -2.0f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

          // bottom 
         -2.0f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
          2.0f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

          2.0f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
          2.0f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

          // top 
         -2.0f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
          2.0f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,

          2.0f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
          2.0f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,

          // TIP OF PEN
          // front 
          2.0f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
          2.5f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
          2.0f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

          // back 
          2.0f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
          2.5f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
          2.0f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

          // top 
          2.0f, 0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
          2.0f, 0.1f, -0.1f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
          2.5f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,

          // bottom 
          2.0f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
          2.0f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
          2.5f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f
    };
    GLfloat watchStrapVerts[] = {
        // Vertex Positions   // Normals // Texture 
        // Strap of watch
         // back 
         -2.0f,  0.03f, -0.3f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
          2.0f,  0.03f, -0.3f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         -2.0f, -0.03f, -0.3f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

          2.0f, -0.03f, -0.3f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         -2.0f, -0.03f, -0.3f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
          2.0f,  0.03f, -0.3f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

          // front 
          -2.0f,  0.03f,  0.3f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
           2.0f,  0.03f,  0.3f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
          -2.0f, -0.03f,  0.3f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

           2.0f, -0.03f,  0.3f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
          -2.0f, -0.03f,  0.3f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
           2.0f,  0.03f,  0.3f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

           // left 
          -2.0f,  0.03f,  0.3f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
          -2.0f,  0.03f, -0.3f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
          -2.0f, -0.03f, -0.3f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

          -2.0f, -0.03f, -0.3f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
          -2.0f, -0.03f,  0.3f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
          -2.0f,  0.03f,  0.3f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

          // right 
           2.0f,  0.03f,  0.3f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
           2.0f,  0.03f, -0.3f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
           2.0f, -0.03f, -0.3f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

           2.0f, -0.03f, -0.3f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
           2.0f, -0.03f,  0.3f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
           2.0f,  0.03f,  0.3f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

          // bottom 
         -2.0f, -0.03f,  0.3f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f, -0.03f, -0.3f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
          2.0f, -0.03f, -0.3f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

          2.0f, -0.03f,  0.3f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f, -0.03f,  0.3f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
          2.0f, -0.03f, -0.3f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

          // top 
         -2.0f,  0.03f,  0.3f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f,  0.03f, -0.3f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
          2.0f,  0.03f, -0.3f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,

          2.0f,  0.03f,  0.3f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         -2.0f,  0.03f,  0.3f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
          2.0f,  0.03f, -0.3f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f
    };
    GLfloat watchBodyVerts[] = {
        // Vertex Positions   // Normals  // Texture 
        // TOP OF WATCH
        // first triangle
        -1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
        -1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

        // second triangle
        -1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         0.0f,  0.25f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

        // third triangle
         0.0f,  0.25f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

        // fourth triangle
         1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

        // fifth triangle
         1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         0.0f,  0.25f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

        // sixth triangle
         0.0f,  0.25f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
        -1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

        // BOTTOM OF WATCH
        // first triangle
        -1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

        // second triangle
        -1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         0.0f, -0.25f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

        // third triangle
         0.0f, -0.25f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

        // fourth triangle
         1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

        // fifth triangle
         1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         0.0f, -0.25f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

        // sixth triangle
         0.0f, -0.25f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

        // LEFT FACE
        -1.0f,  0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -1.0f,  0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -1.0f, -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        -1.0f, -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -1.0f, -0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -1.0f,  0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,

        // RIGHT FACE
         1.0f,  0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         1.0f,  0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         1.0f, -0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

         1.0f, -0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         1.0f, -0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         1.0f,  0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,

        // BACK LEFT FACE
         0.0f,  0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        -1.0f,  0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, -0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.0f, -0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.0f,  0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 1.0f,


        // BACK RIGHT FACE
         0.0f,  0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         1.0f,  0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 0.0f,

         0.0f,  0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         0.0f, -0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         1.0f, -0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 0.0f,


        // FRONT LEFT FACE
        -0.0f,  0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -1.0f,  0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        -0.0f,  0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.0f, -0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,


        // FRONT RIGHT FACE
        -0.0f,  0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
         1.0f,  0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
         1.0f, -0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,

         1.0f, -0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -0.0f, -0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.0f,  0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f
    };
    GLfloat watchFaceVerts[] = {
        // Vertex Positions    // Normals // Texture 
        // TOP OF WATCH
        // first triangle
        -1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
        -1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f, 0.0f,

         // second triangle
         -1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
          0.0f,  0.25f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
          0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f, 0.0f,

          // third triangle
           0.0f,  0.25f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
           1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
           0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f, 0.0f,

           // fourth triangle
            1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
            1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
            0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f, 0.0f,

            // fifth triangle
             1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
             0.0f,  0.25f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
             0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f, 0.0f,

             // sixth triangle
              0.0f,  0.25f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
             -1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
              0.0f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f, 0.0f,

              // BOTTOM OF WATCH
              // first triangle
              -1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
              -1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
               0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

               // second triangle
               -1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                0.0f, -0.25f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                // third triangle
                 0.0f, -0.25f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                 1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                 0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                 // fourth triangle
                  1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                  1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                  0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                  // fifth triangle
                   1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                   0.0f, -0.25f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                   0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                   // sixth triangle
                    0.0f, -0.25f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                   -1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                    0.0f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                    // LEFT FACE
                    -1.0f,  0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
                    -1.0f,  0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
                    -1.0f, -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

                    -1.0f, -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
                    -1.0f, -0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
                    -1.0f,  0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,

                    // RIGHT FACE
                     1.0f,  0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
                     1.0f,  0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
                     1.0f, -0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

                     1.0f, -0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
                     1.0f, -0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
                     1.0f,  0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,

                     // BACK LEFT FACE
                      0.0f,  0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 1.0f,
                     -1.0f,  0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
                     -1.0f, -0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 0.0f,

                     -1.0f, -0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 0.0f,
                      0.0f, -0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 0.0f,
                      0.0f,  0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 1.0f,


                      // BACK RIGHT FACE
                       0.0f,  0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
                       1.0f,  0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 1.0f,
                       1.0f, -0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 0.0f,

                       0.0f,  0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
                       0.0f, -0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 0.0f,
                       1.0f, -0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 0.0f,


                       // FRONT LEFT FACE
                       -0.0f,  0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                       -1.0f,  0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                       -1.0f, -0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

                       -0.0f,  0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                       -0.0f, -0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                       -1.0f, -0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,


                       // FRONT RIGHT FACE
                       -0.0f,  0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                        1.0f,  0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                        1.0f, -0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,

                        1.0f, -0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                       -0.0f, -0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                       -0.0f,  0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f
    };
    GLfloat appleVerts[] = {
        // Vertex Positions    // Normals // Texture
         // TOP OF WATCH
        // first triangle
        -1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
        -1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.7f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

         // second triangle
         -1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
          0.0f,  0.25f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
          0.0f,  0.7f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

          // third triangle
           0.0f,  0.25f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
           1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
           0.0f,  0.7f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

           // fourth triangle
            1.0f,  0.25f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
            1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
            0.0f,  0.7f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

            // fifth triangle
             1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
             0.0f,  0.25f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
             0.0f,  0.7f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

             // sixth triangle
              0.0f,  0.25f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
             -1.0f,  0.25f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
              0.0f,  0.7f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.5f,

              // BOTTOM OF WATCH
              // first triangle
              -1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
              -1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
               0.0f, -0.7f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

               // second triangle
               -1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                0.0f, -0.25f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                0.0f, -0.7f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                // third triangle
                 0.0f, -0.25f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                 1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                 0.0f, -0.7f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                 // fourth triangle
                  1.0f, -0.25f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                  1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                  0.0f, -0.7f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                  // fifth triangle
                   1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                   0.0f, -0.25f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                   0.0f, -0.7f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                   // sixth triangle
                    0.0f, -0.25f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                   -1.0f, -0.25f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
                    0.0f, -0.7f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.5f,

                    // LEFT FACE
                    -1.0f,  0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
                    -1.0f,  0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
                    -1.0f, -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

                    -1.0f, -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
                    -1.0f, -0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
                    -1.0f,  0.25f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,

                    // RIGHT FACE
                     1.0f,  0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
                     1.0f,  0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
                     1.0f, -0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

                     1.0f, -0.25f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
                     1.0f, -0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
                     1.0f,  0.25f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,

                     // BACK LEFT FACE
                      0.0f,  0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 1.0f,
                     -1.0f,  0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
                     -1.0f, -0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 0.0f,

                     -1.0f, -0.25f,  0.5f, -1.0f,  0.0f, -1.0f, 1.0f, 0.0f,
                      0.0f, -0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 0.0f,
                      0.0f,  0.25f,  1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 1.0f,


                      // BACK RIGHT FACE
                       0.0f,  0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
                       1.0f,  0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 1.0f,
                       1.0f, -0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 0.0f,

                       0.0f,  0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 1.0f,
                       0.0f, -0.25f,  1.0f,  1.0f,  0.0f, -1.0f, 1.0f, 0.0f,
                       1.0f, -0.25f,  0.5f,  1.0f,  0.0f, -1.0f, 0.0f, 0.0f,


                       // FRONT LEFT FACE
                       -0.0f,  0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                       -1.0f,  0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                       -1.0f, -0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

                       -0.0f,  0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                       -0.0f, -0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                       -1.0f, -0.25f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,


                       // FRONT RIGHT FACE
                       -0.0f,  0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                        1.0f,  0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                        1.0f, -0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,

                        1.0f, -0.25f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                       -0.0f, -0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                       -0.0f,  0.25f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f
    };


    mesh.nVertices[0] = sizeof(tableVerts) / (sizeof(tableVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[1] = sizeof(penVerts) / (sizeof(penVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[2] = sizeof(watchStrapVerts) / (sizeof(watchStrapVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[3] = sizeof(watchBodyVerts) / (sizeof(watchBodyVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[4] = sizeof(watchFaceVerts) / (sizeof(watchFaceVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[5] = sizeof(appleVerts) / (sizeof(appleVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    // Table mesh
    glGenVertexArrays(1, &mesh.vao[0]); // we can also generate multiple VAOs or buffers at the same time
    glGenBuffers(1, &mesh.vbo[0]);
    glBindVertexArray(mesh.vao[0]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(tableVerts), tableVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);



    // Pen mesh
    glGenVertexArrays(1, &mesh.vao[1]); // we can also generate multiple VAOs or buffers at the same time
    glGenBuffers(1, &mesh.vbo[1]);
    glBindVertexArray(mesh.vao[1]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[1]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(penVerts), penVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);



    // Watch strap mesh
    glGenVertexArrays(1, &mesh.vao[2]); // we can also generate multiple VAOs or buffers at the same time
    glGenBuffers(1, &mesh.vbo[2]);
    glBindVertexArray(mesh.vao[2]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[2]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(watchStrapVerts), watchStrapVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);



    // Watch body mesh
    glGenVertexArrays(1, &mesh.vao[3]); // we can also generate multiple VAOs or buffers at the same time
    glGenBuffers(1, &mesh.vbo[3]);
    glBindVertexArray(mesh.vao[3]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[3]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(watchBodyVerts), watchBodyVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);



    // Watch face mesh
    glGenVertexArrays(1, &mesh.vao[4]); // we can also generate multiple VAOs or buffers at the same time
    glGenBuffers(1, &mesh.vbo[4]);
    glBindVertexArray(mesh.vao[4]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[4]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(watchFaceVerts), watchFaceVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);



    // Apple mesh
    glGenVertexArrays(1, &mesh.vao[5]); // we can also generate multiple VAOs or buffers at the same time
    glGenBuffers(1, &mesh.vbo[5]);
    glBindVertexArray(mesh.vao[5]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[5]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(appleVerts), appleVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(2, mesh.vao);
    glDeleteBuffers(2, mesh.vbo);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
