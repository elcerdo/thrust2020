#version 330

in lowp vec4 pos;

out lowp vec4 finalColor;

uniform sampler2D texture;

void main()
{
  vec2 pos_ =
    pos.y == -1 ? pos.xz :
    pos.y == 1 ? pos.zx :
    pos.x == -1 ? pos.zy :
    pos.x == 1 ? pos.yz :
    pos.z == -1 ? pos.yx :
    pos.xy;
  pos_ = pos_ / 2 + vec2(.5, .5);
  finalColor = texture2D(texture, pos_);
}

