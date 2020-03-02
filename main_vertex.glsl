attribute highp vec4 posAttr;
attribute lowp vec4 colAttr;
varying lowp vec4 col;
varying lowp vec4 pos;
uniform highp mat4 matrix;

void main()
{
   col = colAttr;
   pos = posAttr;
   gl_Position = matrix * posAttr;
}

