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
		: mType( type_ )
	{}

	bounds_primitive( const bounds_primitive& c )
		: mType( c.mType )
	{
	}

	bounds_primitive( const bounds_primitive&& m )
		: mType( m.mType )
	{
	}

public:
	const bounds_primtype mType;

	primitive_lookup*         to_lookup( void );

	const primitive_lookup*   to_lookup( void ) const;

	halfspace*	to_halfspace( void );
};

//-------------------------------------------------------------------------------------------------------
// primitive_lookup
//-------------------------------------------------------------------------------------------------------

struct primitive_lookup : public bounds_primitive
{
	static const int32_t LOOKUP_UNSET = -1;

	bounds_primtype mLookupType;
	int32_t mIndex = 0;

	primitive_lookup( bounds_primtype lookupType_, int32_t index_ = LOOKUP_UNSET )
		: bounds_primitive( BOUNDS_PRIM_LOOKUP ),
		  mLookupType( lookupType_ ),
		  mIndex( index_ )
	{}
};

