/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "internal.h"
#include "fasterhttp.h"

#define DEBUG		0

static void AjustTimeval( struct timeval *ptv , struct timeval *t1 , struct timeval *t2 )
{
#if ( defined _WIN32 )
	ptv->tv_sec -= ( t2->tv_sec - t1->tv_sec ) ;
#elif ( defined __unix ) || ( defined __linux__ )
	ptv->tv_sec -= ( t2->tv_sec - t1->tv_sec ) ;
	ptv->tv_usec -= ( t2->tv_usec - t1->tv_usec ) ;
	while( ptv->tv_usec < 0 )
	{
		ptv->tv_usec += 1000000 ;
		ptv->tv_sec--;
	}
#endif
	if( ptv->tv_sec < 0 )
		ptv->tv_sec = 0 ;
	
	return;
}

int SendHttpBuffer( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
{
	struct timeval	t1 , t2 ;
	fd_set		write_fds ;
	int		len ;
	int		nret = 0 ;
	
	if( b->process_ptr >= b->fill_ptr )
		return 0;
	
#if ( defined _WIN32 )
	time( &(t1.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t1 , NULL );
#endif
	
	FD_ZERO( & write_fds );
	FD_SET( sock , & write_fds );
	
	nret = select( sock+1 , NULL , & write_fds , NULL , &(e->timeout) ) ;
	if( nret == 0 )
	{
		return FASTERHTTP_ERROR_TCP_SELECT_SEND_TIMEOUT;
	}
	else if( nret != 1 )
	{
		return FASTERHTTP_ERROR_TCP_SELECT_SEND;
	}
	
	if( ssl == NULL )
		len = (int)send( sock , b->process_ptr , b->fill_ptr-b->process_ptr , 0 ) ;
	else
		len = (int)SSL_write( ssl , b->process_ptr , b->fill_ptr-b->process_ptr ) ;
	if( len == -1 )
	{
		return FASTERHTTP_ERROR_TCP_SEND;
	}
	
#if DEBUG
printf( "send\n" );
_DumpHexBuffer( stdout , b->process_ptr , len );
#endif

#if ( defined _WIN32 )
	time( &(t2.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t2 , NULL );
#endif
	AjustTimeval( & (e->timeout) , & t1 , & t2 );
	
	b->process_ptr += len ;
	
	if( b->process_ptr >= b->fill_ptr )
		return 0;
	else
		return FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK;
}

int ReceiveHttpBuffer( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
{
	struct timeval	t1 , t2 ;
	fd_set		read_fds ;
	int		len ;
	int		nret = 0 ;
	
	if( b->process_ptr == b->base && b->process_ptr < b->fill_ptr )
		return 0;
	
	while( (b->fill_ptr-b->base) >= b->buf_size-1 )
	{
		nret = ReallocHttpBuffer( b , -1 ) ;
		if( nret )
			return nret;
	}
	
#if ( defined _WIN32 )
	time( &(t1.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t1 , NULL );
#endif
	
	FD_ZERO( & read_fds );
	FD_SET( sock , & read_fds );
	
	nret = select( sock+1 , & read_fds , NULL , NULL , &(e->timeout) ) ;
	if( nret == 0 )
	{
		return FASTERHTTP_ERROR_TCP_SELECT_RECEIVE_TIMEOUT;
	}
	else if( nret != 1 )
	{
		return FASTERHTTP_ERROR_TCP_SELECT_RECEIVE;
	}
	
	if( ssl == NULL )
		len = (int)recv( sock , b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) , 0 ) ;
	else
		len = (int)SSL_read( ssl , b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) ) ;
	if( len == -1 )
	{
		return FASTERHTTP_ERROR_TCP_RECEIVE;
	}
	else if( len == 0 )
	{
		return FASTERHTTP_ERROR_TCP_CLOSE;
	}
	
#if DEBUG
printf( "recv\n" );
_DumpHexBuffer( stdout , b->fill_ptr , len );
#endif

#if ( defined _WIN32 )
	time( &(t2.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t2 , NULL );
#endif
	AjustTimeval( & (e->timeout) , & t1 , & t2 );
	
	b->fill_ptr += len ;
	
	return 0;
}

int ReceiveHttpBuffer1( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
{
	struct timeval	t1 , t2 ;
	fd_set		read_fds ;
	long		len ;
	int		nret = 0 ;
	
	while( (b->fill_ptr-b->base) >= b->buf_size-1 )
	{
		nret = ReallocHttpBuffer( b , -1 ) ;
		if( nret )
			return nret;
	}
	
#if ( defined _WIN32 )
	time( &(t1.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t1 , NULL );
#endif
	
	FD_ZERO( & read_fds );
	FD_SET( sock , & read_fds );
	
	nret = select( sock+1 , & read_fds , NULL , NULL , &(e->timeout) ) ;
	if( nret == 0 )
	{
		return FASTERHTTP_ERROR_TCP_SELECT_RECEIVE_TIMEOUT;
	}
	else if( nret != 1 )
	{
		return FASTERHTTP_ERROR_TCP_SELECT_RECEIVE;
	}
	
	if( ssl == NULL )
		len = (long)recv( sock , b->fill_ptr , 1 , 0 ) ;
	else
		len = (long)SSL_read( ssl , b->fill_ptr , 1 ) ;
	if( len == -1 )
		return FASTERHTTP_ERROR_TCP_RECEIVE;
	else if( len == 0 )
		return FASTERHTTP_ERROR_TCP_CLOSE;
	
#if ( defined _WIN32 )
	time( &(t2.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t2 , NULL );
#endif
	AjustTimeval( & (e->timeout) , & t1 , & t2 );
	
	b->fill_ptr += len ;
	
	return 0;
}

