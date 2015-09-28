#include "bounds_primitive.h"
#include "halfspace.h"
#include "obb.h"

primitive_lookup* bounds_primitive::to_lookup( void )
{
	assert( mType == BOUNDS_PRIM_LOOKUP );
	return ( primitive_lookup* ) this;
}

const primitive_lookup* bounds_primitive::to_lookup( void ) const
{
	assert( mType == BOUNDS_PRIM_LOOKUP );
	return ( const primitive_lookup* ) this;
}


obb* bounds_primitive::to_box( void )
{
	assert( mType == BOUNDS_PRIM_BOX );
	return ( obb* ) this;
}

const obb* bounds_primitive::to_box( void ) const
{
	assert( mType == BOUNDS_PRIM_BOX );
	return ( const obb* ) this;
}

halfspace* bounds_primitive::to_halfspace( void )
{
	assert( mType == BOUNDS_PRIM_BOX );
	return ( halfspace* ) this;
}
