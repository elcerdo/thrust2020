attribute highp vec4 posAttr;
varying lowp vec4 pos;
uniform highp mat4 matrix;

void main()
{
  pos = posAttr;
  gl_Position = matrix * posAttr;
}

