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

namespace detail {

	struct sat_intersection_test
	{
		const glm::vec3 mToCenter;
		const transform_data& mA;
		const transform_data& mB;

		uint32_t mBestSingleAxisSPIndex;
		uint32_t mSmallestPenetrationIndex;
		float mSmallestPenetration;

		sat_intersection_test( const transform_data& a,
							   const transform_data& b );

		// Perform test
		bool operator()( void );
	};

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

} // namespace local
