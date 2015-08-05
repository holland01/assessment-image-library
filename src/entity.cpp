#include "entity.h"
#include "physics.h"
#include "geom.h"

entity_t::entity_t( dependent_t dep, bounding_box_t* bounds_, body_t* body_, const glm::vec4& color_ )
    : depType( dep ),
      color( color_ ),
      bounds( bounds_ ),
      body( body_ )
{
}

void entity_t::Sync( void )
{
    if ( !( bounds && body ) )
    {
        return;
    }

    switch ( depType )
    {
        case entity_t::BOUNDS_DEPENDENT:
            body->SetFromTransform( bounds->GetTransform() );
            break;
        case entity_t::BODY_DEPENDENT:
            bounds->SetCenter( body->GetPosition() );
            bounds->SetOrientation( body->GetOrientation() );
            break;
    }
}
