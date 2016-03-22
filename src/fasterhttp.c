/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "fasterhttp.h"

struct HttpBuffer
{
	long		buf_size ;
	char		*base ;
	char		*fill_ptr ;
	long		fill_len ;
	char		*process_ptr ;
	long		process_len ;
} ;

struct HttpHeaderItem
{
	char		*key_base ;
	long		key_len ;
	char		*value_base ;
	long		value_len ;
} ;

struct HttpHeader
{
	struct HttpHeaderItem	METHOD ;
	struct HttpHeaderItem	URI ;
	struct HttpHeaderItem	VERSION ;
	struct HttpHeaderItem	CONTENT_LENGTH ;
	
	struct HttpHeaderItem	*item_array ;
	long			item_array_size ;
	
	long			content_length ;
} ;

struct HttpEnv
{
	struct timeval		timeout ;
	
	struct HttpBuffer	request_buffer ;
	struct HttpBuffer	response_buffer ;
	
	int			parse_step ;
	struct HttpHeader	header ;
} ;

struct HttpEnv *CreateHttpEnv()
{
	struct HttpEnv	*e = NULL ;
	
	e = (struct HttpEnv *)malloc( sizeof(struct HttpEnv) ) ;
	if( e == NULL )
	{
		DestroyHttpEnv( e );
		return NULL;
	}
	memset( e , 0x00 , sizeof(struct HttpEnv) );
	
	SetHttpTimeout( e , 60 );
	
	e->request_buffer.buf_size = FASTERHTTP_REQUEST_BUFSIZE_DEFAULT ;
	e->request_buffer.base = (char*)malloc( e->request_buffer.buf_size ) ;
	if( e->request_buffer.base == NULL )
	{
		DestroyHttpEnv( e );
		return NULL;
	}
	memset( e->request_buffer.base , 0x00 , sizeof(e->request_buffer.buf_size) );
	e->request_buffer.fill_ptr = e->request_buffer.base ;
	e->request_buffer.process_ptr = e->request_buffer.base ;
	e->request_buffer.fill_len = 0 ;
	e->request_buffer.process_len = 0 ;
	
	e->response_buffer.buf_size = FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT ;
	e->response_buffer.base = (char*)malloc( e->response_buffer.buf_size ) ;
	if( e->response_buffer.base == NULL )
	{
		DestroyHttpEnv( e );
		return NULL;
	}
	memset( e->response_buffer.base , 0x00 , sizeof(e->response_buffer.buf_size) );
	e->response_buffer.fill_ptr = e->response_buffer.base ;
	e->response_buffer.process_ptr = e->response_buffer.base ;
	e->response_buffer.fill_len = 0 ;
	e->response_buffer.process_len = 0 ;
	
	e->header.item_array_size = FASTERHTTP_HEADER_ITEM_ARRAYSIZE_DEFAULT ;
	e->header.item_array = (struct HttpHeaderItem *)malloc( sizeof(struct HttpHeaderItem) * e->header.item_array_size ) ;
	if( e->header.item_array == NULL )
	{
		DestroyHttpEnv( e );
		return NULL;
	}
	memset( e->header.item_array , 0x00 , sizeof(struct HttpHeaderItem) * e->header.item_array_size );
	
	e->parse_step = FASTERHTTP_PARSE_STEP_FIRSTLINE ;
	
	return e;
}

void ResetHttpEnv( struct HttpEnv *e )
{
	if( e->request_buffer.buf_size > FASTERHTTP_REQUEST_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->request_buffer) , FASTERHTTP_REQUEST_BUFSIZE_DEFAULT );
	if( e->response_buffer.buf_size > FASTERHTTP_RESPONSE_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->response_buffer) , FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT );
	
	if( e->header.item_array_size > FASTERHTTP_HEADER_ITEM_ARRAYSIZE_MAX )
	{
		struct HttpHeaderItem	*p = NULL ;
		p = (struct HttpHeaderItem *)malloc( sizeof(struct HttpHeaderItem) * FASTERHTTP_HEADER_ITEM_ARRAYSIZE_DEFAULT ) ;
		if( p )
		{
			memset( p , 0x00 , sizeof(struct HttpHeaderItem) * e->header.item_array_size );
			e->header.item_array = p ;
			e->header.item_array_size = FASTERHTTP_HEADER_ITEM_ARRAYSIZE_DEFAULT ;
		}
	}
	
	CleanHttpBuffer( &(e->request_buffer) );
	CleanHttpBuffer( &(e->response_buffer) );
	
	e->parse_step = FASTERHTTP_PARSE_STEP_FIRSTLINE ;
	
	return;
}

void DestroyHttpEnv( struct HttpEnv *e )
{
	if( e )
	{
		if( e->request_buffer.base )
			free( e->request_buffer.base );
		if( e->response_buffer.base )
			free( e->response_buffer.base );
		
		if( e->header.item_array )
			free( e->header.item_array );
		
		free( e );
	}
	
	return;
}

void SetHttpTimeout( struct HttpEnv *e , long timeout )
{
	e->timeout.tv_sec = 60 ;
	e->timeout.tv_usec = 0 ;
	
	return;
}

struct HttpBuffer *GetHttpRequestBuffer( struct HttpEnv *e )
{
	return &(e->request_buffer);
}

struct HttpBuffer *GetHttpResponseBuffer( struct HttpEnv *e )
{
	return &(e->response_buffer);
}

char *GetHttpBufferBase( struct HttpBuffer *b )
{
	return b->base;
}

long GetHttpBufferLength( struct HttpBuffer *b )
{
	return b->fill_len;
}

void CleanHttpBuffer( struct HttpBuffer *b )
{
	memset( b->base , 0x00 , b->buf_size );
	b->fill_ptr = b->base ;
	b->fill_len = 0 ;
	b->process_ptr = b->base ;
	b->process_len = 0 ;
	
	return;
}

int ReallocHttpBuffer( struct HttpBuffer *b , long new_buf_size )
{
	char	*new_base = NULL ;
	
	if( new_buf_size == -1 )
	{
		if( b->buf_size <= 100*1024*1024 )
			new_buf_size = b->buf_size * 2 ;
		else
			new_buf_size = 100*1024*1024 ;
	}
	new_base = (char *)realloc( b->base , new_buf_size ) ;
	if( new_base == NULL )
		return FASTERHTTP_ERROR_ALLOC;
	memset( new_base + b->fill_len , 0x00 , new_buf_size - b->fill_len );
	b->buf_size = new_buf_size ;
	b->base = new_base ;
	b->fill_ptr = b->base + b->fill_len ;
	b->process_ptr = b->base + b->process_len ;
	
	return 0;
}

int StrcatfHttpBuffer( struct HttpBuffer *b , char *format , ... )
{
	va_list		valist ;
	long		len ;
	
	while(1)
	{
		va_start( valist , format );
		len = VSNPRINTF( b->fill_ptr , b->buf_size-1 - b->fill_len , format , valist ) ;
		va_end( valist );
		if( len == -1 || len == b->buf_size-1 - b->fill_len )
		{
			ReallocHttpBuffer( b , -1 );
		}
		else
		{
			b->fill_len += len ;
			break;
		}
	}
	
	return 0;
}

int MemcatHttpBuffer( struct HttpBuffer *b , char *base , long len )
{
	while( b->fill_len + len > b->buf_size-1 )
	{
		ReallocHttpBuffer( b , -1 );
	}
	
	memcpy( b->fill_ptr , base , len );
	b->fill_len += len ;
	
	return 0;
}

static void AjustTimeval( struct timeval *ptv , struct timeval *t1 , struct timeval *t2 )
{
	ptv->tv_sec -= ( t2->tv_sec - t1->tv_sec ) ;
	ptv->tv_usec -= ( t2->tv_usec - t1->tv_usec ) ;
	
	while( ptv->tv_usec < 0 )
	{
		ptv->tv_usec += 1000000 ;
		ptv->tv_sec--;
	}
	
	return;
}

int WriteHttpBuffer( int sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
{
	struct timeval	t1 , t2 ;
	fd_set		write_fds ;
	long		len ;
	int		nret = 0 ;
	
	if( b->process_len >= b->fill_len )
		return 0;
	
	gettimeofday( & t1 , NULL );
	
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
		len = write( sock , b->process_ptr , b->fill_len-b->process_len ) ;
	else
		len = SSL_write( ssl , b->process_ptr , b->fill_len-b->process_len ) ;
	if( len == -1 )
		return FASTERHTTP_ERROR_TCP_SEND;
	
	gettimeofday( & t2 , NULL );
	AjustTimeval( & (e->timeout) , & t1 , & t2 );
	
	b->process_ptr += len ;
	b->process_len += len ;
	
	if( b->process_len >= b->fill_len )
		return 0;
	else
		return FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK;
}

int ReadHttpBuffer( int sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
{
	struct timeval	t1 , t2 ;
	fd_set		read_fds ;
	long		len ;
	int		nret = 0 ;
	
	while( b->fill_len + FASTERHTTP_READBLOCK_SIZE_DEFAULT > b->buf_size )
	{
		nret = ReallocHttpBuffer( b , -1 ) ;
		if( nret )
			return nret;
	}
	
	gettimeofday( & t1 , NULL );
	
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
		len = (long)read( sock , b->fill_ptr , FASTERHTTP_READBLOCK_SIZE_DEFAULT ) ;
	else
		len = (long)SSL_read( ssl , b->fill_ptr , FASTERHTTP_READBLOCK_SIZE_DEFAULT ) ;
	if( len == -1 )
		return FASTERHTTP_ERROR_TCP_RECEIVE;
	else if( len == 0 )
		return FASTERHTTP_ERROR_HTTP_TRUNCATION;
	
	gettimeofday( & t2 , NULL );
	AjustTimeval( & (e->timeout) , & t1 , & t2 );
	
	b->fill_ptr += len ;
	b->fill_len += len ;
	
	return 0;
}

static int ParseHttpRequestHeaderFirstLine( struct HttpEnv *e , struct HttpBuffer *b )
{
	char	*line_end = NULL ;
	
	line_end = strstr( e->line_begin , "\r\n" ) ;
	if( line_end )
	{
		char	*p = NULL ;
		for( p = e->line_begin ; (*p) == ' ' ; p++ )
			;
		if( (*p) == '\r' )
		e->header.METHOD.value_ptr = p ;
		for( p = e->line_begin ; (*p) != ' ' ; p++ )
			;
		e->header.METHOD.value_len = p - e->header.METHOD.value_ptr ;
	}
	
	
	
	
	
}

static int ParseHttpResponseHeaderFirstLine( struct HttpEnv *e , struct HttpBuffer *b )
{
}

int ParseHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b )
{
	int		nret = 0 ;
	
	while(1)
	{
		switch( e->parse_step )
		{
			case FASTERHTTP_PARSE_STEP_FIRSTLINE :
				
				e->line_begin = b->base ;
				
				if( b == &(e->request_buffer) )
				{
					nret = ParseHttpRequestHeaderFirstLine( e , b ) ;
					if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
						break;
					else if( nret )
						return nret;
					else
						e->parse_step = 
				}
				else if( b == &(e->response_buffer)  )
				{
					nret = ParseHttpResponseHeaderFirstLine( e , b ) ;
					if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
						break;
					else if( nret )
						return nret;
				}
				else
				{
					return FASTERHTTP_ERROR_PARAMTER;
				}
				
				break;
			
			case FASTERHTTP_PARSE_STEP_HEADER :
				
				
				
				break;
			
			case FASTERHTTP_PARSE_STEP_BODY :
				
				
				
				break;
			
			case FASTERHTTP_PARSE_STEP_DONE :
				
				
				
				break;
			
			default :
				return FASTERHTTP_ERROR_INTERNAL;
		}
	}
	
	return nret;
}

int SendHttpRequest( int sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	while(1)
	{
		nret = WriteHttpBuffer( sock , ssl , e , &(e->request_buffer) ) ;
		if( nret == 0 )
			break;
		else if( nret != FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
			return nret;
	}
	
	return 0;
}

int ReceiveHttpResponse( int sock , SSL *ssl , struct HttpEnv *e )
{
	return 0;
}

int ParseHttpResponse( struct HttpEnv *e )
{
	return 0;
}

int ReceiveHttpRequest( int sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	while(1)
	{
		nret = ReadHttpBuffer( sock , ssl , e , &(e->request_buffer) ) ;
		if( nret )
			return nret;
		
		
		
		
	}
	
	return 0;
}

int ParseHttpRequest( struct HttpEnv *e )
{
	
	
	
	
	
	
	
	return 0;
}

int SendHttpResponse( int sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	while(1)
	{
		nret = WriteHttpBuffer( sock , ssl , e , &(e->response_buffer) ) ;
		if( nret == 0 )
			break;
		else if( nret != FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
			return nret;
	}
	
	return 0;
}

