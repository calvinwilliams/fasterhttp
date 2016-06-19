#include "fasterhttp.h"

static int TestParseHttpRequest( struct HttpEnv *e , char *format , ... )
{
	struct HttpBuffer	*b = NULL ;
	va_list			valist ;
	
	int			nret = 0 ;
	
	ResetHttpEnv( e );
	
	b = GetHttpRequestBuffer( e ) ;
	
	va_start( valist , format );
	nret = StrcatvHttpBuffer( b , format , valist ) ;
	va_end( valist );
	if( nret )
	{
		printf( "StrcatvHttpBuffer failed[%d]\n" , nret );
		return -1;
	}
	
	nret = ParseHttpRequest( e ) ;
	if( nret == 0 )
	{
		if( GetHttpHeaderCount( e ) > 0 )
		{
			struct HttpHeader *p_header = NULL ;
			
			p_header = TravelHttpHeaderPtr( e , NULL ) ;
			while( p_header )
			{
				printf( "HTTP HREADER [%.*s] [%.*s]\n" , GetHttpHeaderNameLen(p_header) , GetHttpHeaderNamePtr(p_header,NULL) , GetHttpHeaderValueLen(p_header) , GetHttpHeaderValuePtr(p_header,NULL) );
				p_header = TravelHttpHeaderPtr( e , p_header ) ;
			}
		}
		
		if( GetHttpBodyLen( e ) > 0 )
		{
			printf( "HTTP BODY [%.*s]\n" , GetHttpBodyLen(e) , GetHttpBodyPtr(e,NULL) );
		}
	}
	return nret;
}

static int TestParseHttpResponse( struct HttpEnv *e , char *format , ... )
{
	struct HttpBuffer	*b = NULL ;
	va_list			valist ;
	
	int			nret = 0 ;
	
	ResetHttpEnv( e );
	
	b = GetHttpResponseBuffer( e ) ;
	
	va_start( valist , format );
	nret = StrcatvHttpBuffer( b , format , valist ) ;
	va_end( valist );
	if( nret )
	{
		printf( "StrcatvHttpBuffer failed[%d]\n" , nret );
		return -1;
	}
	
	nret = ParseHttpResponse( e ) ;
	if( nret == 0 )
	{
		if( GetHttpHeaderCount( e ) > 0 )
		{
			struct HttpHeader *p_header = NULL ;
			
			p_header = TravelHttpHeaderPtr( e , NULL ) ;
			while( p_header )
			{
				printf( "HTTP HREADER [%.*s] [%.*s]\n" , GetHttpHeaderNameLen(p_header) , GetHttpHeaderNamePtr(p_header,NULL) , GetHttpHeaderValueLen(p_header) , GetHttpHeaderValuePtr(p_header,NULL) );
				p_header = TravelHttpHeaderPtr( e , p_header ) ;
			}
		}
		
		if( GetHttpBodyLen( e ) > 0 )
		{
			printf( "HTTP BODY [%.*s]\n" , GetHttpBodyLen(e) , GetHttpBodyPtr(e,NULL) );
		}
	}
	return nret;
}

static int test_failure()
{
	struct HttpEnv		*e = NULL ;
	
	int			nret = 0 ;
	e = CreateHttpEnv() ;
	if( e == NULL )
	{
		printf( "CreateHttpEnv failed\n" );
		return -1;
	}
	
	nret = TestParseHttpRequest( e , "GE / HTTP/1.1\r\n\r\n" ) ;
	if( ! nret )
	{
		printf( "%s:%d | test_failure failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test_failure ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "GET HTTP/1.1\r\n\r\n" ) ;
	if( ! nret )
	{
		printf( "%s:%d | test_failure failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test_failure ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "GET / HTTP/1.2\r\n\r\n" ) ;
	if( ! nret )
	{
		printf( "%s:%d | test_failure failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test_failure ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "POST / HTTP/1.1\r\n"
					"Content-Length: 4\r\n"
					"\r\n"
					"xyz" ) ;
	if( ! nret )
	{
		printf( "%s:%d | test_failure failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test_failure ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpResponse( e , "HTTP/1.2 200\r\n"
					"\r\n" );
	if( ! nret )
	{
		printf( "%s:%d | test_failure failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test_failure ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpResponse( e , "HTTP/1.1 200\r\n"
					"\r\n" );
	if( ! nret )
	{
		printf( "%s:%d | test_failure failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test_failure ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	DestroyHttpEnv( e );
	
	printf( "ALL test_failure is ok!!!\n" );
	
	return 0;
}

int main()
{
	return -test_failure();
}

