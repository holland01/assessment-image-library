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