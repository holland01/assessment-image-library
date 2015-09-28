#pragma once

#include "_geom_local.h"
#include "bounds_primitive.h"
#include "transform_data.h"
#include "../collision_contact.h"

//-------------------------------------------------------------------------------------------------------
// half_space_t
//-------------------------------------------------------------------------------------------------------

struct imm_draw;

struct halfspace : public bounds_primitive
{
private:
	transform_data mT;
	float mDistance;

public:
	halfspace( void );

	halfspace( const glm::mat3& axes, const glm::vec3& origin, float distance );

	halfspace( const obb& bounds, const glm::vec3& normal );

	halfspace( const halfspace& c );

	halfspace& operator=( halfspace c );

	halfspace( halfspace&& m );

	halfspace& operator=( halfspace&& m );

	const glm::mat3& axes( void ) const { return mT.mAxes; }

	const glm::vec3& origin( void ) const { return mT.mOrigin; }

	bool intersects( contact::list_t& contacts, const obb& bounds ) const;

	void draw( imm_draw& drawer ) const;
};
