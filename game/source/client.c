#include "client.h"

#include "debug.h"

void client_create( Client* pClient, const IP4Address* pServerAddress, const char* pName )
{
	socket_init();

	pClient->socket			= socket_create();
	pClient->serverAddress	= *pServerAddress;

	pClient->state.id		  = 1u;
	pClient->state.buttonMask = 0u;
	pClient->state.flags	  = ClientStateFlag_Online;
	copyString( pClient->state.name, sizeof( pClient->state.name ), pName );

	pClient->gameState.id = 0u;
	for( uint i = 0u; i < SYS_COUNTOF( pClient->gameState.player ); ++i )
	{
		pClient->gameState.player[ i ].state = PlayerState_InActive;
	}
	for( uint i = 0u; i < SYS_COUNTOF( pClient->gameState.bombs ); ++i )
	{
		pClient->gameState.bombs[ i ].time = 0u;
	}
	for( uint i = 0u; i < SYS_COUNTOF( pClient->gameState.explosions ); ++i )
	{
		pClient->gameState.explosions[ i ].time = 0u;
	}
	for( uint i = 0u; i < SYS_COUNTOF( pClient->explosionActive ); ++i )
	{
		pClient->explosionActive[ i ] = 0;
	}
}

void client_destroy( Client* pClient )
{
	pClient->state.id++;
	pClient->state.buttonMask = 0u;
	pClient->state.flags = 0u;
	socket_send_blocking( pClient->socket, &pClient->serverAddress, &pClient->state, sizeof( pClient->state ) );

	socket_destroy( pClient->socket );
	pClient->socket = InvalidSocket;

	socket_done();
}

void client_update( Client* pClient, uint buttonMask )
{
	pClient->state.id++;
	pClient->state.buttonMask = (uint8)buttonMask;
	socket_send_blocking( pClient->socket, &pClient->serverAddress, &pClient->state, sizeof( pClient->state ) );

	for(;;)
	{
		ClientGameState gameState;
		IP4Address from;
		const int result = socket_receive( pClient->socket, &gameState, sizeof( gameState ), &from );
		if( result > 0 )
		{
			SYS_ASSERT( result == sizeof( gameState ) );
			//SYS_TRACE_DEBUG( "c recv %d\n", gameState.id );

			if( gameState.id > pClient->gameState.id )
			{
				pClient->gameState = gameState;
			}
		}
		else
		{
			break;
		}
	}

	for( uint i = 0u; i < SYS_COUNTOF( pClient->gameState.explosions ); ++i )
	{
		const ClientExplosion* pExplosion = &pClient->gameState.explosions[ i ];

		const int wasActive = ( pClient->explosionActive[ i ] & 1 );
		const int isActive = ( pExplosion->time > 0u ) ? 1 : 0;
		int flank = 0;
		if( isActive && !wasActive )
		{
			flank = 2;
		}
		pClient->explosionActive[ i ] = flank | isActive;
	}
}
