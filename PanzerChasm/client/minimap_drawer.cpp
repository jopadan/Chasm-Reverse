#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "../game_constants.hpp"
#include "../map_loader.hpp"

#include "minimap_drawer.hpp"

namespace PanzerChasm
{

static const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };

static const r_OGLState g_gl_state(
	true, false, false, false,
	g_gl_state_blend_func );

static const unsigned int g_arrow_lines= 3u;
static const unsigned int g_arrow_vertices= g_arrow_lines * 2u;
static const unsigned int g_framing_lines= 4u;
static const unsigned int g_framing_vertices= g_framing_lines * 2u;

static unsigned int CalculateMinimapScale( const Size2& viewport_size )
{
	float scale_f=
		0.65f * std::min(
			float( viewport_size.Width () ) / float( GameConstants::min_screen_width  ),
			float( viewport_size.Height() ) / float( GameConstants::min_screen_height ) );

	return std::max( 1u, static_cast<unsigned int>( scale_f ) );
}

MinimapDrawer::MinimapDrawer(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContext& rendering_context )
	: rendering_context_(rendering_context)
{
	lines_shader_.ShaderSource(
		rLoadShader( "minimap_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "minimap_v.glsl", rendering_context.glsl_version ) );
	lines_shader_.SetAttribLocation( "pos", 0 );
	lines_shader_.Create();
}

MinimapDrawer::~MinimapDrawer()
{
}

void MinimapDrawer::SetMap( const MapDataConstPtr& map_data )
{
	if( current_map_data_ == map_data )
		return;

	current_map_data_= map_data;
	if( map_data == nullptr )
		return;

	std::vector<WallLineVertex> vertices;
	vertices.resize(
		( current_map_data_->static_walls.size() + current_map_data_->dynamic_walls.size() ) * 2u +
		g_arrow_vertices +
		g_framing_vertices );

	first_static_walls_vertex_= 0u * 2u;
	first_dynamic_walls_vertex_= current_map_data_->static_walls.size() * 2u;
	arrow_vertices_offset_= first_dynamic_walls_vertex_ + current_map_data_->dynamic_walls.size() * 2u;
	framing_vertices_offset_= arrow_vertices_offset_ + g_arrow_vertices;

	for( unsigned int w= 0u; w < current_map_data_->static_walls.size(); w++ )
	{
		const MapData::Wall& in_wall= current_map_data_->static_walls[w];
		vertices[ first_static_walls_vertex_ + w * 2u + 0u ]= in_wall.vert_pos[0];
		vertices[ first_static_walls_vertex_ + w * 2u + 1u ]= in_wall.vert_pos[1];
	}

	for( unsigned int w= 0u; w < current_map_data_->dynamic_walls.size(); w++ )
	{
		const MapData::Wall& in_wall= current_map_data_->dynamic_walls[w];
		vertices[ first_dynamic_walls_vertex_ + w * 2u + 0u ]= in_wall.vert_pos[0];
		vertices[ first_dynamic_walls_vertex_ + w * 2u + 1u ]= in_wall.vert_pos[1];
	}

	// Arrow
	vertices[ arrow_vertices_offset_ + 0u + 0u ].x= -0.5f;
	vertices[ arrow_vertices_offset_ + 0u + 0u ].y= -1.0f;
	vertices[ arrow_vertices_offset_ + 0u + 1u ].x= +0.5f;
	vertices[ arrow_vertices_offset_ + 0u + 1u ].y= -1.0f;

	vertices[ arrow_vertices_offset_ + 2u + 0u ].x= -0.5f;
	vertices[ arrow_vertices_offset_ + 2u + 0u ].y= -1.0f;
	vertices[ arrow_vertices_offset_ + 2u + 1u ].x= +0.0f;
	vertices[ arrow_vertices_offset_ + 2u + 1u ].y= +1.0f;

	vertices[ arrow_vertices_offset_ + 4u + 0u ].x= +0.5f;
	vertices[ arrow_vertices_offset_ + 4u + 0u ].y= -1.0f;
	vertices[ arrow_vertices_offset_ + 4u + 1u ].x= +0.0f;
	vertices[ arrow_vertices_offset_ + 4u + 1u ].y= +1.0f;

	// Framing
	// Bottom
	vertices[ framing_vertices_offset_ + 0u + 0u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 0u + 0u ].y= -1.0f;
	vertices[ framing_vertices_offset_ + 0u + 1u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 0u + 1u ].y= -1.0f;
	// Left
	vertices[ framing_vertices_offset_ + 2u + 0u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 2u + 0u ].y= -1.0f;
	vertices[ framing_vertices_offset_ + 2u + 1u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 2u + 1u ].y= +1.0f;
	// Right
	vertices[ framing_vertices_offset_ + 4u + 0u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 4u + 0u ].y= -1.0f;
	vertices[ framing_vertices_offset_ + 4u + 1u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 4u + 1u ].y= +1.0f;
	// Top
	vertices[ framing_vertices_offset_ + 6u + 0u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 6u + 0u ].y= +1.0f;
	vertices[ framing_vertices_offset_ + 6u + 1u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 6u + 1u ].y= +1.0f;

	// Move to GPU
	walls_buffer_.VertexData( vertices.data(), vertices.size() * sizeof(WallLineVertex), sizeof(WallLineVertex) );
	walls_buffer_.VertexAttribPointer( 0, 2, GL_FLOAT, false, 0 );
	walls_buffer_.SetPrimitiveType( GL_LINES );
}

void MinimapDrawer::Draw( const m_Vec2& camera_position, const float view_angle )
{
	const float c_left_offset_px= 16.0f;
	const float c_top_offset_px= 16.0f;
	const float c_bottom_offset_rel= 1.0f / 3.0f;
	const float c_righ_offset_from_center_px= 32.0f;

	const float scale= static_cast<float>( CalculateMinimapScale( rendering_context_.viewport_size ) );

	const unsigned int top= static_cast<unsigned int>( c_top_offset_px * scale );
	const unsigned int left= static_cast<unsigned int>( c_left_offset_px * scale );
	const unsigned int right= rendering_context_.viewport_size.Width() / 2u - static_cast<unsigned int>( c_righ_offset_from_center_px * scale );
	const unsigned int bottom= static_cast<unsigned int>( ( 1.0f - c_bottom_offset_rel ) * static_cast<float>( rendering_context_.viewport_size.Height() ) );
	const unsigned int map_viewport_width = right - left;
	const unsigned int map_viewport_height= bottom - top;

	r_OGLStateManager::UpdateState( g_gl_state );

	lines_shader_.Bind();

	m_Mat4 shift_matrix, scale_matrix, viewport_scale_matrix, viewport_shift_matrix;

	shift_matrix.Translate( m_Vec3( -camera_position, 0.0f ) );

	const float c_base_scale= 1.0f / 32.0f;
	const float aspect= float(rendering_context_.viewport_size.Height()) / float(rendering_context_.viewport_size.Width());
	scale_matrix.Scale( m_Vec3( aspect * c_base_scale, c_base_scale, 1.0f ) );

	const float map_viewport_scale= float(map_viewport_height) / float(rendering_context_.viewport_size.Height());
	viewport_scale_matrix.Scale( m_Vec3( map_viewport_scale, map_viewport_scale, 1.0f ) );

	viewport_shift_matrix.Translate(
		m_Vec3(
			float( int(right + left) - int(rendering_context_.viewport_size.Width ()) ) / float(rendering_context_.viewport_size.Width()),
			float( int(rendering_context_.viewport_size.Height()) - bottom - top ) / float(rendering_context_.viewport_size.Height()),
			0.0f ) );

	walls_buffer_.Bind();

	glLineWidth( scale );

	// Walls
	lines_shader_.Uniform( "color", 1.0f, 0.2f, 1.0f, 0.5f );
	lines_shader_.Uniform( "view_matrix", shift_matrix * scale_matrix * viewport_scale_matrix * viewport_shift_matrix );

	glEnable( GL_SCISSOR_TEST );
	glScissor( left, rendering_context_.viewport_size.Height() - bottom, map_viewport_width, map_viewport_height );
	glDrawArrays( GL_LINES, 0, ( current_map_data_->static_walls.size() + current_map_data_->dynamic_walls.size() ) * 2u );
	glDisable( GL_SCISSOR_TEST );

	// Arrow
	{
		m_Mat4 rotate_matrix;
		rotate_matrix.RotateZ( view_angle );

		lines_shader_.Uniform( "color", 1.0f, 0.2f, 0.2f, 0.5f );
		lines_shader_.Uniform( "view_matrix", rotate_matrix * scale_matrix * viewport_scale_matrix * viewport_shift_matrix );

		glDrawArrays( GL_LINES, arrow_vertices_offset_, g_arrow_vertices );
	}
	// Framing
	{
		lines_shader_.Uniform( "color", 0.2f, 0.2f, 0.2f, 0.5f );

		m_Mat4 framing_scale_mat;
		framing_scale_mat.Scale(
			m_Vec3(
				float(map_viewport_width ) / float(rendering_context_.viewport_size.Width () ),
				float(map_viewport_height) / float(rendering_context_.viewport_size.Height() ),
				1.0f ) );

		lines_shader_.Uniform( "view_matrix", framing_scale_mat * viewport_shift_matrix );

		glLineWidth( 2.0f * scale );
		glDrawArrays( GL_LINES, framing_vertices_offset_, g_framing_vertices );
	}

	glLineWidth( 1.0f );
}

} // namespace PanzerChasm
