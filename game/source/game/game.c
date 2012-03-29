#include "game/game.h"
#include "graphics/renderer.h"
#include "sys/debug.h"

#include <string.h>

typedef struct
{
    float       gameTime;

    float2_t    playerPos;
    uint32_t     lastButtonMask;
} game_t;

static game_t s_game;

void game_init()
{
    SYS_TRACE_DEBUG( "game_init()\n" );
    renderer_init();

    s_game.gameTime = 0.0f;
    float2_set( &s_game.playerPos, 0.0f, 0.0f );
    s_game.lastButtonMask = 0u;
}

void game_done()
{
    renderer_done();
}

void game_update( const gameinput_t* pGameInput )
{
    s_game.gameTime += pGameInput->timeStep;

    float2_t velocity;
    float2_set( &velocity, 0.0f, 0.0f );

    const uint32_t buttonMask = pGameInput->buttonMask;
    const uint32_t buttonDownMask = buttonMask & ~s_game.lastButtonMask;
    //const uint32_t buttonUpMask = ~buttonMask & s_game.lastButtonMask;

    if( buttonMask & ButtonMask_Left )
    {
        velocity.x = -1.0f;
    }
    if( buttonMask & ButtonMask_Right )
    {
        velocity.x += 1.0f;
    }
    if( buttonMask & ButtonMask_Up )
    {
        velocity.y = 1.0f;
    }
    if( buttonMask & ButtonMask_Down )
    {
        velocity.y -= 1.0f;
    }
    if( buttonDownMask & ButtonMask_PlaceBomb )
    {
        SYS_TRACE_DEBUG( "place bomb!\n" );
    }

    const float speed = pGameInput->timeStep * 1.0f;
    float2_scale( &velocity, speed );
    float2_add( &s_game.playerPos, &s_game.playerPos, &velocity );

    s_game.playerPos.x = float_clamp( s_game.playerPos.x, -1.0f, 1.0f );
    s_game.playerPos.y = float_clamp( s_game.playerPos.y, -1.0f, 1.0f );
    s_game.lastButtonMask = buttonMask;
}

void game_render()
{
    framedata_t frame;
    memset( &frame, 0u, sizeof( frame ) );
    frame.time = s_game.gameTime;
    frame.playerPos = s_game.playerPos;
    renderer_drawFrame( &frame );
}

