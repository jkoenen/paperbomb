#include "game.h"
#include "player.h"
#include "renderer.h"
#include "debug.h"
#include "types.h"
#include "sound.h"
#include "font.h"
#include "input.h"
#include "vector.h"
#include "gamestate.h"
#include "world.h"

#include <string.h>

typedef struct
{
    float		gameTime;
	float		updateTime;
	uint32		lastButtonMask;

	float       drawSpeed;
	float       variance;

	GameState	gameState;

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

	s_game.drawSpeed = 1.0f;
    renderer_setDrawSpeed( s_game.drawSpeed );

	s_game.variance = 0.0f;

    s_game.gameTime = 0.0f;
	s_game.updateTime = 0.0f;
	s_game.lastButtonMask = 0u;

	gamestate_init( &s_game.gameState, 2u );
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
	s_game.updateTime += timeStep;

	uint32 buttonMask = pInput->buttonMask;
	while( s_game.updateTime >= GAMETIMESTEP )
	{
		debug_update(buttonMask, s_game.lastButtonMask );

		uint32 playerInputs[ MaxPlayer ];
		memset( playerInputs, sizeof( playerInputs ), 0u );
		playerInputs[ 0u ] = buttonMask & Button_PlayerMask;
		playerInputs[ 1u ] = ( buttonMask >> Button_PlayerShift ) & Button_PlayerMask;

		World world;

		gamestate_update( &s_game.gameState, &world, playerInputs );

		s_game.updateTime -= GAMETIMESTEP;
		s_game.lastButtonMask = buttonMask;
		buttonMask = 0u;
	}
}

void game_render_car( const Player* pPlayer )
{
	float2 carPoints[] =
	{ 
		{ -1.0f,  0.5f },
		{  1.0f,  0.5f },
		{  1.0f, -0.5f },
		{ -1.0f, -0.5f },
		{ -1.0f,  0.5f }
	};

	float2 steerPoints[] =
	{ 
		{  0.0f,  0.0f },
		{  0.5f,  0.0f }
	};

	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );
	float2 worldScale;
	float2_set( &worldScale, 1.0f, 1.0f );

	for( uint i = 0u; i < SYS_COUNTOF( carPoints ); ++i )
	{
		float2_rotate( &carPoints[ i ], pPlayer->direction );
		float2_add( &carPoints[ i ], &carPoints[ i ], &pPlayer->position );
		float2_scale2f( &carPoints[ i ], &worldScale );
		float2_add( &carPoints[ i ], &carPoints[ i ], &worldOffset );
	}

	float2 steerOffset;
	float2_set( &steerOffset, 1.0f, 0.6f );
	for( uint i = 0u; i < SYS_COUNTOF( steerPoints ); ++i )
	{
		float2_rotate( &steerPoints[ i ], pPlayer->steer );
		float2_add( &steerPoints[ i ], &steerPoints[ i ], &steerOffset );
		float2_rotate( &steerPoints[ i ], pPlayer->direction );
		float2_add( &steerPoints[ i ], &steerPoints[ i ], &pPlayer->position );
		float2_scale2f( &steerPoints[ i ], &worldScale );
		float2_add( &steerPoints[ i ], &steerPoints[ i ], &worldOffset );
	}

    float2x3 transform;
    float2x2_identity( &transform.rot );
    float2_set( &transform.pos, 0.0f, 0.0f );

	const StrokeDefinition carStrokes = { carPoints, SYS_COUNTOF( carPoints ) };	
	renderer_addStroke( &carStrokes, Pen_Default, &transform, 0.0f );

	const StrokeDefinition steerStrokes = { steerPoints, SYS_COUNTOF( steerPoints ) };	
	renderer_addStroke( &steerStrokes, Pen_Default, &transform, 0.0f );
}

void game_render_bomb( const Bomb* pBomb )
{
	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );
	float2 worldScale;
	float2_set( &worldScale, 1.0f, 1.0f );

	float2 position;
	float2_set( &position, 0.0f, 0.0f );

	float2 bomb0Points[] =
	{ 
		{ -1.0f,  1.0f },
		{  1.0f,  1.0f },
		{  1.0f, -1.0f },
		{ -1.0f, -1.0f },
		{ -1.0f,  1.0f }
	};

	for( uint i = 0u; i < SYS_COUNTOF( bomb0Points ); ++i )
	{
		float2 scale;
		float2_set( &scale, 1.0f, 0.2f );
		float2_scale2f( &bomb0Points[ i ], &scale );
		float2_rotate( &bomb0Points[ i ], pBomb->direction );
		float2_add( &bomb0Points[ i ], &bomb0Points[ i ], &pBomb->position );
		float2_scale2f( &bomb0Points[ i ], &worldScale );
		float2_add( &bomb0Points[ i ], &bomb0Points[ i ], &worldOffset );
	}

	const StrokeDefinition bomb0Strokes = { bomb0Points, SYS_COUNTOF( bomb0Points ) };	
    float2x3 transform;
    float2x2_identity( &transform.rot );
    transform.pos = position;
	renderer_addStroke( &bomb0Strokes, Pen_Default, &transform, 0.0f );

	float2 bomb1Points[] =
	{ 
		{ -1.0f,  1.0f },
		{  1.0f,  1.0f },
		{  1.0f, -1.0f },
		{ -1.0f, -1.0f },
		{ -1.0f,  1.0f }
	};

	for( uint i = 0u; i < SYS_COUNTOF( bomb1Points ); ++i )
	{
		float2 scale;
		float2_set( &scale, 0.2f, 1.0f );
		float2_scale2f( &bomb1Points[ i ], &scale );
		float2_rotate( &bomb1Points[ i ], pBomb->direction );
		float2_add( &bomb1Points[ i ], &bomb1Points[ i ], &pBomb->position );
		float2_scale2f( &bomb1Points[ i ], &worldScale );
		float2_add( &bomb1Points[ i ], &bomb1Points[ i ], &worldOffset );
	}

	const StrokeDefinition bomb1Strokes = { bomb1Points, SYS_COUNTOF( bomb1Points ) };	
    transform.pos = position;
	renderer_addStroke( &bomb1Strokes, Pen_Default, &transform, 0.0f );
}

void game_render_explosion( const Explosion* pExplosion )
{
	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );
	float2 worldScale;
	float2_set( &worldScale, 1.0f, 1.0f );

	float2 position;
	float2_set( &position, 0.0f, 0.0f );

	float2 explosion0Points[] =
	{ 
		{ -1.0f,  1.0f },
		{  1.0f,  1.0f },
		{  1.0f, -1.0f },
		{ -1.0f, -1.0f },
		{ -1.0f,  1.0f }
	};

	for( uint i = 0u; i < SYS_COUNTOF( explosion0Points ); ++i )
	{
		float2 scale;
		float2_set( &scale, pExplosion->length, 1.0f );
		float2_scale2f( &explosion0Points[ i ], &scale );
		float2_rotate( &explosion0Points[ i ], pExplosion->direction );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &pExplosion->position );
		float2_scale2f( &explosion0Points[ i ], &worldScale );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &worldOffset );
	}

	const StrokeDefinition explosion0Strokes = { explosion0Points, SYS_COUNTOF( explosion0Points ) };	
    float2x3 transform;
    float2x2_identity( &transform.rot );
    transform.pos = position;
	renderer_addStroke( &explosion0Strokes, Pen_Default, &transform, 0.0f );

	float2 explosion1Points[] =
	{ 
		{ -1.0f,  1.0f },
		{  1.0f,  1.0f },
		{  1.0f, -1.0f },
		{ -1.0f, -1.0f },
		{ -1.0f,  1.0f }
	};

	for( uint i = 0u; i < SYS_COUNTOF( explosion1Points ); ++i )
	{
		float2 scale;
		float2_set( &scale, 1.0f, pExplosion->length );
		float2_scale2f( &explosion1Points[ i ], &scale );
		float2_rotate( &explosion1Points[ i ], pExplosion->direction );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &pExplosion->position );
		float2_scale2f( &explosion1Points[ i ], &worldScale );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &worldOffset );
	}

	const StrokeDefinition explosion1Strokes = { explosion1Points, SYS_COUNTOF( explosion1Points ) };	
    transform.pos = position;
	renderer_addStroke( &explosion1Strokes, Pen_Default, &transform, 0.0f );
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

		for( uint i = 0u; i < s_game.gameState.playerCount; ++i )
		{
			const Player* pPlayer = &s_game.gameState.player[ i ];

			game_render_car( pPlayer );
			for( uint j = 0u; j < SYS_COUNTOF( pPlayer->bombs ); ++j )
			{
				const Bomb* pBomb = &pPlayer->bombs[ j ];

				if( pBomb->active )
				{
					game_render_bomb( pBomb );
				}
			}
		}

		for( uint i = 0u; i < s_game.gameState.explosionCount; ++i )
		{
			const Explosion* pExplosion = &s_game.gameState.explosions[ i ];
			game_render_explosion( pExplosion );
		}
	}
    renderer_updatePage( 1.0f / 60.0f );

	FrameData frame;
    //memset( &frame, 0u, sizeof( frame ) );
    //frame.time = s_game.gameTime;
    //frame.playerPos = s_game.player[ 0u ].position;
    renderer_drawFrame( &frame );
}

