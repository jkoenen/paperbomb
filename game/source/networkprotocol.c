#include "networkprotocol.h"

#include "debug.h"

uint network_writeInput( void* pPacket, uint capacity, uint32 buttonMask )
{
	(void)capacity;
	SYS_ASSERT( capacity >= 2u );

	uint8* pBytes = (uint8*)pPacket;

	*pBytes++ = (uint8)PacketId_Input;
	*pBytes++ = (uint8)( buttonMask & 0xffu );

	return 2u;
}

int network_readInput( uint32* pButtonMask, const void* pPacket, uint packetSize )
{
	const uint8* pBytes = (uint8*)pPacket;
	if( *pBytes++ != PacketId_Input )
	{
		return FALSE;
	}

	if( packetSize < 2u )
	{
		return FALSE;
	}

	*pButtonMask = (uint32)*pBytes;

	return TRUE;
}
