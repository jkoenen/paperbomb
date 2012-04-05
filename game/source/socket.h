#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#include "types.h"

typedef int Socket;
typedef struct	
{
	uint32	address;
	uint16	port;
} IP4Address; 

enum
{
	InvalidSocket = -1,
	InvalidIP = 0u
};

void	socket_init();
void	socket_done();

uint32	socket_gethostIP();
uint32	socket_parseIP( const char* pAddress );

Socket	socket_create();
void	socket_destroy( Socket socket );

int		socket_bind( Socket socket, const IP4Address* pAddress );

int		socket_send( Socket socket, const IP4Address* pTo, const void* pData, uint size );
int		socket_receive( Socket socket, void* pData, uint size, IP4Address* pFrom );

#endif