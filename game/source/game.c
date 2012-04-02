#include "game.h"
#include "player.h"
#include "renderer.h"
#include "debug.h"
#include "types.h"
#include "sound.h"
#include "font.h"
#include "input.h"
#include "vector.h"

enum
{
	MaxPlayer = 8u
};

typedef struct
{
    float		gameTime;

	float       drawSpeed;
	float       variance;

	Player		player[ MaxPlayer ];
	uint		playerCount;
} Game;

static Game s_game;

static void debug_update( uint buttonMask, uint lastButtonMask )
{
	const uint32 buttonDownMask = buttonMask & ~lastButtonMask;

	float drawSpeed = s_game.drawSpeed;
	if( buttonDownMask & ButtonMask_CtrlUp )
	{
		drawSpeed *= 1.5f;
	}
	if( buttonDownMask & ButtonMask_CtrlDown )
	{
		drawSpeed /= 1.5f;
	}
	if( s_game.drawSpeed != drawSpeed )
	{
		s_game.drawSpeed = float_saturate( drawSpeed );
		SYS_TRACE_DEBUG( "drawSpeed=%f\n", s_game.drawSpeed );
        renderer_setDrawSpeed( s_game.drawSpeed );
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
    renderer_init();
	font_init();

	s_game.drawSpeed = 0.2f;
    renderer_setDrawSpeed( s_game.drawSpeed );

	s_game.variance = 0.0f;

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

	debug_update( pInput->buttonMask, s_game.player[ 0u ].lastButtonMask );

	player_update( &s_game.player[ 0u ], timeStep, pInput->buttonMask );
}

void game_render_car( const Player* pPlayer )
{
	float2 points[] =
	{ 
		{ -1.0f,  0.5f },
		{  1.0f,  0.5f },
		{  1.0f, -0.5f },
		{ -1.0f, -0.5f },
		{ -1.0f,  0.5f }
	};

	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );

	float2 worldScale;
	float2_set( &worldScale, 1.8f, 1.8f );

	for( uint i = 0u; i < SYS_COUNTOF( points ); ++i )
	{
		float2_rotate( &points[ i ], pPlayer->direction );
		float2_add( &points[ i ], &points[ i ], &pPlayer->position );
		float2_scale2f( &points[ i ], &worldScale );
		float2_add( &points[ i ], &points[ i ], &worldOffset );
	}

	const StrokeDefinition strokeDefinition = { points, SYS_COUNTOF( points ) };
	
    float2x3 transform;
    float2x2_identity( &transform.rot );
    float2_set( &transform.pos, 0.0f, 0.0f );
	renderer_addStroke( &strokeDefinition, Pen_Font, &transform, s_game.variance );
}

void game_render()
{
	if( renderer_isPageDone() )
	{
        // new page:
		renderer_flipPage();

		//const float speed = 5.0f;
		//const float width = 2.0f;
		const float variance = s_game.variance;

		float2 position = { 1.0f, 1.0f };
		font_drawText( &position, 0.8f, variance, "HALLO" );
		position.y += 10.0f;
		font_drawText( &position, 0.8f, 2.0f * variance, "HALLO" );
		position.y += 10.0f;
		font_drawText( &position, 0.8f, 2.0f * variance, "HALLO" );
		position.y += 10.0f;
		font_drawText( &position, 0.8f, 0.5f * variance, "HALLO" );

		const float2 points[] =
		{
			{ 0.0f, 0.0f },
			{ 10.0f, 0.0f },
//			{ 6.0f, 5.0f }
		};

		const StrokeDefinition stroke = { points, SYS_COUNTOF( points ) };
        float2x3 transform;
        float2x2_identity( &transform.rot );
		float2_set( &transform.pos, 40.0f, 10.0f );
		renderer_addStroke( &stroke, Pen_Font, &transform, variance );

		for( uint i = 0u; i < s_game.playerCount; ++i )
		{
//			game_render_car( &s_game.player[ i ] );
		}
	}
    renderer_updatePage( 1.0f / 60.0f );

	FrameData frame;
    //memset( &frame, 0u, sizeof( frame ) );
    //frame.time = s_game.gameTime;
    //frame.playerPos = s_game.player[ 0u ].position;
    renderer_drawFrame( &frame );
}

