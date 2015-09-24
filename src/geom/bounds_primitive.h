#pragma once

#include "_geom_local.h"

//-------------------------------------------------------------------------------------------------------
// bounds_primitive
//-------------------------------------------------------------------------------------------------------

enum bounds_primtype
{   BOUNDS_PRIM_HALFSPACE = 0,
	BOUNDS_PRIM_BOX,
	BOUNDS_PRIM_LOOKUP,
	NUM_BOUNDS_PRIMTYPE
};

struct bounds_primitive
{
protected:
	bounds_primitive( bounds_primtype type_ )
		: type( type_ )
	{}

	bounds_primitive( const bounds_primitive& c )
		: type( c.type )
	{
	}

	bounds_primitive( const bounds_primitive&& m )
		: type( m.type )
	{
	}

public:
	const bounds_primtype type;

	primitive_lookup*         to_lookup( void );

	const primitive_lookup*   to_lookup( void ) const;

	obb*		to_box( void );

	const obb*	to_box( void ) const;

	halfspace*	to_halfspace( void );
};

//-------------------------------------------------------------------------------------------------------
// primitive_lookup
//-------------------------------------------------------------------------------------------------------

struct primitive_lookup : public bounds_primitive
{
	static const int32_t LOOKUP_UNSET = -1;

	bounds_primtype lookupType;
	int32_t index = 0;

	primitive_lookup( bounds_primtype lookupType_, int32_t index_ = LOOKUP_UNSET )
		: bounds_primitive( BOUNDS_PRIM_LOOKUP ),
		  lookupType( lookupType_ ),
		  index( index_ )
	{}
};

