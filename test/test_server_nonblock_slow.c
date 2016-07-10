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
	nret = StrcatHttpBuffer( b ,	"Content-Type: text/html\r\n"
					"Content-Length: 17\r\n"
					"\r\n"
					"hello fasterhttp!" ) ;
	if( nret )
	{
		printf( "StrcatfHttpBuffer failed , errno[%d]\n" , errno );
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	return HTTP_OK;
}

int ReceiveHttpRequestNonblock1( SOCKET sock , SSL *ssl , struct HttpEnv *e );

int test_server_nonblock_slow()
{
	SOCKET			listen_sock ;
	struct sockaddr_in	listen_addr ;
	SOCKET			accept_sock ;
	struct sockaddr_in	accept_addr ;
	SOCKLEN_T		accept_addr_len ;
	int			onoff ;
	
	struct HttpEnv		*e = NULL ;
	
	int			nret = 0 ;
	
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
	listen_addr.sin_addr.s_addr = INADDR_ANY ;
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
		
		SetHttpNonblock( accept_sock );
		
		e = CreateHttpEnv();
		if( e == NULL )
		{
			printf( "CreateHttpEnv failed , errno[%d]\n" , errno );
			CLOSESOCKET( accept_sock );
			return -1;
		}
		
		EnableHttpResponseCompressing( e , 1 );
		
		while(1)
		{
			while(1)
			{
				nret = ReceiveHttpRequestNonblock1( accept_sock , NULL , e ) ;
				if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
				{
					;
				}
				else
				{
					break;
				}
			}
			
			if( nret == FASTERHTTP_ERROR_TCP_CLOSE )
			{
				break;
			}
			else if( nret == FASTERHTTP_INFO_TCP_CLOSE )
			{
				break;
			}
			else if( nret )
			{
				printf( "ReceiveHttpRequestNonblock1 failed[%d]\n" , nret );
				
				nret = FormatHttpResponseStartLine( abs(nret)/1000 , e , 1 ) ;
				if( nret )
					break;
			}
			else
			{
				nret = FormatHttpResponseStartLine( HTTP_OK , e , 0 ) ;
				if( nret )
					break;
				
				nret = ProcessHttpRequest( e , (void*)(&accept_sock) ) ;
				if( nret != HTTP_OK )
				{
					nret = FormatHttpResponseStartLine( nret , e , 1 ) ;
					if( nret )
						break;
				}
			}
			
			while(1)
			{
				nret = SendHttpResponseNonblock( accept_sock , NULL , e ) ;
				if( nret == FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
				{
					;
				}
				else
				{
					break;
				}
			}
			
			if( nret )
			{
				printf( "SendHttpResponseNonblock failed[%d]\n" , nret );
				break;
			}
			
			if( ! CheckHttpKeepAlive(e) )
				break;
			
			ResetHttpEnv(e);
		}
		
		DestroyHttpEnv( e );
		CLOSESOCKET( accept_sock );
	}
	
	CLOSESOCKET( listen_sock );
	
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
	
	nret = test_server_nonblock_slow() ;

#if ( defined _WIN32 )
	WSACleanup();
#endif

	return -nret;
}
