#version 330

in lowp vec4 pos;

out lowp vec4 finalColor;

uniform float time;
uniform vec4 haloOuterColor;
uniform vec4 haloInnerColor;

void main()
{
  float radius = pos.x * pos.x + pos.y * pos.y;
  radius = sqrt(radius);
  const float freq = 60;
  float radius_min = .6 - .05 * sin(freq * time);
  float radius_max = .9 + .05 * sin(freq * time);
  if (radius > radius_max) discard;
  finalColor = radius >= radius_min ? haloOuterColor : haloInnerColor;
}

