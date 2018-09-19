@include "common/header.glsl"

// varying input
in vec2 gua_quad_coords;
// gbuffer input
@include "common/gua_camera_uniforms.glsl"
@include "common/gua_gbuffer_input.glsl"

@include "common/gua_fragment_shader_input.glsl"

layout (location = 0) out vec3 out_accumulated_color;


void main() {

  //gua_out_color = gua_get_color();
  gua_out_color = vec3(1.0, 0.0, 0.0);
  //submit_fragment(gl_FragCoord.z);
}