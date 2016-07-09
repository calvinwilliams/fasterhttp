#include "fasterhttp.h"

funcProcessHttpRequest ProcessHttpRequest ;
int ProcessHttpRequest( struct HttpEnv *e , void *p )
{
	SOCKET			sock = *((SOCKET*)p) ;
	struct sockaddr_in	sockaddr ;
	SOCKLEN_T		socklen ;
	struct HttpHeader	*p_header = NULL ;
	struct HttpBuffer	*b = NULL ;
	int			nret = 0 ;
	
	socklen = sizeof(struct sockaddr) ;
	nret = getsockname( sock , (struct sockaddr *) & sockaddr , & socklen ) ;
	if( nret )
	{
		printf( "getsockname failed , errno[%d]\n" , errno );
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	printf( "--- %s:%d | [%.*s] [%.*s] [%.*s] ------------------\n" , inet_ntoa(sockaddr.sin_addr) , (int)ntohs(sockaddr.sin_port)
		, GetHttpHeaderLen_METHOD(e) , GetHttpHeaderPtr_METHOD(e,NULL)
		, GetHttpHeaderLen_URI(e) , GetHttpHeaderPtr_URI(e,NULL)
		, GetHttpHeaderLen_VERSION(e) , GetHttpHeaderPtr_VERSION(e,NULL) );
	
	p_header = TravelHttpHeaderPtr( e , NULL ) ;
	while( p_header )
	{
		printf( "HTTP HREADER [%.*s] [%.*s]\n" , GetHttpHeaderNameLen(p_header) , GetHttpHeaderNamePtr(p_header,NULL) , GetHttpHeaderValueLen(p_header) , GetHttpHeaderValuePtr(p_header,NULL) );
		p_header = TravelHttpHeaderPtr( e , p_header ) ;
	}
	
	printf( "HTTP BODY    [%.*s]\n" , GetHttpBodyLen(e) , GetHttpBodyPtr(e,NULL) );
	
	b = GetHttpResponseBuffer(e) ;
	nret = StrcatHttpBuffer( b , "HTTP/1.1 200 OK\r\n"
					"Content-Length: 17\r\n"
					"\r\n"
					"hello fasterhttp!" ) ;
	if( nret )
	{
		printf( "StrcatfHttpBuffer failed , errno[%d]\n" , errno );
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	return 0;
}

int ReceiveHttpRequestNonblock1( SOCKET sock , SSL *ssl , struct HttpEnv *e );

int test_server_nonblock_slow_slow()
{
	SOCKET			listen_sock ;
	struct sockaddr_in	listen_addr ;
	SOCKET			accept_sock ;
	struct sockaddr_in	accept_addr ;
	SOCKLEN_T		accept_addr_len ;
	int			onoff ;
	
	struct HttpEnv		*e = NULL ;
	fd_set			read_fds , write_fds ;
	
	int			nret = 0 ;
	
	e = CreateHttpEnv();
	if( e == NULL )
	{
		printf( "CreateHttpEnv failed , errno[%d]\n" , errno );
		return -1;
	}
	
	listen_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( listen_sock == -1 )
	{
		printf( "socket failed , errno[%d]\n" , errno );
		return -1;
	}
	
	onoff = 1 ;
	setsockopt( listen_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & onoff , sizeof(onoff) );
	
	memset( & listen_addr , 0x00 , sizeof(struct sockaddr_in) );
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	listen_addr.sin_port = htons( (unsigned short)9527 );
	
	nret = bind( listen_sock , (struct sockaddr *) & listen_addr , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		printf( "bind failed , errno[%d]\n" , errno );
		return -1;
	}
	
	nret = listen( listen_sock , 1024 ) ;
	if( nret == -1 )
	{
		printf( "listen failed , errno[%d]\n" , errno );
		return -1;
	}
	
	while(1)
	{
		accept_addr_len = sizeof(struct sockaddr) ;
		accept_sock = accept( listen_sock , (struct sockaddr *) & accept_addr, & accept_addr_len );
		if( accept_sock == - 1 )
		{
			printf( "accept failed , errno[%d]\n" , errno );
			break;
		}
		
		ResetHttpEnv( e );
		
		while(1)
		{
			FD_ZERO( & read_fds );
			FD_SET( accept_sock , & read_fds );
			
			nret = select( accept_sock+1 , & read_fds , NULL , NULL , GetHttpElapse(e) ) ;
			if( nret == 0 )
			{
				printf( "select receive timeout , errno[%d]\n" , errno );
				nret = FASTERHTTP_ERROR_TCP_CLOSE ;
				break;
			}
			else if( nret != 1 )
			{
				printf( "select receive failed , errno[%d]\n" , errno );
				nret = FASTERHTTP_ERROR_TCP_CLOSE ;
				break;
			}
			
			nret = ReceiveHttpRequestNonblock( accept_sock , NULL , e ) ;
			if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
			{
				;
			}
			else if( nret )
			{
				printf( "ReceiveHttpRequestNonblock failed[%d]\n" , nret );
				break;
			}
			else
			{
				break;
			}
		}
		
		if( nret == FASTERHTTP_ERROR_TCP_CLOSE )
		{
			CLOSESOCKET( accept_sock );
			continue;
		}
		else if( nret == FASTERHTTP_INFO_TCP_CLOSE )
		{
			CLOSESOCKET( accept_sock );
			continue;
		}
		else if( nret )
		{
			nret = FormatHttpResponseStartLine( abs(nret)/1000 , e , 1 ) ;
			if( nret )
			{
				CLOSESOCKET( accept_sock );
				continue;
			}
		}
		else
		{
			nret = ProcessHttpRequest( e , (void*)(&accept_sock) ) ;
			if( nret )
			{
				nret = FormatHttpResponseStartLine( nret , e , 1 ) ;
				if( nret )
				{
					CLOSESOCKET( accept_sock );
					continue;
				}
			}
		}
		
		while(1)
		{
			FD_ZERO( & write_fds );
			FD_SET( accept_sock , & write_fds );
			
			nret = select( accept_sock+1 , NULL , & write_fds , NULL , GetHttpElapse(e) ) ;
			if( nret == 0 )
			{
				printf( "select send timeout , errno[%d]\n" , errno );
				CLOSESOCKET( accept_sock );
				break;
			}
			else if( nret != 1 )
			{
				printf( "select send failed , errno[%d]\n" , errno );
				CLOSESOCKET( accept_sock );
				break;
			}
			
			nret = SendHttpResponseNonblock( accept_sock , NULL , e ) ;
			if( nret == FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
			{
				;
			}
			else if( nret )
			{
				printf( "SendHttpResponseNonblock failed[%d]\n" , nret );
				CLOSESOCKET( accept_sock );
				break;
			}
			else
			{
				break;
			}
		}
		
		CLOSESOCKET( accept_sock );
	}
	
	CLOSESOCKET( listen_sock );
	
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
	
	nret = test_server_nonblock_slow_slow() ;

#if ( defined _WIN32 )
	WSACleanup();
#endif

	return -nret;
}
