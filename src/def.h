#pragma once
	
#if defined( _WIN32 )
#	include <Windows.h> // This needs to be before GLFW includes to prevent APIENTRY macro redef error
#	define GL_PROC APIENTRY
#elif defined( __GNUC__ ) && defined( __amd64__ )
#	define GL_PROC // leave blank: calling convention should be taken care of on this architecture
#else
#	define GL_PROC __attribute__( ( __cdecl ) ) // default to cdecl calling convention on 32-bit non-MSVC compilers
#endif

#ifdef NDEBUG
#   define APPLICATION_BASE_HEADER "application.h"
#else
#   define APPLICATION_BASE_HEADER "debug_app.h"
#endif // NDEBUG

#define GLM_FORCE_RADIANS

#ifdef EMSCRIPTEN
#	define GLM_FORCE_PURE
#	define OP_CARRIAGE_RETURN "\n"
#   define OP_PHYSICS_DT ( 5.0f / 30.0f )
#else
#	define OP_CARRIAGE_RETURN "\r"
#   define OP_PHYSICS_DT ( 1.0f / 60.0f )
#endif

#define INLINE inline

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE INLINE
#endif // _MSCVER

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

	
