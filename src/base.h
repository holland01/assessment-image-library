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

template < typename type_t >
static INLINE bool file_get_buf( std::vector< type_t >& outBuffer, const std::string& fpath )
{
    FILE* f = fopen( fpath.c_str(), "rb" );
    if ( !f )
    {
        return false;
    }

    fseek( f, 0, SEEK_END );
    size_t count = ftell( f ) / sizeof( type_t );
    fseek( f, 0, SEEK_SET );

    outBuffer.resize( count, 0 );
    fread( &outBuffer[ 0 ], sizeof( type_t ), count, f );
    fclose( f );

    return true;
}

static INLINE size_t file_get_ext( std::string& outExt, const std::string& filename  )
{
    // Second condition is to ensure we actually have a file extension we can use
    size_t index;
    if ( ( index = filename.find_last_of( '.' ) ) != std::string::npos && index != filename.size() - 1 )
    {
        outExt = filename.substr( index + 1 );
    }
    return index;
}

static INLINE void pixels_24to32( uint8_t* destination, const uint8_t* source, int32_t length )
{
    for ( int32_t i = 0; i < length; ++i )
    {
        destination[ i * 4 + 0 ] = source[ i * 3 + 0 ];
        destination[ i * 4 + 1 ] = source[ i * 3 + 1 ];
        destination[ i * 4 + 2 ] = source[ i * 3 + 2 ];
    }
}

template < typename type_t >
static INLINE void vector_remove_ptr( std::vector< type_t >& v, const type_t& t )
{
    static_assert( std::is_pointer< type_t >::value, "Vector_RemovePtr can only be called on vectors storing a pointer to object type" );

    auto del = [ &t ]( type_t& p )
    {
        if ( t == p )
        {
            p = nullptr;
        }
    };

    std::for_each( v.begin(), v.end(), del );
    auto beginRange = std::remove( v.begin(), v.end(), static_cast< type_t >( nullptr ) );
    v.erase( beginRange, v.end() );
}

template < typename type_t >
static INLINE bool vector_contains( const std::vector< type_t >& v, const type_t& t )
{
    for ( const type_t& e: v )
    {
        if ( e == t )
        {
            return true;
        }
    }

    return false;
}

template < typename type_t >
static INLINE void vector_insert_unique( std::vector< type_t >& dest, const std::vector< type_t >& src )
{
    for ( type_t e: src )
    {
        if ( !vector_contains< type_t >( dest, e ) )
        {
            dest.push_back( e );
        }
    }
}

template < typename type_t >
static INLINE void vector_insert_unique( std::vector< type_t >& dest, const type_t& src )
{
    if ( !vector_contains( dest, src ) )
    {
        dest.push_back( src );
    }
}

template < typename type_t, typename predicate_t >
static INLINE void vector_transfer_if( std::vector< type_t >& dest, std::vector< type_t >& src, predicate_t p )
{
    for ( auto i = src.begin(); i != src.end(); )
    {
        if ( p( *i ) )
        {
            dest.push_back( *i );
            src.erase( i );
        }
        else
        {
            ++i;
        }
    }
}

template < typename type_t, typename predicate_t >
static INLINE void vector_erase_if( std::vector< type_t >& src, predicate_t p )
{
    for ( auto i = src.begin(); i != src.end(); )
    {
        type_t& erasure = *i;

        if ( p( erasure ) )
        {
            src.erase( i );
        }
        else
        {
            ++i;
        }
    }
}

template< typename type_t >
static INLINE bool operator == ( const std::weak_ptr< type_t >&a, const std::weak_ptr< type_t >& b )
{
    return !a.owner_before( b ) && !b.owner_before( a );
}

template< typename type_t >
static INLINE bool operator != ( const std::weak_ptr< type_t >&a, const std::weak_ptr< type_t >& b )
{
    return !( a == b );
}

template< typename type_t >
static INLINE bool operator < ( const std::weak_ptr< type_t >& a, const std::weak_ptr< type_t >& b )
{
    return a.owner_before( b );
}

template< typename type_t >
static INLINE type_t log4( const type_t& t )
{
    static_assert( std::numeric_limits< type_t >::is_iec559, "Math_Log4 is called with integral type; type required is floating point" );
    return glm::log( t ) * type_t( 1.602059991 );
}
