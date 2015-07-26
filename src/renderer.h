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

#ifdef OP_GL_USE_ES
#	define GEN_SHADER( data ) "precision mediump float;\n"#data
#else
#   define GEN_SHADER( data ) "#version 330\n"#data
#endif // EMSCRIPTEN

#ifdef __DEBUG_RENDERER__
#   define GL_CHECK( expr )\
        do\
        {\
            ( expr );\
            ExitOnGLError( _LINE_NUM_, #expr, _FUNC_NAME_ );\
        }\
        while ( 0 )
#   define GL_CHECK_WITH_NAME( expr, funcname )\
        do\
        {\
            ( expr );\
            ExitOnGLError( _LINE_NUM_, #expr, funcname );\
        }\
        while ( 0 )
#else
#   define GL_CHECK( expr ) ( expr )
#   define GL_CHECK_WITH_NAME( expr, funcname ) ( expr )
#endif // __DEBUG_RENDERER__

namespace view {
	struct params_t;
}

namespace rend {

class shader_program_t;

//---------------------------------------------------------------------
// Util Functions
//---------------------------------------------------------------------

void BindTexture( GLenum target,
                  GLuint handle, int32_t offset, const std::string& uniform, const shader_program_t& program );
	
static INLINE void MapVec3( int32_t location, size_t offset );

template < typename T >
static INLINE GLuint GenBufferObject( GLenum target, const std::vector< T >& data, GLenum usage );

template < typename T, size_t count >
static INLINE GLuint GenBufferObject( GLenum target, const std::array< T, count >& data, GLenum usage );

template < typename T >
static INLINE void UpdateBufferObject( GLenum target, GLuint obj, GLuint offset, const std::vector< T >& data, bool bindUnbind );

static INLINE void DeleteBufferObject( GLenum target, GLuint obj );

static INLINE uint32_t Texture_GetMaxMipLevels2D( int32_t baseWidth, int32_t baseHeight );

template< typename textureHelper_t >
static INLINE uint32_t Texture_CalcMipLevels2D( const textureHelper_t& tex,
                                                int32_t baseWidth, int32_t baseHeight, int32_t maxLevels );
//---------------------------------------------------------------------
// POD types
//---------------------------------------------------------------------

template < typename pos_type_t, typename normal_type_t, typename tex_coord_type_t, typename color_type_t >
struct vertex_t
{
	pos_type_t position;
	normal_type_t normal;
	tex_coord_type_t texCoord;
	color_type_t color;
};

using draw_vertex_t = vertex_t< float[ 3 ], float[ 3 ], float[ 2 ], uint8_t[ 4 ] >;

static INLINE draw_vertex_t draw_vertex_t_Make( const glm::vec3& position, const glm::vec2& texCoord, const glm::u8vec4& color );
static INLINE draw_vertex_t draw_vertex_t_Make( const glm::vec3& position, const glm::u8vec4& color );
static INLINE draw_vertex_t draw_vertex_t_Make( const glm::vec3& position );

struct triangle_t
{
	uint32_t vertices[ 3 ]; // indices which map to vertices within an arbitrary buffer
};

//---------------------------------------------------------------------
// texture_t
//---------------------------------------------------------------------
struct texture_t
{
	bool srgb: 1;
	bool mipmap: 1;

	GLuint handle;
	GLenum wrap;
	GLenum minFilter;
	GLenum magFilter;
	GLenum format;
	GLenum internalFormat;
	GLenum target;
	GLuint maxMip;

	GLsizei width, height, depth, bpp; // bpp is in bytes

	std::vector< uint8_t > pixels;

	texture_t( void );
	~texture_t( void );
	
	void Bind( void ) const;
	
    void Bind( int32_t offset, const std::string& unif, const shader_program_t& prog ) const;
	
	void Release( void ) const;
	
	void Release( int32_t offset ) const;
	
	void GenHandle( void );
	
	void LoadCubeMap( void );
	
	void LoadSettings( void );
	
	void Load2D( void );
	
	bool LoadFromFile( const char* texPath );
	
	bool SetBufferSize( int32_t width, int32_t height, int32_t bpp, uint8_t fill );

	bool DetermineFormats( void );

	void CalcMipLevel2D( int32_t mip, int32_t width, int32_t height ) const;
};

//---------------------------------------------------------------------
// Program
//---------------------------------------------------------------------
class shader_program_t
{
private:
	GLuint program;

    void GenData( const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs );

public:
	std::unordered_map< std::string, GLint > uniforms;
	std::unordered_map< std::string, GLint > attribs;
	std::unordered_map< std::string, intptr_t > attribPointerOffsets;

	std::vector< std::string > disableAttribs; // Cleared on each invocation of LoadAttribLayout

    shader_program_t( void );

    shader_program_t( const std::string& vertexShader, const std::string& fragmentShader );
	
    shader_program_t( const std::string& vertexShader, const std::string& fragmentShader,
        const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs );
	
    shader_program_t( const std::vector< char >& vertexShader, const std::vector< char >& fragmentShader,
        const std::vector< std::string >& uniforms, const std::vector< std::string >& attribs );

    shader_program_t( const shader_program_t& copy );

    shader_program_t( shader_program_t&& original );

    ~shader_program_t( void );

    shader_program_t& operator=( shader_program_t&& original );

	void AddUnif( const std::string& name );
	void AddAttrib( const std::string& name );

	void LoadMat4( const std::string& name, const glm::mat4& t ) const;
	
	void LoadMat2( const std::string& name, const glm::mat2& t ) const;
	void LoadMat2( const std::string& name, const float* t ) const;

	void LoadMat3( const std::string& name, const glm::mat3& t ) const;

	void LoadVec2( const std::string& name, const glm::vec2& v ) const;
	void LoadVec2( const std::string& name, const float* v ) const;

	void LoadVec2Array( const std::string& name, const float* v, int32_t num ) const;

	void LoadVec3( const std::string& name, const glm::vec3& v ) const;

	void LoadVec3Array( const std::string& name, const float* v, int32_t num ) const;

	void LoadVec4( const std::string& name, const glm::vec4& v ) const;
	void LoadVec4( const std::string& name, const float* v ) const;

	void LoadVec4Array( const std::string& name, const float* v, int32_t num ) const;

	void LoadInt( const std::string& name, int32_t v ) const;
	void LoadFloat( const std::string& name, float v ) const;

	void Bind( void ) const;
	void Release( void ) const;

	static std::vector< std::string > ArrayLocationNames( const std::string& name, int32_t length );

    template < typename vertex_type_t >
	static void LoadAttribLayout( const shader_program_t& program, bool clientArray = false );
};

//---------------------------------------------------------------------
// loadBlend_t: saves current blend state in place of a new one and restores
// the original on destruction
//---------------------------------------------------------------------
struct load_blend_t
{
	GLenum prevSrcFactor, prevDstFactor;

    load_blend_t( GLenum srcFactor, GLenum dstFactor );
   ~load_blend_t( void );
};

//---------------------------------------------------------------------
// rtt_t: Basic FBO wrapper used for rendering to a texture
//---------------------------------------------------------------------
struct rtt_t
{
	texture_t	texture;
	GLuint		fbo;
	GLenum		attachment;

	glm::mat4	view;

	rtt_t( GLenum attachment_, const glm::mat4& view_ );

	~rtt_t( void );

	void Attach( int32_t width, int32_t height, int32_t bpp );

	void Bind( void ) const;
	
	void Release( void ) const;
};

//---------------------------------------------------------------------
// viewportStash_t: store current viewport data, replace with new parameters,
// restore original on destruction
//---------------------------------------------------------------------
struct viewport_stash_t
{
	std::array< GLint, 4 > original; 

    viewport_stash_t( GLint originX, GLint originY, GLint width, GLint height );
    ~viewport_stash_t( void );
};

//---------------------------------------------------------------------
// attribLoader_t: helper functions for loading program attributes, given
// an arbitrary vertex type
//---------------------------------------------------------------------
template < typename vertex_type_t >
struct attrib_loader_t
{
	using loader_func_map_t = std::unordered_map< std::string, std::function< void( const shader_program_t& program, intptr_t attribOffset ) > >;
	static loader_func_map_t functions;
};

//---------------------------------------------------------------------
// draw_buffer_t
//---------------------------------------------------------------------
struct draw_buffer_t
{
	GLuint vbo, ibo;
	GLsizei count;
	GLenum mode;

	draw_buffer_t( const draw_buffer_t& ) = delete;
	draw_buffer_t& operator =( const draw_buffer_t& ) = delete;

	draw_buffer_t( void );
	draw_buffer_t( const std::vector< draw_vertex_t >& vertexData, GLenum mode, GLenum usage );
	draw_buffer_t( const std::vector< draw_vertex_t >& vertexData, const std::vector< GLuint >& indexData, GLenum mode, GLenum usage );
	draw_buffer_t( draw_buffer_t&& m );

	~draw_buffer_t( void );

	draw_buffer_t& operator= ( draw_buffer_t&& m );

	void Render( const shader_program_t& program ) const;
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

	viewport_stash_t originalVp;
	glm::ivec2 splitDims;

	 debug_split_draw( predicate_fn_t predicate, const glm::ivec2& dims );
	~debug_split_draw( void );

	 void operator()(
					  predicate_type_t& predobj,
					  const renderable_t& draw,
					  const shader_program_t& program,
					  const glm::mat4& leftView,
					  const glm::mat4& rightView ) const;
};

//---------------------------------------------------------------------
// billboard_t
//---------------------------------------------------------------------

struct billboard_t
{
	glm::vec3 origin;

	 billboard_t( const glm::vec3& origin );
	~billboard_t( void );
};

//---------------------------------------------------------------------
// pipline_t
//---------------------------------------------------------------------

struct pipeline_t
{
	using program_map_t = std::unordered_map< std::string, shader_program_t >;
	using buffer_map_t = std::unordered_map< std::string, draw_buffer_t >;

	program_map_t programs;
	buffer_map_t drawBuffers;

	pipeline_t( void );
};

} // namespace rend

#include "renderer.inl"


