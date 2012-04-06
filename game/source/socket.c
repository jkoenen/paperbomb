#include "socket.h"

void socket_send_blocking( Socket socket, const IP4Address* pTo, const void* pData, uint size )
{
	const uint8* pSource = (const uint8*)pData;
	uint toSend = size;
	while( toSend > 0u )
	{
		const int sended = socket_send( socket, pTo, pSource, toSend );
		if( sended < 0 )
		{
			break;
		}
		toSend -= (uint)sended;
		pSource += sended;
	}
}

int socket_isAddressEqual( const IP4Address* pAddress1, const IP4Address* pAddress2 )
{
	return ( pAddress1->address == pAddress2->address ) && ( pAddress1->port == pAddress2->port );
}

