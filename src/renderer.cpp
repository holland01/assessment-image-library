#include "renderer.h"
#include <stdlib.h>

//-----------------------------------------------------------
// Shader Util Functions
//-----------------------------------------------------------

static GLuint CompileShaderSource( const char* src, const int length, GLenum type )
{
	GLuint shaderId;
	GL_CHECK(shaderId = glCreateShader(type));
	if (0 != shaderId)
    {
        if ( length > 0 )
		{
			int blength[ 1 ] = { length };
			glShaderSource( shaderId, 1, &src, blength );
		}
		else
		{
			glShaderSource( shaderId, 1, &src, NULL );
		}

        glCompileShader( shaderId );

        GLint compileSuccess;
        glGetShaderiv( shaderId, GL_COMPILE_STATUS, &compileSuccess );

        if ( compileSuccess == GL_FALSE )
        {
            GLint logLen;
            glGetShaderiv( shaderId, GL_INFO_LOG_LENGTH, &logLen );

            char* infoLog = new char[ logLen ]();
            infoLog[ logLen ] = '\0';

            glGetShaderInfoLog( shaderId, logLen, NULL, infoLog );

            MLOG_ERROR( "SHADER COMPILE MLOG_ERROR [ %s ]: %s", ( type == GL_VERTEX_SHADER ) ? "vertex" : "fragment", infoLog );
        }
    }
    else
    {
        MLOG_ERROR( "ERROR: Could not create a shader.\n" );
    }

	return shaderId;
}

static GLuint LinkProgram( GLuint shaders[], int len )
{
    GLuint program = glCreateProgram();

    for ( int i = 0; i < len; ++i )
        glAttachShader( program, shaders[ i ] );

    glLinkProgram( program );

    GLint linkSuccess;
    glGetProgramiv( program, GL_LINK_STATUS, &linkSuccess );

    if ( !linkSuccess )
    {
        GLint logLen;
        glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logLen );

        char* infoLog = new char[ logLen ]();
        glGetProgramInfoLog( program, logLen, NULL, infoLog );

        MLOG_ERROR( "GLSL LINK MLOG_ERROR: %s", infoLog );

		delete[] infoLog;
    }

    for ( int i = 0; i < len; ++i )
    {
        glDetachShader( program, shaders[ i ] );
        glDeleteShader( shaders[ i ] );
    }

    return program;
}

// (Slightly modified) Implementation is copy-pasta from http://code.google.com/p/openglbook-samples/source/browse/trunk/Chapter%204/Utils.c
static GLuint CompileShader( const char* filename, GLenum shader_type )
{
    GLuint shaderId = 0;
    FILE* file;
    long file_size = -1;
    char* glsl_source;

    if (NULL != (file = fopen(filename, "rb")) &&
            0 == fseek(file, 0, SEEK_END) &&
            -1 != (file_size = ftell(file)))
    {
        rewind(file);

        if (NULL != (glsl_source = (char*)malloc(file_size + 1)))
        {
            if (file_size == (long)fread(glsl_source, sizeof(char), file_size, file))
            {
                glsl_source[file_size] = '\0';

                shaderId = CompileShaderSource( glsl_source, file_size, shader_type );
            }
            else
            {
                MLOG_ERROR( "ERROR: Could not read file %s\n", filename );
            }

            free( glsl_source );
        }
        else
        {
            MLOG_ERROR( "ERROR: Could not allocate %ld bytes.\n", file_size );
        }

        fclose(file);
    }
    else
    {
        MLOG_ERROR( "ERROR: Could not open file %s\n", filename );
    }

    return shaderId;
}



//-----------------------------------------------------------
// Vertex Layout Utils
//-----------------------------------------------------------

static INLINE void SetAttribDivisor( GLint location, GLint value )
{
#ifdef R_GL_CORE_PROFILE
	GL_CHECK( glVertexAttribDivisor( location, value ) ); 
#else
	UNUSEDPARAM( location );
	UNUSEDPARAM( value );
#endif
}
 
static std::map< std::string, std::function< void( const Program& program ) > > attribLoadFunctions = 
{
	{
		"position",
		[]( const Program& program ) -> void
		{
			GLint location = program.attribs.at( "position" );
			MapVec3( location, offsetof( vertex_t, position ) );
			SetAttribDivisor( location, 0 );
		}
	},
	{
		"normal",
		[]( const Program& program ) -> void
		{
			GLint location = program.attribs.at( "normal" );
			MapVec3( location, offsetof( vertex_t, normal ) );
			SetAttribDivisor( location, 0 );
		}
	},
	{
		"color",
		[]( const Program& program ) -> void
		{
			GLint location = program.attribs.at( "color" );
			
			GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ), 
				"attribLoadFunctions" );
				 
			GL_CHECK_WITH_NAME( glVertexAttribPointer( location, 
				4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( vertex_t ), 
				( void* ) offsetof( vertex_t, color ) ), "attribLoadFunctions" );
			
			SetAttribDivisor( location, 0 );
		}
	},
	{
		"texCoord",
		[]( const Program& program ) -> void
		{
			GLint location = program.attribs.at( "texCoord" );
			MapVec3( location, offsetof( vertex_t, texCoord ) );
			SetAttribDivisor( location, 0 );
		}
	}
};

//-----------------------------------------------------------
// Texture Utils
//-----------------------------------------------------------

enum texFormat_t
{
#ifdef R_GL_CORE_PROFILE
	TEX_EXTERNAL_R = GL_R,
	TEX_INTERNAL_R = GL_R8,
	TEX_INTERNAL_RGB = GL_SRGB8,
	TEX_INTERNAL_RGBA = GL_SRGB8_ALPHA8,
#else
	TEX_EXTERNAL_R = GL_ALPHA,
	TEX_INTERNAL_R = GL_ALPHA,
	TEX_INTERNAL_RGB = GL_RGB,
	TEX_INTERNAL_RGBA = GL_RGBA,
#endif 
	
	
};

GLuint GenSampler( bool mipmap, GLenum wrap )
{	
#ifdef R_GL_CORE_PROFILE
	GLuint sampler;
	GL_CHECK( glGenSamplers( 1, &sampler ) );
	
	GL_CHECK( glSamplerParameteri( handle, GL_TEXTURE_MIN_FILTER, mipmap? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR ) );
	GL_CHECK( glSamplerParameteri( handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	GL_CHECK( glSamplerParameteri( handle, GL_TEXTURE_WRAP_S, wrap ) );
	GL_CHECK( glSamplerParameteri( handle, GL_TEXTURE_WRAP_T, wrap ) );
	GL_CHECK( glSamplerParameteri( handle, GL_TEXTURE_WRAP_R, wrap ) );

	GLfloat maxSamples;
	GL_CHECK( glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxSamples ) );
	GL_CHECK( glSamplerParameterf( handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxSamples ) );

	return sampler;
#else
	UNUSEDPARAM( mipmap );
	UNUSEDPARAM( wrap );
	return 1; // for if ( !sampler ) checks from client code
#endif
}

void BindTexture( GLenum target, GLuint handle, int32_t offset,  
	int32_t sampler, const std::string& uniform, const Program& program )
{
	GL_CHECK( glActiveTexture( GL_TEXTURE0 + offset ) );
	GL_CHECK( glBindTexture( target, handle ) );

#ifdef R_GL_CORE_PROFILE
	GL_CHECK( glBindSampler( offset, sampler ) );
#else
	UNUSEDPARAM( sampler );
#endif

	program.LoadInt( uniform, offset );
}

/*
static INLINE void SetPixel( byte* dest, const byte* src, int width, int height, int bpp, int srcX, int srcY, int destX, int destY )
{
	int destOfs = ( width * destY + destX ) * bpp;
	int srcOfs = ( width * srcY + srcX ) * bpp;

	for ( int k = 0; k < bpp; ++k )
		dest[ destOfs + k ] = src[ srcOfs + k ];
}

static INLINE void FlipBytes( byte* out, const byte* src, int width, int height, int bpp )
{
	for ( int y = 0; y < height; ++y )
	{
		for ( int x = 0; x < width; ++x )
		{
			SetPixel( out, src, width, height, bpp, x, height - y - 1, x, y );
		}
	}
}
*/

//-------------------------------------------------------------------------------------------------
// texture_t
//-------------------------------------------------------------------------------------------------
texture_t::texture_t( void )
	: srgb( true ), mipmap( false ),
	  handle( 0 ), sampler( 0 ),
	  wrap( GL_REPEAT ), minFilter( GL_LINEAR ), magFilter( GL_LINEAR ), 
	  format( 0 ), internalFormat( 0 ), target( GL_TEXTURE_2D ), maxMip( 0 ),
	  width( 0 ),
	  height( 0 ),
	  depth( 0 ),
	  bpp( 0 )
{
}

texture_t::~texture_t( void )
{
	if ( handle )
	{
		GL_CHECK( glDeleteTextures( 1, &handle ) );
	}

#ifdef R_GL_CORE_PROFILE
	if ( sampler )
	{
		GL_CHECK( glDeleteSamplers( 1, &sampler ) );
	}
#endif
}

void texture_t::Bind( int offset, const std::string& unif, const Program& prog ) const
{
	BindTexture( GL_TEXTURE_2D, handle, offset, sampler, unif, prog );
}

void texture_t::LoadCubeMap( void )
{
	target = GL_TEXTURE_CUBE_MAP;
	
	GenHandle();
	Bind();
	for ( int i = 0; i < 6; ++i )
	{
		GL_CHECK( glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
			internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, &pixels[ 0 ] ) );
	}
	Release();

	LoadSettings();
}

void texture_t::Load2D( void )
{
	target = GL_TEXTURE_2D;
	GenHandle();
	Bind();

	if ( mipmap )
	{
		maxMip = Texture_CalcMipLevels2D< texture_t >( *this, width, height, 0 );

		GL_CHECK( glGenerateMipmap( target ) );
		minFilter = GL_LINEAR_MIPMAP_LINEAR;
	}
	else
	{
		GL_CHECK( glTexImage2D( target, 
			0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, &pixels[ 0 ] ) );
	}
	Release();

	LoadSettings();
}

void texture_t::LoadSettings( void )
{
#ifdef R_GL_CORE_PROFILE
	if ( !sampler )
	{
		sampler = GenSampler( mipmap, wrap );
	}
#endif

	GL_CHECK( glBindTexture( target, handle ) );
	
#ifdef R_GL_CORE_PROFILE
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_BASE_LEVEL, 0 ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_MAX_LEVEL, maxMip ) );
#else
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_MIN_FILTER, mipmap? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_WRAP_S, wrap ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_WRAP_T, wrap ) );
#endif
	
	GL_CHECK( glBindTexture( target, 0 ) );
}

bool texture_t::LoadFromFile( const char* texPath )
{
	std::vector< uint8_t > tmp;
	File_GetPixels( texPath, tmp, bpp, width, height );

	if ( bpp == 3 )
	{
		pixels.resize( width * height * 4, 255 ); 
		Pixels_24BitTo32Bit( &pixels[ 0 ], &tmp[ 0 ], width * height );
		bpp = 4;
	}
	else
	{
		pixels = std::move( tmp );
	}

	if ( !DetermineFormats() )
	{
		MLOG_WARNING( "Unsupported bits per pixel of %i specified; this needs to be fixed. For image file \'%s\'", 
			bpp, texPath );
		return false;
	}
	
	return true;
}

bool texture_t::SetBufferSize( int width0, int height0, int bpp0, uint8_t fill )
{
	width = width0;
	height = height0;
	bpp = bpp0;
	pixels.resize( width * height * bpp, fill );

	return DetermineFormats();
}

bool texture_t::DetermineFormats( void )
{
	switch( bpp )
	{
	case 1:
		format = TEX_EXTERNAL_R;
		internalFormat = TEX_INTERNAL_R;
		break;

	case 3:
		format = GL_RGB;
		internalFormat = TEX_INTERNAL_RGB;
		break;

	case 4:
		format = GL_RGBA;
		internalFormat = TEX_INTERNAL_RGBA;
		break;
	default:
		return false;
		break;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------
// textureArray_t
//-------------------------------------------------------------------------------------------------

#ifdef R_GL_CORE_PROFILE

textureArray_t::mipSetter_t::mipSetter_t( 
	const GLuint handle_,
	const int32_t layerOffset_,
	const int32_t numLayers_,
	const std::vector< uint8_t >& buffer_ )

	:	handle( handle_ ),
		layerOffset( layerOffset_ ),
		numLayers( numLayers_ ),
		buffer( buffer_ )
{
}

void textureArray_t::mipSetter_t::CalcMipLevel2D( int32_t mip, int32_t mipWidth, int32_t mipHeight ) const
{
	GL_CHECK( glTextureSubImage3D( handle, 
			mip, 0, 0, layerOffset, mipWidth, mipHeight, numLayers, GL_RGBA, GL_UNSIGNED_BYTE, &buffer[ 0 ] ) );
}

textureArray_t::textureArray_t( GLsizei width, GLsizei height, GLsizei depth, bool genMipLevels )
	:	megaDims( 
			width, 
			height, 
			depth, 
			genMipLevels? Texture_GetMaxMipLevels2D( width, height ): 1 )
{
	GL_CHECK( glCreateTextures( GL_TEXTURE_2D_ARRAY, 1, &handle ) );

	megaDims.w = glm::min( megaDims.w, GLConfig::MAX_MIP_LEVELS );

	GL_CHECK( glTextureStorage3D( handle, megaDims.w, GL_SRGB8_ALPHA8, megaDims.x, megaDims.y, megaDims.z ) );
	
	std::vector< uint8_t > fill;
	fill.resize( width * height * depth * 4, 255 );
	
	if ( genMipLevels )
	{
		// the vec2 here isn't really necessary, since there is no mip bias for this initial fill
		mipSetter_t ms( handle, 0, depth, fill ); 
		Texture_CalcMipLevels2D< textureArray_t::mipSetter_t >( ms, width, height, megaDims.w );
	}
	else
	{
		GL_CHECK( glTextureSubImage3D( handle, 
			0, 0, 0, 0, width, height, depth, GL_RGBA, GL_UNSIGNED_BYTE, &fill[ 0 ] ) );
	}
	
	samplers.resize( megaDims.z, 0 );
	usedSlices.resize( megaDims.z, 0 );
	biases.resize( megaDims.z, glm::vec3( 0.0f ) );
}
	
textureArray_t::~textureArray_t( void )
{
	GL_CHECK( glDeleteTextures( 1, &handle ) );
	GL_CHECK( glDeleteSamplers( samplers.size(), &samplers[ 0 ] ) );
}

void textureArray_t::LoadSlice( GLuint sampler, const glm::ivec3& dims, const std::vector< uint8_t >& buffer, bool genMipMaps )
{
	if ( genMipMaps )
	{
		mipSetter_t ms( handle, dims.z, 1, buffer ); 
		Texture_CalcMipLevels2D< textureArray_t::mipSetter_t >( ms, dims.x, dims.y, megaDims.w );
	}
	else
	{
		GL_CHECK( glTextureSubImage3D( handle, 0, 0, 0, 
			dims.z, dims.x, dims.y, 1, GL_RGBA, GL_UNSIGNED_BYTE, &buffer[ 0 ] ) );
	}

	samplers[ dims.z ] = sampler;
	usedSlices[ dims.z ] = 1;
	biases[ dims.z ] = glm::vec3( ( float )dims.x / ( float )megaDims.x, ( float )dims.y / ( float )megaDims.y, ( float ) dims.z );
}

void textureArray_t::Bind( GLuint unit, const std::string& samplerName, const Program& program ) const
{
	BindTexture( GL_TEXTURE_2D_ARRAY, handle, unit, 0, samplerName, program );
}
	
void textureArray_t::Release( GLuint unit ) const
{
	GL_CHECK( glActiveTexture( GL_TEXTURE0 + unit ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D_ARRAY, 0 ) );
}

#endif // R_GL_CORE_PROFILE

//-------------------------------------------------------------------------------------------------

#ifdef R_GL_CORE_PROFILE
void ImPrep( const glm::mat4& viewTransform, const glm::mat4& clipTransform )
{
	GL_CHECK( glUseProgram( 0 ) );
	GL_CHECK( glMatrixMode( GL_PROJECTION ) );
	GL_CHECK( glLoadIdentity() );
	GL_CHECK( glLoadMatrixf( glm::value_ptr( clipTransform ) ) );
	GL_CHECK( glMatrixMode( GL_MODELVIEW ) );
	GL_CHECK( glLoadIdentity() );
	GL_CHECK( glLoadMatrixf( glm::value_ptr( viewTransform ) ) );
}

void ImDrawAxes( const float size ) 
{
	std::array< glm::vec3, 6 > axes = 
	{
		glm::vec3( size, 0.0f, 0.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, size, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ),
		glm::vec3( 0.0f, 0.0f, -size ), glm::vec3( 0.0f, 0.0f, 1.0f )
	};
	
	glBegin( GL_LINES );
	for ( int i = 0; i < 6; i += 2 )
	{
		glColor3fv( glm::value_ptr( axes[ i + 1 ] ) );
		glVertex3f( 0.0f, 0.0f, 0.0f );
		glVertex3fv( glm::value_ptr( axes[ i ] ) ); 
	}
	glEnd();
}

void ImDrawBounds( const AABB& bounds, const glm::vec4& color )
{
	const glm::vec3 center( bounds.Center() );

	const std::array< const glm::vec3, 8 > points = 
	{
		bounds.maxPoint - center, 
		glm::vec3( bounds.maxPoint.x, bounds.maxPoint.y, bounds.minPoint.z ) - center,
		glm::vec3( bounds.maxPoint.x, bounds.minPoint.y, bounds.minPoint.z ) - center, 
		glm::vec3( bounds.maxPoint.x, bounds.minPoint.y, bounds.maxPoint.z ) - center,
		
		bounds.minPoint - center,
		glm::vec3( bounds.minPoint.x, bounds.maxPoint.y, bounds.minPoint.z ) - center,
		glm::vec3( bounds.minPoint.x, bounds.minPoint.y, bounds.minPoint.z ) - center, 
		glm::vec3( bounds.minPoint.x, bounds.minPoint.y, bounds.maxPoint.z ) - center,
	};
	
	GL_CHECK( glMatrixMode( GL_MODELVIEW ) );
	GL_CHECK( glPushMatrix() );
	GL_CHECK( glTranslatef( center.x, center.y, center.z ) );
	
	glBegin( GL_LINE_STRIP );
	glColor4fv( glm::value_ptr( color ) );

	for ( int i = 0; i < 8; ++i )
	{
		glVertex3fv( glm::value_ptr( points[ i ] ) );
	}

	glEnd();
	GL_CHECK( glPopMatrix() );
}

void ImDrawPoint( const glm::vec3& point, const glm::vec4& color, float size )
{
	GL_CHECK( glMatrixMode( GL_MODELVIEW ) );
	GL_CHECK( glPushMatrix() );
	GL_CHECK( glTranslatef( point.x, point.y, point.z ) );
	GL_CHECK( glPushAttrib( GL_POINT_BIT ) );
	GL_CHECK( glPointSize( size ) );

	glBegin( GL_POINTS );
	glColor4fv( glm::value_ptr( color ) );
	glVertex3f( 0.0f, 0.0f, 0.0f );
	glEnd();

	GL_CHECK( glPopAttrib() );
	GL_CHECK( glPopMatrix() );
}
#endif // R_GL_CORE_PROFILE

//-------------------------------------------------------------------------------------------------
// Program
//-------------------------------------------------------------------------------------------------
Program::Program( const std::string& vertexShader, const std::string& fragmentShader )
	: program( 0 )
{
	GLuint shaders[] = 
	{
		CompileShaderSource( vertexShader.c_str(), vertexShader.size(), GL_VERTEX_SHADER ),
		CompileShaderSource( fragmentShader.c_str(), fragmentShader.size(), GL_FRAGMENT_SHADER )
	};

	program = LinkProgram( shaders, 2 );
}

Program::Program( const std::string& vertexShader, const std::string& fragmentShader, 
	const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs, bool bindTransformsUbo )
	: Program( vertexShader, fragmentShader )
{
	GenData( uniforms, attribs, bindTransformsUbo );
}

Program::Program( const std::vector< char >& vertexShader, const std::vector< char >& fragmentShader, 
		const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs, bool bindTransformsUbo )
		: Program( std::string( &vertexShader[ 0 ], vertexShader.size() ), 
				std::string( &fragmentShader[ 0 ], fragmentShader.size() ) )
{
	GenData( uniforms, attribs, bindTransformsUbo );
}

Program::Program( const Program& copy )
	: program( copy.program ),
	  uniforms( copy.uniforms ),
	  attribs( copy.attribs )
{
}

Program::~Program( void )
{
	Release();
	GL_CHECK( glDeleteProgram( program ) );
}

void Program::GenData( const std::vector< std::string >& uniforms, 
	const std::vector< std::string >& attribs, bool bindTransformsUbo )
{
	uint32_t max = glm::max( attribs.size(), uniforms.size() );
	for ( uint32_t i = 0; i < max; ++i )
	{
		if ( i < attribs.size() )
		{
			AddAttrib( attribs[ i ] );
		}

		if ( i < uniforms.size() )
		{
			AddUnif( uniforms[ i ] );
		}
	}

	if ( bindTransformsUbo )
	{
		MapProgramToUBO( program, "Transforms" );
	}
}

void Program::LoadAttribLayout( void ) const
{
	for ( int i = 0; i < 5; ++i )
	{
		GL_CHECK( glDisableVertexAttribArray( i ) );
	}

	for ( const auto& attrib: attribs )
	{
		if ( attrib.second != -1 )
		{
			if ( !disableAttribs.empty() )
			{
				auto it = std::find( disableAttribs.begin(), disableAttribs.end(), attrib.first );

				if ( it != disableAttribs.end() )
				{
					GL_CHECK( glDisableVertexAttribArray( attrib.second ) );
					continue;
				}
			}
			
			attribLoadFunctions[ attrib.first ]( *this ); 
		}
	}
}

std::vector< std::string > Program::ArrayLocationNames( const std::string& name, int32_t length )
{
	std::vector< std::string > names;
	names.resize( length );

	for ( int32_t i = 0; i < length; ++i )
	{
		names[ i ] = name + "[" + std::to_string( i ) + "]";
	}
	return names;
}

//-------------------------------------------------------------------------------------------------

loadBlend_t::loadBlend_t( GLenum srcFactor, GLenum dstFactor )
{
	GL_CHECK( glGetIntegerv( GL_BLEND_SRC_RGB, ( GLint* ) &prevSrcFactor ) );
	GL_CHECK( glGetIntegerv( GL_BLEND_DST_RGB, ( GLint* ) &prevDstFactor ) );

	GL_CHECK( glBlendFunc( srcFactor, dstFactor ) );
}

loadBlend_t::~loadBlend_t( void )
{
	GL_CHECK( glBlendFunc( prevSrcFactor, prevDstFactor ) );
}

//-------------------------------------------------------------------------------------------------
