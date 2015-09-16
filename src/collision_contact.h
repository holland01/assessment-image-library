#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <vector>

struct contact
{
    using list_t = std::vector< contact >;

    glm::vec3 mPoint;
    glm::vec3 mNormal;
    float mInterpenDepth;

    contact( const glm::vec3 point = glm::vec3( 0.0f ),
                       const glm::vec3 normal = glm::vec3( 0.0f ),
                       const float interpenDepth = 0.0f )
      : mPoint( std::move( point ) ),
        mNormal( std::move( normal ) ),
        mInterpenDepth( interpenDepth )
  {
  }
};

