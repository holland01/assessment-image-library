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

extern void flag_exit( void ); // should be defined by the user in a different source file
extern float get_time( void );

bool file_get_pixels( const std::string& filepath,
	std::vector< uint8_t >& outBuffer, int32_t& outBpp, int32_t& outWidth, int32_t& outHeight );

void stdoutf( const char* header, const char* fmt, ... );
void fstdoutf( FILE* f, const char* header, const char* fmt, ... );
void get_datetime( const char* format, char* outBuffer, int32_t length );
void exit_on_gl_error( int32_t line, const char* glFunc, const char* callerFunc );

glm::vec4 rand_color( float min = 0.5f, float max = 1.0f, float alpha = 1.0f );

template < typename type_t >
static INLINE void vector_remove_ptr( std::vector< type_t >& v, const type_t& t );

template < typename type_t >
static INLINE bool vector_contains( const std::vector< type_t >& v, const type_t& t );

template < typename type_t >
static INLINE void vector_insert_unique( std::vector< type_t >& dest, const std::vector< type_t >& src );

template < typename type_t >
static INLINE void vector_insert_unique( std::vector< type_t >& dest, const type_t& src );

template < typename type_t, typename predicate_t >
static INLINE void vector_transfer_if( std::vector< type_t >& dest, std::vector< type_t >& src, predicate_t p );

template < typename type_t, typename predicate_t >
static INLINE void vector_erase_if( std::vector< type_t >& src, predicate_t p );

template< typename type_t >
static INLINE bool operator == ( const std::weak_ptr< type_t >&a, const std::weak_ptr< type_t >& b );

template< typename type_t >
static INLINE bool operator != ( const std::weak_ptr< type_t >&a, const std::weak_ptr< type_t >& b );

template< typename type_t >
static INLINE bool operator < ( const std::weak_ptr< type_t >& a, const std::weak_ptr< type_t >& b );

template< typename type_t >
static INLINE type_t log4( const type_t& t );

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
        stdoutf( ( _FUNC_NAME_ ), __VA_ARGS__ );                   \
		puts("=======================");                    \
        flag_exit();                                         \
	}                                                       \
	while( 0 )

#define MLOG_WARNING( ... )                              \
	do                                                      \
	{                                                       \
		puts("======== WARNING ========");                  \
        stdoutf( ( _FUNC_NAME_ ), __VA_ARGS__ );                   \
		puts("=======================");                    \
	}                                                       \
	while( 0 )

#define MLOG_WARNING_SANS_FUNCNAME( title, ... )                              \
	do                                                      \
	{                                                       \
		puts("======== WARNING ========");                  \
        stdoutf( ( title ), __VA_ARGS__ );                 \
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
