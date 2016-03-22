#include "fasterhttp.h"

static int test()
{
	struct HttpEnv		*e = NULL ;
	struct HttpBuffer	*b = NULL ;
	int			nret = 0 ;
	
	e = CreateHttpEnv() ;
	if( e == NULL )
	{
		printf( "CreateHttpEnv failed\n" );
		return -1;
	}
	
	b = GetHttpRequestBuffer( e ) ;
	
	nret = StrcatfHttpBuffer( b , "" ) ;
	if( nret )
	{
		printf( "StrcatfHttpBuffer failed[%d]\n" , nret );
		return -1;
	}
	
	nret = ParseHttpRequest( e ) ;
	if( nret )
	{
		printf( "ParseHttpRequest failed[%d]\n" , nret );
		return -1;
	}
	
	
	
	DestroyHttpEnv( e );
	
	return 0;
}

int main()
{
	return -test();
}

