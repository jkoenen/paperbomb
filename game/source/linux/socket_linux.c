#include "socket.h"
#include "debug.h"

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

void socket_init()
{
}

void socket_done()
{
}

uint32 socket_gethostIP()
{
	struct hostent* pHostInfo = gethostbyname( "localhost" );
	if( pHostInfo == NULL )
	{
		return InvalidIP;
	}

	struct in_addr** ppAddr = (struct in_addr**)pHostInfo->h_addr_list;
	return ppAddr[ 0u ]->s_addr;		
}

uint32 socket_parseIP( const char* pAddress )
{
	return inet_addr( pAddress );
}

Socket socket_create()
{
	Socket s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	const int flags = fcntl( s, F_GETFL, 0 );
	fcntl( s, F_SETFL, flags | O_NONBLOCK );

	return s;
}

void socket_destroy( Socket s )
{
	shutdown( s, SHUT_RDWR );
	close( s );
}

int	socket_bind( Socket s, const IP4Address* pAddress )
{
	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );

	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= /*htonl*/( pAddress->address );
	addr.sin_port			= htons( pAddress->port );

	if( bind( s, ( struct sockaddr* )&addr, sizeof( addr ) ) == -1 )
	{
		return FALSE;
	}

	return TRUE;
}

int	socket_send( Socket s, const IP4Address* pTo, const void* pData, uint size )
{
	if( size == 0u )
	{
		return 0;
	}

	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );

	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= /*htonl*/( pTo->address );
	addr.sin_port			= htons( pTo->port );

	const ssize_t bytesSent = sendto( s, (const char*)pData, size, 0, (struct sockaddr*)&addr, sizeof( addr ) );
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

	return (int)bytesSent;
}

int	socket_receive( Socket s, void* pData, uint size, IP4Address* pFrom )
{
	struct sockaddr_storage addr;
	socklen_t addrLength = sizeof( addr );
	memset( &addr, 0, sizeof( addr ) );

	const ssize_t bytesReceived = recvfrom( s, (char*)pData, size, 0, (struct sockaddr*)&addr, &addrLength );
	if( ( bytesReceived >= 0 ) && ( addrLength > 0 ) )	
	{
		if( addrLength == sizeof( struct sockaddr_in ) )
		{
			struct sockaddr_in* pAddr = (struct sockaddr_in*)&addr;
			pFrom->address	= /*ntohl*/( pAddr->sin_addr.s_addr );
			pFrom->port		= ntohs( pAddr->sin_port );
		}

		return (int)bytesReceived;
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

