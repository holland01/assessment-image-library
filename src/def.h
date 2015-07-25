#pragma once
	
#if defined( _WIN32 )
#	include <Windows.h> // This needs to be before GLFW includes to prevent APIENTRY macro redef error
#	define GL_PROC APIENTRY
#elif defined( __GNUC__ ) && defined( __amd64__ )
#	define GL_PROC // leave blank: calling convention should be taken care of on this architecture
#else
#	define GL_PROC __attribute__( ( __cdecl ) ) // default to cdecl calling convention on 32-bit non-MSVC compilers
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_PURE

#define INLINE inline

#if defined( EMSCRIPTEN ) && defined( DEBUG )
#   define __DEBUG_RENDERER__
#endif // EMSCRIPTEN && DEBUG

// Windows.h defines these for us already
#if !defined( _WIN32 )
#	define TRUE 1 
#	define FALSE 0
static void __nop( void )
{}
#endif // _WIN32

#define UNUSEDPARAM( p ) ( ( void )( p ) )

#define CALL_MEM_FNPTR( obj, ptrMemFn )( ( obj ).*( ptrMemFn ) )

#define _DEBUG_USE_GL_GET_ERR
//#define AABB_MAX_Z_LESS_THAN_MIN_Z // quake 3 maps use this standard in their bounds computations/storage
	
