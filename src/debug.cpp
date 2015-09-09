#include "debug.h"
#include "geom.h"
#include "application.h"
#include "view.h"
#include "renderer.h"
#include <assert.h>

#define DEBUG_FAIL ( assert( false && "Cannot call debug function \"" _FUNC_NAME_ "\" in this build configuration" ) )

#ifdef DEBUG
namespace {
	struct debug_data
	{
		bool	flag = false;
		ray		drawRay;
		std::vector< ray > rayList;
	};

	debug_data gDebug;
}

bool debug_flag_set( void )
{
	return gDebug.flag;
}

void debug_set_flag( bool v )
{
	gDebug.flag = v;
}

void debug_get_ray( ray& r )
{
	r = gDebug.drawRay;
}

void debug_set_ray( const ray& r )
{
	gDebug.drawRay = r;
}

void debug_raylist_get( uint32_t i, ray& r )
{
	r = gDebug.rayList[ i ];
}

debug_raylist_iter_t debug_raylist_begin( void )
{
	return gDebug.rayList.begin();
}

debug_raylist_iter_t debug_raylist_end( void )
{
	return gDebug.rayList.end();
}

void debug_raylist_push( const ray& r )
{
	gDebug.rayList.push_back( r );
}

void debug_raylist_clear( void )
{
	gDebug.rayList.clear();
}

bool debug_raylist_empty( void )
{
	return gDebug.rayList.empty();
}

#else
bool debug_flag_set( void ) { DEBUG_FAIL; }
void debug_set_flag( bool v ) { DEBUG_FAIL; }
void debug_get_ray( ray& r ) { DEBUG_FAIL; }
void debug_set_ray( const ray& r ) { DEBUG_FAIL; }
void debug_raylist_get( uint32_t i, ray& r ) { DEBUG_FAIL; }
debug_raylist_iter_t debug_raylist_begin( void ) { DEBUG_FAIL; }
debug_raylist_iter_t debug_raylist_end( void ) { DEBUG_FAIL; }
void debug_raylist_push( const ray& r ) { DEBUG_FAIL; }
void debug_raylist_clear( void ) { DEBUG_FAIL; }
bool debug_raylist_empty( void ) { DEBUG_FAIL; }
void debug_draw_hud( const application& app ) { DEBUG_FAIL; }
void debug_draw_axes( const application& app, const view_data& vp ) { DEBUG_FAIL; }
void debug_draw_bounds( const application& app, const obb& bounds, const glm::vec3& color, float alpha ) { DEBUG_FAIL; }
void debug_draw_quad( const application& app, const glm::mat4& transform, const glm::vec3& color, float alpha ) { DEBUG_FAIL; }

#endif

#undef DEBUG_FAIL


