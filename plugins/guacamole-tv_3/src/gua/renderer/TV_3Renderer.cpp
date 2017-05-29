/******************************************************************************
 * guacamole - delicious VR                                                   *
 *                                                                            *
 * Copyright: (c) 2011-2013 Bauhaus-Universität Weimar                        *
 * Contact:   felix.lauer@uni-weimar.de / simon.schneegans@uni-weimar.de      *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify it    *
 * under the terms of the GNU General Public License as published by the Free *
 * Software Foundation, either version 3 of the License, or (at your option)  *
 * any later version.                                                         *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program. If not, see <http://www.gnu.org/licenses/>.             *
 *                                                                            *
 ******************************************************************************/

// class header
#include <gua/renderer/TV_3Renderer.hpp>
//#include <gua/renderer/TV_3SurfacePass.hpp>
//#include <gua/renderer/TV_3VolumePass.hpp>

// guacamole headers
#include <gua/renderer/TV_3Resource.hpp>
#include <gua/renderer/Pipeline.hpp>
#include <gua/renderer/ShaderProgram.hpp>
#include <gua/renderer/GBuffer.hpp>
#include <gua/renderer/ResourceFactory.hpp>

#include <gua/node/TV_3Node.hpp>
#include <gua/platform.hpp>
#include <gua/guacamole.hpp>
#include <gua/renderer/View.hpp>

#include <gua/databases.hpp>
#include <gua/utils/Logger.hpp>

#include <gua/config.hpp>

#include <gua/renderer/Window.hpp>

#include <scm/gl_core/data_formats.h>
#include <scm/gl_core/data_types.h>
#include <scm/gl_core/texture_objects/texture_image.h>

#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>

#include <scm/gl_core/shader_objects.h>
#include <scm/gl_core/render_device.h>

// external headers
#include <sstream>
#include <fstream>
#include <regex>
#include <list>
#include <boost/assign/list_of.hpp>

namespace gua {

  //////////////////////////////////////////////////////////////////////////////
  TV_3Renderer::TV_3Renderer(gua::RenderContext const& ctx, gua::SubstitutionMap const& substitution_map) : shaders_loaded_(false),
                                 forward_cube_shader_program_(nullptr),
                                 compositing_shader_program_(nullptr),
                                 no_backface_culling_rasterizer_state_(nullptr),
                                 frontface_culling_rasterizer_state_(nullptr),
                                 global_substitution_map_compressed_(substitution_map),
                                 global_substitution_map_uncompressed_(substitution_map) {

                                 global_substitution_map_uncompressed_["gua_tv_3_sampler_3d_type"] = "sampler3D";
                                 global_substitution_map_compressed_["gua_tv_3_sampler_3d_type"] = "usampler3D";
  }


  ///////////////////////////////////////////////////////////////////////////////
  void TV_3Renderer::_create_fbo_resources(gua::RenderContext const& ctx,
                                           scm::math::vec2ui const& render_target_dims) {
    // initialize FBO lazy during runtime
    volume_raycasting_fbo_.reset();

    // attachments
    volume_raycasting_color_result_ = ctx.render_device
      ->create_texture_2d(render_target_dims,
                          scm::gl::FORMAT_RGBA_8,
                          1, 1, 1);

    volume_raycasting_depth_result_ = ctx.render_device
      ->create_texture_2d(render_target_dims,
                          scm::gl::FORMAT_D32F,
                          1, 1, 1);
  }

  ///////////////////////////////////////////////////////////////////////////////
  void TV_3Renderer::_create_gpu_resources(gua::RenderContext const& ctx,
                                           scm::math::vec2ui const& render_target_dims) {
    //invalidation before first write
    previous_frame_count_ = UINT_MAX;

    _create_fbo_resources(ctx, render_target_dims);

    if(!fullscreen_quad_) {
      fullscreen_quad_.reset(new scm::gl::quad_geometry(ctx.render_device, 
                                                scm::math::vec2(-1.0f, -1.0f), scm::math::vec2(1.0f, 1.0f )));
    }

    no_backface_culling_rasterizer_state_ = ctx.render_device->create_rasterizer_state(scm::gl::FILL_SOLID,
                                            scm::gl::CULL_NONE,
                                            scm::gl::ORIENT_CCW,
                                            true);

    frontface_culling_rasterizer_state_ = ctx.render_device->create_rasterizer_state(scm::gl::FILL_SOLID,
                                                                                     scm::gl::CULL_FRONT,
                                                                                     scm::gl::ORIENT_CCW,
                                                                                     true);


    nearest_sampler_state_ = 
      ctx.render_device->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);

    trilin_sampler_state_ = 
      ctx.render_device->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);

    //box_vertex_array_;
    box_vertex_buffer_ = ctx.render_device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW,  36*3 * sizeof(float));

    scm::gl::render_context_ptr main_ctx = ctx.render_device->main_context();

static const float g_vertex_buffer_data[] = {
    0.0f,0.0f,0.0f, // triangle 1 : begin
    0.0f, 1.0f, 1.0f, // triangle 1 : end
    0.0f,0.0f, 1.0f,

    1.0f, 1.0f,0.0f, // triangle 2 : begin
    0.0f, 1.0f,0.0f, // triangle 2 : end
    0.0f,0.0f,0.0f,

    1.0f,0.0f, 1.0f,
    1.0f,0.0f,0.0f,
    0.0f,0.0f,0.0f,

    1.0f, 1.0f,0.0f,
    0.0f,0.0f,0.0f,
    1.0f,0.0f,0.0f,

    0.0f,0.0f,0.0f,
    0.0f, 1.0f,0.0f,
    0.0f, 1.0f, 1.0f,

    1.0f,0.0f, 1.0f,
    0.0f,0.0f,0.0f,
    0.0f,0.0f, 1.0f,

    0.0f, 1.0f, 1.0f,
    1.0f,0.0f, 1.0f,
    0.0f,0.0f, 1.0f,

    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,0.0f,
    1.0f,0.0f,0.0f,

    1.0f,0.0f,0.0f,
    1.0f,0.0f, 1.0f,
    1.0f, 1.0f, 1.0f,


    1.0f, 1.0f, 1.0f,
    0.0f, 1.0f,0.0f,
    1.0f, 1.0f,0.0f,

    1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f,0.0f,


    1.0f, 1.0f, 1.0f,
    1.0f,0.0f, 1.0f,
    0.0f, 1.0f, 1.0f
    
};


    float* mapped_vertex_buffer = static_cast<float*>(main_ctx->map_buffer(box_vertex_buffer_, scm::gl::ACCESS_WRITE_INVALIDATE_BUFFER));

    for(int i = 0; i < 36*3; ++i) {
      mapped_vertex_buffer[i] = g_vertex_buffer_data[i];
    }

    main_ctx->unmap_buffer(box_vertex_buffer_);


    box_vertex_array_ = ctx.render_device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, 3*sizeof(float)),
                                                               boost::assign::list_of(box_vertex_buffer_));

/*
    box_element_buffer_ = ctx.render_device->create_buffer(scm::gl::BIND_INDEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, 14 * sizeof(uint32_t));
    uint32_t* mapped_element_buffer = static_cast<uint32_t*>(main_ctx->map_buffer(box_element_buffer_, scm::gl::ACCESS_WRITE_INVALIDATE_BUFFER));
    mapped_element_buffer[ 0] = 3;
    mapped_element_buffer[ 1] = 2;
    mapped_element_buffer[ 2] = 6;
    mapped_element_buffer[ 3] = 7;
    mapped_element_buffer[ 4] = 4;
    mapped_element_buffer[ 5] = 2;
    mapped_element_buffer[ 6] = 0;
    mapped_element_buffer[ 7] = 3;
    mapped_element_buffer[ 8] = 1;
    mapped_element_buffer[ 9] = 6;
    mapped_element_buffer[10] = 5;
    mapped_element_buffer[11] = 4;
    mapped_element_buffer[12] = 1;
    mapped_element_buffer[13] = 0;

    main_ctx->unmap_buffer(box_element_buffer_);
*/


/*
    resource_ptrs_.attachments_[plod_shared_resources::AttachmentID::ACCUM_PASS_NORMAL_RESULT] = ctx.render_device
      ->create_texture_2d(render_target_dims,
                          scm::gl::FORMAT_RGB_16F,
                          1, 1, 1);

    resource_ptrs_.attachments_[plod_shared_resources::AttachmentID::ACCUM_PASS_PBR_RESULT] = ctx.render_device
      ->create_texture_2d(render_target_dims,
                          scm::gl::FORMAT_RGB_16F,
                          1, 1, 1);

    resource_ptrs_.attachments_[plod_shared_resources::AttachmentID::ACCUM_PASS_WEIGHT_AND_DEPTH_RESULT] = ctx.render_device
      ->create_texture_2d(render_target_dims,
                          scm::gl::FORMAT_RG_32F,
                          1, 1, 1);

    depth_test_without_writing_depth_stencil_state_ = ctx.render_device
      ->create_depth_stencil_state(true, false, scm::gl::COMPARISON_LESS_EQUAL);

    color_accumulation_state_ = ctx.render_device->create_blend_state(true,
                                                                      scm::gl::FUNC_ONE,
                                                                      scm::gl::FUNC_ONE,
                                                                      scm::gl::FUNC_ONE,
                                                                      scm::gl::FUNC_ONE,
                                                                      scm::gl::EQ_FUNC_ADD,
                                                                      scm::gl::EQ_FUNC_ADD);

    no_backface_culling_rasterizer_state_ = ctx.render_device
      ->create_rasterizer_state(scm::gl::FILL_SOLID,
                                scm::gl::CULL_NONE,
                                scm::gl::ORIENT_CCW,
                                false,
                                false,
                                0.0,
                                false,
                                false,
                                scm::gl::point_raster_state(false));

    _register_shared_resources(shared_resources);
*/

  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  void TV_3Renderer::_check_for_resource_updates(gua::Pipeline const& pipe, RenderContext const& ctx) {

    // get current unique view id and resolution
    auto const& camera = pipe.current_viewstate().camera;
    scm::math::vec2ui const& render_target_dims = camera.config.get_resolution();

    // check if resources for this view and resolution are already available
    bool resolution_available = false;

    resolution_available = (current_rendertarget_dims_ == render_target_dims);

    // if not, allocate
    if (!resolution_available) {
      _create_gpu_resources(ctx, render_target_dims);

      current_rendertarget_dims_ = render_target_dims;
    }
  }

  void TV_3Renderer::_clear_fbo_attachments(gua::RenderContext const& ctx) {
    if (!volume_raycasting_fbo_) {
      volume_raycasting_fbo_ = ctx.render_device->create_frame_buffer();
      volume_raycasting_fbo_->clear_attachments();
      volume_raycasting_fbo_
        ->attach_color_buffer(0, volume_raycasting_color_result_);
      volume_raycasting_fbo_
        ->attach_depth_stencil_buffer(volume_raycasting_depth_result_);

    ctx.render_context
    ->set_frame_buffer( volume_raycasting_fbo_ );
    }

    ctx.render_context
      ->clear_color_buffer(volume_raycasting_fbo_,
      0,
      scm::math::vec4f(0.0f, 0.0f, 0.0f, 0.0f));


    ctx.render_context
      ->clear_depth_stencil_buffer(volume_raycasting_fbo_);

    ctx.render_context
      ->set_frame_buffer(volume_raycasting_fbo_);
  }



/*
  /////////////////////////////////////////////////////////////////////////////////////////////
  void TV_3Renderer::_raycasting_pass(gua::Pipeline& pipe, std::vector<gua::node::Node*> const& sorted_nodes, PipelinePassDescription const& desc) {

    RenderContext const& ctx(pipe.get_context());
    auto& scene = *pipe.current_viewstate().scene;

    auto view_matrix       = scene.rendering_frustum.get_view(); 
    auto projection_matrix = scene.rendering_frustum.get_projection();

    auto eye_position = math::vec4(scene.rendering_frustum.get_camera_position(),1.0);

   for (auto const& object : sorted_nodes) {
      auto tv_3_volume_node(reinterpret_cast<node::TV_3Node*>(object));

      auto model_matrix = tv_3_volume_node->get_world_transform();
      auto mvp_matrix = projection_matrix * view_matrix * model_matrix;
      //forward_cube_shader_program_->apply_uniform(ctx, "gua_model_view_projection_matrix", math::mat4f(mvp_matrix));

      auto inv_model_mat = scm::math::inverse(model_matrix);

      math::vec4 model_space_eye_pos = inv_model_mat * eye_position;

      //forward_cube_shader_program_->apply_uniform(ctx, "gua_model_matrix", math::mat4f(tv_3_volume_node->get_world_transform()) ) ;
      
      forward_cube_shader_program_->use(ctx);
      forward_cube_shader_program_->apply_uniform(ctx, "gua_model_view_projection_matrix", math::mat4f(mvp_matrix));
      forward_cube_shader_program_->apply_uniform(ctx, "ms_eye_pos", math::vec4f(model_space_eye_pos/model_space_eye_pos[3]));
      forward_cube_shader_program_->apply_uniform(ctx, "volume_texture", 0);

      ctx.render_context->bind_vertex_array(box_vertex_array_ );
      //ctx.render_context->bind_index_buffer(box_element_buffer_, scm::gl::PRIMITIVE_TRIANGLE_LIST, scm::gl::TYPE_UINT);
 

      ctx.render_context->set_rasterizer_state(frontface_culling_rasterizer_state_);


     // ctx.render_context->uniform_sampler3D("volume_texture", 0);
      tv_3_volume_node->get_geometry()->bind_volume_texture(ctx, trilin_sampler_state_);
      ctx.render_context->apply();

  
      ctx.render_context->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST, 0, 36*6);

    }
}

  /////////////////////////////////////////////////////////////////////////////////////////////
  void TV_3Renderer::_postprocessing_pass(gua::Pipeline& pipe) {
    RenderContext const& ctx(pipe.get_context());

    auto& target = *pipe.current_viewstate().target;
    bool write_depth = true;
    target.bind(ctx, write_depth);
    target.set_viewport(ctx);

    if(compositing_shader_program_ != nullptr) {
      compositing_shader_program_->use(ctx);
    }
    ctx.render_context->bind_texture(volume_raycasting_color_result_, trilin_sampler_state_, 0);

    compositing_shader_program_->apply_uniform(ctx, "blit_texture", 0);

    auto const& glapi = ctx.render_context->opengl_api();
    ctx.render_context->set_rasterizer_state(no_backface_culling_rasterizer_state_);
    ctx.render_context->apply();
    fullscreen_quad_->draw(ctx.render_context);
  }
*/

  ///////////////////////////////////////////////////////////////////////////////
  void TV_3Renderer::render(gua::Pipeline& pipe, PipelinePassDescription const& desc) {

    RenderContext const& ctx(pipe.get_context());

    ///////////////////////////////////////////////////////////////////////////
    //  retrieve current view state
    ///////////////////////////////////////////////////////////////////////////
    auto& scene = *pipe.current_viewstate().scene;
    auto const& camera = pipe.current_viewstate().camera;
    auto const& frustum = pipe.current_viewstate().frustum;
    auto& target = *pipe.current_viewstate().target;

    std::string cpu_query_name_plod_total = "CPU: Camera uuid: " + std::to_string(pipe.current_viewstate().viewpoint_uuid) + " / LodPass";
    pipe.begin_cpu_query(cpu_query_name_plod_total);
    
    ///////////////////////////////////////////////////////////////////////////
    //  sort nodes 
    ///////////////////////////////////////////////////////////////////////////
    auto sorted_objects(scene.nodes.find(std::type_index(typeid(node::TV_3Node))));

    if (sorted_objects == scene.nodes.end() || sorted_objects->second.empty()) {
      return; // return if no nodes in scene
    }

    std::sort(sorted_objects->second.begin(), sorted_objects->second.end(), [](node::Node* a, node::Node* b) {
      return reinterpret_cast<node::TV_3Node*>(a)->get_material()->get_shader() < reinterpret_cast<node::TV_3Node*>(b)->get_material()->get_shader();
    });


    ///////////////////////////////////////////////////////////////////////////
    // resource initialization
    ///////////////////////////////////////////////////////////////////////////
    scm::math::vec2ui const& render_target_dims = camera.config.get_resolution();
    _check_for_resource_updates(pipe, ctx);

    _clear_fbo_attachments(ctx);
    //target.bind(ctx, write_depth);
    //target.set_viewport(ctx);

    int view_id(camera.config.get_view_id());

    if(!shaders_loaded_) {
      _load_shaders();
    }

    MaterialShader*                current_material(nullptr);
    std::shared_ptr<ShaderProgram> current_shader;
    //auto current_rasterizer_state = rs_cull_back_;



    ctx.render_context->reset_state_objects();




    if(forward_cube_shader_program_ != nullptr) {
      forward_cube_shader_program_->use(ctx);
    }

    auto view_matrix       = scene.rendering_frustum.get_view(); 
    auto projection_matrix = scene.rendering_frustum.get_projection();

    auto vp_matrix = projection_matrix * view_matrix;
    // loop through all objects, sorted by material ----------------------------

    auto eye_position = math::vec4(frustum.get_camera_position(),1.0);


    ///////////////////////////////////////////////////////////////////////////
    // actual volume rendering pass
    ///////////////////////////////////////////////////////////////////////////
    _raycasting_pass(pipe, sorted_objects->second, desc);

    _postprocessing_pass(pipe, desc);

    pipe.end_cpu_query(cpu_query_name_plod_total); 
    
    //dispatch cut updates
    if (previous_frame_count_ != ctx.framecount) {
      previous_frame_count_ = ctx.framecount;
    }
  } 

}