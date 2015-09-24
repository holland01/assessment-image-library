#include "../def.h"
#include "transform_data.h"
#include "../collision_contact.h"
#include "bounds_primitive.h"

//-------------------------------------------------------------------------------------------------------
// obb
//-------------------------------------------------------------------------------------------------------

struct obb : public bounds_primitive
{
private:
	transform_data mT;

public:
	glm::vec4 mColor;

	using pointset3D_t = std::unordered_set< point_project_pair >;

	using pointlist3D_t = std::array< glm::vec3, 8 >;

	using maxmin_pair3D_t = glm::ext::vec3_maxmin_pair_t;

	enum face_type
	{
		FACE_TOP = 0,
		FACE_RIGHT,
		FACE_FRONT,
		FACE_LEFT,
		FACE_BACK,
		FACE_BOTTOM
	};

	enum corner_type
	{
		CORNER_MIN = 0,
		CORNER_NEAR_DOWN_RIGHT,
		CORNER_NEAR_UP_LEFT,
		CORNER_NEAR_UP_RIGHT,
		CORNER_FAR_DOWN_LEFT,
		CORNER_FAR_DOWN_RIGHT,
		CORNER_FAR_UP_LEFT,
		CORNER_MAX = 7
	};

	obb( glm::mat3 axes = glm::mat3( 1.0f ) );

	obb( obb&& m );

	obb( const obb& c );

	obb& operator =( obb c ) = delete;

	bool			encloses( const obb& box ) const;

	const glm::vec3& origin( void ) const { return mT.mOrigin; }

	const glm::vec3& extents( void ) const { return mT.mExtents; }

	glm::vec3       radius( void ) const;

	glm::vec3       corner( corner_type index ) const;

	glm::mat4		world_transform( void ) const { return std::move( mT.world_transform() ); }

	const glm::mat3& axes( void ) const { return mT.mAxes; }

	void            axes( const glm::mat3 m ) { mT.mAxes = std::move( m ); }

	glm::mat3		inv_axes( void ) const { return std::move( glm::inverse( axes() ) ); }

	void			edges_from_corner( corner_type index, glm::mat3& edges ) const;

	void            get_world_space_points( pointlist3D_t& get_world_space_points ) const;

	pointset3D_t    face_project( const plane& facePlane, const pointlist3D_t& sourcePoints ) const;

	void			face_plane( face_type face, plane& plane_t ) const;

	void			color( const glm::vec4& mColor );

	void            origin( const glm::vec3& position );

	// pass isTransformed = "true" if v has already been transformed relative to the linear inverse of this bounds
	bool			range( glm::vec3 v, bool isTransformed ) const;

	bool			intersects( contact::list_t& contacts, const obb& bounds ) const;

	bool			intersects( contact::list_t& contacts, const halfspace& halfSpace ) const;

	bool            ray_intersection( ray& r, bool earlyOut = true ) const;

	maxmin_pair3D_t maxmin( bool inverse ) const;
};

INLINE glm::vec3 obb::corner( corner_type index ) const
{
	return std::move( glm::vec3(
		( ( int32_t ) index & 1 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 2 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 4 ) ? 1.0f : -1.0f
	) );
}

INLINE void obb::get_world_space_points( pointlist3D_t& points ) const
{
	points[ 0 ] = corner( ( corner_type ) 0 );
	points[ 1 ] = corner( ( corner_type ) 1 );
	points[ 2 ] = corner( ( corner_type ) 2 );
	points[ 3 ] = corner( ( corner_type ) 3 );
	points[ 4 ] = corner( ( corner_type ) 4 );
	points[ 5 ] = corner( ( corner_type ) 5 );
	points[ 6 ] = corner( ( corner_type ) 6 );
	points[ 7 ] = corner( ( corner_type ) 7 );
}

INLINE void obb::origin( const glm::vec3& position )
{
	mT.mOrigin = position;
}
