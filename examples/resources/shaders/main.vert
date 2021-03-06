#version 330
#define MAX_BATCH_SIZE 512

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_texc;

uniform mat4 projection;
uniform mat4 view;

uniform mat4 mats[MAX_BATCH_SIZE];
uniform int  flip_x[MAX_BATCH_SIZE];
uniform int  flip_y[MAX_BATCH_SIZE];
uniform vec4 coords[MAX_BATCH_SIZE];
uniform vec4 colors[MAX_BATCH_SIZE];

out vec2 pass_texcoord;
out vec4 pass_color;

void main() {
  vec2 mod_coord = in_texc;
  vec4 raw_coord = coords[gl_InstanceID];

  if (flip_x[gl_InstanceID] == 1) {
    mod_coord.x = 1.0 - mod_coord.x;
  }

  if (flip_y[gl_InstanceID] == 1) {
    mod_coord.y = 1.0 - mod_coord.y;
  }

  // drop 0.5 down to -0.01 (force OpenGL to round down from edges)
  vec2 fix = vec2(sign(mod_coord.x - 0.51) * -0.005, sign(mod_coord.y - 0.51) * -0.005);

  //vec2 tex_size = raw_coord.zw - raw_coord.xy;
  vec2 tex_size = vec2(raw_coord.w - raw_coord.y, raw_coord.z - raw_coord.x);
  //tex_size += fix;

  vec2 offset = raw_coord.xy;
  //offset += fix;

  pass_texcoord = offset + (tex_size *  mod_coord);
  pass_color = colors[gl_InstanceID];

  gl_Position = projection * view * mats[gl_InstanceID] * vec4(in_pos, 1.0f);
}
