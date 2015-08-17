template < typename type_t >
static INLINE bool File_GetBuf( std::vector< type_t >& outBuffer, const std::string& fpath )
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

template < typename type_t >
static INLINE void Vector_RemovePtr( std::vector< type_t >& v, const type_t& t )
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
static INLINE bool Vector_Contains( const std::vector< type_t >& v, const type_t& t )
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
static INLINE void Vector_InsertUnique( std::vector< type_t >& dest, const std::vector< type_t >& src )
{
    for ( type_t e: src )
    {
        if ( !Vector_Contains< type_t >( dest, e ) )
        {
            dest.push_back( e );
        }
    }
}

template < typename type_t >
static INLINE void Vector_InsertUnique( std::vector< type_t >& dest, const type_t& src )
{
    if ( !Vector_Contains( dest, src ) )
    {
        dest.push_back( src );
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
