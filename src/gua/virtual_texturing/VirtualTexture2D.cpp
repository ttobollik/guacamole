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
#include <gua/virtual_texturing/VirtualTexture2D.hpp>

// guacamole headers
#include <gua/platform.hpp>
#include <gua/utils/Logger.hpp>
#include <gua/math/math.hpp>

// external headers
#include <boost/shared_ptr.hpp>
#include <scm/gl_core/data_formats.h>
#include <scm/gl_core/log.h>
#include <scm/gl_core/render_device.h>
#include <scm/gl_core/texture_objects.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/data/imaging/texture_image_data.h>

#include <iostream>
#include <fstream>
#include <regex>

namespace gua {
namespace virtual_texturing {

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}



  VirtualTexture2D::VirtualTexture2D(std::string const& atlas_filename,
                                     std::size_t physical_texture_tile_slot_size,
                                     scm::gl::sampler_state_desc const& state_descripton) {

    std::string const ini_filename = std::regex_replace(atlas_filename, std::regex(".atlas"), ".ini");

    std::size_t tile_size = 0;
    std::string line_buffer{""};

    std::ifstream ini_filestream(ini_filename, std::ios::in);

    while(std::getline(ini_filestream, line_buffer)) {
      trim(line_buffer);
      if(0 == line_buffer.find("TILE_SIZE=")) {
        tile_size = std::stoi(line_buffer.substr(10));
      }
    }
    ini_filestream.close();

    if(physical_texture_tile_slot_size == tile_size) {
      std::cout << "Tile size compatible\n";
    } else {
      std::cout << "Tile size INcompatible\n";
      //gua::Logger << "Warning: Ignoring VT because of unmatching tile size\n";
    }
  }


  void VirtualTexture2D::upload_to(RenderContext const& context) const {

  }

  void VirtualTexture2D::initialize_index_texture(RenderContext const& ctx, uint64_t cut_id) const {

  }

}
}