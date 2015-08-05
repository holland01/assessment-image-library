#pragma once

#include "def.h"
#include <memory>

struct bounding_box_t;
struct body_t;

class entity_t
{
public:
    enum dependent_t
    {
        BOUNDS_DEPENDENT,
        BODY_DEPENDENT
    };

protected:
    dependent_t depType;

public:

    std::shared_ptr< bounding_box_t > bounds;
    std::shared_ptr< body_t > body;

    entity_t( dependent_t dep, bounding_box_t* bounds = nullptr, body_t* body = nullptr );

    virtual void Sync( void );
};



