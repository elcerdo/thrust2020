#version 330
#define PI 3.1415926538

in lowp vec4 pos;

out lowp vec4 finalColor;

uniform float circularSpeed;

void main()
{
  float radius = pos.x * pos.x + pos.y * pos.y;
  //if (radius > 1) discard;
  radius = sqrt(radius);


  float angle = atan(pos.y, pos.x) / PI / 2 + .2 * (1 - radius) * circularSpeed / PI / 2;
  angle *= 14;
  angle = angle - floor(angle);

  vec4 foo = radius < .5 ? vec4(1, 1, 1, 1) : angle > .3333 ? vec4(0, 0, 0, 1) : vec4(1, 1, 1, 1);
  finalColor = vec4(1, 0, 0, 1) + 1e-5 * foo;
}

