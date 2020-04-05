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
uniform float mixColor;
uniform vec4 viscousColor;
uniform vec4 tensibleColor;

out vec4 finalColor;

const uint stuck_flag = 1u << 31;
const uint spring_flag = 1u << 3;
const uint elastic_flag = 1u << 4;
const uint viscous_flag = 1u << 5;
const uint powder_flag = 1u << 6;
const uint tensible_flag = 1u << 7;
const uint repulsive_flag = 1u << 13;

bool is_flag_set(uint value, const uint flag)
{
  return (value & flag) != 0u;
}

void main()
{
  float dotRadius = radiusFactor * radius;
  bool in_dot = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z < dotRadius * dotRadius;
  if (mode > 3 && !in_dot)
    discard;
  vec4 speed_color = mix(waterColor, foamColor, pow(clamp(speed / maxSpeed, 0, 1), pow(5., alpha)));
  bool is_stuck = is_flag_set(flag, stuck_flag);
  vec4 stuck_color = is_stuck ? vec4(1, 0, 0, 1) : vec4(0, 1, 0, 1);
  bool is_springy = is_flag_set(flag, spring_flag) || is_flag_set(flag, elastic_flag);
  bool is_powdery = is_flag_set(flag, powder_flag);
  bool is_repulsive = is_flag_set(flag, repulsive_flag);
  vec4 flag_color =
    is_springy ? vec4(1, 0, 1, 1) :
    is_powdery ? vec4(1, 1, 0, 1) :
    is_repulsive ? vec4(0, 1, 1, 1) :
    vec4(0, 0, 1, 1);
  bool is_viscous = is_flag_set(flag, viscous_flag);
  bool is_tensible = is_flag_set(flag, tensible_flag);
  if (is_viscous && !is_tensible) flag_color = mix(flag_color, viscousColor, mixColor);
  if (!is_viscous && is_tensible) flag_color = mix(flag_color, tensibleColor, mixColor);
  if (is_viscous && is_tensible) flag_color = mix(flag_color, mix(viscousColor, tensibleColor, .5), mixColor);
/*
  finalColor = is_stuck ? vec4(1, 0, 0, 1) : vec4(0, 1, 0, 1);
  finalColor += 1e-9 * col;
  finalColor += 1e-9 * waterColor;
  finalColor += 1e-9 * foamColor;
  finalColor.b += 1e-9 * (speed - maxSpeed + alpha);
*/
  finalColor =
    // full
    mode == 0 ? in_dot ? speed_color : col :
    mode == 1 ? in_dot ? flag_color : speed_color :
    mode == 2 ? in_dot ? speed_color : flag_color :
    mode == 3 ? in_dot ? col : flag_color:
    // dot only
    mode == 4 ? col :
    mode == 5 ? waterColor :
    mode == 6 ? stuck_color :
    mode == 7 ? flag_color :
    speed_color;
}

