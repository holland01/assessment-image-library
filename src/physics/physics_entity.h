#pragma once

#include "def.h"
#include "entity.h"
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <bullet3/btBulletCollisionCommon.h>
#include <bullet3/btBulletDynamicsCommon.h>

//-------------------------------------------------------------------------------
// physics_entity
//-------------------------------------------------------------------------------

class physics_entity
{
public:
    enum type
    {
        type_kinematic = 0,
        type_static,
        type_dynamic
    };

private:
    friend class physics_entity_manager;

    const uint32_t mId;
    const uint32_t mEntityIndex;
    const type mBodyType;

    physics_entity* mNext;

public:
    FORCEINLINE physics_entity( uint32_t id, uint32_t entityIndex, type bodyType )
        : mId( id ),
          mEntityIndex( entityIndex ),
          mBodyType( bodyType ),
          mNext( nullptr )
    {
    }

    FORCEINLINE uint32_t id( void ) const { return mId; }
    FORCEINLINE uint32_t entity_index( void ) const { return mEntityIndex; }
    FORCEINLINE type body_type( void ) const { return mBodyType; }
};

//-------------------------------------------------------------------------------
// physics_entity_manager
//-------------------------------------------------------------------------------

class physics_entity_manager
{
private:
    template < typename buffer_type >
    using managed_buffer_array_t = std::array< std::vector< std::unique_ptr< buffer_type > >, 3 >;

    managed_buffer_array_t< btCollisionShape > mCollisionShapes;
    managed_buffer_array_t< btMotionState > mMotionStates;
    managed_buffer_array_t< btRigidBody > mBodies;

    std::unordered_map< uint32_t, physics_entity > mEntityList;

    uint32_t mEntityCount;

    const entity_manager& mParentMan;

public:
    physics_entity_manager( const entity_manager& parentMan );

    btRigidBody* body( entity e );

    FORCEINLINE const btRigidBody* const_body( entity e ) { return ( const btRigidBody* ) body( e ); }
};

FORCEINLINE btRigidBody* physics_entity_manager::body( entity e )
{
    if ( !mParentMan.alive( e ) )
        return nullptr;

    auto piter = mEntityList.find( e.index() );

    if ( piter == mEntityList.end() )
        return nullptr;

    const physics_entity& p = piter->second;

    return mBodies[ p.body_type() ][ p.id() ].get();
}
