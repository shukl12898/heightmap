In order to compile and run:
cd hw1
make
./hw1 heightmap/{INPUT_IMAGE}

Animation created using: OhioPyle-512.jpg
Screenshots in "Screenshots" folder within main directory (assign1_coreOpenGL_starterCode/hw1)

The key bindings of my program are as follows:
* click = left button
* option + click = middle button
* ctrl + click = right button

ROTATE: click and drag with appropriate button
SCALE: hold down shift and drag with appropriate button
TRANSLATE: hold down 't' and drag with appropriate button

'1' Key: Point rendering mode
'2' Key: Wireframe rendering mode
'3' Key: Triangle rendering mode
'4' Key: Smoothened triangle rendering mode

If in "Smoothened triangle" rendering mode:
* 9 key: Double exponent used in SMOOTH TRIANGLE mode
* 0 key: Half exponent used in SMOOTH TRIANGLE mode
* '-' key: Half scale used in SMOOTH TRIANGLE mode
* '+' key: Double scale used in SMOOTH TRIANGLE mode

I modified the following functions in hw1.cpp:
The below descriptions of code in functions does not include all the global variables added in order to ensure all features required were present.
displayFunc()
- Added in functions in order to perform user's modelling on the object
    * matrix.Translate() based on terrainTranslate that is modified by user input
    * matrix.Rotate() based on terrainRotate[0] that is modified by user input around x axis
    * matrix.Rotate() based on terrainRotate[1] that is modified by user input around y axis
    * matrix.Rotate() based on terrainRotate[2] that is modified by user input around z axis
    * matrix.Scale() based on terrainScale taht is modified by user input
- Set defaults for uniform variables in the vertexShader added to allow SMOOTH TRIANGLES
    * Default mode = 0 (NOT SMOOTH)
    * Default scale = 1.0f
    * Default exponent = 1.0f
- Create logic in order to render object depending on render mode
    * GL_POINTS for POINTS mode
    * GL_LINES for LINES mode
    * GL_TRIANGLES for TRIANGLES mode
    * GL_TRIANGLES for SMOOTH_TRIANGLES mode
- Updated uniform variables for SMOOTH TRIANGLES mode
    * mode = 1 (SMOOTH)
    * scale = smoothScale global variable based on user input
    * exponent = smoothExponent global variable based on user input
initScene()
- For each mode, I set up the global variables required for rendering
    * numVertices
    * VBO for vertex positions
    * VBO for colors
    * VAO for connecting the VBOs, shader variables and pipeline program
- Each mode had differing logic for populating the vertex positions
    * POINTS: determined coordinates of each pixel in the image
    * LINES: created horizontal and vertical lines between all pixels in the image, storing the coordinates of start and end points of the line
    * TRIANGLES: created two triangles for each 'square' between pixels in the image, storing the coordinates of all three vertices of the triangle
    * SMOOTH TRIANGLES: followed logic from triangles, while also storing the left, right, up, and down neighbour for each vertex of each triangle. 
idleFunc()
- Added logic in order to incrementally take screenshots of rendered window
- Ensured only 300 screenshots were taken, and that all screenshots were named appropriately according to instructions
keyboardFunc()
- Altered renderState global variable based on the following:
    * '1' key: POINT render mode
    * '2' key: WIREFRAME render mode
    * '3' key: TRIANGLE render mode
    * '4' key: SMOOTH TRIANGLE render mode
    * 't' key: TRANSLATE control state
    * '+' key: Double scale in SMOOTH TRIANGLE mode
    * '-' key: Halve scale in SMOOTH TRIANGLE mode
    * '9' key: Double exponent in SMOOTH TRIANGLE mode
    * '0' key: Halve exponent in SMOOTH TRIANGLE mode

I made the following modifications in vertexShader.glsl:
- Added more 'in' variables:
    * in vec3 left: To store the left vertex for SMOOTH TRIANGLE mode
    * in vec3 right: To store the right vertex for SMOOTH TRIANGLE mode
    * in vec3 up: To store the up vertex for SMOOTH TRIANGLE mode
    * in vec3 down: To store the down vertex for SMOOTH TRIANGLE mode
- Added more uniform variables:
    * uniform int mode: To store whether the shader was to operate in non-smooth or smooth mode
    * uniform float scale: To store the user determined scale for SMOOTH TRIANGLE mode
    * uniform float exponent: To store the user determined exponent for SMOOTH TRIANGLE mode
- Within main()
    * Added an if statement to differentiate logic in the non-smooth and smooth mode
    * In the non-smooth mode:
        - Maintained original functionality of vertexShader.glsl
    * In the smooth mode:
        - Normalized position of vertex by averaging position of center, left, right, up, down vertices
        - Scaled height using scale * pow(position.y, exponent)
        - Set gl_Position using projectionMatrix, modelViewMatrix and newly calculated position
        - Computed the color values using pow(position.y, exponent)
        - Set col = new colour values