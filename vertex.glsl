attribute highp vec4 posAttr;
//attribute lowp vec4 colAttr;
varying lowp vec4 col;
uniform highp mat4 matrix;

void main()
{
   col = vec4(1, 1, 0, 1);
   gl_Position = matrix * posAttr;
}

