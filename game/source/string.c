#include "string.h"

uint copyString( char* pDest, uint capacity, const char* pSource )
{
	uint size = 0u;
	while( *pSource != '\0' )
	{
		if( size + 1u < capacity )
		{
			*pDest++ = *pSource;
		}
		pSource++;
		size++;
	}
	*pDest = '\0';
	return size;
}
