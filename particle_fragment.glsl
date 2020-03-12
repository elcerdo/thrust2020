#version 330 core

in vec4 pos;
in vec4 col;
uniform vec4 dotColor;
uniform float radius;
uniform float radiusFactor;
uniform int mode;

out vec4 finalColor;

void main()
{
  float dotRadius = radiusFactor * radius;
  bool in_dot = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z < dotRadius * dotRadius;
  if (mode > 2 && !in_dot)
    discard;
  finalColor =
         mode == 0 ? in_dot ? dotColor : col :
         mode == 1 ? col :
         mode == 2 ? dotColor :
         mode == 3 ? col :
         dotColor;
}

