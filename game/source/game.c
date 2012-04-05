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
#include "server.h"
#include <string.h>
#include <memory.h>

enum
{
	NetworkPort = 2357u
};

typedef struct
{
    float		gameTime;
	float		updateTime;
	uint32		lastButtonMask[ MaxPlayer ];

	float       drawSpeed;
	float       variance;

	GameState	gameState;
    uint32      debugLastButtonMask;

	Server		server;

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
        renderer_setVariance( s_game.variance );
	}
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

    for( uint i = 0u; i < MaxPlayer; ++i )
    {
    	s_game.lastButtonMask[ i ] = 0u;
    }

	gamestate_init( &s_game.gameState, 2u );

	server_create( &s_game.server, NetworkPort );
}

void game_done()
{
	server_destroy( &s_game.server );

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
		debug_update(buttonMask, s_game.debugLastButtonMask );
        s_game.debugLastButtonMask = buttonMask;

		PlayerInput playerInputs[ MaxPlayer ];
		memset( playerInputs, 0u, sizeof( playerInputs ) );
		playerInputs[ 0u ].buttonMask = buttonMask & Button_PlayerMask;
        playerInputs[ 0u ].buttonDownMask = playerInputs[ 0u ].buttonMask & ~s_game.lastButtonMask[ 0u ];
        s_game.lastButtonMask[ 0u ] = playerInputs[ 0u ].buttonMask;
		playerInputs[ 1u ].buttonMask = ( buttonMask >> Button_PlayerShift ) & Button_PlayerMask;
        playerInputs[ 1u ].buttonDownMask = playerInputs[ 1u ].buttonMask & ~s_game.lastButtonMask[ 1u ];
        s_game.lastButtonMask[ 1u ] = playerInputs[ 1u ].buttonMask;


		World world;
		gamestate_update( &s_game.gameState, &world, playerInputs );
	
        sound_setEngineFrequency( ( buttonMask & ButtonMask_Up ) ? 1.0f : 0.0f );

		s_game.updateTime -= GAMETIMESTEP;
	}
}

void game_render_car( const Player* pPlayer )
{
	if( pPlayer->age <= s_playerBulletProofAge )
	{
		const int frames = (int)( pPlayer->age * 15.0f );
		if( frames & 1 )
		{
			return;
		}
	}
	
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
		float2_scale2f( &carPoints[ i ], &carPoints[i], &worldScale );
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
		float2_scale2f( &steerPoints[ i ], &steerPoints[i], &worldScale );
		float2_add( &steerPoints[ i ], &steerPoints[ i ], &worldOffset );
	}

    renderer_setTransform( 0 );
    renderer_addLinearStroke( carPoints, SYS_COUNTOF( carPoints ) );
	renderer_addLinearStroke( carPoints, SYS_COUNTOF( carPoints ) );
    //renderer_addStroke( steerPoints, SYS_COUNTOF( steerPoints ) );
}

void game_render_bomb( const Bomb* pBomb )
{
	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );

	float2 bombPoints0[] =
	{ 
		{ -1.0f,  0.2f },
		{  1.0f,  0.2f },
		{  1.0f, -0.2f },
		{ -1.0f, -0.2f },
		{ -1.0f,  0.2f },
        { -1.0f,  0.2f },
		{ -0.2f,  1.0f },
		{  0.2f,  1.0f },
		{  0.2f, -1.0f },
		{ -0.2f, -1.0f },
		{ -0.2f,  1.0f }
	};

    float2x3 bombTransform;
    float2x2_rotationY( &bombTransform.rot, pBomb->direction );
    float2x2_scale2f( &bombTransform.rot, &bombTransform.rot, 1.0f, 1.0f );
    float2_add( &bombTransform.pos, &pBomb->position, &worldOffset );

    renderer_setTransform( &bombTransform );
	renderer_addLinearStroke( bombPoints0, SYS_COUNTOF( bombPoints0 ) );
}

void game_render_explosion( const Explosion* pExplosion )
{
	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );
	float2 worldScale;
	float2_set( &worldScale, 1.0f, 1.0f );

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
		float2_scale2f( &explosion0Points[ i ], &explosion0Points[i], &scale );
		float2_rotate( &explosion0Points[ i ], pExplosion->direction );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &pExplosion->position );
		float2_scale2f( &explosion0Points[ i ], &explosion0Points[i], &worldScale );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &worldOffset );
	}

	renderer_addLinearStroke( explosion0Points, SYS_COUNTOF( explosion0Points ) ); 

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
		float2_scale2f( &explosion1Points[ i ], &explosion1Points[i], &scale );
		float2_rotate( &explosion1Points[ i ], pExplosion->direction );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &pExplosion->position );
		float2_scale2f( &explosion1Points[ i ], &explosion1Points[i], &worldScale );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &worldOffset );
	}

	renderer_addLinearStroke( explosion1Points, SYS_COUNTOF( explosion1Points ) );
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

        renderer_setPen( Pen_Font );
		float2 position = { 1.0f, 1.0f };
		font_drawText( &position, 0.8f, variance, "HALLO" );
		position.y += 10.0f;
		font_drawText( &position, 0.8f, 2.0f * variance, "HALLO" );
		position.y += 10.0f;
		font_drawText( &position, 0.8f, 2.0f * variance, "HALLO" );
		position.y += 10.0f;
		font_drawText( &position, 0.8f, 0.5f * variance, "HALLO" );

        renderer_setTransform( 0 );

        renderer_setPen( Pen_Default );

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

