
#include "base.h"
#include "def.h"
#include "opengl.h"

#include <stb_image.h>

#include <time.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

void stdoutf( const char* header, const char* fmt, ... )
{
    va_list arg;

    va_start( arg, fmt );
    fprintf( stdout, "\n[ %s ]: {\n\n", header );
    vfprintf( stdout, fmt, arg );
    fprintf( stdout, "\n\n}\n\n" );
    va_end( arg );
}

void fstdoutf( FILE* f, const char* header, const char* fmt, ... )
{
    va_list arg;

    va_start( arg, fmt );
    fprintf( f, "\n[ %s ]: {\n\n", header );
    vfprintf( f, fmt, arg );
    fprintf( f, "\n\n}\n\n" );
    va_end( arg );
}

void get_datetime( const char* format, char* outBuffer, int32_t length )
{
    time_t timer;
    struct tm* info;

    time( &timer );

    info = localtime( &timer );

    strftime( outBuffer, length, format, info );
}

void exit_on_gl_error( int32_t line, const char* glFunc, const char* callerFunc )
{
	static bool fetched = false;
    GLenum error = glGetError();

	if ( GL_NO_ERROR != error && !fetched )
    {

#ifndef OP_GL_USE_ES
        const char* errorString = ( const char* ) gluErrorString( error );
#else
        const char* errorString = "Unavailable";
#endif

		fetched = true;
        stdoutf( "GL ERROR", "%s -> [ %s ( %i ) ]: \'0x%x\' => %s", callerFunc, glFunc, line, error, errorString );
        flag_exit();
    }
}

bool file_get_pixels( const std::string& filepath, 
	std::vector< uint8_t >& outBuffer, int32_t& outBpp, int32_t& outWidth, int32_t& outHeight )
{
	// Load image
	// Need to also flip the image, since stbi loads pointer to upper left rather than lower left (what OpenGL expects)
    uint8_t* imagePixels = stbi_load( filepath.c_str(), &outWidth, &outHeight, &outBpp, STBI_default );

	if ( !imagePixels )
	{
		MLOG_WARNING( "No file found for \'%s\'", filepath.c_str() );
		return false;
	}
	
    outBuffer.resize( outWidth * outHeight * outBpp, 255 );

	// Flip image....
	for ( int32_t y = 0; y < outHeight; ++y )
	{
		for ( int32_t x = 0; x < outWidth; ++x )
		{
			memcpy( &outBuffer[ outBpp * ( y * outWidth + x ) ],
				&imagePixels[ outBpp * ( ( outHeight - 1 - y ) * outWidth + x ) ],
					outBpp * sizeof( uint8_t )) ;
		}
	}
		//memcpy( &outBuffer[ 0 ], imagePixels, outBuffer.size() );

	stbi_image_free( imagePixels );

	return true;
}

namespace {
    std::random_device gRandDevice;

    std::mt19937 gRandEngine( gRandDevice() );
}

glm::vec4 rand_color( float min, float max, float alpha )
{
    std::uniform_real_distribution< float > color( min, max );

    glm::vec4 c( color( gRandEngine ),
                 color( gRandEngine ),
                 color( gRandEngine ),
                 alpha );

    return std::move( c );
}

