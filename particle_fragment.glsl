#version 330 core

in vec4 pos;
in float speed;
in vec4 col;
flat in uint flag;
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
  bool is_stuck = flag != 0u; //% 2u != 0u;
/*
  finalColor = is_stuck ? vec4(1, 0, 0, 1) : vec4(0, 1, 0, 1);
  finalColor += 1e-9 * col;
  finalColor += 1e-9 * waterColor;
  finalColor += 1e-9 * foamColor;
  finalColor.b += 1e-9 * (speed - maxSpeed + alpha);
*/
  finalColor = is_stuck ? vec4(1, 0, 0, 1) :
         mode == 0 ? in_dot ? waterColor : col :
         mode == 1 ? col :
         mode == 2 ? waterColor :
         mode == 3 ? col :
         mode == 4 ? waterColor :
         mix(waterColor, foamColor, pow(clamp(speed / maxSpeed, 0, 1), alpha));
}

