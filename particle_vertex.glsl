#version 330 core

in vec4 posAttr;
in vec4 colAttr;

out vec4 colAttr_;

void main()
{
  colAttr_ = colAttr / 255.f;
  gl_Position = posAttr;
}

