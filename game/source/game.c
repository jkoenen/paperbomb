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
	s_game.playerCount = 2u;
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

	player_update( &s_game.player[ 0u ], pInput->buttonMask & Button_PlayerMask );
	player_update( &s_game.player[ 1u ], ( pInput->buttonMask >> Button_PlayerShift ) & Button_PlayerMask );

	s_game.remainingPageTime -= timeStep;
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
	float2_set( &worldScale, 1.8f, 1.8f );

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

	float2 position;
	float2_set( &position, 0.0f, 0.0f );

	const StrokeDefinition carStrokes = { carPoints, SYS_COUNTOF( carPoints ) };	
	renderer_drawStroke( &carStrokes, &position, 1.0f, 2.0f, 0.0f );

	const StrokeDefinition steerStrokes = { steerPoints, SYS_COUNTOF( steerPoints ) };	
	renderer_drawStroke( &steerStrokes, &position, 1.0f, 1.0f, 0.0f );
}

void game_render_bomb( const Bomb* pBomb )
{
	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );
	float2 worldScale;
	float2_set( &worldScale, 1.8f, 1.8f );

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
	renderer_drawStroke( &bomb0Strokes, &position, 1.0f, 2.0f, 0.0f );

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
	renderer_drawStroke( &bomb1Strokes, &position, 1.0f, 2.0f, 0.0f );
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

		for( uint i = 0u; i < s_game.playerCount; ++i )
		{
			game_render_car( &s_game.player[ i ] );
			for( uint j = 0u; j < SYS_COUNTOF( s_game.player[ i ].bombs ); ++j )
			{
				if( s_game.player[ i ].bombs[ j ].active )
				{
					game_render_bomb( &s_game.player[ i ].bombs[ j ] );
				}
			}
		}

		s_game.remainingPageTime = 1.0f / s_game.fps;
	}

	FrameData frame;
    //memset( &frame, 0u, sizeof( frame ) );
    //frame.time = s_game.gameTime;
    //frame.playerPos = s_game.player[ 0u ].position;
    renderer_drawFrame( &frame );
}

