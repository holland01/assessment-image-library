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
	return std::log10( t ) * type_t( 1.602059991 );
}

// 1337 C++ h4x stolen from here, http://baptiste-wicht.com/posts/2015/07/simulate-static_if-with-c11c14.html,
// which in turn was (of course) stolen from the boost mailing list :)
namespace static_if_detail {

struct identity {
	template<typename T>
	T operator()(T&& x) const {
		return std::forward<T>(x);
	}
};

template<bool Cond>
struct statement {
	template<typename F>
	void then(const F& f){
		f(identity());
	}

	template<typename F>
	void else_(const F&){}
};

template<>
struct statement<false> {
	template<typename F>
	void then(const F&){}

	template<typename F>
	void else_(const F& f){
		f(identity());
	}
};

} //end of namespace static_if_detail

template<bool Cond, typename F>
static_if_detail::statement<Cond> static_if(F const& f){
	static_if_detail::statement<Cond> if_;
	if_.then(f);
	return if_;
}
