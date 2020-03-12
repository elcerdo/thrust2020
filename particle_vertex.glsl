#version 330 core

in vec2 posAttr;
in vec2 speedAttr;
in vec4 colAttr;

out float speedAmpl_;
out vec4 colAttr_;

void main()
{
  speedAmpl_ = length(speedAttr);
  colAttr_ = colAttr / 255.f;
  gl_Position = vec4(posAttr, 0, 1);
}

