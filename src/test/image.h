#pragma once

#include "../def.h"
#include APPLICATION_BASE_HEADER
#include "../img.h"
#include "../renderer.h"

struct image_test;
using image_app_t = application< image_test >;

struct image_test : public image_app_t
{
	// These all have a one-one map with each others
	std::vector< std::unique_ptr< texture > > mTextures;
	std::vector< glm::vec3 > mOrigins;
	std::vector< std::string > mTitles;
	std::vector< glm::ivec4 > mViewports;

	draw_text mDrawText;

	// Base class handles initialization of GL, screen initialization, etc.
	image_test( uint32_t width, uint32_t height )
		: image_app_t( width, height ),
		  mDrawText( *mPipeline ) // mPipeline is a parent member which holds all shader programs and vertex buffers being used
	{
		// We  have four examples, each of which use the same image: the well-known lena.
		// Two are greyscale, two are RGB. For both groups, one image is embossed, and the other is
		// its original

		const uint32_t numTextures = 2;
		mTextures.reserve( numTextures );

		uint32_t w = mWidth / 2;
		uint32_t h = mHeight / 2;

		img::rgb_f32_t rgb0 = load_image< float, img::color_format::rgb >( "asset/lena_rgb.jpg" );
		mTextures.push_back( make_texture( rgb0 ) );
		mOrigins.push_back( glm::vec3( 0.0f ) );
		mTitles.push_back( "Lena as RGB" );
		mViewports.push_back( glm::ivec4( 0, 0, w, h ) );

		img::greyscale_u8_t gs0 = load_image< uint8_t, img::color_format::greyscale >( "asset/lena.png" );
		mTextures.push_back( make_texture( gs0 ) );
		mOrigins.push_back( glm::vec3( 0.0f, 0.0f, 0.0f ) );
		mTitles.push_back( "Lena as Greyscale" );
		mViewports.push_back( glm::ivec4( w + mWidth % 2, 0, w, h ) );
	}

	template < typename channel_t, img::color_format format, typename int_t = int32_t >
	img::data< channel_t, format, int_t > load_image( const std::string& path )
	{
		using image_t = img::data< channel_t, format, int_t >;

		img::from_file_error e;
		image_t ii( img::from_file< image_t >( path, &e, true ) );
		MLOG_ASSERT( e == img::from_file_error::none, "img::from_file returned an error..." );
		return std::move( ii );
	}

	template < typename channel_t, img::color_format format, typename int_t = int32_t >
	std::unique_ptr< texture > make_texture( const img::data< channel_t, format, int_t >& image )
	{
		std::unique_ptr< texture > tex( new texture() );

		GLenum fmt, internalFmt;

		if ( format == img::color_format::rgb )
		{
			fmt = GL_RGB;
			internalFmt = GL_RGB8;
		}
		else
		{
			// ES 2 doesn't have GL_LUMINANCE, so we substitute it with GL_ALPHA in that scenario
			fmt = FORMAT_GREYSCALE;
			internalFmt = INTERNAL_FORMAT_GREYSCALE;
		}

		tex->bpp( ( size_t )format * sizeof( channel_t ) );
		tex->width( image.mWidth );
		tex->height( image.mHeight );
		tex->format( fmt );
		tex->internal_format( internalFmt );
		tex->min_filter( GL_LINEAR );
		tex->mag_filter( GL_LINEAR );

		img::raw_buffer buf = img::get_raw_pixels( image );

		GLenum type;
		if ( std::is_same< float, channel_t >::value )
			type = GL_FLOAT;
		else if ( std::is_same< uint32_t, channel_t >::value )
			type = GL_UNSIGNED_INT;
		else
			type = GL_UNSIGNED_BYTE;

		tex->buffer_type( type );
		tex->pixels( buf );
		tex->load_2d();

		return std::move( tex );
	}

	void draw_image( const std::string& title, const std::unique_ptr< texture >& texture_,
					 const glm::vec3& origin )
	{
		mDrawText.draw( title, glm::vec2( -0.5f, 0.9f ) );

		const shader_program& prog = mPipeline->program( "billboard" );
		const draw_buffer& dbuf = mPipeline->buffer( "billboard" );

		bind_program binder( prog );

		prog.load_mat3( "viewOrient", glm::mat3( 1.0f ) );
		prog.load_vec3( "origin", origin );
		prog.load_mat4( "modelToView",
						mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), origin ) );
		prog.load_vec4( "color", glm::vec4( 1.0f ) );

		texture_->bind( 0, "image", prog );
		dbuf.render( prog );
		texture_->release();

		UNUSEDPARAM( binder );
	}

	void draw( void ) override
	{
		for (uint32_t i = 0; i < mTextures.size(); ++i)
		{
			GL_CHECK( glViewport( mViewports[ i ].x, mViewports[ i ].y,
								  mViewports[ i ].z, mViewports[ i ].w ) );
			draw_image( mTitles[ i ], mTextures[ i ], mOrigins[ i ] );
			image_app_t::draw();
		}
	}

	void frame( void ) override
	{
		image_app_t::update();
		draw();
	}
};

