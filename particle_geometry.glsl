#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 8) out;

uniform mat4 matrix;
uniform float radius;
uniform int poly;

in vec4 colAttr_[];

out vec4 pos;
out vec4 col;

void main() {
  col = colAttr_[0];

  if (poly == 3)
  { // triangle
    float ww = radius / 2;
    const float ss = sqrt(3);

    pos = ww * vec4(-2, -ss, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(2, -ss, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(0, ss, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();
  }
  else if (poly == 2)
  { // square
    float ww = radius;

    pos = ww * vec4(-1, -1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(1, -1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(-1, 1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(1, 1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();
  }
  else if (poly == 1)
  { // hexagon
    float ww = radius / 2.;
    const float ss = sqrt(3);

    pos = ww * vec4(-1, -ss, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(1, -ss, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(-2, 0, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(2, 0, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(-1, ss, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(1, ss, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();
  }
  else
  { // octogon
    float ww = radius / 3.f;

    pos = ww * vec4(-1, -3, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(1, -3, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(-3, -1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(3, -1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(-3, 1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(3, 1, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(-1, 3, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();

    pos = ww * vec4(1, 3, 0, 0);
    gl_Position = matrix * ( gl_in[0].gl_Position + pos );
    EmitVertex();
  }

  EndPrimitive();
}

