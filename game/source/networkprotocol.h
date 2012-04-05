#ifndef NETWORKPROTOCOL_H_INCLUDED
#define NETWORKPROTOCOL_H_INCLUDED

#include "types.h"

enum
{
	MaxPacketSize = 1024u
};

enum PacketId
{
	PacketId_Hello,

	PacketId_Input,
	PacketId_State
};

uint	network_writeInput( void* pPacket, uint capacity, uint32 buttonMask );
int		network_readInput( uint32* pButtonMask, const void* pPacket, uint packetSize );

#endif
