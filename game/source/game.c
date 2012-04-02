#include "game.h"
#include "renderer.h"
#include "debug.h"
#include "types.h"
#include "sound.h"
#include "font.h"

#include <string.h>
#include <math.h>

typedef struct
{
    float		gameTime;

    float       fps;
    float       variance;
    float       remainingPageTime;

    float2		playerPos;
    uint32		lastButtonMask;
} Game;

static Game s_game;

void game_init()
{
    SYS_TRACE_DEBUG( "game_init()\n" );
    renderer_init();
    font_init();

    s_game.gameTime = 0.0f;
    float2_set( &s_game.playerPos, 0.0f, 0.0f );
    s_game.lastButtonMask = 0u;

    s_game.fps = 24.0f;
    s_game.remainingPageTime = 0.0f;
    s_game.variance = 0.2f;
}

void game_done()
{
    font_done();
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

    float fps = s_game.fps;
    if( buttonDownMask & ButtonMask_Up )
    {
        fps += 1.0f;
    }
    if( buttonDownMask & ButtonMask_Down )
    {
        fps -= 1.0f;
    }
    if( s_game.fps != fps )
    {
        s_game.fps = float_max( 1.0f, fps );
        SYS_TRACE_DEBUG( "fps=%f\n", s_game.fps );
    }

    float variance = s_game.variance;
    if( buttonDownMask & ButtonMask_Left )
    {
        variance -= 0.02f;
    }
    if( buttonDownMask & ButtonMask_Right )
    {
        variance += 0.02f;
    }
    if( s_game.variance != variance )
    {
        s_game.variance = float_max( 0.0f, variance );
        SYS_TRACE_DEBUG( "variance=%f\n", s_game.variance );
    }

    const float speed = timeStep * 1.0f;
    float2_scale( &velocity, speed );
    float2_add( &s_game.playerPos, &s_game.playerPos, &velocity );

    sound_setEngineFrequency( ( buttonMask & ButtonMask_Up ) ? 1.0f : 0.0f );
//    float_lerp( 40.0f, 200.0f, float_saturate( carSpeed ) ) );

    s_game.playerPos.x = float_clamp( s_game.playerPos.x, -1.0f, 1.0f );
    s_game.playerPos.y = float_clamp( s_game.playerPos.y, -1.0f, 1.0f );
    s_game.lastButtonMask = buttonMask;

    s_game.remainingPageTime -= timeStep;
}

void game_render()
{
    if( s_game.remainingPageTime < 0.0f )
    {
        renderer_flipPage();
        
        //const float speed = 5.0f;
        const float width = 2.0f;
        const float variance = s_game.variance;

        float2 position = { 1.0f, 1.0f };
        font_drawText( &position, 0.8f, width, variance, "HALLO" );
        position.y += 10.0f;
        font_drawText( &position, 0.8f, width, 2.0f * variance, "HALLO" );
        position.y += 10.0f;
        font_drawText( &position, 0.8f, 1.4f * width, 2.0f * variance, "HALLO" );
        position.y += 10.0f;
        font_drawText( &position, 0.8f, width, 0.5f * variance, "HALLO" );

        static const float2 points[] =
        {
            { 0.0f, 1.0f },
            { 1.0f, 0.0f },
            { 6.0f, 5.0f }
        };

        static const StrokeDefinition strokeDefinition = { points, SYS_COUNTOF( points ) };
        //renderer_startStroke( &strokeDefinition, &position, speed, width, variance );
        float2_set( &position, 40.0f, 1.0f );
        renderer_drawStroke( &strokeDefinition, &position, 2.0f, width, variance );

        s_game.remainingPageTime = 1.0f / s_game.fps;
    }
    
    FrameData frame;
    memset( &frame, 0u, sizeof( frame ) );
    //frame.time = s_game.gameTime;
    //frame.playerPos = s_game.playerPos;
    renderer_drawFrame( &frame );
}

