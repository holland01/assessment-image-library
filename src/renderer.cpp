#include "renderer.h"
#include "view.h"
#include <stdlib.h>

//-----------------------------------------------------------
// Shader Util Functions
//-----------------------------------------------------------

static GLuint CompileShaderSource( const char* src, int32_t length, GLenum type )
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
// Texture Utils
//-----------------------------------------------------------

void BindTexture( GLenum target, GLuint handle, int32_t offset, const std::string& uniform, const shader_program_t& program )
{
	GL_CHECK( glActiveTexture( GL_TEXTURE0 + offset ) );
	GL_CHECK( glBindTexture( target, handle ) );

	program.LoadInt( uniform, offset );
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
texture_t::texture_t( void )
	: srgb( false ), mipmap( false ),
      handle( 0 ),
	  wrap( GL_CLAMP_TO_EDGE ), minFilter( GL_LINEAR ), magFilter( GL_LINEAR ),
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
}

void texture_t::Bind( int offset, const std::string& unif, const shader_program_t& prog ) const
{
    BindTexture( GL_TEXTURE_2D, handle, offset, unif, prog );
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
		minFilter = GL_LINEAR_MIPMAP_LINEAR;
		GL_CHECK( glGenerateMipmap( target ) );
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
	Bind();
	// For some reason setting this through SDL's GL ES context on the desktop (in Linux) causes really bad texture sampling to happen,
	// regardless of the value passed. WTF?!

	GL_CHECK( glTexParameteri( target, GL_TEXTURE_MIN_FILTER, minFilter ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_MAG_FILTER, magFilter ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_WRAP_S, wrap ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_WRAP_T, wrap ) );
	
	Release();
}

bool texture_t::LoadFromFile( const char* texPath )
{
	File_GetPixels( texPath, pixels, bpp, width, height );

	if ( !DetermineFormats() )
	{
		MLOG_ERROR( "Unsupported bits per pixel of %i specified; this needs to be fixed. For image file \'%s\'",
			bpp, texPath );
		return false;
	}
	
	return true;
}

bool texture_t::SetBufferSize( int32_t width0, int32_t height0, int32_t bpp0, uint8_t fill )
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
		format = GL_ALPHA;
		internalFormat = GL_ALPHA8;
		break;

	case 3:
		format = GL_RGB;
		internalFormat = GL_RGB8;
		break;

	case 4:
		format = GL_RGBA;
		internalFormat = GL_RGBA8;
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

const shader_program_t* shader_program_t::lastAttribLoad = nullptr;
const draw_buffer_t* shader_program_t::lastDrawBuffer = nullptr;

shader_program_t::shader_program_t( void )
    : program( 0 )
{
}

shader_program_t::shader_program_t( const std::string& vertexShader, const std::string& fragmentShader )
	: program( 0 )
{
	GLuint shaders[] = 
	{
		CompileShaderSource( vertexShader.c_str(), ( int32_t ) vertexShader.size(), GL_VERTEX_SHADER ),
		CompileShaderSource( fragmentShader.c_str(), ( int32_t ) fragmentShader.size(), GL_FRAGMENT_SHADER )
	};

	program = LinkProgram( shaders, 2 );
}

shader_program_t::shader_program_t( const std::string& vertexShader, const std::string& fragmentShader,
    const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs )
    : shader_program_t( vertexShader, fragmentShader )
{
    GenData( uniforms, attribs );
}

shader_program_t::shader_program_t( const std::vector< char >& vertexShader, const std::vector< char >& fragmentShader,
        const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs )
        : shader_program_t( std::string( &vertexShader[ 0 ], vertexShader.size() ),
				std::string( &fragmentShader[ 0 ], fragmentShader.size() ) )
{
    GenData( uniforms, attribs );
}

shader_program_t::shader_program_t( const shader_program_t& copy )
	: program( copy.program ),
	  uniforms( copy.uniforms ),
	  attribs( copy.attribs )
{
}

shader_program_t::shader_program_t( shader_program_t&& original )
{
    *this = std::move( original );
}

shader_program_t& shader_program_t::operator=( shader_program_t&& original )
{
    program = original.program;
    uniforms = std::move( original.uniforms );
    attribs = std::move( original.attribs );
    disableAttribs = std::move( original.disableAttribs );

    original.program = 0;

    return *this;
}

shader_program_t::~shader_program_t( void )
{
    if ( program )
    {
        Release();
        GL_CHECK( glDeleteProgram( program ) );
    }
}

void shader_program_t::GenData( const std::vector< std::string >& uniforms,
    const std::vector< std::string >& attribs )
{
	uint32_t max = ( uint32_t ) glm::max( attribs.size(), uniforms.size() );
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
}

std::vector< std::string > shader_program_t::ArrayLocationNames( const std::string& name, int32_t length )
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
load_blend_t::load_blend_t( GLenum srcFactor, GLenum dstFactor )
{
	GL_CHECK( glGetIntegerv( GL_BLEND_SRC_RGB, ( GLint* ) &prevSrcFactor ) );
	GL_CHECK( glGetIntegerv( GL_BLEND_DST_RGB, ( GLint* ) &prevDstFactor ) );

	GL_CHECK( glBlendFunc( srcFactor, dstFactor ) );
}

load_blend_t::~load_blend_t( void )
{
	GL_CHECK( glBlendFunc( prevSrcFactor, prevDstFactor ) );
}

//---------------------------------------------------------------------
// draw_buffer_t
//---------------------------------------------------------------------

draw_buffer_t::draw_buffer_t( void )
	: vbo(	[]( void ) -> GLuint
			{
				GLuint buf;
				GL_CHECK( glGenBuffers( 1, &buf ) );
				return buf;
			}() ),
	  ibo( 0 ),
	  count( 0 ),
	  mode( 0 ),
	  usage( 0 )
{
}

draw_buffer_t::draw_buffer_t( const std::vector< draw_vertex_t >& vertexData,
							  GLenum mode_, GLenum usage_ )
	: vbo( GenBufferObject< draw_vertex_t >( GL_ARRAY_BUFFER, vertexData, usage_ ) ),
	  ibo( 0 ),
	  count( ( GLsizei ) vertexData.size() ),
	  mode( mode_ ),
	  usage( usage_ )
{
}

draw_buffer_t::draw_buffer_t( const std::vector< draw_vertex_t >& vertexData,
							  const std::vector< GLuint >& indexData,
							  GLenum mode_, GLenum usage_ )
	: vbo( GenBufferObject< draw_vertex_t >( GL_ARRAY_BUFFER, vertexData, usage_ ) ),
	  ibo( GenBufferObject< GLuint >( GL_ELEMENT_ARRAY_BUFFER, indexData, GL_STATIC_DRAW ) ),
	  count( ( GLsizei ) indexData.size() ),
	  mode( mode_ ),
	  usage( usage_ )
{
}

draw_buffer_t::draw_buffer_t( draw_buffer_t&& buffer )
{
	*this = std::move( buffer );
}

draw_buffer_t::~draw_buffer_t( void )
{
	DeleteBufferObject( GL_ARRAY_BUFFER, vbo );
	DeleteBufferObject( GL_ELEMENT_ARRAY_BUFFER, ibo );
}

draw_buffer_t& draw_buffer_t::operator= ( draw_buffer_t&& buffer )
{
	if ( this != &buffer )
	{
		vbo = buffer.vbo;
		ibo = buffer.ibo;
		count = buffer.count;
		mode = buffer.mode;

		buffer.mode = 0;
		buffer.count = 0;
		buffer.ibo = 0;
		buffer.vbo = 0;
	}

	return *this;
}

void draw_buffer_t::Bind( void ) const
{
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, vbo ) );
}

void draw_buffer_t::Release( void ) const
{
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
}

void draw_buffer_t::ReallocVertices( const std::vector< draw_vertex_t >& vertexData )
{
	Bind();
	GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( draw_vertex_t ) * vertexData.size(), &vertexData[ 0 ], usage ) );
	Release();

	if ( !ibo )
	{
		count = vertexData.size();
	}
}

void draw_buffer_t::Update( const std::vector< draw_vertex_t >& vertexData, size_t vertexOffsetIndex ) const
{
	Bind();
	GL_CHECK( glBufferSubData( GL_ARRAY_BUFFER,
							   ( GLintptr )( vertexOffsetIndex * sizeof( draw_vertex_t ) ),
							   sizeof( draw_vertex_t ) * vertexData.size(),
							   &vertexData[ 0 ] ) );
	Release();
}

void draw_buffer_t::Render( const shader_program_t& program ) const
{
	Bind();
    shader_program_t::LoadAttribLayout< draw_vertex_t >( *this, program );

	if ( ibo )
	{
		GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo ) );
		GL_CHECK( glDrawElements( mode, count, GL_UNSIGNED_INT, 0 ) );
		GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );
	}
	else
	{
		GL_CHECK( glDrawArrays( mode, 0, count ) );
	}
	Release();
}

//-------------------------------------------------------------------------------------------------
// imm_draw_t
//-------------------------------------------------------------------------------------------------

std::unique_ptr< draw_buffer_t > imm_draw_t::buffer( nullptr );

imm_draw_t::imm_draw_t( const shader_program_t& prog )
	  : enabled( true ),
		lastSize( 0 ),
		program( prog )
{
}

void imm_draw_t::Begin( GLenum mode )
{
	if ( !enabled )
	{
		return;
	}

	if ( !buffer )
	{
		buffer.reset( new draw_buffer_t() );
		buffer->usage = GL_DYNAMIC_DRAW;
	}

	buffer->mode = mode;
}

void imm_draw_t::Vertex( const draw_vertex_t& v )
{
	if ( !enabled )
	{
		return;
	}

	vertices.push_back( v );
}

void imm_draw_t::Vertex( const glm::vec3& position )
{
	if ( !enabled )
	{
		return;
	}

	vertices.push_back( draw_vertex_t_Make( position ) );
}

void imm_draw_t::End( void )
{
	if ( !enabled )
	{
		return;
	}

	if ( lastSize != vertices.size() )
	{
		buffer->ReallocVertices( vertices );
		lastSize = vertices.size();
	}
	else
	{
		buffer->Update( vertices );
	}

	vertices.clear();
	buffer->Render( program );
}

void imm_draw_t::SetEnabled( bool value )
{
	enabled = value;
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

pipeline_t::pipeline_t( void )
	: vao( 0 )
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

		File_GetBuf( vertexBuf, shaderRootDir + def.vertexName );
		File_GetBuf( fragmentBuf, shaderRootDir + def.fragmentName );

		std::string prepend( OP_GLSL_SHADER_PREPEND );

		vertexBuf.insert( vertexBuf.begin(), prepend.c_str(), prepend.c_str() + prepend.length() );
		fragmentBuf.insert( fragmentBuf.begin(), prepend.c_str(), prepend.c_str() + prepend.length() );

		std::string vshader( &vertexBuf[ 0 ], vertexBuf.size() );
		std::string fshader( &fragmentBuf[ 0 ], fragmentBuf.size() );

		printf( "VERTEX: \n %s \n FRAGMENT: \n %s \n", vshader.c_str(), fshader.c_str() );

		programs[ def.programName ] = std::move( shader_program_t( vertexBuf, fragmentBuf, def.uniforms, def.attribs ) );
	}

	std::array< buffer_def_t, 2 > bufferDefs =
	{{

		 {
			"colored_cube",
			GL_TRIANGLES,
			GL_STATIC_DRAW,
			{
			 // back
				draw_vertex_t_Make( glm::vec3( 1.0f, 1.0f, 1.0f ) ),
				draw_vertex_t_Make( glm::vec3( -1.0f, 1.0f, 1.0f ) ),
				draw_vertex_t_Make( glm::vec3( -1.0f, -1.0f, 1.0f ) ),
				draw_vertex_t_Make( glm::vec3( 1.0f, -1.0f, 1.0f ) ),

			 // front
				draw_vertex_t_Make( glm::vec3( -1.0f, -1.0f, -1.0f ) ),
				draw_vertex_t_Make( glm::vec3( 1.0f, -1.0f, -1.0f ) ),
				draw_vertex_t_Make( glm::vec3( 1.0f, 1.0f, -1.0f ) ),
				draw_vertex_t_Make( glm::vec3( -1.0f, 1.0f, -1.0f ) )
			},

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
			}
		},

		// billboard
		{
			"billboard",
			GL_TRIANGLE_STRIP,
			GL_STATIC_DRAW,
			{
				draw_vertex_t_Make( glm::vec3( -1.0f, -1.0f, 0.0f ), glm::vec2( 0.0f, 0.0f ),  glm::u8vec4( 255 ) ),
				draw_vertex_t_Make( glm::vec3( 1.0f, -1.0f, 0.0f ), glm::vec2( 1.0f, 0.0f ), glm::u8vec4( 255 ) ),
				draw_vertex_t_Make( glm::vec3( -1.0f, 1.0f, 0.0f ), glm::vec2( 0.0f, 1.0f ), glm::u8vec4( 255 ) ),
				draw_vertex_t_Make( glm::vec3( 1.0f, 1.0f, 0.0f ), glm::vec2( 1.0f, 1.0f ), glm::u8vec4( 255 ) )
			},
			{}
        }
	}};

	for ( const buffer_def_t& def: bufferDefs )
	{
		if ( def.indexData.empty() )
		{
			drawBuffers[ def.name ] = std::move( draw_buffer_t( def.vertexData, def.mode, def.usage ) );
		}
		else
		{
			drawBuffers[ def.name ] = std::move( draw_buffer_t( def.vertexData, def.indexData, def.mode, def.usage ) );
		}
	}

#ifndef OP_GL_USE_ES
	GL_CHECK( glGenVertexArrays( 1, &vao ) );
	GL_CHECK( glBindVertexArray( vao ) );
#endif
}

pipeline_t::~pipeline_t( void )
{
#ifndef OP_GL_USE_ES
	if ( vao )
	{
		GL_CHECK( glDeleteVertexArrays( 1, &vao ) );
	}
#endif
}

imm_draw_t* immDrawer = nullptr;
