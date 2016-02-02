#pragma once

#include "def.h"
#include "base.h"
#include "opengl.h"

#include <array>
#include <tuple>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <functional>
#include <algorithm>
#include <memory>

#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define UBO_TRANSFORMS_BLOCK_BINDING 0
#define ATTRIB_OFFSET( type, member )( ( void* ) offsetof( type, member ) ) 

// Extensions
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

#ifdef __DEBUG_RENDERER__
#   define GL_CHECK( expr )\
        do\
        {\
            ( expr );\
            exit_on_gl_error( _LINE_NUM_, #expr, _FUNC_NAME_ );\
        }\
        while ( 0 )
#   define GL_CHECK_WITH_NAME( expr, funcname )\
        do\
        {\
            ( expr );\
            exit_on_gl_error( _LINE_NUM_, #expr, funcname );\
        }\
        while ( 0 )
#else
#   define GL_CHECK( expr ) ( expr )
#   define GL_CHECK_WITH_NAME( expr, funcname ) ( expr )
#endif // __DEBUG_RENDERER__


struct view_data;


struct draw_buffer;
class shader_program;

//---------------------------------------------------------------------
// Util Functions
//---------------------------------------------------------------------

void bind_texture( GLenum target,
                  GLuint handle, int32_t offset, const std::string& uniform, const shader_program& program );
	
template < typename T >
static INLINE GLuint make_vertex_buffer( GLenum target, const std::vector< T >& data, GLenum usage );

template < typename T, size_t count >
static INLINE GLuint make_vertex_buffer( GLenum target, const std::array< T, count >& data, GLenum usage );

template < typename T >
static INLINE void update_vertex_buffer( GLenum target, GLuint obj, GLuint offset, const std::vector< T >& data, bool bindUnbind );

static INLINE void free_vertex_buffer( GLenum target, GLuint obj );

static INLINE uint32_t get_max_mip_level_2d( int32_t baseWidth, int32_t baseHeight );

template< typename texture_helper_t >
static INLINE uint32_t make_mipmaps_2d( const texture_helper_t& tex,
                                                int32_t baseWidth, int32_t baseHeight, int32_t maxLevels );
//---------------------------------------------------------------------
// POD types
//---------------------------------------------------------------------

template < typename pos_type_t, typename normal_type_t, typename tex_coord_type_t, typename color_type_t >
struct draw_vertex_tmpl
{
    pos_type_t mPosition;
    normal_type_t mNormal;
    tex_coord_type_t mTexCoord;
    color_type_t mColor;
};

using draw_vertex_t = draw_vertex_tmpl< float[ 3 ], float[ 3 ], float[ 2 ], uint8_t[ 4 ] >;

static INLINE draw_vertex_t make_draw_vertex( const glm::vec3& position, const glm::vec2& texCoord, const glm::u8vec4& color );
static INLINE draw_vertex_t make_draw_vertex( const glm::vec3& position, const glm::u8vec4& color );
static INLINE draw_vertex_t make_draw_vertex( const glm::vec3& position );

struct triangle
{
    uint32_t mVertices[ 3 ]; // indices which map to vertices within an arbitrary buffer
};

//---------------------------------------------------------------------
// texture_t
//
// FIXME (0): calc_mip_2d shouldn't be strictly a mip function; it should be applicable to
// all texture glTexImage2D functions, because a mip level of 0 is a valid value for any texture.
// So, consider renaming this to something like "load_pixels_2d" or something
//
// FIXME (1): since GL_FLOAT is allowed to be used when uploading a pixel buffer (providing we're not using ES 2 or lower),
// using mBpp to determine both internal and external texture formats is no longer as full proof as it once was:
// a GL_ALPHA format could easily correspond to a BPP of 4, if we're using floats as our color channel primitive, for example.
// Or, if each channel is only a byte in size, a BPP of 4 could refer to GL_RGBA.
// A better heuristic needs to be developed. At the very least, the user should be allowed to specify what they want explicitly without
// side effects happening.
//---------------------------------------------------------------------
struct texture
{
private:
	bool mSrgb;

	bool mMipmap;

    GLuint mHandle;

	GLenum mBufferType;

    GLenum mWrap;

    GLenum mMinFilter;

    GLenum mMagFilter;

    GLenum mFormat;

    GLenum mInternalFormat;

    GLenum mTarget;

    GLuint mMaxMip;

    GLsizei mWidth, mHeight, mDepth, mBpp; // bpp is in bytes

    std::vector< uint8_t > mPixels;

public:
    texture( void );

	~texture( void );
	
    void bind( void ) const;
	
    void bind( int32_t offset, const std::string& unif, const shader_program& prog ) const;
	
    void release( void ) const;
	
    void release( int32_t offset ) const;
	
    void gen_handle( void );
	
    void load_cube_map( void );
	
    void load_settings( void );
	
    void load_2d( void );
	
    bool open_file( const char* texPath );
	
    bool set_buffer_size( int32_t width, int32_t height, int32_t bpp, uint8_t fill );

    bool determine_formats( void );

    void calc_mip_2d( int32_t mip, int32_t width, int32_t height ) const;

	GLuint handle( void ) const { return mHandle; }

	GLenum buffer_type( void ) const { return mBufferType; }

	GLenum wrap_mode( void ) const { return mWrap; }

	GLenum min_filter( void ) const { return mMinFilter; }

	GLenum mag_filter( void ) const { return mMagFilter; }

	GLenum format( void ) const { return mFormat; }

	GLenum internal_format( void ) const { return mInternalFormat; }

	GLenum target( void ) const { return mTarget; }

    GLuint max_mip_levels( void ) const { return mMaxMip; }

	const std::vector< uint8_t >& pixels( void ) const { return mPixels; }

    GLsizei width( void ) const { return mWidth; }

    GLsizei height( void ) const { return mHeight; }

    GLsizei depth( void ) const { return mDepth; }

    GLsizei bpp( void ) const { return mBpp; }

	void buffer_type( GLenum type );

    void width( GLsizei w );

    void height( GLsizei h );

    void depth( GLsizei d );

    void bpp( GLsizei b );

    void wrap_mode( GLenum wrap );

    void min_filter( GLenum minfilter );

    void mag_filter( GLenum magFilter );

    void format( GLenum fmt );

    void internal_format( GLenum internalFmt );

    void target( GLenum targ );

    void max_mip_levels( GLenum maxmiplevs );

    void mip_map( bool mipMap );

    void pixels( std::vector< uint8_t > p );
};

//---------------------------------------------------------------------
// Program
//---------------------------------------------------------------------

class shader_program
{
private:
    GLuint mProgram;

    string_int_map_t mUniforms;

    string_int_map_t mAttribs;

    string_address_map_t mAttribPointerOffsets;

    std::vector< std::string > mDisableAttribs; // Cleared on each invocation of LoadAttribLayout

    void gen_data( const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs );

public:

    shader_program( void );

    shader_program( const std::string& vertexShader, const std::string& fragmentShader );
	
    shader_program( const std::string& vertexShader, const std::string& fragmentShader,
        const std::vector< std::string >& mUniforms, const std::vector< std::string >& mAttribs );
	
    shader_program( const std::vector< char >& vertexShader, const std::vector< char >& fragmentShader,
        const std::vector< std::string >& mUniforms, const std::vector< std::string >& mAttribs );

    shader_program( const shader_program& copy );

    shader_program( shader_program&& original );

    ~shader_program( void );

    shader_program& operator=( shader_program&& original );

    const string_int_map_t& uniforms( void ) const { return mUniforms; }

    const string_int_map_t& attribs( void ) const { return mAttribs; }

    const string_address_map_t& attrib_pointer_offsets( void ) const { return mAttribPointerOffsets; }

    const std::vector< std::string >& disable_attribs( void ) { return mDisableAttribs; }

    void add_unif( const std::string& name );

    void add_attrib( const std::string& name );

    void add_attrib_pointer_offset( const std::string& key, intptr_t offset );

    void add_disable_attrib( const std::string& attribs );

    void clear_disable_attrib( void );

    void load_mat4( const std::string& name, const glm::mat4& t ) const;
	
    void load_mat2( const std::string& name, const glm::mat2& t ) const;

    void load_mat2( const std::string& name, const float* t ) const;

    void load_mat3( const std::string& name, const glm::mat3& t ) const;

    void load_vec2( const std::string& name, const glm::vec2& v ) const;

    void load_vec2( const std::string& name, const float* v ) const;

    void load_vec2_array( const std::string& name, const float* v, int32_t num ) const;

    void load_vec3( const std::string& name, const glm::vec3& v ) const;

    void load_vec3_array( const std::string& name, const float* v, int32_t num ) const;

    void load_vec4( const std::string& name, const glm::vec4& v ) const;

    void load_vec4( const std::string& name, const float* v ) const;

    void load_vec4_array( const std::string& name, const float* v, int32_t num ) const;

    void load_int( const std::string& name, int32_t v ) const;

    void load_float( const std::string& name, float v ) const;

    void bind( void ) const;

    void release( void ) const;

    static const shader_program* lastAttribLoad;

    static const draw_buffer* lastDrawBuffer;

    static std::vector< std::string > array_location_names( const std::string& name, int32_t length );

    template < typename vertex_type_t >
    static void load_attrib_layout( const draw_buffer& buffer, const shader_program& mProgram, bool clientArray = false );
};

//---------------------------------------------------------------------
// loadBlend_t: saves current blend state in place of a new one and restores
// the original on destruction
// Setting "toggle" to true in the ctor will enable blending
// and then turn it off on destruction, after the original functions
// have been restored.
//---------------------------------------------------------------------
struct set_blend_mode
{
    GLenum mPrevSrcFactor, mPrevDstFactor;

	bool mEnabled;

    set_blend_mode( GLenum srcFactor, GLenum dstFactor );

	~set_blend_mode( void );
};

//---------------------------------------------------------------------
// rtt_t: Basic FBO wrapper used for rendering to a texture
//---------------------------------------------------------------------
struct render_to_texture
{
private:
    texture     mTexture;

    GLuint		mFbo;

    GLenum		mAttachment;

    glm::mat4	mView;

public:
    render_to_texture( GLenum attachment_, const glm::mat4& view_ );

    ~render_to_texture( void );

    void attach( int32_t width, int32_t height, int32_t bpp );

    void bind( void ) const;
	
    void release( void ) const;
};

//---------------------------------------------------------------------
// viewportStash_t: store current viewport data, replace with new parameters,
// restore original on destruction
//---------------------------------------------------------------------
struct viewport_stash
{
private:
    std::array< GLint, 4 > mOriginal;

public:
    viewport_stash( GLint originX, GLint originY, GLint width, GLint height );
    ~viewport_stash( void );
};

//---------------------------------------------------------------------
// attribLoader_t: helper functions for loading program attributes, given
// an arbitrary vertex type
//---------------------------------------------------------------------
template < typename vertex_type_t >
struct attrib_loader
{
    using loader_func_map_t = std::unordered_map< std::string, std::function< void( const shader_program& program, intptr_t attribOffset ) > >;
    static loader_func_map_t mFunctions;
};

//---------------------------------------------------------------------
// draw_buffer
//---------------------------------------------------------------------

struct draw_buffer
{
private:
    GLuint mVbo, mIbo;

    GLsizei mCount;

    GLenum mMode, mUsage;

public:
    draw_buffer( const draw_buffer& ) = delete;

    draw_buffer& operator =( const draw_buffer& ) = delete;

    draw_buffer( void );

    draw_buffer( const std::vector< draw_vertex_t >& vertexData, GLenum mMode, GLenum mUsage );

    draw_buffer( const std::vector< draw_vertex_t >& vertexData, const std::vector< GLuint >& indexData, GLenum mMode, GLenum mUsage );

    draw_buffer( draw_buffer&& m );

    ~draw_buffer( void );

    draw_buffer& operator= ( draw_buffer&& m );

    void bind( void ) const;

    void release( void ) const;

    void realloc( const std::vector< draw_vertex_t >& vertexData );

    void update( const std::vector< draw_vertex_t >& vertexData, size_t vertexOffsetIndex = 0 ) const;

    void render( const shader_program& program ) const;

    GLenum mode( void ) const { return mMode; }

    GLenum usage( void ) const { return mUsage; }

    void usage( GLenum u );

    void mode( GLenum m );
};

template< typename T >
using tmpl_predicate_t = std::function< bool( T& ) >;

//---------------------------------------------------------------------
// debug_split_draw_t: draws to view transforms
// in split-screen viewport. The right viewport will only be
// drawn if an arbitrary predicate is met.
//---------------------------------------------------------------------

// renderable_t must have a public member function of the form "void Render( const shader_program_t& program ) const"
template < typename predicate_type_t, typename renderable_t >
struct debug_split_draw
{
	using predicate_fn_t = tmpl_predicate_t< predicate_type_t >;

    predicate_fn_t mRightDraw;

    viewport_stash mOriginalVp;

    glm::ivec2 mSplitDims;

    debug_split_draw( predicate_fn_t predicate, const glm::ivec2& dims );

    ~debug_split_draw( void );

    void operator()(
                      predicate_type_t& predobj,
					  const renderable_t& draw,
                      const shader_program& program,
					  const glm::mat4& leftView,
					  const glm::mat4& rightView ) const;
};

//---------------------------------------------------------------------
// render_pipeline
//---------------------------------------------------------------------

struct render_pipeline
{
    using program_map_t = std::unordered_map< std::string, shader_program >;
    using buffer_map_t = std::unordered_map< std::string, draw_buffer >;

private:
    GLuint mVao;

    program_map_t mPrograms;

    buffer_map_t mDrawBuffers;

public:
    render_pipeline( void );

    ~render_pipeline( void );

    const program_map_t& programs( void ) const { return mPrograms; }

    const shader_program& program( const std::string& prog ) const { return mPrograms.at( prog ); }

    const buffer_map_t& draw_buffers( void ) const { return mDrawBuffers; }

    const draw_buffer& buffer( const std::string& buf ) const { return mDrawBuffers.at( buf ); }
};

//---------------------------------------------------------------------
// pipeline_aware
//---------------------------------------------------------------------

struct pipeline_aware
{
protected:
    friend struct render_pipeline;

    static const render_pipeline* mInstance;
};

//---------------------------------------------------------------------
// bind_program
//---------------------------------------------------------------------

struct bind_program : public pipeline_aware
{
private:
    const shader_program& mProgram;

public:
    bind_program( const std::string& which );

    bind_program( const shader_program& program );

    ~bind_program( void );

    const shader_program& program( void ) const { return mProgram; }
};

//---------------------------------------------------------------------
// bind_buffer
//---------------------------------------------------------------------

struct bind_buffer : public pipeline_aware
{
private:
    const draw_buffer& mDrawBuffer;

public:
    bind_buffer( const std::string& which );

    ~bind_buffer( void );

    const draw_buffer& buffer( void ) const { return mDrawBuffer; }
};

//---------------------------------------------------------------------
// imm_draw_t
//---------------------------------------------------------------------

struct imm_draw
{
private:
    struct buffer_store
    {
        size_t mLastSize;
        std::vector< draw_vertex_t > mVertices;
        std::unique_ptr< draw_buffer > mBuffer;

        buffer_store( void );

        void update( void );
    };

    bool mEnabled;

    const shader_program& mProgram;

    static buffer_store mBufferStore;

public:
    imm_draw( const shader_program& prog );

    void begin( GLenum target );

    void vertex( const draw_vertex_t& v );

    void vertex( const glm::vec3& position );

    void vertex( const glm::vec3& position, const glm::vec4& color );

    void end( void );

    void enabled( bool value );

    bool enabled( void ) const { return mEnabled; }
};

extern imm_draw* gImmDrawer;

#include "renderer.inl"

#define ATTRIB_OFFSET_VBO -1

//-------------------------------------------------------------------------------------------------------
// Global
//-------------------------------------------------------------------------------------------------------

template < typename T >
static INLINE GLuint make_vertex_buffer( GLenum target, const std::vector< T >& data, GLenum usage )
{
    GLuint obj;
    GL_CHECK( glGenBuffers( 1, &obj ) );
    GL_CHECK( glBindBuffer( target, obj ) );
    GL_CHECK( glBufferData( target, data.size() * sizeof( T ), &data[ 0 ], usage ) );
    GL_CHECK( glBindBuffer( target, 0 ) );
    return obj;
}

template < typename T, size_t count >
static INLINE GLuint make_vertex_buffer( GLenum target, const std::array< T, count >& data, GLenum usage )
{
    GLuint obj;
    GL_CHECK( glGenBuffers( 1, &obj ) );
    GL_CHECK( glBindBuffer( target, obj ) );
    GL_CHECK( glBufferData( target, data.size() * sizeof( T ), &data[ 0 ], usage ) );
    GL_CHECK( glBindBuffer( target, 0 ) );
    return obj;
}

template < typename T >
static INLINE void update_vertex_buffer( GLenum target, GLuint obj, GLuint offset, const std::vector< T >& data, bool bindUnbind )
{
    if ( bindUnbind )
    {
        GL_CHECK( glBindBuffer( target, obj ) );
    }

    GL_CHECK( glBufferSubData( target, offset * sizeof( T ), data.size() * sizeof( T ), &data[ 0 ] ) );

    if ( bindUnbind )
    {
        GL_CHECK( glBindBuffer( target, 0 ) );
    }
}

static INLINE void free_vertex_buffer( GLenum target, GLuint obj )
{
    if ( obj )
    {
        // Unbind to prevent driver from lazy deletion
        GL_CHECK( glBindBuffer( target, 0 ) );
        GL_CHECK( glDeleteBuffers( 1, &obj ) );
    }
}

static INLINE uint32_t get_max_mip_level_2d( int32_t baseWidth, int32_t baseHeight )
{
    return glm::min( ( int32_t ) glm::log2( ( float ) baseWidth ), ( int32_t ) glm::log2( ( float ) baseHeight ) );
}

template< typename texture_helper_t >
static INLINE uint32_t make_mipmaps_2d( const texture_helper_t& tex, int32_t baseWidth, int32_t baseHeight, int32_t maxLevels )
{
    if ( !maxLevels )
    {
        maxLevels = get_max_mip_level_2d( baseWidth, baseHeight );
    }

    int32_t w = baseWidth;
    int32_t h = baseHeight;
    int32_t mip;

    for ( mip = 0; h != 1 && w != 1; ++mip )
    {
        tex.calc_mip_2d( mip, w, h );

        if ( h > 1 )
        {
            h /= 2;
        }

        if ( w > 1 )
        {
            w /= 2;
        }
    }

    return mip;
}

//-------------------------------------------------------------------------------------------------------
// POD types
//-------------------------------------------------------------------------------------------------------

static INLINE draw_vertex_t make_draw_vertex( const glm::vec3& position, const glm::vec2& texCoord, const glm::u8vec4& color )
{
    draw_vertex_t v =
    {
        { position.x, position.y, position.z },
        { 0.0f, 0.0f, 0.0f },
        { texCoord.x, texCoord.y },
        { color.r, color.g, color.b, color.a }
    };

    return v;
}

static INLINE draw_vertex_t make_draw_vertex( const glm::vec3& position )
{
    return make_draw_vertex( position, glm::u8vec4( 255 ) );
}

static INLINE draw_vertex_t make_draw_vertex( const glm::vec3& position, const glm::u8vec4& color )
{
    return make_draw_vertex( position, glm::vec2( 0.0f ), color );
}

//-------------------------------------------------------------------------------------------------------
// shader_program
//-------------------------------------------------------------------------------------------------------

INLINE void shader_program::add_unif( const std::string& name )
{
    GL_CHECK( mUniforms[ name ] = glGetUniformLocation( mProgram, name.c_str() ) );
}

INLINE void shader_program::add_attrib( const std::string& name )
{
    GL_CHECK( mAttribs[ name ] = glGetAttribLocation( mProgram, name.c_str() ) );
}

INLINE void shader_program::add_attrib_pointer_offset( const std::string& key, intptr_t offset )
{
    mAttribPointerOffsets[ key ] = offset;
}

INLINE void shader_program::add_disable_attrib( const std::string& attrib )
{
    mDisableAttribs.push_back( attrib );
}

INLINE void shader_program::clear_disable_attrib( void )
{
    mDisableAttribs.clear();
}

INLINE void shader_program::bind( void ) const
{
    GL_CHECK( glUseProgram( mProgram ) );
}

INLINE void shader_program::release( void ) const
{
    GL_CHECK( glUseProgram( 0 ) );
}

INLINE void shader_program::load_mat4( const std::string& name, const glm::mat4& t ) const
{
    GL_CHECK( glUniformMatrix4fv( mUniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program::load_mat2( const std::string& name, const glm::mat2& t ) const
{
    GL_CHECK( glUniformMatrix2fv( mUniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program::load_mat2( const std::string& name, const float* t ) const
{
    GL_CHECK( glUniformMatrix2fv( mUniforms.at( name ), 1, GL_FALSE, t ) );
}

INLINE void shader_program::load_mat3( const std::string& name, const glm::mat3& t ) const
{
    GL_CHECK( glUniformMatrix3fv( mUniforms.at( name ), 1, GL_FALSE, glm::value_ptr( t ) ) );
}

INLINE void shader_program::load_vec2( const std::string& name, const glm::vec2& v ) const
{
    GL_CHECK( glUniform2fv( mUniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program::load_vec2( const std::string& name, const float* v ) const
{
    GL_CHECK( glUniform2fv( mUniforms.at( name ), 1, v ) );
}

INLINE void shader_program::load_vec2_array( const std::string& name, const float* v, int32_t num ) const
{
    GL_CHECK( glUniform2fv( mUniforms.at( name ), num, v ) );
}

INLINE void shader_program::load_vec3( const std::string& name, const glm::vec3& v ) const
{
    GL_CHECK( glUniform3fv( mUniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program::load_vec3_array( const std::string& name, const float* v, int32_t num ) const
{
    GL_CHECK( glUniform3fv( mUniforms.at( name ), num, v ) );
}

INLINE void shader_program::load_vec4( const std::string& name, const glm::vec4& v ) const
{
    GL_CHECK( glUniform4fv( mUniforms.at( name ), 1, glm::value_ptr( v ) ) );
}

INLINE void shader_program::load_vec4( const std::string& name, const float* v ) const
{
    GL_CHECK( glUniform4fv( mUniforms.at( name ), 1, v ) );
}

INLINE void shader_program::load_vec4_array( const std::string& name, const float* v, int32_t num ) const
{
    GL_CHECK( glUniform4fv( mUniforms.at( name ), num, v ) );
}

INLINE void shader_program::load_int( const std::string& name, int v ) const
{
    GL_CHECK( glUniform1i( mUniforms.at( name ), v ) );
}

INLINE void shader_program::load_float( const std::string& name, float f ) const
{
    GL_CHECK( glUniform1f( mUniforms.at( name ), f ) );
}

template < typename vertex_type_t >
INLINE void shader_program::load_attrib_layout( const draw_buffer& buffer, const shader_program& program, bool clientArray )
{
    if ( lastAttribLoad == &program && lastDrawBuffer == &buffer )
    {
        return;
    }

    for ( const auto& attrib: program.mAttribs )
    {
        if ( attrib.second != -1 )
        {
            auto it = std::find( program.mDisableAttribs.cbegin(), program.mDisableAttribs.cend(), attrib.first );

            if ( it != program.mDisableAttribs.cend() )
            {
                GL_CHECK( glDisableVertexAttribArray( attrib.second ) );
                continue;
            }

            intptr_t offset;

            if ( clientArray )
            {
                offset = program.mAttribPointerOffsets.at( attrib.first );
            }
            else
            {
                offset = ATTRIB_OFFSET_VBO;
            }

            attrib_loader< vertex_type_t >::mFunctions[ attrib.first ]( program, offset );
        }
    }

    lastAttribLoad = &program;
    lastDrawBuffer = &buffer;
}

//-------------------------------------------------------------------------------------------------------
// texture_t
//-------------------------------------------------------------------------------------------------------


INLINE void texture::calc_mip_2d( int32_t mip, int32_t mipwidth, int32_t mipheight ) const
{
    GL_CHECK( glTexImage2D( mTarget, mip, mInternalFormat,
				mipwidth, mipheight, 0, mFormat, mBufferType, &mPixels[ 0 ] ) );
}

INLINE void texture::gen_handle( void )
{
    if ( !mHandle )
    {
        GL_CHECK( glGenTextures( 1, &mHandle ) );
    }
}

INLINE void texture::bind( void ) const
{
    GL_CHECK( glBindTexture( mTarget, mHandle ) );
}

INLINE void texture::release( void ) const
{
    GL_CHECK( glBindTexture( mTarget, 0 ) );
}

INLINE void texture::release( int offset ) const
{
    GL_CHECK( glActiveTexture( GL_TEXTURE0 + offset ) );
    GL_CHECK( glBindTexture( mTarget, 0 ) );
}

INLINE void texture::buffer_type( GLenum type )
{
	mBufferType = type;
}

INLINE void texture::width( GLsizei w )
{
    mWidth = w;
}

INLINE void texture::height( GLsizei h )
{
    mHeight = h;
}

INLINE void texture::depth( GLsizei d )
{
    mDepth = d;
}

INLINE void texture::bpp( GLsizei b )
{
    mBpp = b;
}

INLINE void texture::wrap_mode( GLenum wrap )
{
    mWrap = wrap;
}

INLINE void texture::min_filter( GLenum minfilter )
{
    mMinFilter = minfilter;
}

INLINE void texture::mag_filter( GLenum magFilter )
{
    mMagFilter = magFilter;
}

INLINE void texture::format( GLenum fmt )
{
    mFormat = fmt;
}

INLINE void texture::internal_format( GLenum internalFmt )
{
    mInternalFormat = internalFmt;
}

INLINE void texture::target( GLenum targ )
{
    mTarget = targ;
}

INLINE void texture::max_mip_levels( GLenum maxmiplevs )
{
    mMaxMip = maxmiplevs;
}

INLINE void texture::mip_map( bool mipMap )
{
    mMaxMip = mipMap;
}

INLINE void texture::pixels( std::vector< uint8_t > p )
{
    mPixels = std::move( p );
}

//-------------------------------------------------------------------------------------------------------
// rtt_t
//-------------------------------------------------------------------------------------------------------

INLINE render_to_texture::render_to_texture( GLenum attachment_, const glm::mat4& view_ )
    :	mFbo( 0 ),
        mAttachment( attachment_ ),
        mView( view_ )
{
    GL_CHECK( glGenFramebuffers( 1, &mFbo ) );
}

INLINE render_to_texture::~render_to_texture( void )
{
    if ( mFbo )
    {
        GL_CHECK( glDeleteFramebuffers( 1, &mFbo ) );
    }
}

INLINE void render_to_texture::attach( int32_t width, int32_t height, int32_t bpp )
{
//	mTexture.mMipmap = false;
//	mTexture.mWrap = GL_REPEAT;

    mTexture.mip_map( false );
    mTexture.wrap_mode( GL_REPEAT );
    mTexture.set_buffer_size( width, height, bpp, 255 );
    mTexture.load_2d();
    mTexture.load_settings();

    GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, mFbo ) );
    GL_CHECK( glFramebufferTexture2D( GL_FRAMEBUFFER, mAttachment, GL_TEXTURE_2D, mTexture.handle(), 0 ) );

    GLenum fbocheck;
    GL_CHECK( fbocheck = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
    if ( fbocheck != GL_FRAMEBUFFER_COMPLETE )
    {
        MLOG_ERROR( "FBO check incomplete; value returned is 0x%x", fbocheck );
    }
    GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}

INLINE void render_to_texture::bind( void ) const
{
    GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, mFbo ) );
}

INLINE void render_to_texture::release( void ) const
{
    GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}

//-------------------------------------------------------------------------------------------------------
// viewport_stash_t
//-------------------------------------------------------------------------------------------------------

INLINE viewport_stash::viewport_stash( GLint originX, GLint originY, GLint width, GLint height )
{
    GL_CHECK( glGetIntegerv( GL_VIEWPORT, &mOriginal[ 0 ] ) );
    GL_CHECK( glViewport( originX, originY, width, height ) );
}

INLINE viewport_stash::~viewport_stash( void )
{
    GL_CHECK( glViewport( mOriginal[ 0 ], mOriginal[ 1 ], mOriginal[ 2 ], mOriginal[ 3 ] ) );
}

//-------------------------------------------------------------------------------------------------------
// attrib_loader_t: loads vertex attributes from an arbitrary vertex type
//-------------------------------------------------------------------------------------------------------

#define LOADER_FUNC_NAME "attrib_loader::mFunctions::m"
#define MAP_VEC_3( vname, attribname, funcname )\
    do {\
        if ( attribOffset == ATTRIB_OFFSET_VBO )\
        {\
            attribOffset = offsetof( vertex_type_t, vname );\
        }\
        GLint location = program.attribs().at( #attribname );\
        GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ), funcname );\
        GL_CHECK_WITH_NAME( glVertexAttribPointer( location,\
                3, GL_FLOAT, GL_FALSE, sizeof( vertex_type_t ), ( void* ) attribOffset ), funcname );\
    }\
    while( 0 )

template < typename vertex_type_t >
typename attrib_loader< vertex_type_t >::loader_func_map_t attrib_loader< vertex_type_t >::mFunctions =
{
    {
        "position",
        []( const shader_program& program, intptr_t attribOffset ) -> void
        {
            MAP_VEC_3( mPosition, position, LOADER_FUNC_NAME"Position" );
        }
    },
    {
        "normal",
        []( const shader_program& program, intptr_t attribOffset ) -> void
        {
            MAP_VEC_3( mNormal, normal, LOADER_FUNC_NAME"Normal" );
        }
    },
    {
        "color",
        []( const shader_program& program, intptr_t attribOffset ) -> void
        {
            GLint location = program.attribs().at( "color" );

            if ( attribOffset == ATTRIB_OFFSET_VBO )
            {
                attribOffset = offsetof( vertex_type_t, mColor );
            }

            GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ),
                LOADER_FUNC_NAME"Color" );

            GL_CHECK_WITH_NAME( glVertexAttribPointer( location,
                4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( vertex_type_t ),
                ( void* ) attribOffset ), LOADER_FUNC_NAME"Color" );
        }
    },
    {
        "texCoord",
        []( const shader_program& program, intptr_t attribOffset ) -> void
        {
            if ( attribOffset == ATTRIB_OFFSET_VBO )
            {
                attribOffset = offsetof( vertex_type_t, mTexCoord );
            }

            GLint location = program.attribs().at( "texCoord" );
            GL_CHECK_WITH_NAME( glEnableVertexAttribArray( location ), LOADER_FUNC_NAME"TexCoord" );
            GL_CHECK_WITH_NAME( glVertexAttribPointer( location,
                2, GL_FLOAT, GL_FALSE, sizeof( vertex_type_t ), ( void* ) attribOffset ), LOADER_FUNC_NAME"TexCoord" );
        }
    }
};

#undef LOADER_FUNC_NAME
#undef MAP_VEC_3

//---------------------------------------------------------------------
// debug_split_draw
//---------------------------------------------------------------------

template < typename predicate_type_t, typename renderable_t >
debug_split_draw< predicate_type_t, renderable_t >::debug_split_draw(
        typename debug_split_draw< predicate_type_t, renderable_t >::predicate_fn_t predicate_,
        const glm::ivec2& dims_ )
    : mRightDraw( predicate_ ),
      mOriginalVp( 0, 0, dims_.x, dims_.y ),
      mSplitDims( dims_.x / 2, dims_.y )
{
}

template < typename predicate_type_t, typename renderable_t >
debug_split_draw< predicate_type_t, renderable_t >::~debug_split_draw( void )
{
}

template < typename predicate_type_t, typename renderable_t >
void debug_split_draw< predicate_type_t, renderable_t >::operator()(
        predicate_type_t& predobj,
        const renderable_t& draw,
        const shader_program& program,
        const glm::mat4& leftView,
        const glm::mat4& rightView ) const
{
    GL_CHECK( glViewport( 0, 0, mSplitDims.x, mSplitDims.y ) );
    program.load_mat4( "modelToView", leftView );
    draw.Render( program );

    if ( mRightDraw( predobj ) )
    {
        GL_CHECK( glViewport( mSplitDims.x, 0, mSplitDims.x, mSplitDims.y ) );
        program.load_mat4( "modelToView", rightView );
        draw.Render( program );
    }
}

#undef ATTRIB_OFFSET_VBO

//---------------------------------------------------------------------
// draw_buffer usage
//---------------------------------------------------------------------

INLINE void draw_buffer::usage( GLenum u )
{
    mUsage = u;
}

INLINE void draw_buffer::mode( GLenum m )
{
    mMode = m;
}

INLINE void draw_buffer::bind( void ) const
{
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, mVbo ) );
}

INLINE void draw_buffer::release( void ) const
{
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
}

INLINE void draw_buffer::realloc( const std::vector< draw_vertex_t >& vertexData )
{
    bind();
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( draw_vertex_t ) * vertexData.size(), &vertexData[ 0 ], mUsage ) );
    release();

    if ( !mIbo )
    {
        mCount = vertexData.size();
    }
}

INLINE void draw_buffer::update( const std::vector< draw_vertex_t >& vertexData, size_t vertexOffsetIndex ) const
{
    bind();
    GL_CHECK( glBufferSubData( GL_ARRAY_BUFFER,
                               ( GLintptr )( vertexOffsetIndex * sizeof( draw_vertex_t ) ),
                               sizeof( draw_vertex_t ) * vertexData.size(),
                               &vertexData[ 0 ] ) );
    release();
}

struct draw_text
{
	const shader_program& mProgram;
	std::unique_ptr< texture > mTexture;
	std::unique_ptr< imm_draw > mDraw;

	draw_text( const render_pipeline& pipeline );

	void draw( const std::string& text, const glm::vec2& origin );
};

/*
template < GLenum fetchParam,
           typename set_fn_t, set_fn_t set,
           typename get_fn_t, get_fn_t get,
           typename... args >
struct state
{   
    using args_type = std::tuple< args... >;

    args_type mSave;

    state( args... s )
    {
        std::tie< args... > save;

        GL_CHECK( get( fetchParam, ( &save )... ) );
        mSave = std::move( save );
        set( std::forward< args >( s )... );
    }

    ~state( void )
    {
        std::tie< args... > t( mSave );

        set( std::forward( t ) );
    }
};

using gl_push_point_size_t = state< GL_POINT_SIZE,
                                    void ( * )( float ), glPointSize,
                                    void ( * )( GLenum, float* ), glGetFloatv,
                                    float >;

*/

using float_func_t = void ( * )( float param );

template < GLenum param, float_func_t set >
struct gl_push_float_attrib
{
    float mSave = 0.0f;

    gl_push_float_attrib( float s )
    {
        GL_CHECK( glGetFloatv( param, &mSave ) );
        set( s );
    }

    ~gl_push_float_attrib( void )
    {
        set( mSave );
    }
};

struct gl_push_polygon_mode
{
    GLint mSave = 0;

    gl_push_polygon_mode( GLenum s )
    {
        GL_CHECK( glGetIntegerv( GL_POLYGON_MODE, &mSave ) );
        GL_CHECK( glPolygonMode( GL_FRONT_AND_BACK, s ) );
    }

    ~gl_push_polygon_mode( void )
    {
        GL_CHECK( glPolygonMode( GL_FRONT_AND_BACK, mSave ) );
    }
};

template < typename... args >
struct attrib_stack
{

};
