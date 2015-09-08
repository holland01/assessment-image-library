#define ATTRIB_OFFSET_VBO -1

//-------------------------------------------------------------------------------------------------------
// Global
//-------------------------------------------------------------------------------------------------------
template < typename T >
static INLINE GLuint rend_make_buffer( GLenum target, const std::vector< T >& data, GLenum usage )
{
	GLuint obj;
	GL_CHECK( glGenBuffers( 1, &obj ) );
	GL_CHECK( glBindBuffer( target, obj ) );
	GL_CHECK( glBufferData( target, data.size() * sizeof( T ), &data[ 0 ], usage ) );
	GL_CHECK( glBindBuffer( target, 0 ) );
	return obj;
}

template < typename T, size_t count >
static INLINE GLuint rend_make_buffer( GLenum target, const std::array< T, count >& data, GLenum usage )
{
	GLuint obj;
	GL_CHECK( glGenBuffers( 1, &obj ) );
	GL_CHECK( glBindBuffer( target, obj ) );
	GL_CHECK( glBufferData( target, data.size() * sizeof( T ), &data[ 0 ], usage ) );
	GL_CHECK( glBindBuffer( target, 0 ) );
	return obj;
}

template < typename T >
static INLINE void rend_update_buffer( GLenum target, GLuint obj, GLuint offset, const std::vector< T >& data, bool bindUnbind )
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

static INLINE void rend_free_buffer( GLenum target, GLuint obj )
{
	if ( obj )
	{
		// Unbind to prevent driver from lazy deletion
		GL_CHECK( glBindBuffer( target, 0 ) );
		GL_CHECK( glDeleteBuffers( 1, &obj ) );
	}
}

static INLINE uint32_t rend_get_max_mip2d( int32_t baseWidth, int32_t baseHeight )
{
	return glm::min( ( int32_t ) glm::log2( ( float ) baseWidth ), ( int32_t ) glm::log2( ( float ) baseHeight ) );
}

template< typename texture_helper_t >
static INLINE uint32_t rend_get_mip2d( const texture_helper_t& tex, int32_t baseWidth, int32_t baseHeight, int32_t maxLevels )
{
	if ( !maxLevels )
	{
		maxLevels = rend_get_max_mip2d( baseWidth, baseHeight );
	}

	int32_t w = baseWidth;
	int32_t h = baseHeight;
	int32_t mip;

	for ( mip = 0; h != 1 && w != 1; ++mip )
	{
		tex.calc_mip_2d( mip, w, h );

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

static INLINE draw_vertex_t rend_make_draw_vertex( const glm::vec3& position, const glm::vec2& texCoord, const glm::u8vec4& color )
{
	draw_vertex_t v =
	{
		{ position.x, position.y, position.z },
		{ 0.0f, 0.0f, 0.0f },
		{ texCoord.x, texCoord.y },
		{ color.r, color.g, color.b, color.a }
	};

	return v;
}

static INLINE draw_vertex_t rend_make_draw_vertex( const glm::vec3& position )
{
	return rend_make_draw_vertex( position, glm::u8vec4( 255 ) );
}

static INLINE draw_vertex_t rend_make_draw_vertex( const glm::vec3& position, const glm::u8vec4& color )
{
	return rend_make_draw_vertex( position, glm::vec2( 0.0f ), color );
}

//-------------------------------------------------------------------------------------------------------
// shader_program_t
//-------------------------------------------------------------------------------------------------------
INLINE void shader_program::add_unif( const std::string& name )
{
	GL_CHECK( mUniforms[ name ] = glGetUniformLocation( mProgram, name.c_str() ) );
}

INLINE void shader_program::add_attrib( const std::string& name )
{
	GL_CHECK( mAttribs[ name ] = glGetAttribLocation( mProgram, name.c_str() ) );
}

INLINE void shader_program::bind( void ) const
{
	GL_CHECK( glUseProgram( mProgram ) );
}

INLINE void shader_program::release( void ) const
{
	GL_CHECK( glUseProgram( 0 ) );
}

INLINE void shader_program::load_mat4( const std::string& name, const glm::mat4& t ) const
{

	GL_CHECK( glUniformMatrix4fv( mUniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program::load_mat2( const std::string& name, const glm::mat2& t ) const
{
	GL_CHECK( glUniformMatrix2fv( mUniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program::load_mat2( const std::string& name, const float* t ) const
{
	GL_CHECK( glUniformMatrix2fv( mUniforms.at( name ), 1, GL_FALSE, t ) );
}

INLINE void shader_program::load_mat3( const std::string& name, const glm::mat3& t ) const
{
	GL_CHECK( glUniformMatrix3fv( mUniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program::load_vec2( const std::string& name, const glm::vec2& v ) const
{
	GL_CHECK( glUniform2fv( mUniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program::load_vec2( const std::string& name, const float* v ) const
{
	GL_CHECK( glUniform2fv( mUniforms.at( name ), 1, v ) );
}

INLINE void shader_program::load_vec2_array( const std::string& name, const float* v, int32_t num ) const
{
	GL_CHECK( glUniform2fv( mUniforms.at( name ), num, v ) );
}

INLINE void shader_program::load_vec3( const std::string& name, const glm::vec3& v ) const
{
	GL_CHECK( glUniform3fv( mUniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program::load_vec3_array( const std::string& name, const float* v, int32_t num ) const
{
	GL_CHECK( glUniform3fv( mUniforms.at( name ), num, v ) );
}

INLINE void shader_program::load_vec4( const std::string& name, const glm::vec4& v ) const
{
	GL_CHECK( glUniform4fv( mUniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program::load_vec4( const std::string& name, const float* v ) const
{
	GL_CHECK( glUniform4fv( mUniforms.at( name ), 1, v ) );
}

INLINE void shader_program::load_vec4_array( const std::string& name, const float* v, int32_t num ) const
{
	GL_CHECK( glUniform4fv( mUniforms.at( name ), num, v ) );
}

INLINE void shader_program::load_int( const std::string& name, int v ) const
{
	GL_CHECK( glUniform1i( mUniforms.at( name ), v ) );
}

INLINE void shader_program::load_float( const std::string& name, float f ) const
{
	GL_CHECK( glUniform1f( mUniforms.at( name ), f ) );
}

template < typename vertex_type_t >
INLINE void shader_program::load_attrib_layout( const draw_buffer& buffer, const shader_program& program, bool clientArray )
{
	if ( lastAttribLoad == &program && lastDrawBuffer == &buffer )
	{
		return;
	}

	for ( const auto& attrib: program.mAttribs )
	{
		if ( attrib.second != -1 )
		{
			if ( !program.mDisableAttribs.empty() )
			{
				auto it = std::find( program.mDisableAttribs.cbegin(), program.mDisableAttribs.cend(), attrib.first );

				if ( it != program.mDisableAttribs.cend() )
				{
					GL_CHECK( glDisableVertexAttribArray( attrib.second ) );
					continue;
				}
			}

			intptr_t offset;

			if ( clientArray )
			{
				offset = program.mAttribPointerOffsets.at( attrib.first );
			}
			else
			{
				offset = ATTRIB_OFFSET_VBO;
			}

			attrib_loader< vertex_type_t >::mFunctions[ attrib.first ]( program, offset );
		}
	}

	lastAttribLoad = &program;
	lastDrawBuffer = &buffer;
}

//-------------------------------------------------------------------------------------------------------
// texture_t
//-------------------------------------------------------------------------------------------------------
INLINE void texture::calc_mip_2d( int32_t mip, int32_t mipwidth, int32_t mipheight ) const
{
	GL_CHECK( glTexImage2D( mTarget, mip, mInternalFormat,
				mipwidth, mipheight, 0, mFormat, GL_UNSIGNED_BYTE, &mPixels[ 0 ] ) );
}

INLINE void texture::gen_handle( void )
{
	if ( !mHandle )
	{
		GL_CHECK( glGenTextures( 1, &mHandle ) );
	}
}

INLINE void texture::bind( void ) const
{
	GL_CHECK( glBindTexture( mTarget, mHandle ) );
}

INLINE void texture::release( void ) const
{
	GL_CHECK( glBindTexture( mTarget, 0 ) );
}

INLINE void texture::release( int offset ) const
{
	GL_CHECK( glActiveTexture( GL_TEXTURE0 + offset ) );
	GL_CHECK( glBindTexture( mTarget, 0 ) );
}
//-------------------------------------------------------------------------------------------------------
// rtt_t
//-------------------------------------------------------------------------------------------------------
INLINE render_to_texture::render_to_texture( GLenum attachment_, const glm::mat4& view_ )
	:	fbo( 0 ),
		attachment( attachment_ ),
		view( view_ )
{
	GL_CHECK( glGenFramebuffers( 1, &fbo ) );
}

INLINE render_to_texture::~render_to_texture( void )
{
	if ( fbo )
	{
		GL_CHECK( glDeleteFramebuffers( 1, &fbo ) );
	}
}

INLINE void render_to_texture::attach( int32_t width, int32_t height, int32_t bpp )
{
	mTexture.mMipmap = false;
	mTexture.mWrap = GL_REPEAT;
	mTexture.set_buffer_size( width, height, bpp, 255 );
	mTexture.load_2d();
	mTexture.load_settings();

	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, fbo ) );
	GL_CHECK( glFramebufferTexture2D( GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, mTexture.mHandle, 0 ) );

	GLenum fbocheck;
	GL_CHECK( fbocheck = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
	if ( fbocheck != GL_FRAMEBUFFER_COMPLETE )
	{
		MLOG_ERROR( "FBO check incomplete; value returned is 0x%x", fbocheck );
	}
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}

INLINE void render_to_texture::bind( void ) const
{
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, fbo ) );
}

INLINE void render_to_texture::release( void ) const
{
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}
//-------------------------------------------------------------------------------------------------------
// viewport_stash_t
//-------------------------------------------------------------------------------------------------------
INLINE viewport_stash::viewport_stash( GLint originX, GLint originY, GLint width, GLint height )
{
	GL_CHECK( glGetIntegerv( GL_VIEWPORT, &original[ 0 ] ) );
	GL_CHECK( glViewport( originX, originY, width, height ) );
}

INLINE viewport_stash::~viewport_stash( void )
{
	GL_CHECK( glViewport( original[ 0 ], original[ 1 ], original[ 2 ], original[ 3 ] ) );
}

//-------------------------------------------------------------------------------------------------------
// attrib_loader_t: loads vertex attributes from an arbitrary vertex type
//-------------------------------------------------------------------------------------------------------

#define LOADER_FUNC_NAME "attrib_loader::mFunctions::m"
#define MAP_VEC_3( vname, attribname, funcname )\
	do {\
		if ( attribOffset == ATTRIB_OFFSET_VBO )\
		{\
			attribOffset = offsetof( vertex_type_t, vname );\
		}\
		GLint location = program.mAttribs.at( #attribname );\
		GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ), funcname );\
		GL_CHECK_WITH_NAME( glVertexAttribPointer( location,\
				3, GL_FLOAT, GL_FALSE, sizeof( vertex_type_t ), ( void* ) attribOffset ), funcname );\
	}\
	while( 0 )

template < typename vertex_type_t >
typename attrib_loader< vertex_type_t >::loader_func_map_t attrib_loader< vertex_type_t >::mFunctions =
{
	{
		"position",
		[]( const shader_program& program, intptr_t attribOffset ) -> void
		{
			MAP_VEC_3( mPosition, position, LOADER_FUNC_NAME"Position" );
		}
	},
	{
		"normal",
		[]( const shader_program& program, intptr_t attribOffset ) -> void
		{
			MAP_VEC_3( mNormal, normal, LOADER_FUNC_NAME"Normal" );
		}
	},
	{
		"color",
		[]( const shader_program& program, intptr_t attribOffset ) -> void
		{
			GLint location = program.mAttribs.at( "color" );

			if ( attribOffset == ATTRIB_OFFSET_VBO )
			{
				attribOffset = offsetof( vertex_type_t, mColor );
			}

			GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ),
				LOADER_FUNC_NAME"Color" );

			GL_CHECK_WITH_NAME( glVertexAttribPointer( location,
				4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( vertex_type_t ),
				( void* ) attribOffset ), LOADER_FUNC_NAME"Color" );
		}
	},
	{
		"texCoord",
		[]( const shader_program& program, intptr_t attribOffset ) -> void
		{
			if ( attribOffset == ATTRIB_OFFSET_VBO )
			{
				attribOffset = offsetof( vertex_type_t, mTexCoord );
			}

			GLint location = program.mAttribs.at( "texCoord" );
			GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ), LOADER_FUNC_NAME"TexCoord" );
			GL_CHECK_WITH_NAME( glVertexAttribPointer( location,
				2, GL_FLOAT, GL_FALSE, sizeof( vertex_type_t ), ( void* ) attribOffset ), LOADER_FUNC_NAME"TexCoord" );
		}
	}
};

#undef LOADER_FUNC_NAME
#undef MAP_VEC_3

//---------------------------------------------------------------------
// debug_split_draw
//---------------------------------------------------------------------

template < typename predicate_type_t, typename renderable_t >
debug_split_draw< predicate_type_t, renderable_t >::debug_split_draw(
		typename debug_split_draw< predicate_type_t, renderable_t >::predicate_fn_t predicate_,
		const glm::ivec2& dims_ )
	: rightDraw( predicate_ ),
	  originalVp( 0, 0, dims_.x, dims_.y ),
	  splitDims( dims_.x / 2, dims_.y )
{
}

template < typename predicate_type_t, typename renderable_t >
debug_split_draw< predicate_type_t, renderable_t >::~debug_split_draw( void )
{
}

template < typename predicate_type_t, typename renderable_t >
void debug_split_draw< predicate_type_t, renderable_t >::operator()(
		predicate_type_t& predobj,
		const renderable_t& draw,
		const shader_program& program,
		const glm::mat4& leftView,
		const glm::mat4& rightView ) const
{
	GL_CHECK( glViewport( 0, 0, splitDims.x, splitDims.y ) );
	program.load_mat4( "modelToView", leftView );
	draw.Render( program );

	if ( rightDraw( predobj ) )
	{
		GL_CHECK( glViewport( splitDims.x, 0, splitDims.x, splitDims.y ) );
		program.load_mat4( "modelToView", rightView );
		draw.Render( program );
	}
}

#undef ATTRIB_OFFSET_VBO
