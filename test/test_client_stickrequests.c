#include "fasterhttp.h"

int test_client_stickrequests()
{
	struct HttpEnv		*e = NULL ;
	
	SOCKET			connect_sock ;
	struct sockaddr_in	connect_addr ;
	
	struct HttpBuffer	*b = NULL ;
	
	int			nret = 0 ;
	
	e = CreateHttpEnv();
	if( e == NULL )
	{
		printf( "CreateHttpEnv failed , errno[%d]\n" , errno );
		return -1;
	}
	
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
	nret = StrcpyHttpBuffer( b , "GET / HTTP/1.1\r\n"
					"\r\n"
					"POST / HTTP/1.1\r\n" ) ;
	if( nret )
	{
		printf( "StrcatfHttpBuffer failed[%d] , errno[%d]\n" , nret , errno );
		return -1;
	}
	
	nret = SendHttpRequest( connect_sock , NULL , e ) ;
	if( nret )
	{
		printf( "SendHttpRequest failed[%d] , errno[%d]\n" , nret , errno );
		return -1;
	}
	
	ResetHttpEnv( e );
	
	b = GetHttpRequestBuffer(e) ;
	nret = StrcpyHttpBuffer( b , "Content-Length: 3\r\n"
					"\r\n"
					"xyz"
					"Content-Length: 20\r\n"
					"\r\n"
					"hello stickrequests!"
					) ;
	if( nret )
	{
		printf( "StrcatfHttpBuffer failed[%d] , errno[%d]\n" , nret , errno );
		return -1;
	}
	
	nret = SendHttpRequest( connect_sock , NULL , e ) ;
	if( nret )
	{
		printf( "SendHttpRequest failed[%d] , errno[%d]\n" , nret , errno );
		return -1;
	}
	
	ResetHttpEnv( e );
	
	nret = ReceiveHttpResponse( connect_sock , NULL , e ) ;
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
	
	ResetHttpEnv( e );
	
	nret = ReceiveHttpResponse( connect_sock , NULL , e ) ;
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
	
	CLOSESOCKET( connect_sock );
	
	DestroyHttpEnv( e );
	
	printf( "ALL test is ok!!!\n" );
	
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
	
	nret = test_client_stickrequests() ;

#if ( defined _WIN32 )
	WSACleanup();
#endif

	return -nret;
}
