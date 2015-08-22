
static INLINE void M_ComputeTileCoords( int32_t& x, int32_t& z, const glm::vec3& v )
{
   x = ( int32_t )( v.x * 0.5f );
   z = ( int32_t )( v.z * 0.5f );
}

static INLINE glm::mat4 M_TransformFromTile( const map_tile_t& tile )
{
    glm::mat4 t( glm::translate( glm::mat4( 1.0f ),
                 glm::vec3( tile_generator_t::TRANSLATE_STRIDE * tile.x,
                            0.0f,
                            tile_generator_t::TRANSLATE_STRIDE * tile.z ) ) );

    return std::move( t * tile.GenScaleTransform() );
}


