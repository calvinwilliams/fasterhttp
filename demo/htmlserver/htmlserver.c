#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "fasterhttp.h"
#include "LOGC.h"

#define MAX_EPOLL_EVENTS	1024

static int ProcessHttpRequest( struct HttpEnv *e , int sock , char *wwwroot )
{
	char			pathfilename[ 1024 + 1 ] ;
	struct stat		st ;
	int			filesize ;
	struct HttpBuffer	*b = NULL ;
	
	SOCKLEN_T		socklen ;
	struct sockaddr_in	sockaddr ;
	
	int			nret = 0 ;
	
	memset( pathfilename , 0x00 , sizeof(pathfilename) );
	snprintf( pathfilename , sizeof(pathfilename)-1 , "%s%.*s" , wwwroot , GetHttpHeaderLen_URI(e) , GetHttpHeaderPtr_URI(e,NULL) );
	
	nret = stat( pathfilename , & st ) ;
	if( nret == -1 )
		return FASTERHTTP_ERROR_FILE_NOT_FOUND;
	filesize = st.st_size ;
	
	b = GetHttpResponseBuffer(e) ;
	nret = StrcatfHttpBuffer( b ,	"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/html; charset=gb18030\r\n"
					"Content-Length: %d\r\n"
					"\r\n"
					, filesize ) ;
	if( nret )
		return nret;
	
	nret = StrcatHttpBufferFromFile( b , pathfilename , &filesize ) ;
	if( nret )
		return nret;
	
	socklen = sizeof(struct sockaddr) ;
	nret = getsockname( sock , (struct sockaddr *) & sockaddr , & socklen ) ;
	if( nret )
	{
		printf( "getsockname failed , errno[%d]\n" , errno );
		return -1;
	}
	
	InfoLog( __FILE__ , __LINE__ , "%s:%d | %.*s %.*s %.*s | 200"
		, inet_ntoa(sockaddr.sin_addr) , (int)ntohs(sockaddr.sin_port)
		, GetHttpHeaderLen_METHOD(e) , GetHttpHeaderPtr_METHOD(e,NULL)
		, GetHttpHeaderLen_URI(e) , GetHttpHeaderPtr_URI(e,NULL)
		, GetHttpHeaderLen_VERSION(e) , GetHttpHeaderPtr_VERSION(e,NULL)
		);
	
	return 0;
}

static int OnAcceptingSocket( int epoll_fd , int listen_sock )
{
	SOCKET			accept_sock ;
	struct sockaddr_in	accept_addr ;
	SOCKLEN_T		accept_addr_len ;
	int			opts ;
	
	struct epoll_event	event ;
	
	struct HttpEnv		*e = NULL ;
	
	int			nret = 0 ;
	
	accept_addr_len = sizeof(struct sockaddr) ;
	accept_sock = accept( listen_sock , (struct sockaddr *) & accept_addr, & accept_addr_len );
	if( accept_sock == - 1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "accept failed , errno[%d]" , errno );
		return -1;
	}
	
	opts = fcntl( accept_sock , F_GETFL ) ;
	opts = opts | O_NONBLOCK ;
	fcntl( accept_sock , F_SETFL , opts );
	
	e = CreateHttpEnv() ;
	if( e == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "CreateHttpEnv failed , errno[%d]" , errno );
		CLOSESOCKET( accept_sock );
		return -1;
	}
	
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = e ;
	SetParserCustomIntData( e , accept_sock );
	nret = epoll_ctl( epoll_fd , EPOLL_CTL_ADD , accept_sock , & event ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "epoll_ctl failed , errno[%d]" , errno );
		DestroyHttpEnv( e );
		CLOSESOCKET( accept_sock );
		return -1;
	}
	
	return 0;
}

static int OnReceivingSocket( int epoll_fd , int accept_sock , struct HttpEnv *e , char *wwwroot )
{
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	nret = ReceiveHttpRequestNonblock( accept_sock , NULL , e ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
	{
printf( "111\n" );
		;
	}
	else if( nret )
	{
printf( "222\n" );
		if( nret == FASTERHTTP_INFO_TCP_CLOSE )
		{
			InfoLog( __FILE__ , __LINE__ , "accepted socket closed detected" );
			return -1;
		}
		else
		{
			ErrorLog( __FILE__ , __LINE__ , "ReceiveHttpRequestNonblock failed[%d] , errno[%d]" , nret , errno );
			
			nret = FormatHttpResponseStartLine( abs(nret)/1000 , e ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "FormatHttpResponseStartLine failed[%d] , errno[%d]" , nret , errno );
				return -2;
			}
			
			nret = StrcatHttpBuffer( GetHttpResponseBuffer(e) , HTTP_RETURN_NEWLINE ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "StrcatHttpBuffer failed[%d] , errno[%d]" , nret , errno );
				return -2;
			}
			
			return 0;
		}
	}
	else
	{
printf( "333\n" );
		nret = ProcessHttpRequest( e , GetParserCustomIntData(e) , wwwroot ) ;
		if( nret )
		{
			nret = FormatHttpResponseStartLine( HTTP_SERVICE_UNAVAILABLE , e ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "FormatHttpResponseStartLine failed[%d] , errno[%d]" , nret , errno );
				return -2;
			}
		}
			
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.events = EPOLLOUT | EPOLLERR ;
		event.data.ptr = e ;
		nret = epoll_ctl( epoll_fd , EPOLL_CTL_MOD , accept_sock , & event ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "epoll_ctl failed , errno[%d]" , errno );
			return -2;
		}
	}
	
	return 0;
}

static int OnSendingSocket( int epoll_fd , int accept_sock , struct HttpEnv *e )
{
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	nret = SendHttpResponseNonblock( accept_sock , NULL , e ) ;
	if( nret == FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
	{
		;
	}
	else if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "SendHttpResponseNonblock failed[%d] , errno[%d]" , nret , errno );
		return -1;
	}
	else
	{
		if( CheckHttpKeepAlive(e) )
		{
			ResetHttpEnv(e);
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.events = EPOLLIN | EPOLLERR ;
			event.data.ptr = e ;
			nret = epoll_ctl( epoll_fd , EPOLL_CTL_MOD , accept_sock , & event ) ;
			if( nret == -1 )
			{
				ErrorLog( __FILE__ , __LINE__ , "epoll_ctl failed , errno[%d]" , errno );
				return -2;
			}
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "close client socket" );
			return -1;
		}
	}
	
	return 0;
}

static int htmlserver( short port , char *wwwroot )
{
	int			epoll_fd ;
	struct epoll_event	event , *p_event = NULL ;
	struct epoll_event	events[ MAX_EPOLL_EVENTS ] ;
	int			nfds , i ;
	
	struct HttpEnv		*e = NULL ;
	
	SOCKET			listen_sock ;
	struct sockaddr_in	listen_addr ;
	int			onoff ;
	
	int			nret = 0 ;
	
	epoll_fd = epoll_create( 1024 ) ;
	if( epoll_fd == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "epoll_create ok" );
	}
	
	listen_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( listen_sock == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "socket failed , errno[%d]" , errno );
		CLOSESOCKET( epoll_fd );
		return -1;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "socket ok" );
	}
	
	onoff = 1 ;
	setsockopt( listen_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & onoff , sizeof(onoff) );
	
	memset( & listen_addr , 0x00 , sizeof(struct sockaddr_in) ) ;
	listen_addr.sin_family = AF_INET ;
	listen_addr.sin_addr.s_addr = INADDR_ANY ;
	listen_addr.sin_port = htons( (unsigned short)9527 ) ;
	
	nret = bind( listen_sock , (struct sockaddr *) & listen_addr , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "bind failed , errno[%d]" , errno );
		CLOSESOCKET( listen_sock );
		CLOSESOCKET( epoll_fd );
		return -1;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "bind ok" );
	}
	
	nret = listen( listen_sock , 1024 ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "listen failed , errno[%d]" , errno );
		CLOSESOCKET( listen_sock );
		CLOSESOCKET( epoll_fd );
		return -1;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "listen ok" );
	}
	
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = NULL ;
	nret = epoll_ctl( epoll_fd , EPOLL_CTL_ADD , listen_sock , & event ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "epoll_ctl failed , errno[%d]" , errno );
		CLOSESOCKET( listen_sock );
		CLOSESOCKET( epoll_fd );
		return -1;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "epoll_ctl ok" );
	}
	
	while(1)
	{
		memset( events , 0x00 , sizeof(events) );
		nfds = epoll_wait( epoll_fd , events , MAX_EPOLL_EVENTS , -1 ) ;
		if( nfds == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "epoll_wait failed , errno[%d]" , errno );
			CLOSESOCKET( listen_sock );
			CLOSESOCKET( epoll_fd );
			return -1;
		}
		
		for( i = 0 , p_event = events ; i < nfds ; i++ , p_event++ )
		{
			if( p_event->data.ptr == NULL )
			{
				if( p_event->events & EPOLLIN )
				{
					nret = OnAcceptingSocket( epoll_fd , listen_sock ) ;
					if( nret == -1 )
					{
						ErrorLog( __FILE__ , __LINE__ , "OnAcceptingSocket failed , errno[%d]" , errno );
						CLOSESOCKET( listen_sock );
						CLOSESOCKET( epoll_fd );
						return nret;
					}
				}
				else if( p_event->events & EPOLLERR )
				{
					ErrorLog( __FILE__ , __LINE__ , "listen_sock epoll EPOLLERR" );
					CLOSESOCKET( listen_sock );
					CLOSESOCKET( epoll_fd );
					return -1;
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "listen_sock epoll event invalid[%d]" , p_event->events );
					CLOSESOCKET( listen_sock );
					CLOSESOCKET( epoll_fd );
					return -1;
				}
			}
			else
			{
				int	accept_sock ;
				
				e = p_event->data.ptr ;
				accept_sock = GetParserCustomIntData(e) ;
				
				if( p_event->events & EPOLLIN )
				{
DebugLog( __FILE__ , __LINE__ , "OnReceivingSocket" );
					nret = OnReceivingSocket( epoll_fd , accept_sock , e , wwwroot ) ;
					if( nret == -1 )
					{
						epoll_ctl( epoll_fd , EPOLL_CTL_DEL , accept_sock , NULL );
						CLOSESOCKET( accept_sock );
					}
					else if( nret == -2 )
					{
						epoll_ctl( epoll_fd , EPOLL_CTL_DEL , accept_sock , NULL );
						CLOSESOCKET( accept_sock );
						CLOSESOCKET( listen_sock );
						CLOSESOCKET( epoll_fd );
						return nret;
					}
				}
				else if( p_event->events & EPOLLOUT )
				{
DebugLog( __FILE__ , __LINE__ , "OnSendingSocket" );
					nret = OnSendingSocket( epoll_fd , accept_sock , e ) ;
					if( nret == -1 )
					{
						epoll_ctl( epoll_fd , EPOLL_CTL_DEL , accept_sock , NULL );
						CLOSESOCKET( accept_sock );
					}
					else if( nret == -2 )
					{
						epoll_ctl( epoll_fd , EPOLL_CTL_DEL , accept_sock , NULL );
						CLOSESOCKET( accept_sock );
						CLOSESOCKET( listen_sock );
						CLOSESOCKET( epoll_fd );
						return nret;
					}
				}
				else if( p_event->events & EPOLLERR )
				{
DebugLog( __FILE__ , __LINE__ , "EPOLLERR" );
					ErrorLog( __FILE__ , __LINE__ , "accept_sock epoll EPOLLERR" );
					epoll_ctl( epoll_fd , EPOLL_CTL_DEL , accept_sock , NULL );
					CLOSESOCKET( accept_sock );
				}
				else
				{
DebugLog( __FILE__ , __LINE__ , "invalid" );
					ErrorLog( __FILE__ , __LINE__ , "accept_sock epoll event invalid[%d]" , p_event->events );
					epoll_ctl( epoll_fd , EPOLL_CTL_DEL , accept_sock , NULL );
					CLOSESOCKET( accept_sock );
				}
			}
		}
	}
	
	CLOSESOCKET( listen_sock );
	CLOSESOCKET( epoll_fd );
	
	return 0;
}

static void usage()
{
	printf( "USAGE : htmlserver port\n" );
	return;
}

int main( int argc , char *argv[] )
{
	SetLogFile( "%s/log/htmlserver.log" , getenv("HOME") );
	SetLogLevel( LOGLEVEL_DEBUG );
	
	if( argc == 1 + 2 )
	{
		return -htmlserver( (short)atoi(argv[1]) , argv[2] );
	}
	else
	{
		usage();
		exit(9);
	}
}
