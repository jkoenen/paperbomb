#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include "types.h"
#include "socket.h"
#include "settings.h"

typedef struct 
{
	uint8	time;
	int16	posX;
	int16	posY;
	uint8	direction;
	uint8	length;

} ClientBomb;

typedef struct 
{
	uint8	time;
	int16	posX;
	int16	posY;
	uint8	direction;
	uint8	length;

} ClientExplosion;

enum
{
	PlayerState_InActive,
	PlayerState_Active
};

typedef struct 
{
	uint8		state;
	uint8		frags;
	char		name[ 12u ];
	int16		posX;
	int16		posY;
	uint16		age;
	uint8		direction;
	uint8		steer;

} ClientPlayer;

typedef struct 
{
	uint				id;

	ClientPlayer		player[ MaxPlayer ];
	ClientBomb			bombs[ MaxBombs ];
	ClientExplosion		explosions[ MaxExplosions ];

} ClientGameState;

enum
{
	ClientStateFlag_Online = 1u,
	ServerFlagOffline	   = 1u << 31u
};

typedef struct 
{
	uint	id;
	uint8	flags;
	uint8	buttonMask;
	char	name[ 12u ];

} ClientState;

typedef struct 
{
	Socket			socket;
	IP4Address		serverAddress;

	int				explosionActive[ MaxExplosions ];
    int             explosionTriggered[ MaxExplosions ];
	ClientGameState	gameState;

	ClientState		state;

} Client;

void	client_create( Client* pClient, const IP4Address* pServerAddress, const char* pName );
void	client_destroy( Client* pClient );
int		client_update( Client* pClient, uint buttonMask );

#endif
