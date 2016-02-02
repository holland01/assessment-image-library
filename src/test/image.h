#pragma once

#include "../def.h"
#include APPLICATION_BASE_HEADER
#include "../img.h"
#include "../renderer.h"

struct image_test;
using image_app_t = application< image_test >;

struct image_test : public image_app_t
{

	// Renders a group of images; channel_t corresponds to the type
	// which the image data is based off of. In this case, we use either 8 bits per channel,
	// or a 32 float bits per channel.
	template < typename channel_t >
	struct test_bundle
	{
		draw_text* mDrawText;

		const image_test& mImageTest;

		// These all have a one-one map with each other
		std::vector< std::unique_ptr< texture > > mTextures;
		std::vector< glm::vec3 > mOrigins;
		std::vector< std::string > mSubTitles;
		std::vector< glm::ivec4 > mViewports;

		static const bool IS_FLOAT = std::is_same< float, channel_t >::value;
		static const bool DO_KERNEL_NORMALIZE = true;

		// Some convenience typedefs
		using image_rgb_t = img::data< channel_t, img::color_format::rgb >;
		using image_greyscale_t = img::data< channel_t, img::color_format::greyscale >;

		// Regardless of type, all kernel computations are computed
		// as floats; this is because precision is nice. A fixed point
		// alternative could definitely be provided, but that would take more time...
		glm::tmat3x3< float > KERNEL;

		// The title of the bundle itself; either "byte" or "float", depending on the current group we're displaying.
		const std::string mTitle;

		test_bundle( const std::string& title, const image_test& parent )
			: mDrawText( nullptr ),
			  mImageTest( parent ),
			  mTitle( title )
		{

			glm::mat3 kernel( img::kernel_emboss( DO_KERNEL_NORMALIZE ) );

			// We  have four examples, each of which use the same image: the lovely and all-too-well-known lena.
			// Two are greyscale, two are RGB. For both groups, one image is embossed, and the other is
			// its original. Each image has its own viewport, so we split the screen into four sections.

			const uint32_t numTextures = 4;
			mTextures.reserve( numTextures );
			mOrigins.reserve( numTextures );
			mSubTitles.reserve( numTextures );
			mViewports.reserve( numTextures );

			uint32_t w = mImageTest.mWidth / 2;
			uint32_t h = mImageTest.mHeight / 2;

			image_rgb_t rgb0 = load_image< img::color_format::rgb >( "asset/lena_rgb.jpg" );
			mTextures.push_back( make_texture( rgb0 ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mSubTitles.push_back( "RGB" );
			mViewports.push_back( glm::ivec4( 0, 0, w, h ) );

			mTextures.push_back( make_texture( img::apply_kernel( rgb0, kernel ) ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mSubTitles.push_back( "EMBOSS RGB" );
			mViewports.push_back( glm::ivec4( 0, h, w, h ) );

			image_greyscale_t gs0 = load_image< img::color_format::greyscale >( "asset/lena.png" );
			mTextures.push_back( make_texture( gs0 ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mSubTitles.push_back( "GREYSCALE" );
			mViewports.push_back( glm::ivec4( w + mImageTest.mWidth % 2, 0, w, h ) );

			mTextures.push_back( make_texture( img::apply_kernel( gs0, kernel ) ) );
			mOrigins.push_back( glm::vec3( 0.0f ) );
			mSubTitles.push_back( "EMBOSS GREYSCALE" );
			mViewports.push_back( glm::ivec4( w + mImageTest.mWidth % 2, h, w, h ) );
		}

		// Helper image loader; performs some weak error checking in the process.
		template < img::color_format format, typename int_t = int32_t >
		img::data< channel_t, format, int_t > load_image( const std::string& path )
		{
			using image_t = img::data< channel_t, format, int_t >;

			img::from_file_error e;
			image_t ii( img::from_file< image_t >( path, &e, true ) );
			MLOG_ASSERT( e == img::from_file_error::none, "img::from_file returned an error..." );
			return std::move( ii );
		}

		// Here we make a texture which is rendered to a quad using OpenGL and a corresponding image.
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
				// Technically speaking, this is irrelevant, because there is no ES 2 build for this
				// test in particular. But, it explains why we're using FORMAT_GREYSCALE and its
				// internal counterpart...
				fmt = FORMAT_GREYSCALE;
				internalFmt = INTERNAL_FORMAT_GREYSCALE;
			}

			// Define a few properties
			tex->bpp( ( size_t )format * sizeof( channel_t ) );
			tex->width( image.mWidth );
			tex->height( image.mHeight );
			tex->format( fmt );
			tex->internal_format( internalFmt );
			tex->min_filter( GL_LINEAR );
			tex->mag_filter( GL_LINEAR );

			// Our texture API expects its data as set of raw bytes.
			// If we're using floats in our image, we just perform the conversion
			// and return the buffer.
			img::raw_buffer buf = img::get_raw_pixels( image );

			// use the greatness of compile time metaprogramming to make decisions.
			// (side note: Both D and Rust have superior metaprogramming syntax which is cleaner...however,
			// I've never used either).
			GLenum type;
			if ( IS_FLOAT )
				type = GL_FLOAT;
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
			// Text is drawn in screen space (i.e., x and y are in the range [-1, 1]), so the text to be drawn is basically
			// towards the upper left domain of the viewport
			mDrawText->draw( title, glm::vec2( -0.5f, 0.9f ) );

			// Technically, we're not drawing billboards: this shader program's
			// been edited specifically for this purposes of this demo (branching is a wonderful thing).
			const shader_program& prog = mImageTest.pipeline().program( "billboard" );

			// draw_buffer is basically just a glorified wrapper over a GL vertex buffer object
			const draw_buffer& dbuf = mImageTest.pipeline().buffer( "billboard" );

			// RAII FTW
			bind_program binder( prog );

			prog.load_mat3( "viewOrient", glm::mat3( 1.0f ) );
			prog.load_vec3( "origin", origin );
			prog.load_mat4( "modelToView",
							mImageTest.mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), origin ) );
			prog.load_vec4( "color", glm::vec4( 1.0f ) );

			texture_->bind( 0, "image", prog );
			dbuf.render( prog );
			texture_->release();

			// /compiler complaint
			UNUSEDPARAM( binder );
		}

		void draw( void )
		{
			// Draw the images...
			for (uint32_t i = 0; i < mTextures.size(); ++i)
			{
				GL_CHECK( glViewport( mViewports[ i ].x, mViewports[ i ].y,
									  mViewports[ i ].z, mViewports[ i ].w ) );
				draw_image( mSubTitles[ i ], mTextures[ i ], mOrigins[ i ] );
			}

			// Restore our viewport so we can render the main title across all subsections.
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

	// Base class image_app_t handles initialization of GL, screen initialization, etc.
	image_test( uint32_t width, uint32_t height )
		: image_app_t( width, height ),
		  mDrawText( *mPipeline ),
		  mF32( new test_bundle< float >( "FLOAT", *this ) ),
		  mU8( new test_bundle< uint8_t >( "BYTE", *this ) ), // mPipeline is a parent member which holds all shader programs and vertex buffers being used
		  mUseF32( true )
	{
		// Start directly facing images
		mCamPtr->position( glm::vec3( 0.0f, 0.0f, 2.0f ) );

		mF32->mDrawText = &mDrawText;
		mU8->mDrawText = &mDrawText;

		// We turn these off because it's distracting from the ponint of the demo.
		mDrawAxes = false;
		mDrawHUD = false;
	}

	// Main draw function
	void draw( void ) override
	{
		if ( mUseF32 )
			mF32->draw();
		else
			mU8->draw();
		image_app_t::draw();
	}

	// A frame function is nice for flexibility...
	void frame( void ) override
	{
		image_app_t::update();
		draw();
	}

	// Pretty simple here; see application.h if you're curious about the internals.
	void handle_event( const SDL_Event& e )
	{
		image_app_t::handle_event( e );

		// Press UP to switch from rendering float channel images
		// to 8bit channel images, and vice-versa
		if ( e.type == SDL_KEYDOWN )
			if ( e.key.keysym.sym == SDLK_UP )
				mUseF32 = !mUseF32;
	}
};
