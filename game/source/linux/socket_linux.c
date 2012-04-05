#include "socket.h"
#include "debug.h"

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <errno.h>

void socket_init()
{
}

void socket_done()
{
}

uint32 socket_gethostIP()
{
	hostent* pHostInfo = gethostbyname( "localhost" );
	if( pHostInfo == NULL )
	{
		return InvalidIP;
	}

	in_addr** ppAddr = (in_addr**)pHostInfo->h_addr_list;
	return ppAddr[ 0u ]->s_addr;		
}

uint32 socket_parseIP( const char* pAddress )
{
	return inet_addr( pAddress );
}

Socket socket_create()
{
	Socket socket = (int)::socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	const int flags = fcntl( socket, F_GETFL, 0 );
	fcntl( pSocket->socket, F_SETFL, flags | O_NONBLOCK );

	return socket;
}

void socket_destroy( Socket socket )
{
	::shutdown( (uint)socket, SHUT_RDWR );
	::close( (uint)socket );
}

int	socket_bind( Socket socket, const IP4Address* pAddress )
{
	sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );

	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= /*htonl*/( pAddress->address );
	addr.sin_port			= htons( pAddress->port );

	if( ::bind( (uint)socket, ( sockaddr* )&addr, sizeof( addr ) ) == -1 )
	{
		return FALSE;
	}

	return TRUE;
}

int	socket_send( Socket socket, const IP4Address* pTo, const void* pData, uint size )
{
	if( size == 0u )
	{
		return 0;
	}

	sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );

	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= /*htonl*/( pTo->address );
	addr.sin_port			= htons( pTo->port );

	const int bytesSent = ::sendto( (uint)socket, (const char*)pData, (int)size, 0, (sockaddr*)&addr, sizeof( addr ) );
	if( bytesSent < 0 )
	{
		if( errno == EWOULDBLOCK || errno == EAGAIN )
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}

	return bytesSent;
}

int	socket_receive( Socket socket, void* pData, uint size, IP4Address* pFrom )
{
	sockaddr_storage addr;
	int addrLength = sizeof( addr );
	memset( &addr, 0, sizeof( addr ) );

	const int bytesReceived = ::recvfrom( (uint)socket, (char*)pData, (int)size, 0, (sockaddr*)&addr, &addrLength );
	if( ( bytesReceived >= 0 ) && ( addrLength > 0 ) )	
	{
		if( addrLength == sizeof( sockaddr_in ) )
		{
			sockaddr_in* pAddr = (sockaddr_in*)&addr;
			pFrom->address	= /*ntohl*/( pAddr->sin_addr.s_addr );
			pFrom->port		= ntohs( pAddr->sin_port );
		}

		return bytesReceived;
	}
	else
	{
		if( errno == EWOULDBLOCK || errno == EAGAIN )
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
}
