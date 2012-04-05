#ifndef NETWORKPROTOCOL_H_INCLUDED
#define NETWORKPROTOCOL_H_INCLUDED

#include "types.h"

enum
{
	MaxPacketSize = 1024u,
};

enum PacketId
{
	PacketId_Hello,
	PacketId_Input,
	PacketId_State
};

typedef struct 
{
	uint8	id;
	uint8	size;
	uint8	data[ MaxPacketSize - 2u ];
} Packet;

void	packet_write_hello( Packet* pPacket, const char* pName );
void	packet_read_hello( char* pName, uint capacity, const Packet* pPacket );

void	packet_write_input( Packet* pPacket, uint32 buttonMask );
void	packet_read_input( uint32* pButtonMask, const Packet* pPacket );

#endif
