#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "types.h"
#include "socket.h"

enum
{
	MaxClients = 8u
};

typedef struct 
{
	IP4Address	address;

} ClientData;

typedef struct 
{
	Socket			socket;
	ClientData		clients[ MaxClients ];

} Server;

void	server_create( Server* pServer, uint16 port );
void	server_destroy( Server* pServer );
void	server_update( Server* pServer );

#endif

