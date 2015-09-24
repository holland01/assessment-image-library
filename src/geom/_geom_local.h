#pragma once

#include "../def.h"
#include "../glm_ext.hpp"
#include <array>

struct transform_data;
struct primitive_lookup;
struct obb;
struct halfspace;
struct point_project_pair;
struct halfspace;
struct plane;
struct ray;

namespace local {

	struct sat_intersection_test
	{
		const glm::vec3& mToCenter;
		const transform_data& mA;
		const transform_data& mB;

		uint32_t mBestSingleAxisSPIndex;
		uint32_t mSmallestPenetrationIndex;
		float mSmallestPenetration;

		sat_intersection_test( const glm::vec3& toCenter,
							   const transform_data& a,
							   const transform_data& b );
	};

	float penetration_depth( const glm::vec3& axis, const sat_intersection_test& test );

	bool eval_axis( uint32_t index, sat_intersection_test& test, const glm::vec3& axis );

	bool has_intersection( sat_intersection_test& test );
}

namespace local {
#if GLM_ARCH != GLM_ARCH_PURE
	using vec_t = glm::simdVec4;
	using mat_t = glm::simdMat4;

	template< size_t numVectors, size_t numMatrices >
	struct simd_conversion_t
	{
		std::array< vec_t, numVectors > vectors;
		std::array< mat_t, numMatrices > matrices;

		simd_conversion_t( const std::array< glm::vec3, numVectors >& v,
						   const std::array< glm::mat3, numMatrices >& m );
	};

	template< size_t numVectors, size_t numMatrices >
	simd_conversion_t< numVectors, numMatrices >::simd_conversion_t(
			const std::array< glm::vec3, numVectors >& v,
			const std::array< glm::mat3, numMatrices >& m )
	{
		for ( uint32_t i = 0; i < numVectors; ++i )
		{
			vectors[ i ] = std::move( vec_t( v[ i ], 1.0f ) );
		}

		for ( uint32_t i = 0; i < numMatrices; ++i )
		{
			mat3_to_simd( matrices[ i ], m[ i ] );
		}
	}

	using simd_intersect_convert_t = simd_conversion_t< 2, 2 >;

#else
	using vec_t = glm::vec3;
	using mat_t = glm::mat3;
#endif // GLM_ARCH_PURE

	struct intersect_comp_t
	{
		struct sat_params_t
		{
			const vec_t& p0;
			const vec_t& d0;
			const vec_t& origin;
			const mat_t extents;
		};

		const vec_t& origin;
		const vec_t& srcOrigin;

		const mat_t& extents;
		const mat_t& srcExtents;

		float sizeLength;

		std::array< vec_t, 3 > sourcePoints;

		intersect_comp_t(
			const vec_t& origin_,
			const mat_t& extents_,
			const vec_t& srcOrigin_,
			const mat_t& srcExtents_ );

		bool ValidateDistance( void );

		bool TestIntersection( const vec_t& p0, const vec_t& d0, const vec_t& origin_, const mat_t& extents_ );
	};

	intersect_comp_t::intersect_comp_t(
		const vec_t& origin_,
		const mat_t& extents_,
		const vec_t& srcOrigin_,
		const mat_t& srcExtents_ )
		: origin( origin_ ),
		  srcOrigin( srcOrigin_ ),
		  extents( extents_ ),
		  srcExtents( srcExtents_ ),
		  sizeLength( glm::length( extents[ 0 ] + extents[ 1 ] + extents[ 2 ] ) ),
		  sourcePoints( {{
			srcOrigin_ + srcExtents_[ 0 ],
			srcOrigin_ + srcExtents_[ 1 ],
			srcOrigin_ + srcExtents_[ 2 ]
		  }} )

	{}

	bool intersect_comp_t::ValidateDistance( void )
	{
		if ( glm::distance( origin, sourcePoints[ 0 ] ) > sizeLength ) return false;
		if ( glm::distance( origin, sourcePoints[ 1 ] ) > sizeLength ) return false;
		if ( glm::distance( origin, sourcePoints[ 2 ] ) > sizeLength ) return false;

		return true;
	}

	bool intersect_comp_t::TestIntersection( const vec_t& p0, const vec_t& d0, const vec_t& origin_, const mat_t& extents_ )
	{
		vec_t a( origin_ + extents_[ 0 ] ),
			  b( origin_ + extents_[ 1 ] ),
			  c( origin_ + extents_[ 2 ] );

		vec_t ax( a - p0 );
		vec_t ay( b - p0 );
		vec_t az( c - p0 );

		vec_t dx( glm::proj( ax, d0 ) );
		vec_t dy( glm::proj( ay, d0 ) );
		vec_t dz( glm::proj( az, d0 ) );

		float dlen = glm::length( d0 );

		if ( glm::length( dx ) > dlen ) return false;
		if ( glm::length( dy ) > dlen ) return false;
		if ( glm::length( dz ) > dlen ) return false;

		return true;
	}
}
