#include "renderer.h"
#include "view.h"
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
// Texture Utils
//-----------------------------------------------------------

enum texFormat_t
{
	TEX_EXTERNAL_R = GL_ALPHA,
	TEX_INTERNAL_R = GL_ALPHA,
	TEX_INTERNAL_RGB = GL_RGB,
	TEX_INTERNAL_RGBA = GL_RGBA,	
};

namespace rend {

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

		GL_CHECK( glGenerateMipmap( target ) );
		minFilter = GL_LINEAR_MIPMAP_LINEAR;
	}
	else
	{
		GL_CHECK( glTexImage2D( target, 
			0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, &pixels[ 0 ] ) );
	}
	Release();

	//LoadSettings();
}

void texture_t::LoadSettings( void )
{
	Bind();
	// For some reason setting this through SDL's GL ES context on the desktop (in Linux) causes really bad texture sampling to happen,
	// regardless of the value passed. WTF?!
//#if defined( EMSCRIPTEN ) || defined( _WIN32 )
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ) );
//#endif
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_WRAP_S, wrap ) );
	GL_CHECK( glTexParameteri( target, GL_TEXTURE_WRAP_T, wrap ) );
	
	Release();
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
// Program
//-------------------------------------------------------------------------------------------------
shader_program_t::shader_program_t( void )
    : program( 0 )
{
}

shader_program_t::shader_program_t( const std::string& vertexShader, const std::string& fragmentShader )
	: program( 0 )
{
	GLuint shaders[] = 
	{
		CompileShaderSource( vertexShader.c_str(), vertexShader.size(), GL_VERTEX_SHADER ),
		CompileShaderSource( fragmentShader.c_str(), fragmentShader.size(), GL_FRAGMENT_SHADER )
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

std::unique_ptr< billboard_t::draw_t > billboard_t::drawBuffer( nullptr );
std::unique_ptr< shader_program_t > billboard_t::program( nullptr );

billboard_t::billboard_t( const glm::vec3& origin_, const texture_t& image_ )
	: origin( origin_ ),
	  image( image_ )
{
	if ( !drawBuffer )
	{
		std::vector< draw_vertex_t > v =
		{
			draw_vertex_t_Make( glm::vec3( -1.0f, -1.0f, 0.0f ), glm::vec2( 0.0f, 0.0f ),  glm::u8vec4( 255 ) ),
			draw_vertex_t_Make( glm::vec3( 1.0f, -1.0f, 0.0f ), glm::vec2( 1.0f, 0.0f ), glm::u8vec4( 255 ) ),
			draw_vertex_t_Make( glm::vec3( -1.0f, 1.0f, 0.0f ), glm::vec2( 0.0f, 1.0f ), glm::u8vec4( 255 ) ),
			draw_vertex_t_Make( glm::vec3( 1.0f, 1.0f, 0.0f ), glm::vec2( 1.0f, 1.0f ), glm::u8vec4( 255 ) )
		};

		drawBuffer.reset( new draw_t( v ) );
	}

	if ( !program )
	{
		std::string vshader( GEN_V_SHADER(
			attribute vec3 position;
			attribute vec2 texCoord;
			attribute vec4 color;

			uniform vec3 origin;
			//uniform mat3 viewOrient;
			uniform mat4 modelToView;
			uniform mat4 viewToClip;

			varying vec4 frag_Color;
			varying vec2 frag_TexCoord;

			void main( void )
			{
				//mat3 orient = viewOrient;
				//orient[ 2 ] = -viewOrient[ 2 ];

				gl_Position = viewToClip * modelToView * vec4( origin + position, 1.0 );
				frag_Color = color;
				frag_TexCoord = texCoord;
			}
		) );

		std::string fshader( GEN_F_SHADER(
			varying vec4 frag_Color;
			varying vec2 frag_TexCoord;
			uniform sampler2D image;

			void main( void )
			{
				vec4 tex = texture2D( image, frag_TexCoord );
				gl_FragColor = frag_Color * tex;
			}
		) );

		program.reset( new shader_program_t( vshader,
											 fshader,
											{ "origin", "viewOrient", "modelToView", "viewToClip", "image" },
											{ "position", "texCoord", "color" } ) );
	}
}

billboard_t::~billboard_t( void )
{
}

void billboard_t::Render( const view::params_t& params )
{
	program->Bind();

	program->LoadMat4( "modelToView", params.transform );
	program->LoadMat4( "viewToClip", params.clipTransform );
	program->LoadVec3( "origin", origin );
	//program->LoadMat3( "viewOrient", glm::mat3( params.inverseOrient ) );

	image.Bind( 0, "image", *program );
	drawBuffer->Render( *program );
	image.Release();

	program->Release();
}

} // namespace glrend
