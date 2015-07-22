namespace rend {

//-------------------------------------------------------------------------------------------------------
// Global
//-------------------------------------------------------------------------------------------------------
template < typename T >
static INLINE GLuint GenBufferObject( GLenum target, const std::vector< T >& data, GLenum usage )
{
	GLuint obj;
	GL_CHECK( glGenBuffers( 1, &obj ) );
	GL_CHECK( glBindBuffer( target, obj ) );
	GL_CHECK( glBufferData( target, data.size() * sizeof( T ), &data[ 0 ], usage ) );
	GL_CHECK( glBindBuffer( target, 0 ) );
	return obj;
}

template < typename T, size_t count >
static INLINE GLuint GenBufferObject( GLenum target, const std::array< T, count >& data, GLenum usage )
{
	GLuint obj;
	GL_CHECK( glGenBuffers( 1, &obj ) );
	GL_CHECK( glBindBuffer( target, obj ) );
	GL_CHECK( glBufferData( target, data.size() * sizeof( T ), &data[ 0 ], usage ) );
	GL_CHECK( glBindBuffer( target, 0 ) );
	return obj;
}

template < typename T >
static INLINE void UpdateBufferObject( GLenum target, GLuint obj, GLuint offset, const std::vector< T >& data, bool bindUnbind ) 
{
	if ( bindUnbind )
	{
		GL_CHECK( glBindBuffer( target, obj ) );
	}

	GL_CHECK( glBufferSubData( target, offset * sizeof( T ), data.size() * sizeof( T ), &data[ 0 ] ) );

	if ( bindUnbind )
	{
		GL_CHECK( glBindBuffer( target, 0 ) );
	}
}

static INLINE void DeleteBufferObject( GLenum target, GLuint obj )
{
	if ( obj )
	{
		// Unbind to prevent driver from lazy deletion
		GL_CHECK( glBindBuffer( target, 0 ) );
		GL_CHECK( glDeleteBuffers( 1, &obj ) );
	}
}

static INLINE void DrawElementBuffer( GLuint ibo, size_t numIndices )
{
	GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo ) );
	GL_CHECK( glDrawElements( GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr ) );
	GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );
}

static INLINE uint32_t Texture_GetMaxMipLevels2D( int32_t baseWidth, int32_t baseHeight )
{
	return glm::min( ( int32_t ) glm::log2( ( float ) baseWidth ), ( int32_t ) glm::log2( ( float ) baseHeight ) );
}

template< typename texture_helper_t >
static INLINE uint32_t Texture_CalcMipLevels2D( const texture_helper_t& tex, int32_t baseWidth, int32_t baseHeight, int32_t maxLevels )
{
	if ( !maxLevels )
	{
		maxLevels = Texture_GetMaxMipLevels2D( baseWidth, baseHeight );
	}

	int32_t w = baseWidth;
	int32_t h = baseHeight;
	int32_t mip;

	for ( mip = 0; h != 1 && w != 1; ++mip )
	{
		tex.CalcMipLevel2D( mip, w, h );

		if ( h > 1 )
		{
			h /= 2;
		}

		if ( w > 1 )
		{
			w /= 2;
		}
	}

	return mip;
}

//-------------------------------------------------------------------------------------------------------
// POD types
//-------------------------------------------------------------------------------------------------------

static INLINE draw_vertex_t draw_vertex_t_Make( const glm::vec3& position, const glm::u8vec4& color )
{
	draw_vertex_t v =
	{
		position,
		glm::vec3( 0.0f ),
		glm::vec2( 0.0f ),
		color
	};

	return v;
}

//-------------------------------------------------------------------------------------------------------
// shader_program_t
//-------------------------------------------------------------------------------------------------------
INLINE void shader_program_t::AddUnif( const std::string& name )
{
    GL_CHECK( uniforms[ name ] = glGetUniformLocation( program, name.c_str() ) );
}

INLINE void shader_program_t::AddAttrib( const std::string& name )
{
    GL_CHECK( attribs[ name ] = glGetAttribLocation( program, name.c_str() ) );
}

INLINE void shader_program_t::Bind( void ) const
{
    GL_CHECK( glUseProgram( program ) );
}

INLINE void shader_program_t::Release( void ) const
{
    GL_CHECK( glUseProgram( 0 ) );
}

INLINE void shader_program_t::LoadMat4( const std::string& name, const glm::mat4& t ) const
{

	GL_CHECK( glUniformMatrix4fv( uniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program_t::LoadMat2( const std::string& name, const glm::mat2& t ) const
{
	GL_CHECK( glUniformMatrix2fv( uniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program_t::LoadMat2( const std::string& name, const float* t ) const
{
	GL_CHECK( glUniformMatrix2fv( uniforms.at( name ), 1, GL_FALSE, t ) );
}

INLINE void shader_program_t::LoadVec2( const std::string& name, const glm::vec2& v ) const
{
	GL_CHECK( glUniform2fv( uniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program_t::LoadVec2( const std::string& name, const float* v ) const
{
	GL_CHECK( glUniform2fv( uniforms.at( name ), 1, v ) );
}

INLINE void shader_program_t::LoadVec2Array( const std::string& name, const float* v, int32_t num ) const
{
	GL_CHECK( glUniform2fv( uniforms.at( name ), num, v ) );
}

INLINE void shader_program_t::LoadVec3( const std::string& name, const glm::vec3& v ) const
{
	GL_CHECK( glUniform3fv( uniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program_t::LoadVec3Array( const std::string& name, const float* v, int32_t num ) const
{
	GL_CHECK( glUniform3fv( uniforms.at( name ), num, v ) );
}

INLINE void shader_program_t::LoadVec4( const std::string& name, const glm::vec4& v ) const
{
	GL_CHECK( glUniform4fv( uniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program_t::LoadVec4( const std::string& name, const float* v ) const
{
	GL_CHECK( glUniform4fv( uniforms.at( name ), 1, v ) );
}

INLINE void shader_program_t::LoadVec4Array( const std::string& name, const float* v, int32_t num ) const
{
	GL_CHECK( glUniform4fv( uniforms.at( name ), num, v ) );
}

INLINE void shader_program_t::LoadInt( const std::string& name, int v ) const
{
	GL_CHECK( glUniform1i( uniforms.at( name ), v ) );
}

INLINE void shader_program_t::LoadFloat( const std::string& name, float f ) const
{
	GL_CHECK( glUniform1f( uniforms.at( name ), f ) );
}

template < typename vertex_type_t >
INLINE void shader_program_t::LoadAttribLayout( const shader_program_t& program )
{
    for ( const auto& attrib: program.attribs )
    {
        if ( attrib.second != -1 )
        {
            if ( !program.disableAttribs.empty() )
            {
                auto it = std::find( program.disableAttribs.cbegin(), program.disableAttribs.cend(), attrib.first );

                if ( it != program.disableAttribs.cend() )
                {
                    GL_CHECK( glDisableVertexAttribArray( attrib.second ) );
                    continue;
                }
            }

            attrib_loader_t< vertex_type_t >::functions[ attrib.first ]( program );
        }
    }
}

//-------------------------------------------------------------------------------------------------------
// texture_t
//-------------------------------------------------------------------------------------------------------
INLINE void texture_t::CalcMipLevel2D( int32_t mip, int32_t mipwidth, int32_t mipheight ) const
{
	GL_CHECK( glTexImage2D( target, mip, internalFormat, 
				mipwidth, mipheight, 0, format, GL_UNSIGNED_BYTE, &pixels[ 0 ] ) );
}

INLINE void texture_t::GenHandle( void )
{
	if ( !handle )
	{
		GL_CHECK( glGenTextures( 1, &handle ) );
	}
}

INLINE void texture_t::Bind( void ) const
{
	GL_CHECK( glBindTexture( target, handle ) );
}

INLINE void texture_t::Release( void ) const
{
	GL_CHECK( glBindTexture( target, 0 ) );
}

INLINE void texture_t::Release( int offset ) const
{
	GL_CHECK( glActiveTexture( GL_TEXTURE0 + offset ) );
	GL_CHECK( glBindTexture( target, 0 ) );
}
//-------------------------------------------------------------------------------------------------------
// rtt_t
//-------------------------------------------------------------------------------------------------------
INLINE rtt_t::rtt_t( GLenum attachment_, const glm::mat4& view_ ) 
	:	fbo( 0 ),
		attachment( attachment_ ),
		view( view_ )
{
	GL_CHECK( glGenFramebuffers( 1, &fbo ) );
}

INLINE rtt_t::~rtt_t( void )
{
	if ( fbo )
	{
		GL_CHECK( glDeleteFramebuffers( 1, &fbo ) );
	}
}

INLINE void rtt_t::Attach( int32_t width, int32_t height, int32_t bpp )
{
	texture.mipmap = false;
	texture.wrap = GL_REPEAT;
	texture.SetBufferSize( width, height, bpp, 255 );
	texture.Load2D();
	texture.LoadSettings();

	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, fbo ) );
	GL_CHECK( glFramebufferTexture2D( GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.handle, 0 ) );

	GLenum fbocheck;
	GL_CHECK( fbocheck = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
	if ( fbocheck != GL_FRAMEBUFFER_COMPLETE )
	{
		MLOG_ERROR( "FBO check incomplete; value returned is 0x%x", fbocheck );	
	}
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}

INLINE void rtt_t::Bind( void ) const
{
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, fbo ) );
}

INLINE void rtt_t::Release( void ) const
{
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}
//-------------------------------------------------------------------------------------------------------
// viewport_stash_t
//-------------------------------------------------------------------------------------------------------
INLINE viewport_stash_t::viewport_stash_t( GLint originX, GLint originY, GLint width, GLint height )
{
	GL_CHECK( glGetIntegerv( GL_VIEWPORT, &original[ 0 ] ) );
	GL_CHECK( glViewport( originX, originY, width, height ) );
}

INLINE viewport_stash_t::~viewport_stash_t( void )
{
	GL_CHECK( glViewport( original[ 0 ], original[ 1 ], original[ 2 ], original[ 3 ] ) );
}

//-------------------------------------------------------------------------------------------------------
// attrib_loader_t: loads vertex attributes from an arbitrary vertex type
//-------------------------------------------------------------------------------------------------------

#define LOADER_FUNC_NAME "attrib_loader_t::functions::"
#define MAP_VEC_3( name, funcname )\
    do {\
        GLint location = program.attribs.at( #name );\
        GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ), funcname );\
        GL_CHECK_WITH_NAME( glVertexAttribPointer( location,\
                3, GL_FLOAT, GL_FALSE, sizeof( vertex_type_t ), ( void* ) offsetof( vertex_type_t, name ) ), funcname );\
    }\
    while( 0 )

template < typename vertex_type_t >
typename attrib_loader_t< vertex_type_t >::loader_func_map_t attrib_loader_t< vertex_type_t >::functions =
{
    {
        "position",
        []( const shader_program_t& program ) -> void
        {
            MAP_VEC_3( position, LOADER_FUNC_NAME"position" );
        }
    },
    {
        "normal",
        []( const shader_program_t& program ) -> void
        {
            MAP_VEC_3( normal, LOADER_FUNC_NAME"normal" );
        }
    },
    {
        "color",
        []( const shader_program_t& program ) -> void
        {
            GLint location = program.attribs.at( "color" );

            GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ),
                "attribLoadFunctions" );

            GL_CHECK_WITH_NAME( glVertexAttribPointer( location,
                4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( vertex_type_t ),
                ( void* ) offsetof( vertex_type_t, color ) ), LOADER_FUNC_NAME"color" );
        }
    },
    {
        "texCoord",
        []( const shader_program_t& program ) -> void
        {
            GLint location = program.attribs.at( "texCoord" );
            GL_CHECK_WITH_NAME( glVertexAttribPointer( location,
                2, GL_FLOAT, GL_FALSE, sizeof( vertex_type_t ), ( void* ) offsetof( vertex_type_t, texCoord ) ), LOADER_FUNC_NAME"texCoord" );
        }
    }
};

#undef LOADER_FUNC_NAME
#undef MAP_VEC_3

//---------------------------------------------------------------------
// draw_buffer_t
//---------------------------------------------------------------------
template < size_t vertexCount >
draw_buffer_t< vertexCount >::draw_buffer_t( const std::array< draw_vertex_t, vertexCount >& vertexData, GLenum usage )
	: vbo( GenBufferObject< draw_vertex_t, vertexCount >( GL_ARRAY_BUFFER, vertexData, usage ) )
{
}

template < size_t vertexCount >
draw_buffer_t< vertexCount >::~draw_buffer_t( void )
{
	if ( vbo )
	{
		GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
		GL_CHECK( glDeleteBuffers( 1, &vbo ) );
	}
}

template < size_t vertexCount >
void draw_buffer_t< vertexCount >::Render( GLenum mode, const shader_program_t& program, const glm::mat4& modelToView ) const
{
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, vbo ) );
	shader_program_t::LoadAttribLayout< rend::draw_vertex_t >( program );

	program.Bind();
	program.LoadMat4( "modelToView", modelToView );
	GL_CHECK( glDrawArrays( mode, 0, vertexCount ) );
	program.Release();

	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
}


} // namespace glrend
