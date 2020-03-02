varying lowp vec4 pos;
varying lowp vec4 col;
uniform highp vec4 dotColor;

void main()
{
  const float radius = 1.1;
  bool is_dot = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z < radius * radius;
  gl_FragColor = is_dot ? dotColor : col;
}

