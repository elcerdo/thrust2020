attribute highp vec4 posAttr;
varying lowp vec4 col;
uniform highp mat4 matrix;

void main()
{
   col = posAttr;
   gl_Position = matrix * posAttr;
}

