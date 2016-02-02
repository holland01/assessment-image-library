#pragma once

#include "../def.h"
#include APPLICATION_BASE_HEADER
#include "../img.h"
#include "../renderer.h"

struct image_test;
using image_app_t = application< image_test >;

struct image_test : public image_app_t
{
	template < typename channel_t >
	struct test_bundle
	{
		draw_text* mDrawText;

		const image_test& mImageTest;

		// These all have a one-one map with each other
		std::vector< std::unique_ptr< texture > > mTextures;
		std::vector< glm::vec3 > mOrigins;
		std::vector< std::string > mTitles;
		std::vector< glm::ivec4 > mViewports;

		static const bool IS_FLOAT = std::is_same< float, channel_t >::value;
		static const bool DO_KERNEL_NORMALIZE = true;

		using image_rgb_t = img::data< channel_t, img::color_format::rgb >;
		using image_greyscale_t = img::data< channel_t, img::color_format::greyscale >;

		glm::tmat3x3< channel_t > KERNEL;

		const std::string mTitle;

		test_bundle( const std::string& title, const image_test& parent )
			: mDrawText( nullptr ),
			  mImageTest( parent ),
			  mTitle( title )
		{

			glm::mat3 kernel( img::emboss_f32( DO_KERNEL_NORMALIZE ) );

			// We  have four examples, each of which use the same image: the lovely and all-too-well-known lena.
			// Two are greyscale, two are RGB. For both groups, one image is embossed, and the other is
			// its original

			const uint32_t numTextures = 4;
			mTextures.reserve( numTextures );
			mOrigins.reserve( numTextures );
			mTitles.reserve( numTextures );
			mViewports.reserve( numTextures );

			uint32_t w = mImageTest.mWidth / 2;
			uint32_t h = mImageTest.mHeight / 2;

			image_rgb_t rgb0 = load_image< img::color_format::rgb >( "asset/lena_rgb.jpg" );
			mTextures.push_back( make_texture( rgb0 ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mTitles.push_back( "RGB" );
			mViewports.push_back( glm::ivec4( 0, 0, w, h ) );

			mTextures.push_back( make_texture( img::apply_kernel( rgb0, kernel ) ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mTitles.push_back( "EMBOSS RGB" );
			mViewports.push_back( glm::ivec4( 0, h, w, h ) );

			image_greyscale_t gs0 = load_image< img::color_format::greyscale >( "asset/lena.png" );
			mTextures.push_back( make_texture( gs0 ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mTitles.push_back( "GREYSCALE" );
			mViewports.push_back( glm::ivec4( w + mImageTest.mWidth % 2, 0, w, h ) );

			mTextures.push_back( make_texture( img::apply_kernel( gs0, kernel ) ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mTitles.push_back( "EMBOSS GREYSCALE" );
			mViewports.push_back( glm::ivec4( w + mImageTest.mWidth % 2, h, w, h ) );
		}

		template < img::color_format format, typename int_t = int32_t >
		img::data< channel_t, format, int_t > load_image( const std::string& path )
		{
			using image_t = img::data< channel_t, format, int_t >;

			img::from_file_error e;
			image_t ii( img::from_file< image_t >( path, &e, true ) );
			MLOG_ASSERT( e == img::from_file_error::none, "img::from_file returned an error..." );
			return std::move( ii );
		}

		template < img::color_format format, typename int_t = int32_t >
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
			mDrawText->draw( title, glm::vec2( -0.5f, 0.9f ) );

			const shader_program& prog = mImageTest.pipeline().program( "billboard" );
			const draw_buffer& dbuf = mImageTest.pipeline().buffer( "billboard" );

			bind_program binder( prog );

			prog.load_mat3( "viewOrient", glm::mat3( 1.0f ) );
			prog.load_vec3( "origin", origin );
			prog.load_mat4( "modelToView",
							mImageTest.mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), origin ) );
			prog.load_vec4( "color", glm::vec4( 1.0f ) );

			texture_->bind( 0, "image", prog );
			dbuf.render( prog );
			texture_->release();

			UNUSEDPARAM( binder );
		}

		void draw( void )
		{
			for (uint32_t i = 0; i < mTextures.size(); ++i)
			{
				GL_CHECK( glViewport( mViewports[ i ].x, mViewports[ i ].y,
									  mViewports[ i ].z, mViewports[ i ].w ) );
				draw_image( mTitles[ i ], mTextures[ i ], mOrigins[ i ] );
			}

			GL_CHECK( glViewport( 0, 0, mImageTest.mWidth, mImageTest.mHeight ) );
			mDrawText->draw( mTitle, glm::vec2( -0.5f, 0.0f ) );
		}
	};

	friend struct test_bundle< float >;
	friend struct test_bundle< uint8_t >;

	draw_text mDrawText;

	std::unique_ptr< test_bundle< float > > mF32;
	std::unique_ptr< test_bundle< uint8_t > > mU8;

	bool mUseF32;

	// Base class handles initialization of GL, screen initialization, etc.
	image_test( uint32_t width, uint32_t height )
		: image_app_t( width, height ),
		  mDrawText( *mPipeline ),
		  mF32( new test_bundle< float >( "FLOAT", *this ) ),
		  mU8( new test_bundle< uint8_t >( "BYTE", *this ) ), // mPipeline is a parent member which holds all shader programs and vertex buffers being used
		  mUseF32( true )
	{
		mCamPtr->position( glm::vec3( 0.0f, 0.0f, 2.0f ) );

		mF32->mDrawText = &mDrawText;
		mU8->mDrawText = &mDrawText;

		mDrawAxes = false;
		mDrawHUD = false;
	}

	void draw( void ) override
	{
		if ( mUseF32 )
			mF32->draw();
		else
			mU8->draw();
		image_app_t::draw();
	}

	void frame( void ) override
	{
		image_app_t::update();
		draw();
	}

	void handle_event( const SDL_Event& e )
	{
		image_app_t::handle_event( e );

		if ( e.type == SDL_KEYDOWN )
			if ( e.key.keysym.sym == SDLK_UP )
				mUseF32 = !mUseF32;
	}
};
