#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "types.h"
#include "socket.h"
#include "settings.h"
#include "client.h"
#include "world.h"

typedef struct 
{
	uint	player;
	float2	position;
	float	direction;
	float	length;
	float	time;

} ServerBomb;

typedef struct 
{
	uint	player;
	float2	position;
	float	direction;
	float	length;
	float	time;

} ServerExplosion;

typedef struct 
{
	IP4Address		address;
	ClientState		state;
	uint			lastButtonMask;

	uint			frags;
	uint			playerState;
	char			name[ 12u ];
	float			age;
	float2			position;
	float			direction;
	float			steer;
	float2			velocity;
	uint			maxBombs;
	float			bombLength;

} ServerPlayer;

typedef struct 
{
	uint				id;

	ServerPlayer		player[ MaxPlayer ];
	ServerBomb			bombs[ MaxBombs ];
	ServerExplosion		explosions[ MaxExplosions ];

} ServerGameState;

typedef struct 
{
	Socket			socket;
	ServerGameState	gameState;

} Server;

void	server_create( Server* pServer, uint16 port );
void	server_destroy( Server* pServer );
void	server_update( Server* pServer, World* pWorld );

#endif

