#version 330 core

in vec4 pos;
in float speed;
in vec4 col;
uniform vec4 waterColor;
uniform vec4 foamColor;
uniform float radius;
uniform float radiusFactor;
uniform int mode;
uniform float maxSpeed;
uniform float alpha;

out vec4 finalColor;

void main()
{
  float dotRadius = radiusFactor * radius;
  bool in_dot = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z < dotRadius * dotRadius;
  if (mode > 2 && !in_dot)
    discard;
  finalColor =
         mode == 0 ? in_dot ? waterColor : col :
         mode == 1 ? col :
         mode == 2 ? waterColor :
         mode == 3 ? col :
         mode == 4 ? waterColor :
         mix(waterColor, foamColor, pow(clamp(speed / maxSpeed, 0, 1), alpha));
}

