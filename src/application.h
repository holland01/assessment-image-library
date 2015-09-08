#pragma once


#include "geom.h"
#include "input.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <memory>

struct application
{
public:
    bool running = false;

    bool mouseShown = true;

    bool drawAll = false;

    SDL_Window* window = nullptr;

    SDL_Renderer* renderer = nullptr;

    SDL_GLContext context = nullptr;

    uint32_t width, height;

    float frameTime, lastTime, startTime;

    std::unique_ptr< render_pipeline > pipeline;

    input_client player, spec;

    input_client* camera;

    obb* drawBounds;

    physics_world world;

    view_frustum frustum;

    application( uint32_t width, uint32_t height );

    virtual ~application( void );

    void make_body( input_client& dest, input_client::client_mode mode, const glm::vec3& pos, float mass );

    void toggle_culling( void );

    virtual void draw( void );

    virtual void handle_event( const SDL_Event& e );

    static application& get_instance( void );
};


