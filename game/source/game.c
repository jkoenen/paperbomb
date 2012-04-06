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
#include <stdio.h>

enum 
{
	GameState_Menu,
	GameState_Play
};

typedef struct
{
    float		renderTime;
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

	s_game.variance = 0.05f;
	renderer_setVariance( s_game.variance );

    s_game.renderTime = 0.0f;
	s_game.updateTime = 0.0f;

    for( uint i = 0u; i < MaxPlayer; ++i )
    {
    	s_game.lastButtonMask[ i ] = 0u;
    }

	copyString( s_game.serverIP, sizeof( s_game.serverIP ), "10.1.11.5" );
	copyString( s_game.playerName, sizeof( s_game.playerName ), "Horst" );

	float2_set( &s_game.world.borderMin, -20.0f, -20.0f );
	float2_set( &s_game.world.borderMax,  20.0f,  20.0f );

	float2x2_identity( &s_game.world.worldTransform.rot );
	float2x2_scale2f( &s_game.world.worldTransform.rot, &s_game.world.worldTransform.rot, 0.7f, 0.7f );
	float2_set( &s_game.world.worldTransform.pos, 32.0f, 20.0f );

	float2_set( &s_game.world.rockz[ 0u ].center, -10.0f, -10.0f );
	s_game.world.rockz[ 0u ].radius = 2.5f;
	float2_set( &s_game.world.rockz[ 1u ].center,  10.0f, -10.0f );
	s_game.world.rockz[ 1u ].radius = 2.5f;
	float2_set( &s_game.world.rockz[ 2u ].center,  10.0f,  10.0f );
	s_game.world.rockz[ 2u ].radius = 2.5f;
	float2_set( &s_game.world.rockz[ 3u ].center, -10.0f,  10.0f );
	s_game.world.rockz[ 3u ].radius = 2.5f;

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

    s_game.renderTime += timeStep;

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
				s_game.updateTime += timeStep;

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

	float2 position;
	position.x = float_unquantize( pPlayer->posX );
	position.y = float_unquantize( pPlayer->posY );

	const float direction = angle_unquantize( pPlayer->direction );
	const float steer = angle_unquantize( pPlayer->steer );

	for( uint i = 0u; i < SYS_COUNTOF( carPoints ); ++i )
	{
		float2_scale1f( &carPoints[ i ], &carPoints[ i ], 1.5f );
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
	float2 points[] =
	{ 
		{  pWorld->borderMin.x, pWorld->borderMin.y },
		{  pWorld->borderMax.x, pWorld->borderMin.y },
		{  pWorld->borderMax.x, pWorld->borderMax.y },
		{  pWorld->borderMin.x, pWorld->borderMax.y },
		{  pWorld->borderMin.x, pWorld->borderMin.y },
	};
	
	renderer_setTransform( &pWorld->worldTransform );
	renderer_addLinearStroke( points, SYS_COUNTOF( points ) );

	for( uint i = 0u; i < SYS_COUNTOF( pWorld->rockz ); ++i )
	{
		renderer_addCircle( &pWorld->rockz[ i ] );
	}
}

static void game_render_bomb( const ClientBomb* pBomb, const float2x3* pWorldTransform )
{
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

	for( uint i = 0u; i < SYS_COUNTOF( bombPoints0 ); ++i )
	{
		float2_rotate( &bombPoints0[ i ], direction );
		float2_add( &bombPoints0[ i ], &bombPoints0[ i ], &position );
	}

    renderer_setTransform( pWorldTransform );
	renderer_addLinearStroke( bombPoints0, SYS_COUNTOF( bombPoints0 ) );
}

static void game_render_item( const ClientItem* pItem, const float2x3* pWorldTransform )
{
	float2 position;
	position.x = float_unquantize( pItem->posX );
	position.y = float_unquantize( pItem->posY );

	renderer_setTransform( pWorldTransform );

	float2 points[] =
	{ 
		{ -1.0f, -1.0f },
		{  1.0f, -1.0f },
		{  1.0f,  1.0f },
		{ -1.0f,  1.0f },
		{ -1.0f, -1.0f },
	};

	for( uint i = 0u; i < SYS_COUNTOF( points ); ++i )
	{
		float2_add( &points[ i ], &points[ i ], &position );
	}

	switch( pItem->type )
	{
		case ItemType_BombRange:
			renderer_setPen( Pen_DebugRed );
			break;

		case ItemType_ExtraBomb:
			renderer_setPen( Pen_DebugGreen );
			break;
	}

	renderer_addLinearStroke( points, SYS_COUNTOF( points ) );
}

static void game_render_burnhole( const ClientExplosion* pExplosion, const float2x3* pWorldTransform )
{
	float2 position;
	position.x = float_unquantize( pExplosion->posX );
	position.y = float_unquantize( pExplosion->posY );

	const float direction = angle_unquantize( pExplosion->direction );
	const float length0 = (float)pExplosion->length[ 0u ] - s_burnRadius;
	const float length1 = (float)pExplosion->length[ 2u ] - s_burnRadius;
	const float length2 = (float)pExplosion->length[ 1u ] - s_burnRadius;
	const float length3 = (float)pExplosion->length[ 3u ] - s_burnRadius;

	renderer_setTransform( pWorldTransform );

	if( length0 + length1 > 0.0f )
	{
		float2 explosion0Points[] =
		{ 
			{ -length1,  0.0f },
			{  length0,  0.0f },
		};

		for( uint i = 0u; i < SYS_COUNTOF( explosion0Points ); ++i )
		{
			float2_rotate( &explosion0Points[ i ], direction );
			float2_add( &explosion0Points[ i ], &explosion0Points[ i ], &position );
		}

		renderer_addBurnHole( &explosion0Points[ 0u ], &explosion0Points[ 1u ], s_burnRadius ); 
	}

	if( length2 + length3 > 0.0f )
	{
		float2 explosion1Points[] =
		{ 
			{  0.0f,  length2 },
			{  0.0f, -length3 },
		};

		for( uint i = 0u; i < SYS_COUNTOF( explosion1Points ); ++i )
		{
			float2_rotate( &explosion1Points[ i ], direction );
			float2_add( &explosion1Points[ i ], &explosion1Points[ i ], &position );
		}

		renderer_addBurnHole( &explosion1Points[ 0u ], &explosion1Points[ 1u ], s_burnRadius ); 
	}
}

void game_render()
{
	//if( s_game.renderTime < GAMETIMESTEP )
	//{
	//	return;
	//}

	if( renderer_isPageDone() )
	{
		// new page:
		renderer_flipPage();

		//const float speed = 5.0f;
		//const float width = 2.0f;
		const float variance = s_game.variance;

		renderer_setPen( Pen_Font );
		float2 position = { 1.0f, 1.0f };
		font_drawText( &position, 2.0f, variance, "HALLO" );
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

			float2 fontPos;
			float2_set( &fontPos, 5.0f, 20.0f );

			for( uint i = 0u; i < SYS_COUNTOF( pGameState->player ); ++i )
			{
				const ClientPlayer* pPlayer = &pGameState->player[ i ];
				if( pPlayer->state != PlayerState_InActive )
				{
					game_render_car( pPlayer, &s_game.world.worldTransform );

					char frags[ 16u ];
					sprintf( frags, "%d", pPlayer->frags );
					font_drawText( &fontPos, 0.5f, 0.1f, frags );
					fontPos.y -= 3.0f;
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
			for( uint i = 0u; i < SYS_COUNTOF( pGameState->items ); ++i )
			{
				const ClientItem* pItem = &pGameState->items[ i ];
				if( pItem->type != ItemType_None )
				{
					game_render_item( pItem, &s_game.world.worldTransform );
				}
			}
			for( uint i = 0u; i < SYS_COUNTOF( pGameState->explosions ); ++i )
			{
				const ClientExplosion* pExplosion = &pGameState->explosions[ i ];
				//if( pExplosion->time > 0u )
				//{
				//	game_render_explosion( pExplosion, &s_game.world.worldTransform );
				//}
				if( s_game.client.explosionTriggered[ i ] )
				{
					game_render_burnhole( pExplosion, &s_game.world.worldTransform );
					s_game.client.explosionTriggered[i] = 0;
				}
			}
		}
	}
	renderer_updatePage( GAMETIMESTEP );

	FrameData frame;
	//memset( &frame, 0u, sizeof( frame ) );
	//frame.time = s_game.gameTime;
	//frame.playerPos = s_game.player[ 0u ].position;
	renderer_drawFrame( &frame );

	//s_game.renderTime = 0.0f;
}
