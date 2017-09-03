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

#ifndef GUA_LINE_STRIP_IMPORTER_HPP
#define GUA_LINE_STRIP_IMPORTER_HPP

// guacamole headers
#include <gua/config.hpp>
#include <gua/platform.hpp>

// external headers
#include <scm/gl_core.h>
#include <scm/core/math/quat.h>

#include <vector>


namespace gua {


class TransformNode;
/**
 * @brief holds vertex information of one mesh
 */
class GUA_DLL LineStripImporter {

  friend class LineStrip;
  
  public:

    bool parsing_successful() const;	
    
    void read_file(std::string const& file_name);

  private:

  	bool parsing_successful_ = false;

	using IndexTriplet = std::tuple<int,int,int>;

  	struct LineObject {
  	  std::vector<scm::math::vec3f> vertex_position_database;
  	  std::vector<scm::math::vec4f> vertex_color_database;
  	  std::vector<float> vertex_thickness_database;

  	  //not used in the first version of the importer
  	  std::vector<IndexTriplet> vertex_attribute_ids;
  	};



	using NamedLineObject = std::pair<std::string, LineObject>;
  	std::vector<NamedLineObject> parsed_line_objects_;



};


}

#endif //GUA_MESH_HPP
