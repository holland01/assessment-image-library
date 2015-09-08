//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

static INLINE glm::vec3 get_collision_normal( const glm::vec3& normal, const rigid_body& a, const rigid_body& b )
{
	return std::move( normal * a.mass() * b.mass() );
}

static INLINE glm::mat3 get_block_inertia( const glm::vec3& halfSize, float mass )
{
	glm::vec3 squared( halfSize * halfSize );
	const float thirdmass = mass * 0.3f;

	glm::mat3 inertia( 1.0f );
	inertia[ 0 ][ 0 ] = thirdmass * ( squared.y + squared.z );
	inertia[ 1 ][ 1 ] = thirdmass * ( squared.x + squared.z );
	inertia[ 2 ][ 2 ] = thirdmass * ( squared.x + squared.y );

	return std::move( inertia );
}

//-------------------------------------------------------------------------------------------------------
// rigid body
//-------------------------------------------------------------------------------------------------------

INLINE void rigid_body::apply_force( const glm::vec3& force )
{
	mForceAccum += force;
}

INLINE void rigid_body::apply_force_at_local_point( const glm::vec3& f, const glm::vec3& point )
{
	glm::vec3 worldP( mOrientation * point );

	apply_force_at_point( f, worldP );
}

INLINE void rigid_body::apply_torque( const glm::vec3& f, const glm::vec3& p )
{
	mTorqueAccum += glm::cross( p, f );
}

INLINE void rigid_body::apply_torque_from_center( const glm::vec3& f )
{
	apply_torque( f, position() );
}

INLINE void rigid_body::apply_force_at_point( const glm::vec3& f, const glm::vec3& point )
{
	apply_force( f );
	apply_torque( f, point );
}

INLINE void rigid_body::apply_velocity( const glm::vec3& v )
{
	mLinearVelocity += v;
}

INLINE void rigid_body::linear_damping( float d )
{
	mLinearDamping = d;
}

INLINE void rigid_body::angular_damping( float d )
{
	mAngularDamping = d;
}

INLINE void rigid_body::position( const glm::vec3& p )
{
	mPosition = p;
}

INLINE void rigid_body::position( uint32_t axis, float v )
{
	assert( axis < 3 );

	mPosition[ axis ] = v;
}

INLINE void rigid_body::orientation( const glm::mat4& orientation )
{
	this->mOrientation = std::move( glm::quat_cast( orientation ) );
}

INLINE void rigid_body::orientation( const glm::mat3& orientation )
{
	this->mOrientation = std::move( glm::quat_cast( orientation ) );
}

INLINE void rigid_body::iit_local( const glm::mat3& m )
{
	mIitLocal = m;
}

INLINE void rigid_body::set( const glm::mat4& t )
{
	mPosition = glm::vec3( t[ 3 ] );
	mOrientation = std::move( glm::quat_cast( t ) );
}
