//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <commonservices.h>
#include "esp82xxutil.h"

uint8_t dmxsend[1024];
int dmxframesize;
int senddmxframe;

int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len) {
	char * buffend = buffer;

	switch( pusrdata[1] ) {
		// Custom command test
		case 'D': case 'd':
		{
			// "CDxx" command
			int offset = pusrdata[2] | (pusrdata[3]<<8);
			int dlen = len - 4;
			if( offset + dlen > sizeof(dmxsend) ) dlen = 512-offset;
			if( dlen > 0 ) ets_memcpy( dmxsend + offset, pusrdata + 4, dlen );
			buffend += ets_sprintf( buffend, "CD" );
			senddmxframe = 1;
			return buffend-buffer;
		}

		case 'X': case 'x':
		{
			dmxframesize = pusrdata[2] | (pusrdata[3]<<8);
			if( dmxframesize > sizeof(dmxsend) ) dmxframesize = sizeof(dmxsend);
			buffend += ets_sprintf( buffend, "CX%d", dmxframesize );
			senddmxframe = 1;
			return buffend-buffer;			
		}

		// Echo to UART
		case 'E': case 'e':
			if( retsize <= len ) return -1;
			ets_memcpy( buffend, &(pusrdata[2]), len-2 );
			buffend[len-2] = '\0';
			os_printf( "%s\n", buffend );
			return len-2;
		break;
	}

	return -1;
}
