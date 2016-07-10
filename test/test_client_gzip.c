#include "fasterhttp.h"

static int TestParseHttpRequest( struct HttpEnv *e , char *str )
{
	SOCKET			connect_sock ;
	struct sockaddr_in	connect_addr ;
	
	struct HttpBuffer	*b = NULL ;
	
	int			nret = 0 ;
	
	ResetHttpEnv( e );
	
	connect_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( connect_sock == -1 )
	{
		printf( "socket failed , errno[%d]\n" , errno );
		return -1;
	}
	
	memset( & connect_addr , 0x00 , sizeof(struct sockaddr_in) );
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	connect_addr.sin_port = htons( (unsigned short)9527 );
	
	nret = connect( connect_sock , (struct sockaddr *) & connect_addr , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		printf( "connect failed , errno[%d]\n" , errno );
		return -1;
	}
	
	b = GetHttpRequestBuffer(e) ;
	nret = StrcatHttpBuffer( b , str ) ;
	if( nret )
	{
		printf( "StrcatfHttpBuffer failed[%d] , errno[%d]\n" , nret , errno );
		return -1;
	}
	
	nret = RequestHttp( connect_sock , NULL , e ) ;
	CLOSESOCKET( connect_sock );
	if( nret )
	{
		printf( "RequestHttp failed[%d] , errno[%d]\n" , nret , errno );
		return -1;
	}
	else
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
	
	return 0;
}

int test_client_gzip()
{
	struct HttpEnv		*e = NULL ;
	
	int			nret = 0 ;
	
	e = CreateHttpEnv();
	if( e == NULL )
	{
		printf( "CreateHttpEnv failed , errno[%d]\n" , errno );
		return -1;
	}
	
	nret = TestParseHttpRequest( e , "POST / HTTP/1.1\r\n"
					"Content-Length: 3\r\n"
					"Accept-Encoding: gzip,deflate,*\r\n"
					"\r\n"
					"xyz" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "POST / HTTP/1.1\r\n"
					"Content-Length: 3\r\n"
					"Accept-Encoding: gzip2,deflate,*\r\n"
					"\r\n"
					"xyz" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "POST / HTTP/1.1\r\n"
					"Content-Length: 3\r\n"
					"Accept-Encoding: gzip2,deflate2,*\r\n"
					"\r\n"
					"xyz" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	DestroyHttpEnv( e );
	
	return 0;
}

int main()
{
#if ( defined _WIN32 )
	WSADATA		wsaData;
#endif
	int		nret = 0 ;
	
#if ( defined _WIN32 )
	nret = WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) ;
	if( nret )
	{
		printf( "WSAStartup failed[%d] , errno[%d]\n" , nret , GetLastError() );
		return 1;
	}
#endif
	
	nret = test_client_gzip() ;

#if ( defined _WIN32 )
	WSACleanup();
#endif

	return -nret;
}
