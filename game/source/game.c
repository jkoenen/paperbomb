#include "game.h"
#include "renderer.h"
#include "debug.h"
#include "types.h"
#include "sound.h"
#include "font.h"
#include "input.h"
#include "vector.h"
#include "world.h"
#include "server.h"
#include "client.h"

#include <string.h>
#include <memory.h>

enum 
{
	GameState_Menu,
	GameState_Play
};

typedef struct
{
    float		gameTime;
	float		updateTime;
	uint32		lastButtonMask[ MaxPlayer ];

	float       drawSpeed;
	float       variance;

    uint32      debugLastButtonMask;

	World		world;

	char		playerName[ 12u ];
	char		serverIP[ 16u ];
	int			state;
	int			isServer;

	Client		client;
	Server		server;

} Game;

static Game s_game;

static void game_switch_state( int state )
{
	if( s_game.state == GameState_Play )
	{
		client_destroy( &s_game.client );

		if( s_game.isServer )
		{
			server_destroy( &s_game.server );
		}
	}

	if( state == GameState_Play )
	{
        SYS_TRACE_DEBUG( "starting game\n" );
		IP4Address address;
		address.port = NetworkPort;

		if( s_game.isServer )
		{
			server_create( &s_game.server, NetworkPort );

			address.address = socket_gethostIP();
		}
		else
		{
			address.address = socket_parseIP( s_game.serverIP );
		}

		client_create( &s_game.client, &address, s_game.playerName );
	}

	s_game.state = state;
}

static void debug_update( uint buttonMask, uint buttonDownMask )
{
	(void)buttonMask;

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

	copyString( s_game.serverIP, sizeof( s_game.serverIP ), "10.1.11.4" );
	copyString( s_game.playerName, sizeof( s_game.playerName ), "Horst" );

	float2x2_identity( &s_game.world.worldTransform.rot );
	float2x2_scale2f( &s_game.world.worldTransform.rot, &s_game.world.worldTransform.rot, 0.8f, 0.8f );
	float2_set( &s_game.world.worldTransform.pos, 32.0f, 16.0f );

	float2_set( &s_game.world.borderMin, -16.0f, -16.0f );
	float2_set( &s_game.world.borderMax,  16.0f,  16.0f );

	s_game.state = GameState_Menu;
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

	const uint32 buttonMask = pInput->buttonMask;
	const uint32 buttonDownMask = buttonMask & ~s_game.debugLastButtonMask;
	s_game.debugLastButtonMask = buttonMask;

	debug_update( buttonMask, buttonDownMask );

	switch( s_game.state )
	{	
		case GameState_Menu:
			{
				if( buttonDownMask & ButtonMask_Client )
				{
					s_game.isServer = FALSE;
					game_switch_state( GameState_Play );
				}
				else if( buttonDownMask & ButtonMask_Server )
				{
					s_game.isServer = TRUE;
					game_switch_state( GameState_Play );
				}
			}
			break;

		case GameState_Play:
			{
				int quit = 0;
				while( s_game.updateTime >= GAMETIMESTEP )
				{
					quit |= client_update( &s_game.client, buttonMask & Button_PlayerMask );
					if( s_game.isServer )
					{
						server_update( &s_game.server, &s_game.world );
					}
					sound_setEngineFrequency( ( buttonMask & ButtonMask_Up ) ? 1.0f : 0.0f );

					s_game.updateTime -= GAMETIMESTEP;
				}

				if( quit || ( buttonDownMask & ButtonMask_Leave ) )
				{
					game_switch_state( GameState_Menu );
				}
			}
			break;

		default:
			break;
	}
}

static void game_render_car( const ClientPlayer* pPlayer, const float2x3* pWorldTransform )
{
	if( pPlayer->age <= time_quantize( 1.0f ) )
	{
		if( ( pPlayer->age / 4u ) & 1 )
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

	float2 position;
	position.x = float_unquantize( pPlayer->posX );
	position.y = float_unquantize( pPlayer->posY );

	const float direction = angle_unquantize( pPlayer->direction );

	const float steer = angle_unquantize( pPlayer->steer );

	for( uint i = 0u; i < SYS_COUNTOF( carPoints ); ++i )
	{
		float2_rotate( &carPoints[ i ], direction );
		float2_add( &carPoints[ i ], &carPoints[ i ], &position );
	}

	float2 steerOffset;
	float2_set( &steerOffset, 1.0f, 0.6f );
	for( uint i = 0u; i < SYS_COUNTOF( steerPoints ); ++i )
	{
		float2_rotate( &steerPoints[ i ], steer );
		float2_add( &steerPoints[ i ], &steerPoints[ i ], &steerOffset );
		float2_rotate( &steerPoints[ i ], direction );
		float2_add( &steerPoints[ i ], &steerPoints[ i ], &position );
	}

    renderer_setTransform( pWorldTransform );
    renderer_addLinearStroke( carPoints, SYS_COUNTOF( carPoints ) );
	renderer_addLinearStroke( carPoints, SYS_COUNTOF( carPoints ) );
    //renderer_addStroke( steerPoints, SYS_COUNTOF( steerPoints ) );
}

static void game_render_world( const World* pWorld )
{
	renderer_setVariance( 0.0f );
	renderer_setPen( Pen_DebugGreen );

	renderer_setTransform( &pWorld->worldTransform );
	
	float2 points[] =
	{ 
		{  pWorld->borderMin.x, pWorld->borderMin.y },
		{  pWorld->borderMax.x, pWorld->borderMin.y },
		{  pWorld->borderMax.x, pWorld->borderMax.y },
		{  pWorld->borderMin.x, pWorld->borderMax.y },
		{  pWorld->borderMin.x, pWorld->borderMin.y },
	};
	
	renderer_addLinearStroke( points, SYS_COUNTOF( points ) );
}

static void game_render_bomb( const ClientBomb* pBomb, const float2x3* pWorldTransform )
{
	(void)pWorldTransform;

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

	float2 position;
	position.x = float_unquantize( pBomb->posX );
	position.y = float_unquantize( pBomb->posY );

	const float direction = angle_unquantize( pBomb->direction );

    float2x3 bombTransform;
    float2x2_rotationY( &bombTransform.rot, direction );
    float2x2_scale2f( &bombTransform.rot, &bombTransform.rot, 1.0f, 1.0f );

	renderer_setVariance( 0.0f );
	renderer_setPen( Pen_DebugGreen );

    renderer_setTransform( &bombTransform );
	renderer_addLinearStroke( bombPoints0, SYS_COUNTOF( bombPoints0 ) );
}

static void game_render_explosion( const ClientExplosion* pExplosion )
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

	float2 position;
	position.x = float_unquantize( pExplosion->posX );
	position.y = float_unquantize( pExplosion->posY );

	const float direction = angle_unquantize( pExplosion->direction );
	const float length = (float)pExplosion->length;

	renderer_setPen( Pen_DebugRed );

	for( uint i = 0u; i < SYS_COUNTOF( explosion0Points ); ++i )
	{
		float2 scale;
		float2_set( &scale, length, 1.0f );
		float2_scale2f( &explosion0Points[ i ], &explosion0Points[i], &scale );
		float2_rotate( &explosion0Points[ i ], direction );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &position );
		float2_scale2f( &explosion0Points[ i ], &explosion0Points[i], &worldScale );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &worldOffset );
	}

	renderer_setTransform( 0 );

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
		float2_set( &scale, 1.0f, length );
		float2_scale2f( &explosion1Points[ i ], &explosion1Points[i], &scale );
		float2_rotate( &explosion1Points[ i ], direction );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &position );
		float2_scale2f( &explosion1Points[ i ], &explosion1Points[i], &worldScale );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &worldOffset );
	}

	renderer_addLinearStroke( explosion1Points, SYS_COUNTOF( explosion1Points ) );
}

static void game_render_burnhole( const ClientExplosion* pExplosion )
{
	float2 worldOffset;
	float2_set( &worldOffset, 32.0f, 18.0f );
	float2 worldScale;
	float2_set( &worldScale, 1.0f, 1.0f );

	float2 explosion0Points[] =
	{ 
		{ -1.0f,  0.0f },
		{  1.0f,  0.0f },
	};

	float2 position;
	position.x = float_unquantize( pExplosion->posX );
	position.y = float_unquantize( pExplosion->posY );

	const float direction = angle_unquantize( pExplosion->direction );
	const float length = (float)pExplosion->length;

	for( uint i = 0u; i < SYS_COUNTOF( explosion0Points ); ++i )
	{
		float2 scale;
		float2_set( &scale, length, 1.0f );
		float2_scale2f( &explosion0Points[ i ], &explosion0Points[i], &scale );
		float2_rotate( &explosion0Points[ i ], direction );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &position );
		float2_scale2f( &explosion0Points[ i ], &explosion0Points[i], &worldScale );
		float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &worldOffset );
	}

	renderer_setTransform( 0 );
	renderer_addBurnHole( &explosion0Points[ 0u ], &explosion0Points[ 1u ], 2.0f ); 

	float2 explosion1Points[] =
	{ 
		{  0.0f,  1.0f },
		{  0.0f, -1.0f },
	};

	for( uint i = 0u; i < SYS_COUNTOF( explosion1Points ); ++i )
	{
		float2 scale;
		float2_set( &scale, 1.0f, length );
		float2_scale2f( &explosion1Points[ i ], &explosion1Points[i], &scale );
		float2_rotate( &explosion1Points[ i ], direction );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &position );
		float2_scale2f( &explosion1Points[ i ], &explosion1Points[i], &worldScale );
		float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &worldOffset );
	}

	renderer_addBurnHole( &explosion1Points[ 0u ], &explosion1Points[ 1u ], 2.0f ); 
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

		if( s_game.state == GameState_Play )
		{
			ClientGameState* pGameState = &s_game.client.gameState;

			game_render_world( &s_game.world );

			for( uint i = 0u; i < SYS_COUNTOF( pGameState->player ); ++i )
			{
				const ClientPlayer* pPlayer = &pGameState->player[ i ];
				if( pPlayer->state != PlayerState_InActive )
				{
					game_render_car( pPlayer, &s_game.world.worldTransform );
				}
			}
			for( uint i = 0u; i < SYS_COUNTOF( pGameState->bombs ); ++i )
			{
				const ClientBomb* pBomb = &pGameState->bombs[ i ];
				if( pBomb->time > 0u )
				{
					game_render_bomb( pBomb, &s_game.world.worldTransform );
				}
			}
			for( uint i = 0u; i < SYS_COUNTOF( pGameState->explosions ); ++i )
			{
				const ClientExplosion* pExplosion = &pGameState->explosions[ i ];
				if( pExplosion->time > 0u )
				{
					game_render_explosion( pExplosion );
				}
				if( s_game.client.explosionTriggered[ i ] )
				{
					game_render_burnhole( pExplosion );
					s_game.client.explosionTriggered[i] = 0;
				}
			}
		}
	}
    renderer_updatePage( 1.0f / 60.0f );

	FrameData frame;
    //memset( &frame, 0u, sizeof( frame ) );
    //frame.time = s_game.gameTime;
    //frame.playerPos = s_game.player[ 0u ].position;
    renderer_drawFrame( &frame );
}

