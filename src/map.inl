template< typename type_t >
static INLINE void get_tile_coords( type_t& x, type_t& z, const glm::vec3& v )
{
   x = ( type_t )( v.x * 0.5f );
   z = ( type_t )( v.z * 0.5f );
}

static INLINE glm::mat4 get_tile_transform( const map_tile& tile )
{
	glm::mat4 t( glm::translate( glm::mat4( 1.0f ),
				 glm::vec3( map_tile_generator::TRANSLATE_STRIDE * tile.mX,
							0.0f,
							map_tile_generator::TRANSLATE_STRIDE * tile.mZ ) ) );

	return std::move( t * tile.scale_transform() );
}


