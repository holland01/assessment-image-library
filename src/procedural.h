#pragma once

#include "def.h"
#include <float.h>
#include <random>

struct randomize
{
private:
    mutable std::random_device mDev;
    mutable std::mt19937 mAlgol;
    mutable std::uniform_real_distribution< float > mDist;

public:
    FORCEINLINE randomize( float min = FLT_MIN, float max = FLT_MAX )
        : mAlgol( mDev() ),
          mDist( min, max )
    {
    }

    FORCEINLINE float operator()( void ) const
    {
        return mDist( mAlgol );
    }
};
