varying lowp vec4 pos;
uniform highp vec4 color;
uniform highp float radius;

void main()
{
  bool in_dot = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z < radius * radius;
  gl_FragColor = in_dot ? vec4(1, 1, 1, 1) : color;
}

