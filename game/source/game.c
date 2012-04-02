#include "game.h"
#include "player.h"
#include "renderer.h"
#include "debug.h"
#include "types.h"
#include "sound.h"
#include "font.h"
#include "input.h"

#include <math.h>

enum
{
	MaxPlayer = 8u
};

typedef struct
{
    float		gameTime;

	float       fps;
	float       variance;
	float       remainingPageTime;

	Player		player[ MaxPlayer ];
	uint		playerCount;
} Game;

static Game s_game;

static void debug_update( uint buttonMask, uint lastButtonMask )
{
	const uint32 buttonDownMask = buttonMask & ~lastButtonMask;

	float fps = s_game.fps;
	if( buttonDownMask & ButtonMask_CtrlUp )
	{
		fps += 1.0f;
	}
	if( buttonDownMask & ButtonMask_CtrlDown )
	{
		fps -= 1.0f;
	}
	if( s_game.fps != fps )
	{
		s_game.fps = float_max( 1.0f, fps );
		SYS_TRACE_DEBUG( "fps=%f\n", s_game.fps );
	}

	float variance = s_game.variance;
	if( buttonDownMask & ButtonMask_CtrlLeft )
	{
		variance -= 0.02f;
	}
	if( buttonDownMask & ButtonMask_CtrlRight )
	{
		variance += 0.02f;
	}
	if( s_game.variance != variance )
	{
		s_game.variance = float_max( 0.0f, variance );
		SYS_TRACE_DEBUG( "variance=%f\n", s_game.variance );
	}

	sound_setEngineFrequency( ( buttonMask & ButtonMask_CtrlUp ) ? 1.0f : 0.0f );

}

void game_init()
{
    SYS_TRACE_DEBUG( "game_init()\n" );
    renderer_init();
	font_init();

	s_game.fps = 24.0f;
	s_game.remainingPageTime = 0.0f;
	s_game.variance = 0.2f;

    s_game.gameTime = 0.0f;
	s_game.playerCount = 1u;
	for( uint i = 0u; i < s_game.playerCount; ++i )
	{
		player_reset( &s_game.player[ i ] );
	}
}

void game_done()
{
    renderer_done();
	font_done();
}

void game_update( const GameInput* pInput )
{
    const float timeStep = pInput->timeStep;

    s_game.gameTime += timeStep;

	debug_update( pInput->buttonMask, s_game.player[ 0u ].lastInputMask );

	player_update( &s_game.player[ 0u ], timeStep, pInput->buttonMask );

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
    //memset( &frame, 0u, sizeof( frame ) );
    //frame.time = s_game.gameTime;
    //frame.playerPos = s_game.player[ 0u ].position;
    renderer_drawFrame( &frame );
}

