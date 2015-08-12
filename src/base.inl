template < typename T >
static INLINE bool File_GetBuf( std::vector< T >& outBuffer, const std::string& fpath )
{
	FILE* f = fopen( fpath.c_str(), "rb" );
	if ( !f )
	{
		return false;
	}

	fseek( f, 0, SEEK_END );
	size_t count = ftell( f ) / sizeof( T );
	fseek( f, 0, SEEK_SET );

	outBuffer.resize( count, 0 );
    fread( &outBuffer[ 0 ], sizeof( T ), count, f );
	fclose( f );

	return true;
}

static INLINE size_t File_GetExt( std::string& outExt, const std::string& filename  )
{
	// Second condition is to ensure we actually have a file extension we can use
	size_t index;
	if ( ( index = filename.find_last_of( '.' ) ) != std::string::npos && index != filename.size() - 1 )
	{
		outExt = filename.substr( index + 1 );
	}
	return index;
}

static INLINE void Pixels_24BitTo32Bit( uint8_t* destination, const uint8_t* source, int32_t length )
{
	for ( int32_t i = 0; i < length; ++i )
	{
		destination[ i * 4 + 0 ] = source[ i * 3 + 0 ];
		destination[ i * 4 + 1 ] = source[ i * 3 + 1 ];
		destination[ i * 4 + 2 ] = source[ i * 3 + 2 ];
	}
}

template < typename T >
static INLINE void Vector_RemovePtr( std::vector< T >& v, const T& t )
{
    static_assert( std::is_pointer< T >::value, "Vector_RemovePtr can only be called on vectors storing a pointer to object type" );

    auto del = [ &t ]( T& p )
    {
        if ( t == p )
        {
            p = nullptr;
        }
    };

    std::for_each( v.begin(), v.end(), del );
    auto beginRange = std::remove( v.begin(), v.end(), static_cast< T >( nullptr ) );
    v.erase( beginRange, v.end() );

}

template < typename T >
static INLINE bool Vector_Contains( const std::vector< T >& v, const T& t )
{
    for ( const T& e: v )
    {
        if ( e == t )
        {
            return true;
        }
    }

    return false;
}

template < typename T >
static INLINE void Vector_InsertUnique( std::vector< T >& dest, const std::vector< T >& src )
{
    for ( T e: src )
    {
        if ( !Vector_Contains< T >( dest, e ) )
        {
            dest.push_back( e );
        }
    }
}
