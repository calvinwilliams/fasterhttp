#include "internal.h"
#include "fasterhttp.h"

void _DumpHexBuffer( FILE *fp , char *buf , long buflen )
{
	int		row_offset , col_offset ;
	
	fprintf( fp , "             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    0123456789ABCDEF\n" );
	
	row_offset = 0 ;
	col_offset = 0 ;
	while(1)
	{
		fprintf( fp , "0x%08X   " , row_offset * 16 );
		for( col_offset = 0 ; col_offset < 16 ; col_offset++ )
		{
			if( row_offset * 16 + col_offset < buflen )
			{
				fprintf( fp , "%02X " , *((unsigned char *)buf+row_offset*16+col_offset));
			}
			else
			{
				fprintf( fp , "   " );
			}
		}
		fprintf( fp , "  " );
		for( col_offset = 0 ; col_offset < 16 ; col_offset++ )
		{
			if( row_offset * 16 + col_offset < buflen )
			{
				if( isprint( (int)*(buf+row_offset*16+col_offset) ) )
				{
					fprintf( fp , "%c" , *((unsigned char *)buf+row_offset*16+col_offset) );
				}
				else
				{
					fprintf( fp , "." );
				}
			}
			else
			{
				fprintf( fp , " " );
			}
		}
		fprintf( fp , "\n" );
		if( row_offset * 16 + col_offset >= buflen )
			break;
		row_offset++;
	}
	
	return;
}

