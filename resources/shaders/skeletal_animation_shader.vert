@include "shaders/common/header.glsl"

layout(location=0) in vec3 gua_in_position;
layout(location=1) in vec2 gua_in_texcoords;
layout(location=2) in vec3 gua_in_normal;
layout(location=3) in vec3 gua_in_tangent;
layout(location=4) in vec3 gua_in_bitangent;
layout(location=5) in vec4 gua_bone_weights;
layout(location=6) in ivec4 gua_bone_ids;

@include "shaders/common/gua_camera_uniforms.glsl"

@material_uniforms

@include "shaders/common/gua_vertex_shader_output.glsl"

@include "shaders/common/gua_global_variable_declaration.glsl"

@material_method_declarations

void main() {

  @material_input

  gua_position  = (gua_model_matrix * vec4(gua_in_position, 1.0)).xyz;
  gua_normal    = (gua_normal_matrix * vec4(gua_in_normal, 0.0)).xyz;
  gua_tangent   = (gua_normal_matrix * vec4(gua_in_tangent, 0.0)).xyz;
  gua_bitangent = (gua_normal_matrix * vec4(gua_in_bitangent, 0.0)).xyz;
  gua_texcoords = gua_in_texcoords;
  gua_specularity = 0;
  gua_shinyness   = 50;
  gua_emissivity  = 0;

  @material_method_calls

  @include "shaders/common/gua_varyings_assignment.glsl"

  gl_Position = gua_projection_matrix * gua_view_matrix * vec4(gua_position, 1.0);
}
