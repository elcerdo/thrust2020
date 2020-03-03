#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 8) out;

uniform mat4 matrix;

in vec4 colAttr_[];

out vec4 pos;
out vec4 col;

void main() {
  col = colAttr_[0];

  float ww = 1.f / 3.f;

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

  EndPrimitive();
}

