#version 330

in vec4 posAttr;

out vec4 pos;

uniform mat4 cameraMatrix;
uniform mat4 worldMatrix;

void main()
{
   pos = posAttr;
   gl_Position = cameraMatrix * worldMatrix * posAttr;
}

