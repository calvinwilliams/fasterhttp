/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "fasterhttp.h"

int _FASTERHTTP_VERSION_1_0_0 = 0 ;

struct HttpBuffer
{
	int			buf_size ;
	char			*base ;
	char			ref_flag ;
	char			*fill_ptr ;
	char			*process_ptr ;
} ;

struct HttpHeader
{
	char			*name_ptr ;
	int			name_len ;
	char			*value_ptr ;
	int			value_len ;
} ;

struct HttpHeaders
{
	struct HttpHeader	METHOD ;
	struct HttpHeader	URI ;
	struct HttpHeader	VERSION ;
	
	struct HttpHeader	STATUSCODE ;
	struct HttpHeader	REASONPHRASE ;
	
	int			content_length ;
	
	char			transfer_encoding__chunked ;
	struct HttpHeader	TRAILER ;
	
	struct HttpHeader	*header_array ;
	int			header_array_size ;
	int			header_array_count ;
} ;

struct HttpEnv
{
	struct timeval		timeout ;
	
	struct HttpBuffer	request_buffer ;
	struct HttpBuffer	response_buffer ;
	
	int			parse_step ;
	struct HttpHeaders	headers ;
	
	char			*body ;
	
	char			*chunked_body ;
	char			*chunked_body_end ;
	int			chunked_length ;
	int			chunked_length_length ;
} ;

static void CleanHttpHeader( struct HttpHeaders *p_headers )
{
	memset( p_headers->header_array , 0x00 , sizeof(struct HttpHeader) * p_headers->header_array_size );
	p_headers->header_array_count = 0 ;
	return;
}

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
	
	SetHttpTimeout( e , FASTERHTTP_TIMEOUT_DEFAULT );
	
	e->request_buffer.buf_size = FASTERHTTP_REQUEST_BUFSIZE_DEFAULT ;
	e->request_buffer.base = (char*)malloc( e->request_buffer.buf_size ) ;
	if( e->request_buffer.base == NULL )
	{
		DestroyHttpEnv( e );
		return NULL;
	}
	memset( e->request_buffer.base , 0x00 , sizeof(e->request_buffer.buf_size) );
	e->request_buffer.ref_flag = 0 ;
	e->request_buffer.fill_ptr = e->request_buffer.base ;
	e->request_buffer.process_ptr = e->request_buffer.base ;
	
	e->response_buffer.buf_size = FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT ;
	e->response_buffer.base = (char*)malloc( e->response_buffer.buf_size ) ;
	if( e->response_buffer.base == NULL )
	{
		DestroyHttpEnv( e );
		return NULL;
	}
	memset( e->response_buffer.base , 0x00 , sizeof(e->response_buffer.buf_size) );
	e->response_buffer.ref_flag = 0 ;
	e->response_buffer.fill_ptr = e->response_buffer.base ;
	e->response_buffer.process_ptr = e->response_buffer.base ;
	
	e->headers.header_array_size = FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT ;
	e->headers.header_array = (struct HttpHeader *)malloc( sizeof(struct HttpHeader) * e->headers.header_array_size ) ;
	if( e->headers.header_array == NULL )
	{
		DestroyHttpEnv( e );
		return NULL;
	}
	memset( e->headers.header_array , 0x00 , sizeof(struct HttpHeader) * e->headers.header_array_size );
	e->headers.header_array_count = 0 ;
	
	e->parse_step = FASTERHTTP_PARSESTEP_BEGIN ;
	
	return e;
}

void ResetHttpEnv( struct HttpEnv *e )
{
	struct HttpHeaders	*p_headers = &(e->headers) ;
	
	/* struct HttpEnv */
	
	SetHttpTimeout( e , FASTERHTTP_TIMEOUT_DEFAULT );
	
	if( UNLIKELY( e->request_buffer.ref_flag == 0 && e->request_buffer.buf_size > FASTERHTTP_REQUEST_BUFSIZE_MAX ) )
		ReallocHttpBuffer( &(e->request_buffer) , FASTERHTTP_REQUEST_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->request_buffer) );
	if( UNLIKELY( e->response_buffer.ref_flag == 0 && e->response_buffer.buf_size > FASTERHTTP_RESPONSE_BUFSIZE_MAX ) )
		ReallocHttpBuffer( &(e->response_buffer) , FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->response_buffer) );
	
	e->parse_step = FASTERHTTP_PARSESTEP_BEGIN ;
	
	e->body = NULL ;
	
	e->chunked_body = NULL ;
	e->chunked_body_end = NULL ;
	e->chunked_length = 0 ;
	e->chunked_length_length = 0 ;
	
	/* struct HttpHeaders */
	
	p_headers->METHOD.value_ptr = NULL ;
	p_headers->METHOD.value_len = 0 ;
	p_headers->URI.value_ptr = NULL ;
	p_headers->URI.value_len = 0 ;
	p_headers->VERSION.value_ptr = NULL ;
	p_headers->VERSION.value_len = 0 ;
	p_headers->STATUSCODE.value_ptr = NULL ;
	p_headers->STATUSCODE.value_len = 0 ;
	p_headers->REASONPHRASE.value_ptr = NULL ;
	p_headers->REASONPHRASE.value_len = 0 ;
	p_headers->content_length = 0 ;
	p_headers->TRAILER.value_ptr = NULL ;
	p_headers->TRAILER.value_len = 0 ;
	p_headers->transfer_encoding__chunked = 0 ;
	
	if( UNLIKELY( p_headers->header_array_size > FASTERHTTP_HEADER_ARRAYSIZE_MAX ) )
	{
		struct HttpHeader	*p = NULL ;
		p = (struct HttpHeader *)realloc( p_headers->header_array , sizeof(struct HttpHeader) * FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT ) ;
		if( p )
		{
			memset( p , 0x00 , sizeof(struct HttpHeader) * p_headers->header_array_size );
			p_headers->header_array = p ;
			p_headers->header_array_size = FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT ;
		}
	}
	CleanHttpHeader( p_headers );
	
	return;
}

void DestroyHttpEnv( struct HttpEnv *e )
{
	if( e )
	{
		if( e->request_buffer.ref_flag == 0 && e->request_buffer.base )
			free( e->request_buffer.base );
		if( e->response_buffer.ref_flag == 0 && e->response_buffer.base )
			free( e->response_buffer.base );
		
		if( e->headers.header_array )
			free( e->headers.header_array );
		
		free( e );
	}
	
	return;
}

void SetHttpTimeout( struct HttpEnv *e , long timeout )
{
	e->timeout.tv_sec = timeout ;
	e->timeout.tv_usec = 0 ;
	
	return;
}

struct timeval *GetHttpElapse( struct HttpEnv *e )
{
	return &(e->timeout);
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

int GetHttpBufferLength( struct HttpBuffer *b )
{
	return b->fill_ptr-b->base;
}

char *GetHttpBufferFillPtr( struct HttpBuffer *b )
{
	return b->fill_ptr;
}

int GetHttpBufferRemainLength( struct HttpBuffer *b )
{
	return b->buf_size-1-(b->fill_ptr-b->base);
}

void OffsetHttpBufferFillPtr( struct HttpBuffer *b , int len )
{
	b->fill_ptr += len ;
	return;
}

void CleanHttpBuffer( struct HttpBuffer *b )
{
	b->fill_ptr = b->base ;
	b->process_ptr = b->base ;
	
	return;
}

int ReallocHttpBuffer( struct HttpBuffer *b , long new_buf_size )
{
	char	*new_base = NULL ;
	int	fill_len = b->fill_ptr - b->base ;
	int	process_len = b->process_ptr - b->base ;
	
	if( b->ref_flag == 1 )
		return FASTERHTTP_ERROR_USING;
	
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
	memset( new_base + fill_len , 0x00 , new_buf_size - fill_len );
	b->buf_size = new_buf_size ;
	b->base = new_base ;
	b->fill_ptr = b->base + fill_len ;
	b->process_ptr = b->base + process_len ;
	
	return 0;
}

int StrcpyHttpBuffer( struct HttpBuffer *b , char *str )
{
	int		len ;
	
	len = strlen(str) ;
	while( len > b->buf_size-1 )
	{
		ReallocHttpBuffer( b , -1 );
	}
	
	memcpy( b->base , str , len );
	b->fill_ptr = b->base + len ;
	
	return 0;
}

int StrcpyfHttpBuffer( struct HttpBuffer *b , char *format , ... )
{
	va_list		valist ;
	int		len ;
	
	while(1)
	{
		va_start( valist , format );
		len = VSNPRINTF( b->base , b->buf_size-1 , format , valist ) ;
		va_end( valist );
		if( len == -1 || len == b->buf_size-1 )
		{
			ReallocHttpBuffer( b , -1 );
		}
		else
		{
			b->fill_ptr = b->base + len ;
			break;
		}
	}
	
	return 0;
}

int StrcpyvHttpBuffer( struct HttpBuffer *b , char *format , va_list valist )
{
	int		len ;
	
	while(1)
	{
		len = VSNPRINTF( b->base , b->buf_size-1 , format , valist ) ;
		if( len == -1 || len == b->buf_size-1 )
		{
			ReallocHttpBuffer( b , -1 );
		}
		else
		{
			b->fill_ptr = b->base + len ;
			break;
		}
	}
	
	return 0;
}

int StrcatHttpBuffer( struct HttpBuffer *b , char *str )
{
	long		len ;
	
	len = strlen(str) ;
	while( (b->fill_ptr-b->base) + len > b->buf_size-1 )
	{
		ReallocHttpBuffer( b , -1 );
	}
	
	memcpy( b->fill_ptr , str , len );
	b->fill_ptr += len ;
	
	return 0;
}

int StrcatfHttpBuffer( struct HttpBuffer *b , char *format , ... )
{
	va_list		valist ;
	long		len ;
	
	while(1)
	{
		va_start( valist , format );
		len = VSNPRINTF( b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) , format , valist ) ;
		va_end( valist );
		if( len == -1 || len == b->buf_size-1 - (b->fill_ptr-b->base) )
		{
			ReallocHttpBuffer( b , -1 );
		}
		else
		{
			b->fill_ptr += len ;
			break;
		}
	}
	
	return 0;
}

int StrcatvHttpBuffer( struct HttpBuffer *b , char *format , va_list valist )
{
	int		len ;
	
	while(1)
	{
		len = VSNPRINTF( b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) , format , valist ) ;
		if( len == -1 || len == b->buf_size-1 - (b->fill_ptr-b->base) )
		{
			ReallocHttpBuffer( b , -1 );
		}
		else
		{
			b->fill_ptr += len ;
			break;
		}
	}
	
	return 0;
}

int MemcatHttpBuffer( struct HttpBuffer *b , char *base , long len )
{
	while( (b->fill_ptr-b->base) + len > b->buf_size-1 )
	{
		ReallocHttpBuffer( b , -1 );
	}
	
	memcpy( b->fill_ptr , base , len );
	b->fill_ptr += len ;
	
	return 0;
}

void SetHttpBufferPtr( struct HttpBuffer *b , char *ptr , long size )
{
	if( b->ref_flag == 0 && b->buf_size > 0 && b->base )
	{
		free( b->base );
	}
	
	b->base = ptr ;
	b->buf_size = size ;
	b->fill_ptr = b->base + size-1 ;
	b->process_ptr = b->base ;
	
	b->ref_flag = 1 ;
	
	return;
}

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

static int SendHttpBuffer( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
{
	struct timeval	t1 , t2 ;
	fd_set		write_fds ;
	long		len ;
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
		len = send( sock , b->process_ptr , b->fill_ptr-b->process_ptr , 0 ) ;
	else
		len = SSL_write( ssl , b->process_ptr , b->fill_ptr-b->process_ptr ) ;
	if( len == -1 )
		return FASTERHTTP_ERROR_TCP_SEND;
	
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

static int ReceiveHttpBuffer( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
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
		len = (long)recv( sock , b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) , 0 ) ;
	else
		len = (long)SSL_read( ssl , b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) ) ;
	if( len == -1 )
		return FASTERHTTP_ERROR_TCP_RECEIVE;
	else if( len == 0 )
		return FASTERHTTP_ERROR_HTTP_TRUNCATION;
	
#if ( defined _WIN32 )
	time( &(t2.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t2 , NULL );
#endif
	AjustTimeval( & (e->timeout) , & t1 , & t2 );
	
	b->fill_ptr += len ;
	
	return 0;
}

static int ReceiveHttpBuffer1( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b )
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
		return FASTERHTTP_ERROR_HTTP_TRUNCATION;
	
#if ( defined _WIN32 )
	time( &(t2.tv_sec) );
#elif ( defined __unix ) || ( defined __linux__ )
	gettimeofday( & t2 , NULL );
#endif
	AjustTimeval( & (e->timeout) , & t1 , & t2 );
	
	b->fill_ptr += len ;
	
	return 0;
}

#define DEBUG_PARSE	0

int ParseHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b )
{
	register char		*p = b->process_ptr ;
	char			*p2 = NULL ;
	
	int			*p_parse_step = &(e->parse_step) ;
	struct HttpHeader	*p_METHOD = &(e->headers.METHOD) ;
	struct HttpHeader	*p_URI = &(e->headers.URI) ;
	struct HttpHeader	*p_VERSION = &(e->headers.VERSION) ;
	struct HttpHeader	*p_STATUSCODE = &(e->headers.STATUSCODE) ;
	struct HttpHeader	*p_REASONPHRASE = &(e->headers.REASONPHRASE) ;
	struct HttpHeader	*p_TRAILER = &(e->headers.TRAILER) ;
	
	struct HttpHeader	*p_header = &(e->headers.header_array[e->headers.header_array_count]) ;
	char			*fill_ptr = b->fill_ptr ;
	
	int			*p_content_length = &(e->headers.content_length) ;
	char			*p_transfer_encoding__chunked = & (e->headers.transfer_encoding__chunked) ;
	
#if DEBUG_PARSE
	printf( "DEBUG_PARSE >>>>>>>>> ParseHttpBuffer - b->process_ptr[0x%02X...][%.*s]\n" , b->process_ptr[0] , (int)(b->fill_ptr-b->process_ptr) , b->process_ptr );
#endif
	if( UNLIKELY( *(p_parse_step) == FASTERHTTP_PARSESTEP_BEGIN ) )
	{
		if( b == &(e->request_buffer) )
		{
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD0 ;
		}
		else if( b == &(e->response_buffer) )
		{
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION0 ;
		}
		else
		{
			return FASTERHTTP_ERROR_PARAMTER;
		}
	}
	
	switch( *(p_parse_step) )
	{
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_METHOD->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_METHOD->value_len = p - p_METHOD->value_ptr ;
			p++;
			
			p2 = p_METHOD->value_ptr ;
			if( LIKELY( p_METHOD->value_len == 3 ) )
			{
				if( LIKELY( *(p2) == HTTP_METHOD_GET[0] && *(p2+1) == HTTP_METHOD_GET[1] && *(p2+2) == HTTP_METHOD_GET[2] ) )
					;
				else if( *(p2) == HTTP_METHOD_PUT[0] && *(p2+1) == HTTP_METHOD_PUT[1] && *(p2+2) == HTTP_METHOD_PUT[2] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( LIKELY( p_METHOD->value_len == 4 ) )
			{
				if( LIKELY( *(p2) == HTTP_METHOD_POST[0] && *(p2+1) == HTTP_METHOD_POST[1] && *(p2+2) == HTTP_METHOD_POST[2]
					&& *(p2+3) == HTTP_METHOD_POST[3] ) )
					;
				else if( *(p2) == HTTP_METHOD_HEAD[0] && *(p2+1) == HTTP_METHOD_HEAD[1] && *(p2+2) == HTTP_METHOD_HEAD[2]
					&& *(p2+3) == HTTP_METHOD_HEAD[3] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( p_METHOD->value_len == 5 )
			{
				if( *(p2) == HTTP_METHOD_TRACE[0] && *(p2+1) == HTTP_METHOD_TRACE[1] && *(p2+2) == HTTP_METHOD_TRACE[2]
					&& *(p2+3) == HTTP_METHOD_TRACE[3] && *(p2+4) == HTTP_METHOD_TRACE[4] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( p_METHOD->value_len == 6 )
			{
				if( *(p2) == HTTP_METHOD_DELETE[0] && *(p2+1) == HTTP_METHOD_DELETE[1] && *(p2+2) == HTTP_METHOD_DELETE[2]
					&& *(p2+3) == HTTP_METHOD_DELETE[3] && *(p2+4) == HTTP_METHOD_DELETE[4] && *(p2+5) == HTTP_METHOD_DELETE[5] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( p_METHOD->value_len == 7 )
			{
				if( *(p2) == HTTP_METHOD_OPTIONS[0] && *(p2+1) == HTTP_METHOD_OPTIONS[1] && *(p2+2) == HTTP_METHOD_OPTIONS[2]
					&& *(p2+3) == HTTP_METHOD_OPTIONS[3] && *(p2+4) == HTTP_METHOD_OPTIONS[4] && *(p2+5) == HTTP_METHOD_OPTIONS[5]
					&& *(p2+6) == HTTP_METHOD_OPTIONS[6] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else
			{
				return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI0 ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_URI->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_URI->value_len = p - p_URI->value_ptr ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION0 ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_VERSION->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION\n" );
#endif
			
			for( ; LIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p_VERSION->value_len = p - p_VERSION->value_ptr ;
			if( LIKELY( *(p-1) == HTTP_RETURN ) )
				p_VERSION->value_len--;
			p++;
			
			p2 = p_VERSION->value_ptr ;
			if( LIKELY( p_VERSION->value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
			{
				if( LIKELY( *(p2) == HTTP_VERSION_1_0[0] && *(p2+1) == HTTP_VERSION_1_0[1] && *(p2+2) == HTTP_VERSION_1_0[2]
					&& *(p2+3) == HTTP_VERSION_1_0[3] && *(p2+4) == HTTP_VERSION_1_0[4] && *(p2+5) == HTTP_VERSION_1_0[5]
					&& *(p2+6) == HTTP_VERSION_1_0[6] ) )
				{
					if( UNLIKELY( *(p2+7) == HTTP_VERSION_1_0[7] ) )
						;
					else if( LIKELY( *(p2+7) == HTTP_VERSION_1_1[7] ) )
						;
					else
						return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
				}
			}
			else
			{
				return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
			}
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_VERSION->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_VERSION->value_len = p - p_VERSION->value_ptr ;
			p++;
			
			p2 = p_VERSION->value_ptr ;
			if( LIKELY( p_VERSION->value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
			{
				if( LIKELY( *(p2) == HTTP_VERSION_1_0[0] && *(p2+1) == HTTP_VERSION_1_0[1] && *(p2+2) == HTTP_VERSION_1_0[2]
					&& *(p2+3) == HTTP_VERSION_1_0[3] && *(p2+4) == HTTP_VERSION_1_0[4] && *(p2+5) == HTTP_VERSION_1_0[5]
					&& *(p2+6) == HTTP_VERSION_1_0[6] ) )
				{
					if( UNLIKELY( *(p2+7) == HTTP_VERSION_1_0[7] ) )
						;
					else if( LIKELY( *(p2+7) == HTTP_VERSION_1_1[7] ) )
						;
					else
						return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
				}
			}
			else
			{
				return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
			}
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE0 ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_STATUSCODE->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_STATUSCODE->value_len = p - p_STATUSCODE->value_ptr ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE0 ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_REASONPHRASE->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE\n" );
#endif
			
			for( ; LIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p_REASONPHRASE->value_len = p - p_REASONPHRASE->value_ptr ;
			if( LIKELY( *(p-1) == HTTP_RETURN ) )
				p_REASONPHRASE->value_len--;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_HEADER_NAME0 :
			
_GOTO_PARSESTEP_HEADER_NAME0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_NAME0\n" );
#endif
			
			/*
			while( UNLIKELY( (*p) == ' ' && p < fill_ptr ) )
				p++;
			*/
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( LIKELY( (*p) == HTTP_RETURN ) )
			{
				if( UNLIKELY( p+1 >= fill_ptr ) )
				{
					return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
				}
				else if( LIKELY( *(p+1) == HTTP_NEWLINE ) )
				{
					p += 2 ;
				}
				else
				{
					p++;
				}
				

#define _IF_THEN_GO_PARSING_BODY \
				if( *(p_content_length) > 0 ) \
				{ \
					e->body = p ; \
					*(p_parse_step) = FASTERHTTP_PARSESTEP_BODY ; \
					goto _GOTO_PARSESTEP_BODY; \
				} \
				else if( *(p_transfer_encoding__chunked) == 1 ) \
				{ \
					e->body = p ; \
					e->chunked_body_end = p ; \
					*(p_parse_step) = FASTERHTTP_PARSESTEP_CHUNKED_SIZE ; \
					goto _GOTO_PARSESTEP_CHUNKED_SIZE; \
				} \
				else \
				{ \
					b->process_ptr = p ; \
					*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ; \
					return 0; \
				}
				
				_IF_THEN_GO_PARSING_BODY
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
			{
				p++;
				
				_IF_THEN_GO_PARSING_BODY
			}
			else
			{
				p_header->name_ptr = p ;
				p++;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME ;
			}
			
		case FASTERHTTP_PARSESTEP_HEADER_NAME :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_NAME\n" );
#endif
			
			while( LIKELY( (*p) != ' ' && (*p) != ':' && (*p) != HTTP_NEWLINE && p < fill_ptr ) )
				p++;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			p_header->name_len = p - p_header->name_ptr ;
			
			while( UNLIKELY( (*p) != ':' && (*p) != HTTP_NEWLINE && p < fill_ptr ) )
				p++;
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			else if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_VALUE0 ;
			
		case FASTERHTTP_PARSESTEP_HEADER_VALUE0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_VALUE0\n" );
#endif
			
			while( LIKELY( (*p) == ' ' && p < fill_ptr ) )
				p++;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			p_header->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_VALUE ;
			
		case FASTERHTTP_PARSESTEP_HEADER_VALUE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_VALUE\n" );
#endif
			
			while( LIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) )
				p++;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p_header->value_len = p - p_header->value_ptr ;
			p2 = p - 1 ;
			while( LIKELY( (*p2) == ' ' || (*p2) == HTTP_RETURN ) )
			{
				p_header->value_len--;
				p2--;
			}
			p++;
			
			if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_CONTENT_LENGTH , sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 ) ) )
			{
				*(p_content_length) = strtol( p_header->value_ptr , NULL , 10 ) ;
			}
			else if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_TRANSFERENCODING)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_TRANSFERENCODING , sizeof(HTTP_HEADER_TRANSFERENCODING)-1 ) ) && UNLIKELY( p_header->value_len == sizeof(HTTP_HEADER_TRANSFERENCODING__CHUNKED)-1 && STRNICMP( p_header->value_ptr , == , HTTP_HEADER_TRANSFERENCODING__CHUNKED , sizeof(HTTP_HEADER_TRANSFERENCODING__CHUNKED)-1 ) ) )
			{
				*(p_transfer_encoding__chunked) = 1 ;
			}
			else if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_TRAILER)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_TRAILER , sizeof(HTTP_HEADER_TRAILER)-1 ) ) )
			{
				p_TRAILER->value_ptr = p_header->value_ptr ;
				p_TRAILER->value_len = p_header->value_len ;
			}
			e->headers.header_array_count++;
			
			if( *(p_transfer_encoding__chunked) == 1 && *(p_content_length) > 0 && UNLIKELY( p_header->name_len == p_TRAILER->value_len && STRNICMP( p_header->name_ptr , == , p_TRAILER->value_ptr , p_header->name_len ) ) )
			{
				b->process_ptr = fill_ptr ;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ;
				return 0;
			}
			
			if( UNLIKELY( e->headers.header_array_count+1 > e->headers.header_array_size ) )
			{
				int			new_header_array_size ;
				struct HttpHeader	*new_header_array ;
				
				new_header_array_size = e->headers.header_array_size * 2 ;
				new_header_array = (struct HttpHeader *)realloc( e->headers.header_array , sizeof(struct HttpHeader) * new_header_array_size ) ;
				if( new_header_array == NULL )
					return FASTERHTTP_ERROR_ALLOC;
				memset( new_header_array + e->headers.header_array_count-1 , 0x00 , sizeof(struct HttpHeader) * (new_header_array_size-e->headers.header_array_count+1) );
				e->headers.header_array_size = new_header_array_size ;
				e->headers.header_array = new_header_array ;
			}
			p_header = & (e->headers.header_array[e->headers.header_array_count]) ;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_BODY :
			
_GOTO_PARSESTEP_BODY :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_BODY\n" );
#endif
			
			if( LIKELY( fill_ptr - e->body >= *(p_content_length) ) )
			{
				b->process_ptr = fill_ptr ;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ;
				return 0;
			}
			else
			{
				b->process_ptr = fill_ptr ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			
		case FASTERHTTP_PARSESTEP_CHUNKED_SIZE :
			
_GOTO_PARSESTEP_CHUNKED_SIZE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_CHUNKED_SIZE\n" );
#endif
			
			for( ; UNLIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
			{
				if( LIKELY( '0' <= (*p) && (*p) <= '9' ) )
					e->chunked_length = e->chunked_length * 10 + (*p) - '0' ;
				else if( 'a' <= (*p) && (*p) <= 'f' )
					e->chunked_length = e->chunked_length * 10 + (*p) - 'a' + 10 ;
				else if( 'A' <= (*p) && (*p) <= 'F' )
					e->chunked_length = e->chunked_length * 10 + (*p) - 'A' + 10 ;
				e->chunked_length_length++;
			}
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			e->chunked_length_length++;
			p++;
			if( e->chunked_length == 0 )
			{
				if( p_TRAILER->value_ptr )
				{
					*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
					goto _GOTO_PARSESTEP_HEADER_NAME0;
				}
				
				b->process_ptr = fill_ptr ;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ;
				return 0;
			}
			
			e->chunked_body = p ;
			*(p_parse_step) = FASTERHTTP_PARSESTEP_CHUNKED_DATA ;
			
		case FASTERHTTP_PARSESTEP_CHUNKED_DATA :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_CHUNKED_DATA\n" );
#endif
			
			if( LIKELY( fill_ptr - e->chunked_body >= e->chunked_length + 2 ) )
			{
				memmove( e->chunked_body_end , e->chunked_body , e->chunked_length );
				p = e->chunked_body + e->chunked_length ;
				e->chunked_body_end = e->chunked_body_end + e->chunked_length ;
				if( (*p) == '\r' && *(p+1) == '\n' )
					p+=2;
				else if( (*p) == '\n' )
					p++;
				
				*(p_content_length) += e->chunked_length ;
				
				*(p_parse_step) = FASTERHTTP_PARSESTEP_CHUNKED_SIZE ;
				e->chunked_length = 0 ;
				e->chunked_length_length = 0 ;
				goto _GOTO_PARSESTEP_CHUNKED_SIZE;
			}
			else
			{
				b->process_ptr = fill_ptr ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			
		case FASTERHTTP_PARSESTEP_DONE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_DONE\n" );
#endif
			
			return 0;

		default :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> default\n" );
#endif
			return FASTERHTTP_ERROR_INTERNAL;
	}
}

int RequestHttp( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = SendHttpRequest( sock , ssl , e ) ;
	if( nret )
		return nret;
	
	nret = ReceiveHttpResponse( sock , ssl , e ) ;
	if( nret )
		return nret;
	
	return 0;
}

int ResponseHttp( SOCKET sock , SSL *ssl , struct HttpEnv *e , funcProcessHttpRequest *pfuncProcessHttpRequest , void *p )
{
	int		nret = 0 ;
	
	nret = ReceiveHttpRequest( sock , ssl , e ) ;
	if( nret )
		goto _GOTO_ON_ERROR;
	
	nret = pfuncProcessHttpRequest( e , p ) ;
	if( nret )
	{
		nret = HTTP_SERVICE_UNAVAILABLE ;
		goto _GOTO_ON_ERROR;
	}
	
	nret = SendHttpResponse( sock , ssl , e ) ;
	if( nret )
		return nret;
	
_GOTO_ON_ERROR :
	
	nret = FormatHttpResponseStartLine( abs(nret)/1000 , e ) ;
	if( nret )
		return nret;
	
	nret = StrcatHttpBuffer( GetHttpResponseBuffer(e) , HTTP_RETURN_NEWLINE ) ;
	if( nret )
		return nret;
	
	nret = SendHttpResponse( sock , ssl , e ) ;
	if( nret )
		return nret;
	
	return 0;
}

int SendHttpRequest( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	while(1)
	{
		nret = SendHttpBuffer( sock , ssl , e , &(e->request_buffer) ) ;
		if( nret == 0 )
			break;
		else if( nret != FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
			return nret;
	}
	
	return 0;
}

int ReceiveHttpResponse( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	while(1)
	{
		nret = ReceiveHttpBuffer( sock , ssl , e , &(e->response_buffer) ) ;
		if( nret )
			return nret;
		
		nret = ParseHttpBuffer( e , &(e->response_buffer) ) ;
		if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
			;
		else if( nret )
			return nret;
		else
			break;
	}
	
	return 0;
}

int ReceiveHttpRequest( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	while(1)
	{
		nret = ReceiveHttpBuffer( sock , ssl , e , &(e->request_buffer) ) ;
		if( nret )
			return nret;
		
		nret = ParseHttpBuffer( e , &(e->request_buffer) ) ;
		if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
			;
		else if( nret )
			return nret;
		else
			break;
	}
	
	return 0;
}

int FormatHttpResponseStartLine( int status_code , struct HttpEnv *e )
{
	struct HttpBuffer	*b = GetHttpResponseBuffer(e) ;
	
	int			nret = 0 ;
	
	switch( status_code )
	{
		case HTTP_CONTINUE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_CONTINUE_S " Continue" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_SWITCHING_PROTOCOL :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_SWITCHING_PROTOCOL_S " Switching Protocol" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_OK :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_OK_S " OK" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_CREATED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_CREATED_S " Created" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_ACCEPTED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_ACCEPTED_S " Accepted" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NON_AUTHORITATIVE_INFORMATION :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NON_AUTHORITATIVE_INFORMATION_S " Non Authoritative Information" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NO_CONTENT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NO_CONTENT_S " No Content" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_RESET_CONTENT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_RESET_CONTENT_S " Reset Content" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PARTIAL_CONTENT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PARTIAL_CONTENT_S " Partial Content" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_MULTIPLE_CHOICES :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_MULTIPLE_CHOICES_S " Multiple Choices" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_MOVED_PERMANNETLY :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_MOVED_PERMANNETLY_S " Moved Permannetly" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_FOUND :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_FOUND_S " Found" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_SEE_OTHER :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_SEE_OTHER_S " See Other" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_MODIFIED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_MODIFIED_S " Not Modified" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_USE_PROXY :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_USE_PROXY_S " Use Proxy" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_TEMPORARY_REDIRECT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_TEMPORARY_REDIRECT_S " Temporary Redirect" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_BAD_REQUEST :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_BAD_REQUEST_S " Bad Request" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_UNAUTHORIZED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_UNAUTHORIZED_S " Unauthorized" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PAYMENT_REQUIRED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PAYMENT_REQUIRED_S " Payment Required" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_FORBIDDEN :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_FORBIDDEN_S " Forbidden" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_FOUND :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_FOUND_S " Not Found" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_METHOD_NOT_ALLOWED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_METHOD_NOT_ALLOWED_S " Method Not Allowed" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_ACCEPTABLE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_ACCEPTABLE_S " Not Acceptable" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PROXY_AUTHENTICATION_REQUIRED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PROXY_AUTHENTICATION_REQUIRED_S " Proxy Authentication Required" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_REQUEST_TIMEOUT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_REQUEST_TIMEOUT_S " Request Timeout" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_CONFLICT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_CONFLICT_S " Conflict" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_GONE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_GONE_S " Gone" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_LENGTH_REQUIRED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_LENGTH_REQUIRED_S " Length Request" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PRECONDITION_FAILED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PRECONDITION_FAILED_S " Precondition Failed" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_REQUEST_ENTITY_TOO_LARGE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_REQUEST_ENTITY_TOO_LARGE_S " Request Entity Too Large" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_URI_TOO_LONG :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_URI_TOO_LONG_S " Request URI Too Long" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_UNSUPPORTED_MEDIA_TYPE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_UNSUPPORTED_MEDIA_TYPE_S " Unsupported Media Type" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_REQUESTED_RANGE_NOT_SATISFIABLE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_REQUESTED_RANGE_NOT_SATISFIABLE_S " Requested Range Not Satisfiable" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_EXPECTATION_FAILED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_EXPECTATION_FAILED_S " Expectation Failed" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_INTERNAL_SERVER_ERROR :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_INTERNAL_SERVER_ERROR_S " Internal Server Error" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_IMPLEMENTED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_IMPLEMENTED_S " Not Implemented" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_BAD_GATEWAY :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_BAD_GATEWAY_S " Bad Gateway" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_SERVICE_UNAVAILABLE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_SERVICE_UNAVAILABLE_S " Service Unavailable" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_GATEWAY_TIMEOUT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_GATEWAY_TIMEOUT_S " Gateway Timeout" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_HTTP_VERSION_NOT_SUPPORTED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_HTTP_VERSION_NOT_SUPPORTED_S " HTTP Version Not Supported" HTTP_RETURN_NEWLINE ) ;
			break;
		default :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_INTERNAL_SERVER_ERROR_S " Internal Server Error" HTTP_RETURN_NEWLINE ) ;
			break;
	}
	
	return nret;
}

int SendHttpResponse( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	while(1)
	{
		nret = SendHttpBuffer( sock , ssl , e , &(e->response_buffer) ) ;
		if( nret == 0 )
			break;
		else if( nret != FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
			return nret;
	}
	
	return 0;
}

int SendHttpRequestNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = SendHttpBuffer( sock , ssl , e , &(e->request_buffer) ) ;
	if( nret == FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
		return FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK; 
	else if( nret )
		return nret;
	else
		return 0;
}

int ReceiveHttpResponseNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ReceiveHttpBuffer( sock , ssl , e , &(e->response_buffer) ) ;
	if( nret )
		return nret;
	
	nret = ParseHttpBuffer( e , &(e->response_buffer) ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
		return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
	else if( nret )
		return nret;
	else
		return 0;
}

int ReceiveHttpResponseNonblock1( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ReceiveHttpBuffer1( sock , ssl , e , &(e->response_buffer) ) ;
	if( nret )
		return nret;
	
	nret = ParseHttpBuffer( e , &(e->response_buffer) ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
		return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
	else if( nret )
		return nret;
	else
		return 0;
}

int ReceiveHttpRequestNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ReceiveHttpBuffer( sock , ssl , e , &(e->request_buffer) ) ;
	if( nret )
		return nret;
	
	nret = ParseHttpBuffer( e , &(e->request_buffer) ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
		return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
	else if( nret )
		return nret;
	else
		return 0;
}

int ReceiveHttpRequestNonblock1( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ReceiveHttpBuffer1( sock , ssl , e , &(e->request_buffer) ) ;
	if( nret )
		return nret;
	
	nret = ParseHttpBuffer( e , &(e->request_buffer) ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
		return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
	else if( nret )
		return nret;
	else
		return 0;
}

int SendHttpResponseNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = SendHttpBuffer( sock , ssl , e , &(e->response_buffer) ) ;
	if( nret == FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK )
		return FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK; 
	else if( nret )
		return nret;
	else
		return 0;
}

int ParseHttpResponse( struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ParseHttpBuffer( e , &(e->response_buffer) ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
		return FASTERHTTP_ERROR_HTTP_TRUNCATION;
	else if( nret )
		return nret;
	
	return 0;
}

int ParseHttpRequest( struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ParseHttpBuffer( e , &(e->request_buffer) ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
		return FASTERHTTP_ERROR_HTTP_TRUNCATION;
	else if( nret )
		return nret;
	
	return 0;
}

char *GetHttpHeaderPtr_METHOD( struct HttpEnv *e , int *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.METHOD.value_len ;
	return e->headers.METHOD.value_ptr;
}

int GetHttpHeaderLen_METHOD( struct HttpEnv *e )
{
	return e->headers.METHOD.value_len;
}

char *GetHttpHeaderPtr_URI( struct HttpEnv *e , int *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.URI.value_len ;
	return e->headers.URI.value_ptr;
}

int GetHttpHeaderLen_URI( struct HttpEnv *e )
{
	return e->headers.URI.value_len;
}

char *GetHttpHeaderPtr_VERSION( struct HttpEnv *e , int *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.VERSION.value_len ;
	return e->headers.VERSION.value_ptr;
}

int GetHttpHeaderLen_VERSION( struct HttpEnv *e )
{
	return e->headers.VERSION.value_len;
}

char *GetHttpHeaderPtr_STATUSCODE( struct HttpEnv *e , int *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.STATUSCODE.value_len ;
	return e->headers.STATUSCODE.value_ptr;
}

int GetHttpHeaderLen_STATUSCODE( struct HttpEnv *e )
{
	return e->headers.STATUSCODE.value_len;
}

char *GetHttpHeaderPtr_REASONPHRASE( struct HttpEnv *e , int *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.REASONPHRASE.value_len ;
	return e->headers.REASONPHRASE.value_ptr;
}

int GetHttpHeaderLen_REASONPHRASE( struct HttpEnv *e )
{
	return e->headers.REASONPHRASE.value_len;
}

char *GetHttpHeaderPtr_TRAILER( struct HttpEnv *e , int *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.TRAILER.value_len ;
	return e->headers.TRAILER.value_ptr;
}

int GetHttpHeaderLen_TRAILER( struct HttpEnv *e )
{
	return e->headers.TRAILER.value_len;
}

struct HttpHeader *QueryHttpHeader( struct HttpEnv *e , char *name )
{
	int			i ;
	struct HttpHeader	*p_header = NULL ;
	int			name_len ;
	
	name_len = strlen(name) ;
	for( i = 0 , p_header = e->headers.header_array ; i < e->headers.header_array_size ; i++ , p_header++ )
	{
		if( p_header->name_ptr && p_header->name_len == name_len && MEMCMP( p_header->name_ptr , == , name , name_len ) )
		{
			return p_header;
		}
	}
	
	return NULL;
}

char *GetHttpHeaderPtr( struct HttpEnv *e , char *name , int *p_value_len )
{
	struct HttpHeader	*p_header = NULL ;
	
	p_header = QueryHttpHeader( e , name ) ;
	if( p_header == NULL )
		return NULL;
	
	if( p_value_len )
		(*p_value_len) = p_header->value_len ;
	return p_header->value_ptr;
}

int GetHttpHeaderLen( struct HttpEnv *e , char *name )
{
	struct HttpHeader	*p_header = NULL ;
	
	p_header = QueryHttpHeader( e , name ) ;
	if( p_header == NULL )
		return 0;
	
	return p_header->value_len;
}

int GetHttpHeaderCount( struct HttpEnv *e )
{
	return e->headers.header_array_count;
}

struct HttpHeader *TravelHttpHeaderPtr( struct HttpEnv *e , struct HttpHeader *p_header )
{
	if( p_header == NULL )
		p_header = e->headers.header_array ;
	else
		p_header++;
	
	for( ; p_header < e->headers.header_array + e->headers.header_array_size ; p_header++ )
	{
		if( p_header->name_ptr )
		{
			return p_header;
		}
	}
	
	return NULL;
}

char *GetHttpHeaderNamePtr( struct HttpHeader *p_header , int *p_name_len )
{
	if( p_header == NULL )
		return NULL;
	
	if( p_name_len )
		(*p_name_len) = p_header->name_len ;
	return p_header->name_ptr;
}

int GetHttpHeaderNameLen( struct HttpHeader *p_header )
{
	return p_header->name_len;
}

char *GetHttpHeaderValuePtr( struct HttpHeader *p_header , int *p_value_len )
{
	if( p_header == NULL )
		return NULL;
	
	if( p_value_len )
		(*p_value_len) = p_header->value_len ;
	return p_header->value_ptr;
}

int GetHttpHeaderValueLen( struct HttpHeader *p_header )
{
	return p_header->value_len;
}

char *GetHttpBodyPtr( struct HttpEnv *e , int *p_body_len )
{
	if( p_body_len )
		(*p_body_len) = e->headers.content_length ;
	
	if( e->headers.content_length > 0 )
	{
		return e->body;
	}
	else
	{
		return NULL;
	}
}

int GetHttpBodyLen( struct HttpEnv *e )
{
	return e->headers.content_length;
}
