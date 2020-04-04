#version 330

in highp vec4 posAttr;

out lowp vec4 pos;

uniform highp mat4 matrix;

void main()
{
   pos = posAttr;
   gl_Position = matrix * posAttr;
}

