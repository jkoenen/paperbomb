#include "client.h"

#include "debug.h"

void client_create( Client* pClient, uint16 port, const IP4Address* pServerAddress, const char* pName )
{
	socket_init();

	pClient->socket			= socket_create();
	pClient->serverAddress	= *pServerAddress;

	IP4Address address;
	address.address = socket_gethostIP();
	address.port	= port;

	socket_bind( pClient->socket, &address );

	pClient->state.id		  = 1u;
	pClient->state.buttonMask = 0u;
	pClient->state.flags	  = ClientStateFlag_Online;
	copyString( pClient->state.name, sizeof( pClient->state.name ), pName );

	pClient->gameStateId = 0u;
}

void client_destroy( Client* pClient )
{
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
}