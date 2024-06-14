#version 150

in vec3 position;
in vec3 left;
in vec3 right;
in vec3 up;
in vec3 down;
in vec4 color;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform int mode;
uniform float scale;
uniform float exponent;

void main()
{
  // non-smooth mode
  if (mode == 0){
    // compute the transformed and projected vertex position (into gl_Position) 
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    // compute the vertex color (into col)
    col = color;
  } 
  // smooth mode
  else {
    // normalize position of vertex using all in components
    vec3 actualPosition = (position + left + right + down + up) / 5;
    // uses uniform scale and exponent variables to determine height
    actualPosition.y = scale * pow(actualPosition.y, exponent);
    // computes the transformed and projected vertex position (into gl_Position)
    gl_Position = projectionMatrix * modelViewMatrix * vec4(actualPosition, 1.0f);
    // computes the color values using scale and exponent uniform variables
    float intensity = pow(position.y, exponent);
    // compute the vertex color based on above (into col)
    col = vec4(intensity, intensity, intensity, 1.0f);
  }
  
}

