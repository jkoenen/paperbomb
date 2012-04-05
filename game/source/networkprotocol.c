#include "networkprotocol.h"

#include "debug.h"
#include "string.h"

void packet_write_input( Packet* pPacket, uint32 buttonMask )
{
	pPacket->id		= PacketId_Input;
	pPacket->size	= 1u;

	pPacket->data[ 0u ]	= (uint8)( buttonMask & 0xffu );
}

void packet_read_input( uint32* pButtonMask, const Packet* pPacket )
{
	SYS_ASSERT( pPacket->id == PacketId_Input );

	*pButtonMask = (uint32)pPacket->data[ 0u ];
}

void packet_write_hello( Packet* pPacket, const char* pName )
{
	pPacket->id	= PacketId_Hello;

	char* pPacketName = (char*)pPacket->data;

	const uint size = copyString( pPacketName, sizeof( pPacket->data ), pName ) + 1u;

	pPacket->size = (uint8)( ( size + 3u ) / 4u );
}

void packet_read_hello( char* pName, uint capacity, const Packet* pPacket )
{
	SYS_ASSERT( pPacket->id == PacketId_Hello );

	copyString( pName, capacity, (char*)pPacket->data );
}