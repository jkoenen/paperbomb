#include "game.h"
#include "renderer.h"
#include "debug.h"
#include "types.h"
#include "sound.h"

#include <string.h>
#include <math.h>

typedef struct
{
    float		gameTime;

    float2		playerPos;
    uint32		lastButtonMask;
} Game;

static Game s_game;

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

void game_update( const GameInput* pInput )
{
    const float timeStep = pInput->timeStep;

    s_game.gameTime += timeStep;

    float2 velocity;
    float2_set( &velocity, 0.0f, 0.0f );

    const uint32 buttonMask = pInput->buttonMask;
    const uint32 buttonDownMask = buttonMask & ~s_game.lastButtonMask;
    //const uint32 buttonUpMask = ~buttonMask & s_game.lastButtonMask;

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

    const float speed = timeStep * 1.0f;
    float2_scale( &velocity, speed );
    float2_add( &s_game.playerPos, &s_game.playerPos, &velocity );

    sound_setEngineFrequency( ( buttonMask & ButtonMask_Up ) ? 1.0f : 0.0f );
//    float_lerp( 40.0f, 200.0f, float_saturate( carSpeed ) ) );

    s_game.playerPos.x = float_clamp( s_game.playerPos.x, -1.0f, 1.0f );
    s_game.playerPos.y = float_clamp( s_game.playerPos.y, -1.0f, 1.0f );
    s_game.lastButtonMask = buttonMask;
}

void game_render()
{
    FrameData frame;
    memset( &frame, 0u, sizeof( frame ) );
    frame.time = s_game.gameTime;
    frame.playerPos = s_game.playerPos;
    renderer_drawFrame( &frame );
}

