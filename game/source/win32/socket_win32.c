#include "socket.h"
#include "debug.h"

#include <memory.h>
#include <winsock2.h>
#include <mswsock.h>

static uint s_libCount = 0;

void socket_init()
{
	if( s_libCount == 0u )
	{
		WSADATA wsaData;
		WORD versionRequested = MAKEWORD( 2, 2 );
		const int result = WSAStartup( versionRequested, &wsaData );
		if( result != 0 ) 
		{
			SYS_BREAK( "winsock lib" );
		}
	}
	s_libCount++;
}

void socket_done()
{
	if( s_libCount > 0u )
	{
		s_libCount--;
		if( s_libCount == 0u ) 
		{
			WSACleanup();
		}
	}
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

	DWORD bytesReturned = 0;
	BOOL isNewBehavior = FALSE;

	WSAIoctl( (uint)socket, SIO_UDP_CONNRESET, &isNewBehavior, sizeof( isNewBehavior ), NULL, 0, &bytesReturned, NULL,	NULL );

	unsigned long doNotBlock = 1;
	if( ioctlsocket( (uint)socket, FIONBIO, &doNotBlock ) == SOCKET_ERROR )
	{
		return InvalidSocket;
	}

	return socket;
}

void socket_destroy( Socket socket )
{
	::shutdown( (uint)socket, SD_BOTH );
	::closesocket( (uint)socket );
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
		const int result = WSAGetLastError();
		if( result == WSAEWOULDBLOCK || result == WSAENOTCONN )
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
		if( addrLength==sizeof( sockaddr_in ) )
		{
			sockaddr_in* pAddr = (sockaddr_in*)&addr;
			pFrom->address	= /*ntohl*/( pAddr->sin_addr.s_addr );
			pFrom->port		= ntohs( pAddr->sin_port );
		}

		return bytesReceived;
	}
	else
	{
		const int result = WSAGetLastError();
		if( result == WSAEWOULDBLOCK || result == WSAENOTCONN )
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
}
