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

void rend_bind_texture( GLenum target,
                  GLuint handle, int32_t offset, const std::string& uniform, const shader_program& program );
	
template < typename T >
static INLINE GLuint rend_make_buffer( GLenum target, const std::vector< T >& data, GLenum usage );

template < typename T, size_t count >
static INLINE GLuint rend_make_buffer( GLenum target, const std::array< T, count >& data, GLenum usage );

template < typename T >
static INLINE void rend_update_buffer( GLenum target, GLuint obj, GLuint offset, const std::vector< T >& data, bool bindUnbind );

static INLINE void rend_free_buffer( GLenum target, GLuint obj );

static INLINE uint32_t rend_get_max_mip2d( int32_t baseWidth, int32_t baseHeight );

template< typename textureHelper_t >
static INLINE uint32_t rend_get_mip2d( const textureHelper_t& tex,
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

static INLINE draw_vertex_t rend_make_draw_vertex( const glm::vec3& position, const glm::vec2& texCoord, const glm::u8vec4& color );
static INLINE draw_vertex_t rend_make_draw_vertex( const glm::vec3& position, const glm::u8vec4& color );
static INLINE draw_vertex_t rend_make_draw_vertex( const glm::vec3& position );

struct triangle
{
    uint32_t mVertices[ 3 ]; // indices which map to vertices within an arbitrary buffer
};

//---------------------------------------------------------------------
// texture_t
//---------------------------------------------------------------------
struct texture
{
    bool mSrgb: 1;
    bool mMipmap: 1;

    GLuint mHandle;
    GLenum mWrap;
    GLenum mMinFilter;
    GLenum mMagFilter;
    GLenum mFormat;
    GLenum mInternalFormat;
    GLenum mTarget;
    GLuint mMaxMip;

    GLsizei mWidth, mHeight, mDepth, mBpp; // bpp is in bytes

    std::vector< uint8_t > mPixels;

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
	
    bool load_from_file( const char* texPath );
	
    bool set_buffer_size( int32_t width, int32_t height, int32_t bpp, uint8_t fill );

    bool determine_formats( void );

    void calc_mip_2d( int32_t mip, int32_t width, int32_t height ) const;

	GLuint handle( void ) const { return mHandle; }

	GLenum wrap_mode( void ) const { return mWrap; }

	GLenum min_filter( void ) const { return mMinFilter; }

	GLenum mag_filter( void ) const { return mMagFilter; }

	GLenum format( void ) const { return mFormat; }

	GLenum internal_format( void ) const { return mInternalFormat; }

	GLenum target( void ) const { return mTarget; }

	GLuint max_mip_levels( void ) const { mMaxMip; }

	const std::vector< uint8_t >& pixels( void ) const { return mPixels; }

	GLsizei width( void ) const { return mWidth; }

	GLsizei height( void ) const { return mHeight; }

	GLsizei depth( void ) const { return mDepth; }

	GLsizei bpp( void ) const { return mBpp; }
};

//---------------------------------------------------------------------
// Program
//---------------------------------------------------------------------
class shader_program
{
private:
    GLuint mProgram;

    void gen_data( const std::vector< std::string >& mUniforms, const std::vector< std::string >& mAttribs );

public:
    std::unordered_map< std::string, GLint > mUniforms;
    std::unordered_map< std::string, GLint > mAttribs;
    std::unordered_map< std::string, intptr_t > mAttribPointerOffsets;

    std::vector< std::string > mDisableAttribs; // Cleared on each invocation of LoadAttribLayout

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

    void add_unif( const std::string& name );
    void add_attrib( const std::string& name );

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
    texture     mTexture;

	GLuint		fbo;

	GLenum		attachment;

	glm::mat4	view;

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
	std::array< GLint, 4 > original; 

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
// draw_buffer_t
//---------------------------------------------------------------------
struct draw_buffer
{
    GLuint mVbo, mIbo;

    GLsizei mCount;

    GLenum mMode, mUsage;

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

    predicate_fn_t rightDraw;

    viewport_stash originalVp;

    glm::ivec2 splitDims;

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
// pipeline_t
//---------------------------------------------------------------------

struct render_pipeline
{
    using program_map_t = std::unordered_map< std::string, shader_program >;
    using buffer_map_t = std::unordered_map< std::string, draw_buffer >;

    GLuint mVao;
    program_map_t mPrograms;
    buffer_map_t mDrawBuffers;

    render_pipeline( void );
    ~render_pipeline( void );
};

//-------------------------------------------------------------------------------------------------
// buffer_store_t
//-------------------------------------------------------------------------------------------------

struct buffer_store
{
    size_t mLastSize;
    std::vector< draw_vertex_t > mVertices;
    std::unique_ptr< draw_buffer > mBuffer;

    buffer_store( void );

    void update( void );
};

//---------------------------------------------------------------------
// imm_draw_t
//---------------------------------------------------------------------

struct imm_draw
{
private:
    bool mEnabled;
    const shader_program& mProgram;

    static buffer_store mBufferStore;

public:
    imm_draw( const shader_program& prog );

    void begin( GLenum target );

    void vertex( const draw_vertex_t& v );

    void vertex( const glm::vec3& position );

    void end( void );

    void enabled( bool value );

    bool enabled( void ) const { return mEnabled; }
};

extern imm_draw* gImmDrawer;

#include "renderer.inl"
