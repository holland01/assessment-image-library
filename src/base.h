#pragma once

#include "def.h"
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <limits>
#include <glm/glm.hpp>

extern void FlagExit( void ); // should be defined by the user in a different source file
extern float GetTime( void );

bool File_GetPixels( const std::string& filepath, 
	std::vector< uint8_t >& outBuffer, int32_t& outBpp, int32_t& outWidth, int32_t& outHeight );

void MyPrintf( const char* header, const char* fmt, ... );
void MyFprintf( FILE* f, const char* header, const char* fmt, ... );
void MyDateTime( const char* format, char* outBuffer, int32_t length );
void ExitOnGLError( int32_t line, const char* glFunc, const char* callerFunc );

template < typename type_t >
static INLINE void Vector_RemovePtr( std::vector< type_t >& v, const type_t& t );

template < typename type_t >
static INLINE bool Vector_Contains( const std::vector< type_t >& v, const type_t& t );

template < typename type_t >
static INLINE void Vector_InsertUnique( std::vector< type_t >& dest, const std::vector< type_t >& src );

template < class type_t >
static INLINE void Vector_InsertUnique( std::vector< type_t >& dest, const type_t& src );

template< typename type_t >
static INLINE bool operator == ( const std::weak_ptr< type_t >&a, const std::weak_ptr< type_t >& b );

template< typename type_t >
static INLINE bool operator != ( const std::weak_ptr< type_t >&a, const std::weak_ptr< type_t >& b );

template< typename type_t >
static INLINE bool operator < ( const std::weak_ptr< type_t >& a, const std::weak_ptr< type_t >& b );

template< typename type_t >
static INLINE type_t Math_Log4( const type_t& t );

#if defined(__GNUC__) || defined(__clang__)
#	if defined(__GNUC__)
#		define _FUNC_NAME_ __func__
#	else
#		define _FUNC_NAME_ __PRETTY_FUNCTION__
#	endif // __GNUC__
#	define _LINE_NUM_ __LINE__
#elif defined (_MSC_VER)
#	define _FUNC_NAME_ __FUNCTION__
#	define _LINE_NUM_ __LINE__
#else
#	error "Unsupported compiler found"
#endif // __GNUC__

#define MLOG_ERROR( ... )                                \
	do                                                      \
	{                                                       \
		puts("======== ERROR ========");                    \
		MyPrintf( ( _FUNC_NAME_ ), __VA_ARGS__ );                   \
		puts("=======================");                    \
        FlagExit();                                         \
	}                                                       \
	while( 0 )

#define MLOG_WARNING( ... )                              \
	do                                                      \
	{                                                       \
		puts("======== WARNING ========");                  \
		MyPrintf( ( _FUNC_NAME_ ), __VA_ARGS__ );                   \
		puts("=======================");                    \
	}                                                       \
	while( 0 )

#define MLOG_WARNING_SANS_FUNCNAME( title, ... )                              \
	do                                                      \
	{                                                       \
		puts("======== WARNING ========");                  \
		MyPrintf( ( title ), __VA_ARGS__ );                 \
		puts("=======================");                    \
	}                                                       \
	while( 0 )

#define MLOG_ASSERT( condition, ... )    \
	do                                      \
	{                                       \
		if ( !( condition ) )               \
		{                                   \
			MLOG_ERROR( __VA_ARGS__ );           \
		}                                   \
	}                                       \
	while( 0 )
	

#include "base.inl"
