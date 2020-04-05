#version 330 core

in vec2 posAttr;
in vec2 speedAttr;
in uint flagAttr;
in vec4 colAttr;

out float speedAmpl_;
flat out uint flagAttr_;
out vec4 colAttr_;

void main()
{
  speedAmpl_ = length(speedAttr);
  colAttr_ = colAttr;
  flagAttr_ = flagAttr;
  gl_Position = vec4(posAttr, 0, 1);
}

