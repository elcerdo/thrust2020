#version 330

in highp vec4 posAttr;

out lowp vec4 col;

uniform highp mat4 matrix;

void main()
{
   col = posAttr;
   gl_Position = matrix * posAttr;
}

