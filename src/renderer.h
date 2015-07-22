#pragma once

#include "def.h"
#include "base.h"

#include <array>
#include <tuple>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <algorithm>

#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include OPENGL_API_H
#include OPENGL_API_EXT_H

#define UBO_TRANSFORMS_BLOCK_BINDING 0
#define ATTRIB_OFFSET( type, member )( ( void* ) offsetof( type, member ) ) 

// Extensions
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

#define GEN_V_SHADER( data ) #data

#ifdef EMSCRIPTEN
#   define GEN_F_SHADER( data ) "precision mediump float;\n"#data
#else
#   define GEN_F_SHADER( data ) #data
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
static INLINE void DrawElementBuffer( GLuint ibo, size_t numIndices );

static INLINE uint32_t Texture_GetMaxMipLevels2D( int32_t baseWidth, int32_t baseHeight );

template< typename textureHelper_t >
static INLINE uint32_t Texture_CalcMipLevels2D( const textureHelper_t& tex,
                                                int32_t baseWidth, int32_t baseHeight, int32_t maxLevels );
//---------------------------------------------------------------------
// POD types
//---------------------------------------------------------------------
struct draw_vertex_t
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::u8vec4 color;
};

static INLINE draw_vertex_t draw_vertex_t_Make( const glm::vec3& position, const glm::u8vec4& color );

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
	std::map< std::string, GLint > uniforms; 
	std::map< std::string, GLint > attribs;

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

	void LoadAttribLayout( void ) const;

	void LoadMat4( const std::string& name, const glm::mat4& t ) const;
	
	void LoadMat2( const std::string& name, const glm::mat2& t ) const;
	void LoadMat2( const std::string& name, const float* t ) const;

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
    static void LoadAttribLayout( const shader_program_t& program );
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
template < typename vertexType_t >
struct attrib_loader_t
{
    using loader_func_map_t = std::map< std::string, std::function< void( const shader_program_t& program ) > >;
    static loader_func_map_t functions;
};

//---------------------------------------------------------------------
// draw_buffer_t
//---------------------------------------------------------------------
template < GLenum mode, GLenum usage >
struct draw_buffer_t
{
	GLuint vbo;
	size_t count;

	draw_buffer_t( const std::vector< draw_vertex_t >& vertexData );
	~draw_buffer_t( void );

	void Render( const shader_program_t& program, const glm::mat4& modelToView ) const;
};

} // namespace rend

#include "renderer.inl"

