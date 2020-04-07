#version 330

in vec4 posAttr;
in vec4 colAttr;

out vec4 col;

uniform mat4 cameraMatrix;
uniform mat4 worldMatrix;

void main()
{
   col = colAttr;
   gl_Position = cameraMatrix * worldMatrix * posAttr;
}

