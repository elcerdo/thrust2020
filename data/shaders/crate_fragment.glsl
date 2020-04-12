#version 330

in lowp vec4 pos;

out lowp vec4 finalColor;

uniform sampler2D texture;
uniform vec4 baseColor;
uniform int tag;
uniform int maxTag;

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
  int tag_ = tag % maxTag;
  vec2 delta = vec2(tag_ % 8, 7 - tag_ / 8);
  vec4 sampledColor = texture2D(texture, (pos_ + delta) / 8);
  finalColor = sampledColor.a * sampledColor + (1 - sampledColor.a) * baseColor;
}

