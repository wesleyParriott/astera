#version 330
in vec2 pass_texcoords;

uniform vec2 screen_size = vec2(1280, 720);
uniform vec2 offset;
uniform sampler2D screen_tex;
uniform float inner_radius = 0.3, outter_radius = 0.4;
uniform float intensity = 0.85;

out vec4 out_color;

void main(){
  vec4 sample_color = texture(screen_tex, pass_texcoords);

  vec2 uv = gl_FragCoord.xy / screen_size;
  uv += offset;
  uv.x *= screen_size.x / screen_size.y;
  float len = length(uv);
  float vig = smoothstep(outter_radius, inner_radius, len);
  vec4 color = vec4(mix(sample_color.rgb, sample_color.rgb * vig, intensity), 1);

  out_color = sample_color;
}
