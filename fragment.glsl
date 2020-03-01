varying lowp vec4 pos;
varying lowp vec4 col;

void main()
{
  bool is_dot = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z < 1.f * 1.f;
  gl_FragColor = is_dot ? vec4(1, 1, 1, 1) : col;
}

