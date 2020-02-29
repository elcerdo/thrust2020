varying lowp vec4 pos;
varying lowp vec4 col;

void main()
{
  bool is_dot = pos.x * pos.x + pos.y * pos.y < .1f * .1f;
  gl_FragColor = is_dot ? vec4(1, 1, 1, 1) : col;
}

