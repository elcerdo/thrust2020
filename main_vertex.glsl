#version 330

in vec4 posAttr;
in lowp vec4 colAttr;

out vec4 col;

uniform highp mat4 matrix;

void main()
{
   col = colAttr;
   gl_Position = matrix * posAttr;
}

