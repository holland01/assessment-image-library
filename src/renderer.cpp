#include "renderer.h"
#include "view.h"
#include <stdlib.h>

//-------------------------------------------------------------------------------------------------
// global
//-------------------------------------------------------------------------------------------------

imm_draw* gImmDrawer = nullptr;

//-----------------------------------------------------------
// Shader Util Functions
//-----------------------------------------------------------
namespace {

GLuint compile_shader_source( const char* src, int32_t length, GLenum type )
{
	GLuint shaderId;
    GL_CHECK(shaderId = glCreateShader(type));
	if (0 != shaderId)
    {
        if ( length > 0 )
		{
			int32_t blength[ 1 ] = { length };
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

			// May not be null terminated, so just wrap it safely...
			std::string srcStr;
			if ( length > 0 )
			{
				srcStr = std::string( src, length );
			}
			else
			{
				srcStr = std::string( src );
			}

			MLOG_ERROR( "SHADER COMPILE MLOG_ERROR [ %s ]: %s\n Message: %s",
						srcStr.c_str(),
						( type == GL_VERTEX_SHADER ) ? "vertex" : "fragment",
						infoLog );
        }
    }
    else
    {
        MLOG_ERROR( "ERROR: Could not create a shader.\n" );
    }

	return shaderId;
}

GLuint link_program( GLuint shaders[], int len )
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
GLuint compile_shader( const char* filename, GLenum shader_type )
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

                shaderId = compile_shader_source( glsl_source, file_size, shader_type );
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

} // namespace

//-----------------------------------------------------------
// Texture Utils
//-----------------------------------------------------------

void bind_texture( GLenum target, GLuint handle, int32_t offset, const std::string& uniform, const shader_program& program )
{
	GL_CHECK( glActiveTexture( GL_TEXTURE0 + offset ) );
	GL_CHECK( glBindTexture( target, handle ) );

    program.load_int( uniform, offset );
}

// Saved until further notice
/*
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
texture::texture( void )
    : mSrgb( false ), mMipmap( false ),
      mHandle( 0 ),
      mWrap( GL_CLAMP_TO_EDGE ), mMinFilter( GL_LINEAR ), mMagFilter( GL_LINEAR ),
      mFormat( 0 ), mInternalFormat( 0 ), mTarget( GL_TEXTURE_2D ), mMaxMip( 0 ),
      mWidth( 0 ),
      mHeight( 0 ),
      mDepth( 0 ),
      mBpp( 0 )
{
}

texture::~texture( void )
{
    if ( mHandle )
	{
        GL_CHECK( glDeleteTextures( 1, &mHandle ) );
	}
}

void texture::bind( int offset, const std::string& unif, const shader_program& prog ) const
{
    bind_texture( GL_TEXTURE_2D, mHandle, offset, unif, prog );
}

void texture::load_cube_map( void )
{
    mTarget = GL_TEXTURE_CUBE_MAP;
	
    gen_handle();
    bind();
	for ( int i = 0; i < 6; ++i )
	{
		GL_CHECK( glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
            mInternalFormat, mWidth, mHeight, 0, mFormat, GL_UNSIGNED_BYTE, &mPixels[ 0 ] ) );
	}
    release();

    load_settings();
}

void texture::load_2d( void )
{
    mTarget = GL_TEXTURE_2D;
    gen_handle();
    bind();

    if ( mMipmap )
	{
        mMaxMip = make_mipmaps_2d< texture >( *this, mWidth, mHeight, 0 );
        mMinFilter = GL_LINEAR_MIPMAP_LINEAR;
        GL_CHECK( glGenerateMipmap( mTarget ) );
	}
	else
	{
        GL_CHECK( glTexImage2D( mTarget,
            0, mInternalFormat, mWidth, mHeight, 0, mFormat, GL_UNSIGNED_BYTE, &mPixels[ 0 ] ) );
	}

    release();

    load_settings();
}

void texture::load_settings( void )
{
    bind();
	// For some reason setting this through SDL's GL ES context on the desktop (in Linux) causes really bad texture sampling to happen,
	// regardless of the value passed. WTF?!

    GL_CHECK( glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, mMinFilter ) );
    GL_CHECK( glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, mMagFilter ) );
    GL_CHECK( glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, mWrap ) );
    GL_CHECK( glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, mWrap ) );
	
    release();
}

bool texture::open_file( const char* texPath )
{
    file_get_pixels( texPath, mPixels, mBpp, mWidth, mHeight );

    if ( !determine_formats() )
	{
		MLOG_ERROR( "Unsupported bits per pixel of %i specified; this needs to be fixed. For image file \'%s\'",
            mBpp, texPath );
		return false;
	}
	
	return true;
}

bool texture::set_buffer_size( int32_t width0, int32_t height0, int32_t bpp0, uint8_t fill )
{
    mWidth = width0;
    mHeight = height0;
    mBpp = bpp0;
    mPixels.resize( mWidth * mHeight * mBpp, fill );

    return determine_formats();
}

bool texture::determine_formats( void )
{
    switch( mBpp )
	{
	case 1:
        mFormat = GL_ALPHA;
        mInternalFormat = GL_ALPHA8;
		break;

	case 3:
        mFormat = GL_RGB;
        mInternalFormat = GL_RGB8;
		break;

	case 4:
        mFormat = GL_RGBA;
        mInternalFormat = GL_RGBA8;
		break;

	default:
		return false;
		break;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------
// shader_program_t
//-------------------------------------------------------------------------------------------------

const shader_program* shader_program::lastAttribLoad = nullptr;
const draw_buffer* shader_program::lastDrawBuffer = nullptr;

shader_program::shader_program( void )
    : mProgram( 0 )
{
}

shader_program::shader_program( const std::string& vertexShader, const std::string& fragmentShader )
    : mProgram( 0 )
{
	GLuint shaders[] = 
	{
        compile_shader_source( vertexShader.c_str(), ( int32_t ) vertexShader.size(), GL_VERTEX_SHADER ),
        compile_shader_source( fragmentShader.c_str(), ( int32_t ) fragmentShader.size(), GL_FRAGMENT_SHADER )
	};

    mProgram = link_program( shaders, 2 );
}

shader_program::shader_program( const std::string& vertexShader, const std::string& fragmentShader,
    const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs )
    : shader_program( vertexShader, fragmentShader )
{
    gen_data( uniforms, attribs );
}

shader_program::shader_program( const std::vector< char >& vertexShader, const std::vector< char >& fragmentShader,
        const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs )
        : shader_program( std::string( &vertexShader[ 0 ], vertexShader.size() ),
				std::string( &fragmentShader[ 0 ], fragmentShader.size() ) )
{
    gen_data( uniforms, attribs );
}

shader_program::shader_program( const shader_program& copy )
    : mProgram( copy.mProgram ),
      mUniforms( copy.mUniforms ),
      mAttribs( copy.mAttribs )
{
}

shader_program::shader_program( shader_program&& original )
{
    *this = std::move( original );
}

shader_program& shader_program::operator=( shader_program&& original )
{
    mProgram = original.mProgram;
    mUniforms = std::move( original.mUniforms );
    mAttribs = std::move( original.mAttribs );
    mDisableAttribs = std::move( original.mDisableAttribs );

    original.mProgram = 0;

    return *this;
}

shader_program::~shader_program( void )
{
    if ( mProgram )
    {
        release();
        GL_CHECK( glDeleteProgram( mProgram ) );
    }
}

void shader_program::gen_data( const std::vector< std::string >& uniforms,
    const std::vector< std::string >& attribs )
{
	uint32_t max = ( uint32_t ) glm::max( attribs.size(), uniforms.size() );
	for ( uint32_t i = 0; i < max; ++i )
	{
		if ( i < attribs.size() )
		{
            add_attrib( attribs[ i ] );
		}

		if ( i < uniforms.size() )
		{
            add_unif( uniforms[ i ] );
		}
	}
}

std::vector< std::string > shader_program::array_location_names( const std::string& name, int32_t length )
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
// loadBlend_t
//-------------------------------------------------------------------------------------------------
set_blend_mode::set_blend_mode( GLenum srcFactor, GLenum dstFactor )
    : mPrevSrcFactor( 0 ),
      mPrevDstFactor( 0 ),
      mEnabled( true )
{
    GL_CHECK( mEnabled = !!glIsEnabled( GL_BLEND ) );

    GL_CHECK( glGetIntegerv( GL_BLEND_SRC_RGB, ( GLint* ) &mPrevSrcFactor ) );
    GL_CHECK( glGetIntegerv( GL_BLEND_DST_RGB, ( GLint* ) &mPrevDstFactor ) );

    if ( !mEnabled )
    {
        GL_CHECK( glEnable( GL_BLEND ) );
    }

	GL_CHECK( glBlendFunc( srcFactor, dstFactor ) );
}

set_blend_mode::~set_blend_mode( void )
{
    GL_CHECK( glBlendFunc( mPrevSrcFactor, mPrevDstFactor ) );

    if ( !mEnabled )
    {
        GL_CHECK( glDisable( GL_BLEND ) );
    }
}

//---------------------------------------------------------------------
// draw_buffer_t
//---------------------------------------------------------------------

draw_buffer::draw_buffer( void )
    : mVbo(	[]( void ) -> GLuint
			{
				GLuint buf;
				GL_CHECK( glGenBuffers( 1, &buf ) );
				return buf;
			}() ),
      mIbo( 0 ),
      mCount( 0 ),
      mMode( 0 ),
      mUsage( 0 )
{
}

draw_buffer::draw_buffer( const std::vector< draw_vertex_t >& vertexData,
							  GLenum mode_, GLenum usage_ )
    : mVbo( make_vertex_buffer< draw_vertex_t >( GL_ARRAY_BUFFER, vertexData, usage_ ) ),
      mIbo( 0 ),
      mCount( ( GLsizei ) vertexData.size() ),
      mMode( mode_ ),
      mUsage( usage_ )
{
}

draw_buffer::draw_buffer( const std::vector< draw_vertex_t >& vertexData,
							  const std::vector< GLuint >& indexData,
							  GLenum mode_, GLenum usage_ )
    : mVbo( make_vertex_buffer< draw_vertex_t >( GL_ARRAY_BUFFER, vertexData, usage_ ) ),
      mIbo( make_vertex_buffer< GLuint >( GL_ELEMENT_ARRAY_BUFFER, indexData, GL_STATIC_DRAW ) ),
      mCount( ( GLsizei ) indexData.size() ),
      mMode( mode_ ),
      mUsage( usage_ )
{
}

draw_buffer::draw_buffer( draw_buffer&& buffer )
{
	*this = std::move( buffer );
}

draw_buffer::~draw_buffer( void )
{
    free_vertex_buffer( GL_ARRAY_BUFFER, mVbo );
    free_vertex_buffer( GL_ELEMENT_ARRAY_BUFFER, mIbo );
}

draw_buffer& draw_buffer::operator= ( draw_buffer&& buffer )
{
	if ( this != &buffer )
	{
        mVbo = buffer.mVbo;
        mIbo = buffer.mIbo;
        mCount = buffer.mCount;
        mMode = buffer.mMode;

        buffer.mMode = 0;
        buffer.mCount = 0;
        buffer.mIbo = 0;
        buffer.mVbo = 0;
	}

	return *this;
}

void draw_buffer::render( const shader_program& program ) const
{
    bind();
    shader_program::load_attrib_layout< draw_vertex_t >( *this, program );

    if ( mIbo )
	{
        GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIbo ) );
        GL_CHECK( glDrawElements( mMode, mCount, GL_UNSIGNED_INT, 0 ) );
		GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );
	}
	else
	{
        GL_CHECK( glDrawArrays( mMode, 0, mCount ) );
	}
    release();
}

//-------------------------------------------------------------------------------------------------
// imm_draw_t
//-------------------------------------------------------------------------------------------------

imm_draw::buffer_store::buffer_store( void )
    : mLastSize( 0 ),
      mBuffer( nullptr )
{
}

void imm_draw::buffer_store::update( void )
{
    if ( mLastSize != mVertices.size() )
    {
        mBuffer->realloc( mVertices );
        mLastSize = mVertices.size();
    }
    else
    {
        mBuffer->update( mVertices );
    }

    mVertices.clear();
}

imm_draw::buffer_store imm_draw::mBufferStore;

imm_draw::imm_draw( const shader_program& prog )
      : mEnabled( true ),
        mProgram( prog )
{
}

void imm_draw::begin( GLenum mode )
{
    if ( !mEnabled )
	{
		return;
	}

    if ( !mBufferStore.mBuffer )
	{
        mBufferStore.mBuffer.reset( new draw_buffer() );
        mBufferStore.mBuffer->usage( GL_DYNAMIC_DRAW );
	}

    mBufferStore.mBuffer->mode( mode );
}

void imm_draw::vertex( const draw_vertex_t& v )
{
    if ( !mEnabled )
	{
		return;
	}

    mBufferStore.mVertices.push_back( v );
}

void imm_draw::vertex( const glm::vec3& position )
{
    if ( !mEnabled )
	{
		return;
	}

    mBufferStore.mVertices.push_back( make_draw_vertex( position ) );
}

void imm_draw::end( void )
{
    if ( !mEnabled )
	{
		return;
	}

    mBufferStore.update();
    mBufferStore.mBuffer->render( mProgram );
}

void imm_draw::enabled( bool value )
{
    mEnabled = value;
}


//-------------------------------------------------------------------------------------------------
// pipeline_t
//-------------------------------------------------------------------------------------------------

#ifdef OP_GL_USE_ES
#	define OP_GLSL_SHADER_PREPEND "precision mediump float;\n"
#	define OP_GLSL_SHADER_DIR "es"
#else
#	define OP_GLSL_SHADER_PREPEND "#version 130\n"
#	define OP_GLSL_SHADER_DIR "desktop"
#endif // OP_USE_GL_eS

namespace {
	struct program_def_t
	{
		std::string programName;
		std::string vertexName;
		std::string fragmentName;
		std::vector< std::string > uniforms;
		std::vector< std::string > attribs;
	};

	struct buffer_def_t
	{
		std::string name;
		GLenum mode;
		GLenum usage;
		std::vector< draw_vertex_t > vertexData;
		std::vector< GLuint > indexData;
	};
}

render_pipeline::render_pipeline( void )
    : mVao( 0 )
{
    std::array< program_def_t, 3 > defs =
	{{
		{
			"single_color",
			"single_color.vert",
			"single_color.frag",
			{ "color", "modelToView", "viewToClip" },
			{ "position" }
		},
        {
            "single_color_ss",
            "single_color_ss.vert",
            "single_color_ss.frag",
            { "color" },
            { "position" }
        },
		{
			"billboard",
			"billboard.vert",
			"billboard.frag",
            { "origin", "viewOrient", "modelToView", "viewToClip", "image", "color" },
			{ "position", "texCoord" }
		}
	}};

	std::string shaderRootDir( "asset/shader/" OP_GLSL_SHADER_DIR "/" );

	for ( program_def_t& def: defs )
	{
		std::vector< char > vertexBuf, fragmentBuf;

        file_get_buf( vertexBuf, shaderRootDir + def.vertexName );
        file_get_buf( fragmentBuf, shaderRootDir + def.fragmentName );

		std::string prepend( OP_GLSL_SHADER_PREPEND );

		vertexBuf.insert( vertexBuf.begin(), prepend.c_str(), prepend.c_str() + prepend.length() );
		fragmentBuf.insert( fragmentBuf.begin(), prepend.c_str(), prepend.c_str() + prepend.length() );

		std::string vshader( &vertexBuf[ 0 ], vertexBuf.size() );
		std::string fshader( &fragmentBuf[ 0 ], fragmentBuf.size() );

		printf( "VERTEX: \n %s \n FRAGMENT: \n %s \n", vshader.c_str(), fshader.c_str() );

        mPrograms[ def.programName ] = std::move( shader_program( vertexBuf, fragmentBuf, def.uniforms, def.attribs ) );
	}

    std::vector< draw_vertex_t > cubeVertexData =
    {
     // back
        make_draw_vertex( glm::vec3( 1.0f, 1.0f, 1.0f ) ),
        make_draw_vertex( glm::vec3( -1.0f, 1.0f, 1.0f ) ),
        make_draw_vertex( glm::vec3( -1.0f, -1.0f, 1.0f ) ),
        make_draw_vertex( glm::vec3( 1.0f, -1.0f, 1.0f ) ),

     // front
        make_draw_vertex( glm::vec3( -1.0f, -1.0f, -1.0f ) ),
        make_draw_vertex( glm::vec3( 1.0f, -1.0f, -1.0f ) ),
        make_draw_vertex( glm::vec3( 1.0f, 1.0f, -1.0f ) ),
        make_draw_vertex( glm::vec3( -1.0f, 1.0f, -1.0f ) )
    };


    std::vector< GLuint > cubeIndexData =
    // Draw order:
    // back face, right face, front face, bottom face, left face, top face
    {
        0x00000002u, 0x00000003u, 0x00000000u,
        0x00000000u, 0x00000001u, 0x00000002u,

        0x00000003u, 0x00000005u, 0x00000006u,
        0x00000006u, 0x00000000u, 0x00000003u,

        0x00000005u, 0x00000004u, 0x00000007u,
        0x00000007u, 0x00000006u, 0x00000005u,

        0x00000003u, 0x00000002u, 0x00000004u,
        0x00000004u, 0x00000005u, 0x00000003u,

        0x00000002u, 0x00000001u, 0x00000007u,
        0x00000007u, 0x00000004u, 0x00000002u,

        0x00000001u, 0x00000000u, 0x00000006u,
        0x00000006u, 0x00000007u, 0x00000001u
    };

    std::array< buffer_def_t, 3 > bufferDefs =
	{{

        {
			"colored_cube",
			GL_TRIANGLES,
			GL_STATIC_DRAW,
            cubeVertexData,
            cubeIndexData
		},

        {
            "lined_cube",
             GL_LINE_STRIP,
             GL_STATIC_DRAW,
             cubeVertexData,
             cubeIndexData
        },

		// billboard
		{
			"billboard",
			GL_TRIANGLE_STRIP,
			GL_STATIC_DRAW,
			{
                make_draw_vertex( glm::vec3( -1.0f, -1.0f, 0.0f ), glm::vec2( 0.0f, 0.0f ),  glm::u8vec4( 255 ) ),
                make_draw_vertex( glm::vec3( 1.0f, -1.0f, 0.0f ), glm::vec2( 1.0f, 0.0f ), glm::u8vec4( 255 ) ),
                make_draw_vertex( glm::vec3( -1.0f, 1.0f, 0.0f ), glm::vec2( 0.0f, 1.0f ), glm::u8vec4( 255 ) ),
                make_draw_vertex( glm::vec3( 1.0f, 1.0f, 0.0f ), glm::vec2( 1.0f, 1.0f ), glm::u8vec4( 255 ) )
			},
			{}
        }
	}};

	for ( const buffer_def_t& def: bufferDefs )
	{
		if ( def.indexData.empty() )
		{
            mDrawBuffers[ def.name ] = std::move( draw_buffer( def.vertexData, def.mode, def.usage ) );
		}
		else
		{
            mDrawBuffers[ def.name ] = std::move( draw_buffer( def.vertexData, def.indexData, def.mode, def.usage ) );
		}
	}

#ifndef OP_GL_USE_ES
    GL_CHECK( glGenVertexArrays( 1, &mVao ) );
    GL_CHECK( glBindVertexArray( mVao ) );
#endif

    bind_program::mInstance = this;
}

render_pipeline::~render_pipeline( void )
{
#ifndef OP_GL_USE_ES
    if ( mVao )
	{
        GL_CHECK( glDeleteVertexArrays( 1, &mVao ) );
	}
#endif
}

//-------------------------------------------------------------------------------------------------
// bind_program
//-------------------------------------------------------------------------------------------------

const render_pipeline* bind_program::mInstance = nullptr;

bind_program::bind_program( const std::string& what )
    : mProgram( mInstance->program( what ) )
{
    mProgram.bind();
}

bind_program::~bind_program( void )
{
    mProgram.release();
}
