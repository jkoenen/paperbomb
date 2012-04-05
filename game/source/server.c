#include "server.h"

void server_create( Server* pServer, uint16 port )
{
	socket_init();

	pServer->socket = socket_create();
	for( uint i = 0u; i < SYS_COUNTOF( pServer->clients ); ++i )
	{
		pServer->clients[ i ].address.address = InvalidIP;
	}

	IP4Address address;
	address.address = socket_gethostIP();
	address.port	= port;
}

void server_destroy( Server* pServer )
{
	socket_destroy( pServer->socket );
	pServer->socket = InvalidSocket;

	socket_done();
}

void server_update( Server* pServer )
{
	(void)pServer;
}

