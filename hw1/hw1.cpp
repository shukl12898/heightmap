/*
  CSCI 420 Computer Graphics, Computer Science, USC
  Assignment 1: Height Fields with Shaders.
  C/C++ starter code

  Student username: shuklaak
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <iostream>
#include <cstring>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0;   // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0;  // 1 if pressed, 0 if not

typedef enum
{
  ROTATE, // Triggered by dragging of mouse
  TRANSLATE, // Triggered by holding down 't' key and dragging of mouse
  SCALE // Triggered by holding down 'Shift' key and dragging of mouse
} CONTROL_STATE;
CONTROL_STATE controlState = ROTATE; // Default control state is ROTATE

typedef enum
{
  POINTS, // Triggered by pressing '1' key
  LINES, // Triggered by pressing '2' key
  TRIANGLES, // Triggered by pressing '3' key
  SMOOTH_TRIANGLES // Triggered by pressing '4' key
} RENDER_STATE;
RENDER_STATE renderState = POINTS; // Initial render state is POINTS

// Transformations of the terrain.
float terrainRotate[3] = {0.0f, 0.0f, 0.0f};
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = {0.0f, 0.0f, 0.0f};
float terrainScale[3] = {1.0f, 1.0f, 1.0f};

float smoothScale = 1.0f; // Default value for scale used in vertexShader
float smoothExponent = 1.0f; // Default value for exponent used in vertexShader

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 1";

// Stores the image loaded from disk.
ImageIO *heightmapImage;

// Number of vertices required for different render modes
int numVerticesPoints;
int numVerticesLines;
int numVerticesTriangles;
int numVerticesSmooth;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram *pipelineProgram = nullptr;

// VBO and VAO global variables for point rendering
VBO *vboVerticesPoints = nullptr;
VBO *vboColorsPoints = nullptr;
VAO *vaoPoints = nullptr;

// VBO and VAO global variables for line (wireframe) rendering
VBO *vboVerticesLines = nullptr;
VBO *vboColorsLines = nullptr;
VAO *vaoLines = nullptr;

// VBO and VAO global variables for triangle rendering
VBO *vboVerticesTriangles = nullptr;
VBO *vboColorsTriangles = nullptr;
VAO *vaoTriangles = nullptr;

// VBO and VAO global variables for smoothened triangle rendering
VBO* vboCenterSmooth = nullptr;
VBO* vboLeftSmooth = nullptr;
VBO* vboRightSmooth = nullptr;
VBO* vboUpSmooth = nullptr;
VBO* vboDownSmooth = nullptr;
VBO* vboColorsSmooth = nullptr;
VAO* vaoSmooth = nullptr;

// Write a screenshot to the specified filename.
void saveScreenshot(const char *filename)
{
  unsigned char *screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else
    cout << "Failed to save file " << filename << '.' << endl;

  delete[] screenshotData;
}

void idleFunc()
{
  // Static variable to count frame rate
  static int counter = 3;
  // Static variable to count number of screenshots
  static int screenshotCounter = 0;

  // Frequency of screenshots: Capture a screenshot every 10 frames
  const int frequency = 10; 

  // Increment the counter
  counter++;

  // Check if it's time to take a screenshot and that the max number of screenshots has not been exceeded
  if ((counter % frequency == 0)&&(screenshotCounter < 301)){
  
    // Format the screenshot filename with the current screenshot number as specified in instructions
    char filename[128];
    sprintf(filename, "%03d.jpg", screenshotCounter);

    // Call saveScreenshot helper function with appropriate filename
    saveScreenshot(filename);
    
    // Increment number of screenshots saved
    screenshotCounter++;
  }

  // Notify GLUT that it should call displayFunc.
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar.
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 10000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = {x - mousePos[0], y - mousePos[1]};

  switch (controlState)
  {
  // translate the terrain
  case TRANSLATE:
    if (leftMouseButton)
    {
      // control x,y translation via the left mouse button
      terrainTranslate[0] += mousePosDelta[0] * 0.01f;
      terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
    }
    if (middleMouseButton)
    {
      // control z translation via the middle mouse button
      terrainTranslate[2] += mousePosDelta[1] * 0.01f;
    }
    break;

  // rotate the terrain
  case ROTATE:
    if (leftMouseButton)
    {
      // control x,y rotation via the left mouse button
      terrainRotate[0] += mousePosDelta[1];
      terrainRotate[1] += mousePosDelta[0];
    }
    if (middleMouseButton)
    {
      // control z rotation via the middle mouse button
      terrainRotate[2] += mousePosDelta[1];
    }
    break;

  // scale the terrain
  case SCALE:
    if (leftMouseButton)
    {
      // control x,y scaling via the left mouse button
      terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
      terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
    }
    if (middleMouseButton)
    {
      // control z scaling via the middle mouse button
      terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
    }
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
  case GLUT_LEFT_BUTTON:
    leftMouseButton = (state == GLUT_DOWN);
    break;

  case GLUT_MIDDLE_BUTTON:
    middleMouseButton = (state == GLUT_DOWN);
    break;

  case GLUT_RIGHT_BUTTON:
    rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
  
  // Unused case due to M1 Macbook Pro, instead bound to the 't' key
  case GLUT_ACTIVE_CTRL:
    controlState = TRANSLATE;
    break;

  case GLUT_ACTIVE_SHIFT:
    controlState = SCALE;
    break;

  // If CTRL and SHIFT are not pressed, we are in rotate mode.
  default:
    controlState = ROTATE;
    break;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
  case 27:   // ESC key
    exit(0); // exit the program
    break;

  case ' ':
    cout << "You pressed the spacebar." << endl;
    break;

  case 49: // '1' key
    cout << "Render the height field as points" << endl; // Inform user that point mode has been entered
    renderState = POINTS; // Set renderState to POINTS
    break;

  case 50: // '2' key
    cout << "Render the height field as lines" << endl; // Inform user that wireframe mode has been entered 
    renderState = LINES; // Set renderState to LINES
    break;

  case 51: // '3' key
    cout << "Render the height field as triangles" << endl; // Inform user that triangle mode has been entered
    renderState = TRIANGLES; // Set renderState to TRIANGLES
    break;

  case 52: // '4' key
    cout << "Render the height field as smoothened triangles" << endl; // Inform user that smoothened triangle mode has been entered
    renderState = SMOOTH_TRIANGLES; // Set renderState to SMOOTH_TRIANGLES
    break;

  case 't':
    controlState = TRANSLATE; // Invoke translation here instead of 'ctrl' key
    break;

  case 'x':
    // Take a screenshot.
    saveScreenshot("screenshot.jpg");
    break;

  case '+':
    smoothScale *= 2; // Double scale used in smoothened triangle mode
    break;

  case '-':
    smoothScale /= 2; // Half scale used in smoothened triangle mode
    break;

  case '9':
    smoothExponent *= 2; // Double exponent used in smoothened triangle mode
    break;
  
  case '0':
    smoothExponent /= 2; // Half exponent used in smoothened triangle mode
    break;

  }
}

void displayFunc()
{
  // This function performs the actual rendering.

  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(0.0, 0.0, 2.0,
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0);

  // Additional modeling performed on the object: translations, rotations and scales.
  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
  matrix.Rotate(terrainRotate[0], 1.0f, 0.0f, 0.0f); // Rotation on x-Axis
  matrix.Rotate(terrainRotate[1], 0.0f, 1.0f, 0.0f); // Rotation on y-Axis
  matrix.Rotate(terrainRotate[2], 0.0f, 0.0f, 1.0f); // Rotation on z-Axis
  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
  // Important: these matrices must be uploaded to *all* pipeline programs used.
  // In hw1, there is only one pipeline program, but in hw2 there will be several of them.
  // In such a case, you must separately upload to *each* pipeline program.
  // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
  pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // Upload the default mode, scale, and exponent "uniform" variables to GPU for un-smoothened rendering
  pipelineProgram->SetUniformVariablei("mode", 0);
  pipelineProgram->SetUniformVariablef("scale", 1.0f);
  pipelineProgram->SetUniformVariablef("exponent", 1.0f);

  // Execute the rendering.
  // Bind the VAO that we want to render. Remember, one object = one VAO.
  if (renderState == POINTS)
  {
    vaoPoints->Bind();
    glDrawArrays(GL_POINTS, 0, numVerticesPoints); // Render the VAO using GL_POINTS
  }
  else if (renderState == LINES)
  {
    vaoLines->Bind();
    glDrawArrays(GL_LINES, 0, numVerticesLines); // Render the VAO using GL_LINES
  }
  else if (renderState == TRIANGLES)
  {
    vaoTriangles->Bind();
    glDrawArrays(GL_TRIANGLES, 0, numVerticesTriangles); // Render the VAO using GL_TRIANGLES
  }
  else{
    vaoSmooth->Bind();

    // Upload the smoothened mode, altered scale, and altered exponent "uniform" variables to GPU for smooth rendering
    pipelineProgram->SetUniformVariablei("mode", 1);
    pipelineProgram->SetUniformVariablef("scale", smoothScale);
    pipelineProgram->SetUniformVariablef("exponent", smoothExponent);
    glDrawArrays(GL_TRIANGLES, 0, numVerticesSmooth); // Render the VAO using GL_TRIANGLES
  }

  // Swap the double-buffers.
  glutSwapBuffers();
}

void initScene(int argc, char *argv[])
{
  // Load the image from a jpeg disk file into main memory.
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
  // In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
  // In hw2, we will need to shade different objects with different shaders, and therefore, we will have
  // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
  pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  }
  cout << "Successfully built the pipeline program." << endl;

  // Bind the pipeline program that we just created.
  // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
  // any object rendered from that point on, will use those shaders.
  // When the application starts, no pipeline program is bound, which means that rendering is not set up.
  // So, at some point (such as below), we need to bind a pipeline program.
  // From that point on, exactly one pipeline program is bound at any moment of time.
  pipelineProgram->Bind();

  // Extract width and height of image to be used for vertex generation
  int resolutionWidth = heightmapImage->getWidth();
  int resolutionHeight = heightmapImage->getHeight();

  // POINT RENDERING INITIALIZATION
  // Multiply width and height to determine vertices required for point rendering
  numVerticesPoints = resolutionWidth * resolutionHeight;

  // Vertex positions and colors
  float *positionsPoints = (float *)malloc(numVerticesPoints * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *colorsPoints = (float *)malloc(numVerticesPoints * 4 * sizeof(float));    // 4 floats per vertex, i.e., r,g,b,a

  // Indexes to iterate through positionsPoints and colorsPoints
  int indexPos = 0;
  int indexCol = 0;

  // Loop through all the pixels in the image
  for (int i = 0; i < resolutionWidth; i++)
  {
    for (int j = 0; j < resolutionHeight; j++)
    {
      // Normalize and prevent integer division for x, y, z coordinates
      float x = 1.0 * i / (resolutionWidth - 1);
      positionsPoints[indexPos++] = x;
      float y = 1.0 * heightmapImage->getPixel(i, j, 0) / 255.0f;
      // Multiply y position by 0.3 for aesthetic purposes 
      positionsPoints[indexPos++] = 0.3 * y;
      float z = 1.0 * j / (resolutionHeight - 1);
      positionsPoints[indexPos++] = z;

      // Store height of pixel for RGB values, and 1.0f for alpha
      colorsPoints[indexCol++] = y;
      colorsPoints[indexCol++] = y;
      colorsPoints[indexCol++] = y;
      colorsPoints[indexCol++] = 1.0f;
    }
  }

  // Create the VBOs.
  // We make a separate VBO for vertices and colors.
  // This operation must be performed BEFORE we initialize any VAOs.
  vboVerticesPoints = new VBO(numVerticesPoints, 3, positionsPoints, GL_STATIC_DRAW); // 3 values per position
  vboColorsPoints = new VBO(numVerticesPoints, 4, colorsPoints, GL_STATIC_DRAW);      // 4 values per color

  // Create the VAOs. There is a single VAO in this example.
  // Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
  // A VAO contains the geometry for a single object. There should be one VAO per object.
  // In this homework, "geometry" means vertex positions and colors. In homework 2, it will also include
  // vertex normal and vertex texture coordinates for texture mapping.
  vaoPoints = new VAO();

  // Set up the relationship between the "position" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.
  vaoPoints->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVerticesPoints, "position");

  // Set up the relationship between the "color" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.
  vaoPoints->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsPoints, "color");

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(positionsPoints);
  free(colorsPoints);

  // WIREFRAME RENDERING INITIALIZATION
  // Calculations to determine the number of lines required to create the wireframe based on height and width
  int numHorizontalLines = (resolutionWidth - 1) * resolutionHeight;
  int numVerticalLines = (resolutionHeight - 1) * resolutionWidth;
  int totalLines = numHorizontalLines + numVerticalLines;

  // Each line must have a start point and end point, so numVertices = double the amount of lines
  numVerticesLines = totalLines * 2;

  // Vertex positions and colors
  float *positionsLines = (float *)malloc(numVerticesLines * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *colorsLines = (float *)malloc(numVerticesLines * 4 * sizeof(float));    // 4 floats per vertex, i.e., r,g,b,a
  
  // Reset indexes to iterate through positionsLines and colorsLines
  indexPos = 0;
  indexCol = 0;

  // Loop through all pixels in the image
  for (int i = 0; i < resolutionWidth; i++)
  {
    for (int j = 0; j < resolutionHeight; j++)
    {
      // Stores start and end vertices for the horizontal lines
      // Consider the edge case (last pixel on width) which should not be the start point of a new line
      if (i < resolutionWidth - 1)
      {
        // Similar to Point mode, determine vertex coordinates for start point of line
        float xStart = 1.0 * i / (resolutionWidth - 1);
        positionsLines[indexPos++] = xStart;
        float yStart = 1.0 * heightmapImage->getPixel(i, j, 0) / 255.0f;
        positionsLines[indexPos++] = 0.3 * yStart;
        float zStart = 1.0 * j / (resolutionHeight - 1);
        positionsLines[indexPos++] = zStart;

        // Determine vertex coordinates for end point of line, incrementing x coordinate for all calculations (horizontal line)
        float xEnd = 1.0 * (i + 1) / (resolutionWidth - 1);
        positionsLines[indexPos++] = xEnd;
        float yEnd = 1.0 * heightmapImage->getPixel(i + 1, j, 0) / 255.0f;
        positionsLines[indexPos++] = 0.3 * yEnd;
        float zEnd = 1.0 * j / (resolutionHeight - 1);
        positionsLines[indexPos++] = zEnd;

        // Use height values as colour inputs for both start point and end point
        colorsLines[indexCol++] = yStart;
        colorsLines[indexCol++] = yStart;
        colorsLines[indexCol++] = yStart;
        colorsLines[indexCol++] = 1.0f;

        colorsLines[indexCol++] = yEnd;
        colorsLines[indexCol++] = yEnd;
        colorsLines[indexCol++] = yEnd;
        colorsLines[indexCol++] = 1.0f;
      }

      // Stores start and end vertices for the vertical lines
      // Consider the edge case (last pixel on height) which should not be the start point of a new line
      if (j < resolutionHeight - 1)
      {
        // Similar to Point mode, determine vertex coordinates for start point of line
        float xStart = 1.0 * i / (resolutionWidth - 1);
        positionsLines[indexPos++] = xStart;
        float yStart = 1.0 * heightmapImage->getPixel(i, j, 0) / 255.0f;
        positionsLines[indexPos++] = 0.3 * yStart;
        float zStart = 1.0 * j / (resolutionHeight - 1);
        positionsLines[indexPos++] = zStart;

        // Determine vertex coordinates for end point of line, incrementing z coordinate for all calculations ('vertical' line)
        float xEnd = 1.0 * i / (resolutionWidth - 1);
        positionsLines[indexPos++] = xEnd;
        float yEnd = 1.0 * heightmapImage->getPixel(i, j + 1, 0) / 255.0f;
        positionsLines[indexPos++] = 0.3 * yEnd;
        float zEnd = 1.0 * (j + 1) / (resolutionHeight - 1);
        positionsLines[indexPos++] = zEnd;

        // Use height values as colour inputs for both start point and end point
        colorsLines[indexCol++] = yStart;
        colorsLines[indexCol++] = yStart;
        colorsLines[indexCol++] = yStart;
        colorsLines[indexCol++] = 1.0f;

        colorsLines[indexCol++] = yEnd;
        colorsLines[indexCol++] = yEnd;
        colorsLines[indexCol++] = yEnd;
        colorsLines[indexCol++] = 1.0f;
      }
    }
  }

  // Create the VBOs.
  vboVerticesLines = new VBO(numVerticesLines, 3, positionsLines, GL_STATIC_DRAW); // 3 values per position
  vboColorsLines = new VBO(numVerticesLines, 4, colorsLines, GL_STATIC_DRAW);      // 4 values per color

  // Create the VAOs.
  vaoLines = new VAO();

  // Set up the relationship between the "position" shader variable and the VAO.
  vaoLines->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVerticesLines, "position");

  // Set up the relationship between the "color" shader variable and the VAO.
  vaoLines->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsLines, "color");

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(positionsLines);
  free(colorsLines);

  // TRIANGLE RENDERING INITIALIZATION
  // Calculate the total number of squares present in the image
  int numSquares = (resolutionWidth - 1) * (resolutionHeight - 1);
  // Each square will consist of 2 triangles, and each triangle will consist of 3 vertices, multiply numSquares by 6 to determine numVertices
  numVerticesTriangles = numSquares * 6;

  // Vertex positions and colors
  float *positionsTriangles = (float *)malloc(numVerticesTriangles * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *colorsTriangles = (float *)malloc(numVerticesTriangles * 4 * sizeof(float));    // 4 floats per vertex, i.e., r,g,b,a
  
  // Reset indexes to iterate through positionsTriangles and colorsTriangles
  indexPos = 0;
  indexCol = 0;

  // Loop through entire image except the last pixel (will not start a triangle from boundary of image)
  for (int i = 0; i < resolutionWidth - 1; i++)
  {
    for (int j = 0; j < resolutionHeight - 1; j++)
    {
      // Points for a square in the image: (i, j), (i+1, j), (i, j+1), (i+1, j+1)
      // Create helper arrays to store algebraic coordinates of each triangle present in a square
      // Triangle 1: (i, j) -> (i+1, j) -> (i, j+1)
      int pointsT1[3][2] = {{i, j}, {i + 1, j}, {i, j + 1}};

      // Triangle 2: (i+1, j) -> (i+1, j+1) -> (i, j+1)
      int pointsT2[3][2] = {{i + 1, j}, {i + 1, j + 1}, {i, j + 1}};

      // Loop three times in order to generate each vertex of Triangle 1 of a square begining at (i,j)
      for (int k = 0; k < 3; k++)
      {
        // Grab appropriate i and j values for the corresponding point from array, normalize and prevent integer division
        float x = (1.0f * pointsT1[k][0]) / (resolutionWidth - 1);
        float z = (1.0f * pointsT1[k][1]) / (resolutionHeight - 1);
        float y = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1], 0)) / 255.0f;

        // Store triangle vertex in positionsTriangle, multiplying height by 0.3 for aesthetic purposes
        positionsTriangles[indexPos++] = x;       
        positionsTriangles[indexPos++] = 0.3 * y; 
        positionsTriangles[indexPos++] = z;       

        // Use height values as colour inputs for vertex
        colorsTriangles[indexCol++] = y;    
        colorsTriangles[indexCol++] = y;    
        colorsTriangles[indexCol++] = y;    
        colorsTriangles[indexCol++] = 1.0f; 
      }

      // Loop three times in order to generate each vertex of Triangle 2 of a square begining at (i,j)
      for (int k = 0; k < 3; k++)
      {
        // Grab appropriate i and j values for the corresponding point from array, normalize and prevent integer division
        float x = (1.0f * pointsT2[k][0]) / (resolutionWidth - 1);
        float z = (1.0f * pointsT2[k][1]) / (resolutionHeight - 1);
        float y = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1], 0)) / 255.0f;

        // Store triangle vertex in positionsTriangle, multiplying height by 0.3 for aesthetic purposes
        positionsTriangles[indexPos++] = x;       
        positionsTriangles[indexPos++] = 0.3 * y; 
        positionsTriangles[indexPos++] = z;       

        // Use height values as colour inputs for vertex
        colorsTriangles[indexCol++] = y;    
        colorsTriangles[indexCol++] = y;    
        colorsTriangles[indexCol++] = y;    
        colorsTriangles[indexCol++] = 1.0f; 
      }
    }
  }

  // Create the VBOs.
  vboVerticesTriangles = new VBO(numVerticesTriangles, 3, positionsTriangles, GL_STATIC_DRAW); // 3 values per position
  vboColorsTriangles = new VBO(numVerticesTriangles, 4, colorsTriangles, GL_STATIC_DRAW);      // 4 values per color

  // Create the VAOs.
  vaoTriangles = new VAO();

  // Set up the relationship between the "position" shader variable and the VAO.
  vaoTriangles->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVerticesTriangles, "position");

  // Set up the relationship between the "color" shader variable and the VAO.
  vaoTriangles->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsTriangles, "color");

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(positionsTriangles);
  free(colorsTriangles);

  // SMOOTH TRIANGLE MODE
  // There will be the same number of vertices as in triangle mode, calculated simply here
  numVerticesSmooth = (resolutionWidth - 1) * (resolutionHeight - 1) * 6;

  // Vertex positions and colors
  float *centerSmooth = (float *)malloc(numVerticesSmooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *leftSmooth = (float *)malloc(numVerticesSmooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *rightSmooth = (float *)malloc(numVerticesSmooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *downSmooth = (float *)malloc(numVerticesSmooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *upSmooth = (float *)malloc(numVerticesSmooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float *colorsSmooth = (float *)malloc(numVerticesSmooth * 4 * sizeof(float));    // 4 floats per vertex, i.e., r,g,b,a
  
  // Reset indexes to iterate through all the directionSmooth and colorsSmooth
  indexPos = 0;
  indexCol = 0;

  // Loop through entire image except the last pixel (will not start a triangle from boundary of image)
  for (int i = 0; i < resolutionWidth - 1; i++)
  {
    for (int j = 0; j < resolutionHeight - 1; j++)
    {
      // Points for the square: (i, j), (i+1, j), (i, j+1), (i+1, j+1)
      // Create helper arrays to store algebraic coordinates of each triangle present in a square
      // Triangle 1: (i, j) -> (i+1, j) -> (i, j+1)
      int pointsT1[3][2] = {{i, j}, {i + 1, j}, {i, j + 1}};

      // Triangle 2: (i+1, j) -> (i+1, j+1) -> (i, j+1)
      int pointsT2[3][2] = {{i + 1, j}, {i + 1, j + 1}, {i, j + 1}};

      // Loop three times in order to generate each vertex of Triangle 1 of a square begining at (i,j)
      for (int k = 0; k < 3; k++)
      {
        // Same generation of x coordinate as in triangle mode for the center coordinate
        float x = (1.0f * pointsT1[k][0]) / (resolutionWidth - 1);
        // Apply modifications to 'i' variable in order to account for the left and right vertices
        float leftX = (1.0f * pointsT1[k][0] - 1) / (resolutionWidth - 1);
        float rightX = (1.0f * pointsT1[k][0] + 1) / (resolutionWidth - 1);
        // The 'i' variable (and thus the x coordinate) of the up and down vertices remains the same
        float upX = x;
        float downX = x;

        // Considers if the x coordinate of left or right vertices are beyond image boundaries, clamps them at center x coordinate
        if (leftX < 0){
          leftX = x;
        }
        if (rightX > resolutionWidth){
          rightX = x;
        }

        // Same generation of z coordinate as in triangle mode for the center coordinate
        float z = (1.0f * pointsT1[k][1]) / (resolutionHeight - 1);
        // The 'j' variable (and thus the z coordinate) of the left and right vertices remains the same
        float leftZ = z;
        float rightZ = z;
        // Apply modifications to 'j' variable in order to account for the up and down vertices
        float upZ = (1.0f * pointsT1[k][1] - 1) / (resolutionHeight - 1);
        float downZ = (1.0f * pointsT1[k][1] + 1) / (resolutionHeight - 1);

        // Considers if the z coordinate of up or down vertices are beyond image boundaries, clamps them at center z coordinate
        if (upZ < 0){
          upZ = z;
        }
        if (downZ > resolutionHeight){
          downZ = z;
        }

        // Same generation of y coordinate as in triangle mode for the center coordinate
        float y = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1], 0)) / 255.0f;
        // Initializes all other points (left, right, up, down) to be clamped at the height of the center coordinate
        float leftY = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1], 0)) / 255.0f;
        float rightY = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1], 0)) / 255.0f;
        float upY = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1], 0)) / 255.0f;
        float downY = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1], 0)) / 255.0f;
        // Considers if the x coordinate left point has not been clamped
        if (pointsT1[k][0]-1 > 0){
          // Then, recalculates height based on the unclamped left x coordinate
          leftY = (1.0f * heightmapImage->getPixel(pointsT1[k][0]-1, pointsT1[k][1], 0)) / 255.0f;
        }
        // Considers if the x coordinate of the right point has not been clamped
        if (pointsT1[k][0]+1 < resolutionWidth){
          // Then, recalculates height based on the unclamped right x coordinate
          rightY = (1.0f * heightmapImage->getPixel(pointsT1[k][0]+1, pointsT1[k][1], 0)) / 255.0f;
        }
        // Considers if the z coordinate of the up point has not been clamped
        if (pointsT1[k][1]-1 > 0){
          // Then, recalculates height based on the unclamped up z coordinate
          upY = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1]-1, 0)) / 255.0f;
        }
        // Considers if the z coordinate of the down point has not been clamped
        if (pointsT1[k][1]+1 < resolutionHeight){
          // Then, recalculates height based on the unclamped down z coordinate
          downY = (1.0f * heightmapImage->getPixel(pointsT1[k][0], pointsT1[k][1]+1, 0)) / 255.0f;
        }
        
        // Stores smooth triangle positions in the appropriate smooth vertices, incrementing index variable appropriately
        centerSmooth[indexPos] = x;  
        leftSmooth[indexPos] = leftX;
        rightSmooth[indexPos] = rightX;
        upSmooth[indexPos] = upX;
        downSmooth[indexPos] = downX;
        indexPos++;     
        
        centerSmooth[indexPos] = y;
        leftSmooth[indexPos] = leftY;
        rightSmooth[indexPos] = rightY;
        upSmooth[indexPos] = upY;
        downSmooth[indexPos] = downY;
        indexPos++; 

        centerSmooth[indexPos] = z;
        leftSmooth[indexPos] = leftZ;
        rightSmooth[indexPos] = rightZ;
        upSmooth[indexPos] = upZ;
        downSmooth[indexPos] = downZ;     
        indexPos++;  

        // Use height values as color inputs for vertex
        colorsSmooth[indexCol++] = y;    
        colorsSmooth[indexCol++] = y;    
        colorsSmooth[indexCol++] = y;    
        colorsSmooth[indexCol++] = 1.0f; 
      }

      // Loop three times in order to generate each vertex of Triangle 2 of a square begining at (i,j)
      // Contents of loop are the same as the first vertex, except referring to the reference array of Triangle 2 instead of Triangle 1
      for (int k = 0; k < 3; k++)
      {
        float x = (1.0f * pointsT2[k][0]) / (resolutionWidth - 1);
        float leftX = (1.0f * pointsT2[k][0] - 1) / (resolutionWidth - 1);
        float rightX = (1.0f * pointsT2[k][0] + 1) / (resolutionWidth - 1);
        float upX = x;
        float downX = x;

        if (leftX < 0){
          leftX = x;
        }

        if (rightX > resolutionWidth){
          rightX = x;
        }

        float z = (1.0f * pointsT2[k][1]) / (resolutionHeight - 1);
        float leftZ = z;
        float rightZ = z;
        float upZ = (1.0f * pointsT2[k][1] - 1) / (resolutionHeight - 1);
        float downZ = (1.0f * pointsT2[k][1] + 1) / (resolutionHeight - 1);

        if (upZ < 0){
          upZ = z;
        }

        if (downZ > resolutionHeight){
          downZ = z;
        }

        float y = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1], 0)) / 255.0f;
        float leftY = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1], 0)) / 255.0f;
        float rightY = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1], 0)) / 255.0f;
        float upY = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1], 0)) / 255.0f;
        float downY = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1], 0)) / 255.0f;
        if (pointsT2[k][0]-1 > 0){
          leftY = (1.0f * heightmapImage->getPixel(pointsT2[k][0]-1, pointsT2[k][1], 0)) / 255.0f;
        }
        if (pointsT2[k][0]+1 < resolutionWidth){
          rightY = (1.0f * heightmapImage->getPixel(pointsT2[k][0]+1, pointsT2[k][1], 0)) / 255.0f;
        }
        if (pointsT2[k][1]-1 > 0){
          upY = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1]-1, 0)) / 255.0f;
        }
        if (pointsT2[k][1]+1 < resolutionHeight){
          downY = (1.0f * heightmapImage->getPixel(pointsT2[k][0], pointsT2[k][1]+1, 0)) / 255.0f;
        }

        centerSmooth[indexPos] = x;  
        leftSmooth[indexPos] = leftX;
        rightSmooth[indexPos] = rightX;
        upSmooth[indexPos] = upX;
        downSmooth[indexPos] = downX;
        indexPos++;     
        
        centerSmooth[indexPos] = y;
        leftSmooth[indexPos] = leftY;
        rightSmooth[indexPos] = rightY;
        upSmooth[indexPos] = upY;
        downSmooth[indexPos] = downY;
        indexPos++; 

        centerSmooth[indexPos] = z;
        leftSmooth[indexPos] = leftZ;
        rightSmooth[indexPos] = rightZ;
        upSmooth[indexPos] = upZ;
        downSmooth[indexPos] = downZ;     
        indexPos++;       

        colorsSmooth[indexCol++] = y;    
        colorsSmooth[indexCol++] = y;    
        colorsSmooth[indexCol++] = y;    
        colorsSmooth[indexCol++] = 1.0f; 
      }
    }
  }

  // Create the VBOs.
  vboCenterSmooth = new VBO(numVerticesSmooth, 3, centerSmooth, GL_STATIC_DRAW); // 3 values per position
  vboLeftSmooth = new VBO(numVerticesSmooth, 3, leftSmooth, GL_STATIC_DRAW); // 3 values per position
  vboRightSmooth = new VBO(numVerticesSmooth, 3, rightSmooth, GL_STATIC_DRAW); // 3 values per position
  vboUpSmooth = new VBO(numVerticesSmooth, 3, upSmooth, GL_STATIC_DRAW); // 3 values per position
  vboDownSmooth = new VBO(numVerticesSmooth, 3, downSmooth, GL_STATIC_DRAW); // 3 values per position
  vboColorsSmooth = new VBO(numVerticesSmooth, 4, colorsSmooth, GL_STATIC_DRAW);      // 4 values per color

  // Create the VAO.
  vaoSmooth = new VAO();

  // Set up the relationship between the "position" shader variable and the VAO, this represents the center coordinate.
  vaoSmooth->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboCenterSmooth, "position");
  // Set up the relationship between the "left" shader variable and the VAO, this represents the left coordinate.
  vaoSmooth->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboLeftSmooth, "left");
  // Set up the relationship between the "right" shader variable and the VAO, this represents the right coordinate.
  vaoSmooth->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboRightSmooth, "right");
  // Set up the relationship between the "up" shader variable and the VAO, this represents the up coordinate.
  vaoSmooth->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboUpSmooth, "up");
  // Set up the relationship between the "down" shader variable and the VAO, this represents the down coordinate.
  vaoSmooth->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboDownSmooth, "down");

  // Set up the relationship between the "color" shader variable and the VAO.
  vaoSmooth->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsSmooth, "color");

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(centerSmooth);
  free(leftSmooth);
  free(rightSmooth);
  free(upSmooth);
  free(downSmooth);
  free(colorsSmooth);

  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc, argv);

  cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
  glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
  // This is needed on recent Mac OS X versions to correctly display the window.
  glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
  GLint result = glewInit();
  if (result != GLEW_OK)
  {
    cout << "error: " << glewGetErrorString(result) << endl;
    exit(EXIT_FAILURE);
  }
#endif

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}
