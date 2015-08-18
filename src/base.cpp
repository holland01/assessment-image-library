
#include "base.h"
#include "def.h"
#include "opengl.h"

#include <stb_image.h>

#include <time.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

void MyPrintf( const char* header, const char* fmt, ... )
{
    va_list arg;

    va_start( arg, fmt );
    fprintf( stdout, "\n[ %s ]: {\n\n", header );
    vfprintf( stdout, fmt, arg );
    fprintf( stdout, "\n\n}\n\n" );
    va_end( arg );
}

void MyFprintf( FILE* f, const char* header, const char* fmt, ... )
{
    va_list arg;

    va_start( arg, fmt );
    fprintf( f, "\n[ %s ]: {\n\n", header );
    vfprintf( f, fmt, arg );
    fprintf( f, "\n\n}\n\n" );
    va_end( arg );
}

void MyDateTime( const char* format, char* outBuffer, int32_t length )
{
    time_t timer;
    struct tm* info;

    time( &timer );

    info = localtime( &timer );

    strftime( outBuffer, length, format, info );
}

void ExitOnGLError( int32_t line, const char* glFunc, const char* callerFunc )
{
    GLenum error = glGetError();

    if ( GL_NO_ERROR != error )
    {

#ifndef OP_GL_USE_ES
        const char* errorString = ( const char* ) gluErrorString( error );
#else
        const char* errorString = "Unavailable";
#endif

        MyPrintf( "GL ERROR", "%s -> [ %s ( %i ) ]: \'0x%x\' => %s", callerFunc, glFunc, line, error, errorString );
        FlagExit();
    }
}

bool File_GetPixels( const std::string& filepath, 
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
	memcpy( &outBuffer[ 0 ], imagePixels, outBuffer.size() ); 

	stbi_image_free( imagePixels );

	return true;
}
