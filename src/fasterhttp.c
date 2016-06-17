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
	
	struct HttpHeader	CONTENT_LENGTH ;
	int			content_length ;
	
	struct HttpHeader	TRANSFERENCODING ;
	struct HttpHeader	TRAILER ;
	int			transfer_encoding__chunked ;
	
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
	/* struct HttpEnv */
	
	SetHttpTimeout( e , FASTERHTTP_TIMEOUT_DEFAULT );
	
	if( e->request_buffer.ref_flag == 0 && e->request_buffer.buf_size > FASTERHTTP_REQUEST_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->request_buffer) , FASTERHTTP_REQUEST_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->request_buffer) );
	if( e->response_buffer.ref_flag == 0 && e->response_buffer.buf_size > FASTERHTTP_RESPONSE_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->response_buffer) , FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->response_buffer) );
	
	e->parse_step = FASTERHTTP_PARSESTEP_BEGIN ;
	
	e->body = NULL ;
	
	e->chunked_body = NULL ;
	e->chunked_length = 0 ;
	e->chunked_length_length = 0 ;
	
	/* struct HttpHeaders */
	
	e->headers.content_length = 0 ;
	
	e->headers.transfer_encoding__chunked = 0 ;
	
	if( e->headers.header_array_size > FASTERHTTP_HEADER_ARRAYSIZE_MAX )
	{
		struct HttpHeader	*p = NULL ;
		p = (struct HttpHeader *)realloc( e->headers.header_array , sizeof(struct HttpHeader) * FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT ) ;
		if( p )
		{
			memset( p , 0x00 , sizeof(struct HttpHeader) * e->headers.header_array_size );
			e->headers.header_array = p ;
			e->headers.header_array_size = FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT ;
			e->headers.header_array_count = 0 ;
		}
	}
	CleanHttpHeader( &(e->headers) );
	
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

long GetHttpBufferLength( struct HttpBuffer *b )
{
	return b->fill_ptr-b->base;
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
	long		len ;
	
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

#define DEBUG_TIMEVAL	0

#if DEBUG_TIMEVAL
#define TIMEVAL_COUNT	40
static struct timeval	tv1[TIMEVAL_COUNT] = { {0,0} } , tv2[TIMEVAL_COUNT] = { {0,0} } , tvdiff[TIMEVAL_COUNT] = { {0,0} } ;
static void SetTimeval( int index )
{
	gettimeofday( tv1+index , NULL );
	
	return;
}
	
static void SetTimevalAndDiff( int index )
{
	gettimeofday( tv2+index , NULL );
	
	tvdiff[index].tv_sec += ( tv2[index].tv_sec - tv1[index].tv_sec ) ;
	tvdiff[index].tv_usec += ( tv2[index].tv_usec - tv1[index].tv_usec ) ;
	while( tvdiff[index].tv_usec < 0 )
	{
		tvdiff[index].tv_usec += 1000000 ;
		tvdiff[index].tv_sec--;
	}
	while( tvdiff[index].tv_usec > 1000000 )
	{
		tvdiff[index].tv_usec -= 1000000 ;
		tvdiff[index].tv_sec++;
	}
	
	return;
}
#endif

void _FASTERHTTP_PrintTimevalDiff()
{
#if DEBUG_TIMEVAL
	int	index ;
	
	for( index = 0 ; index < sizeof(tvdiff)/sizeof(tvdiff[0]) ; index++ )
	{
		printf( "index[%d] tvdiff[%06ld.%06ld]\n" , index , tvdiff[index].tv_sec , tvdiff[index].tv_usec );
	}
#endif
	
	return;
}

int ParseHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b )
{
	register char		*p = b->process_ptr ;
	char			*p2 = NULL ;
	
	struct HttpHeader	*p_header = & (e->headers.header_array[e->headers.header_array_count]) ;
	char			*fill_ptr = b->fill_ptr ;
	
#if DEBUG_TIMEVAL
	SetTimeval( 0 );
#endif
	
	if( UNLIKELY( e->parse_step == FASTERHTTP_PARSESTEP_BEGIN ) )
	{
		if( b == &(e->request_buffer) )
		{
			e->parse_step = FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_METHOD0 ;
		}
		else if( b == &(e->response_buffer) )
		{
			e->parse_step = FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_VERSION0 ;
		}
		else
		{
			return FASTERHTTP_ERROR_PARAMTER;
		}
	}
	
#if DEBUG_TIMEVAL
	SetTimevalAndDiff( 0 );
#endif
	
	switch( e->parse_step )
	{
		case FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_METHOD0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 1 );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.METHOD.name_ptr = HTTP_HEADER_METHOD ;
			e->headers.METHOD.name_len = sizeof(HTTP_HEADER_METHOD)-1 ;
			e->headers.METHOD.value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 1 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_METHOD ;
			
		case FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_METHOD :
			
#if DEBUG_TIMEVAL
			SetTimeval( 2 );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.METHOD.value_len = p - e->headers.METHOD.value_ptr ;
			p++;
			
			p2 = e->headers.METHOD.value_ptr ;
			if( e->headers.METHOD.value_len == 3 )
			{
				if( *(p2) == HTTP_METHOD_GET[0] && *(p2+1) == HTTP_METHOD_GET[1] && *(p2+2) == HTTP_METHOD_GET[2] )
					;
				else if( *(p2) == HTTP_METHOD_PUT[0] && *(p2+1) == HTTP_METHOD_PUT[1] && *(p2+2) == HTTP_METHOD_PUT[2] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
			}
			else if( e->headers.METHOD.value_len == 4 )
			{
				if( *(p2) == HTTP_METHOD_POST[0] && *(p2+1) == HTTP_METHOD_POST[1] && *(p2+2) == HTTP_METHOD_POST[2]
					&& *(p2+3) == HTTP_METHOD_POST[3] )
					;
				else if( *(p2) == HTTP_METHOD_HEAD[0] && *(p2+1) == HTTP_METHOD_HEAD[1] && *(p2+2) == HTTP_METHOD_HEAD[2]
					&& *(p2+3) == HTTP_METHOD_HEAD[3] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
			}
			else if( e->headers.METHOD.value_len == 5 )
			{
				if( *(p2) == HTTP_METHOD_TRACE[0] && *(p2+1) == HTTP_METHOD_TRACE[1] && *(p2+2) == HTTP_METHOD_TRACE[2]
					&& *(p2+3) == HTTP_METHOD_TRACE[3] && *(p2+4) == HTTP_METHOD_TRACE[4] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
			}
			else if( e->headers.METHOD.value_len == 6 )
			{
				if( *(p2) == HTTP_METHOD_DELETE[0] && *(p2+1) == HTTP_METHOD_DELETE[1] && *(p2+2) == HTTP_METHOD_DELETE[2]
					&& *(p2+3) == HTTP_METHOD_DELETE[3] && *(p2+4) == HTTP_METHOD_DELETE[4] && *(p2+5) == HTTP_METHOD_DELETE[5] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
			}
			else if( e->headers.METHOD.value_len == 7 )
			{
				if( *(p2) == HTTP_METHOD_OPTIONS[0] && *(p2+1) == HTTP_METHOD_OPTIONS[1] && *(p2+2) == HTTP_METHOD_OPTIONS[2]
					&& *(p2+3) == HTTP_METHOD_OPTIONS[3] && *(p2+4) == HTTP_METHOD_OPTIONS[4] && *(p2+5) == HTTP_METHOD_OPTIONS[5]
					&& *(p2+6) == HTTP_METHOD_OPTIONS[6] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
			}
			else
			{
				return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
			}
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 2 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_URI0 ;
			
		case FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_URI0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 3 );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.URI.name_ptr = HTTP_HEADER_URI ;
			e->headers.URI.name_len = sizeof(HTTP_HEADER_URI)-1 ;
			e->headers.URI.value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 3 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_URI ;
			
		case FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_URI :
			
#if DEBUG_TIMEVAL
			SetTimeval( 4 );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.URI.value_len = p - e->headers.URI.value_ptr ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 4 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_VERSION0 ;
			
		case FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_VERSION0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 5 );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.VERSION.name_ptr = HTTP_HEADER_VERSION ;
			e->headers.VERSION.name_len = sizeof(HTTP_HEADER_VERSION)-1 ;
			e->headers.VERSION.value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 5 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_VERSION ;
			
		case FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_VERSION :
			
#if DEBUG_TIMEVAL
			SetTimeval( 6 );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			e->headers.VERSION.value_len = p - e->headers.VERSION.value_ptr ;
			for( ; LIKELY( (*p) != HTTP_NEWLINE ) ; p++ )
				;
			if( LIKELY( *(p-1) == HTTP_RETURN ) )
				e->headers.VERSION.value_len--;
			p++;
			
			p2 = e->headers.VERSION.value_ptr ;
			if( LIKELY( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
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
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 6 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_VERSION0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 7 );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.VERSION.name_ptr = HTTP_HEADER_VERSION ;
			e->headers.VERSION.name_len = sizeof(HTTP_HEADER_VERSION)-1 ;
			e->headers.VERSION.value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 7 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_VERSION ;
			
		case FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_VERSION :
			
#if DEBUG_TIMEVAL
			SetTimeval( 8 );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.VERSION.value_len = p - e->headers.VERSION.value_ptr ;
			p++;
			
			p2 = e->headers.VERSION.value_ptr ;
			if( LIKELY( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
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
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 8 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_STATUSCODE0 ;
			
		case FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_STATUSCODE0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 9 );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.STATUSCODE.name_ptr = HTTP_HEADER_STATUSCODE ;
			e->headers.STATUSCODE.name_len = sizeof(HTTP_HEADER_STATUSCODE)-1 ;
			e->headers.STATUSCODE.value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 9 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_STATUSCODE ;
			
		case FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_STATUSCODE :
			
#if DEBUG_TIMEVAL
			SetTimeval( 10 );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.STATUSCODE.value_len = p - e->headers.STATUSCODE.value_ptr ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 10 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_REASONPHRASE0 ;
			
		case FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_REASONPHRASE0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 11 );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.REASONPHRASE.name_ptr = HTTP_HEADER_REASONPHRASE ;
			e->headers.REASONPHRASE.name_len = sizeof(HTTP_HEADER_REASONPHRASE)-1 ;
			e->headers.REASONPHRASE.value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 11 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_REASONPHRASE ;
			
		case FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_REASONPHRASE :
			
#if DEBUG_TIMEVAL
			SetTimeval( 12 );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			e->headers.REASONPHRASE.value_len = p - e->headers.REASONPHRASE.value_ptr ;
			for( ; (*p) != HTTP_NEWLINE ; p++ )
				;
			if( LIKELY( *(p-1) == HTTP_RETURN ) )
				e->headers.REASONPHRASE.value_len--;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 12 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_HEADER_NAME0 :
			
_GOTO_PARSESTEP_HEADER_NAME0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 13 );
#endif
			
			/*
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			*/
			if( LIKELY( (*p) == HTTP_RETURN ) )
			{
				if( UNLIKELY( p+1 >= fill_ptr ) )
				{
					return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
				}
				else if( LIKELY( *(p+1) == HTTP_NEWLINE ) )
				{
					p += 2 ;
					if( e->headers.content_length > 0 )
					{
						e->body = p ;
						e->parse_step = FASTERHTTP_PARSESTEP_BODY ;
						goto _GOTO_PARSESTEP_BODY;
					}
					else if( e->headers.transfer_encoding__chunked == 1 )
					{
						e->body = p ;
						e->parse_step = FASTERHTTP_PARSESTEP_CHUNKED_SIZE ;
						goto _GOTO_PARSESTEP_CHUNKED_SIZE;
					}
					else
					{
						b->process_ptr = p ;
						e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
						return 0;
					}
				}
			}
			else if( LIKELY( (*p) == HTTP_NEWLINE ) )
			{
				if( e->headers.content_length > 0 )
				{
					e->body = p + 1 ;
					e->parse_step = FASTERHTTP_PARSESTEP_BODY ;
					goto _GOTO_PARSESTEP_BODY;
				}
				else
				{
					b->process_ptr = p + 1 ;
					e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
					return 0;
				}
			}
			p_header->name_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 13 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_NAME ;
			
		case FASTERHTTP_PARSESTEP_HEADER_NAME :
			
#if DEBUG_TIMEVAL
			SetTimeval( 14 );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != ':' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			p_header->name_len = p - p_header->name_ptr ;
			
			for( ; LIKELY( (*p) != ':' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			else if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 14 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_VALUE0 ;
			
		case FASTERHTTP_PARSESTEP_HEADER_VALUE0 :
			
#if DEBUG_TIMEVAL
			SetTimeval( 15 );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			p_header->value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 15 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_VALUE ;
			
		case FASTERHTTP_PARSESTEP_HEADER_VALUE :
			
#if DEBUG_TIMEVAL
			SetTimeval( 16 );
#endif
			
			for( ; LIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p_header->value_len = p - p_header->value_ptr ;
			for( p2 = p - 1 ; UNLIKELY( (*p2) == ' ' || (*p2) == HTTP_RETURN ) ; p2-- )
				p_header->value_len--;
			p++;
			
			if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_CONTENT_LENGTH , sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 ) ) )
				e->headers.content_length = strtol( p_header->value_ptr , NULL , 10 ) ;
			if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_TRANSFERENCODING)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_TRANSFERENCODING , sizeof(HTTP_HEADER_TRANSFERENCODING)-1 ) ) && UNLIKELY( p_header->value_len == sizeof(HTTP_HEADER_TRANSFERENCODING__CHUNKED)-1 && STRNICMP( p_header->value_ptr , == , HTTP_HEADER_TRANSFERENCODING__CHUNKED , sizeof(HTTP_HEADER_TRANSFERENCODING__CHUNKED)-1 ) ) )
			{
				memcpy( &(e->headers.TRANSFERENCODING) , p_header , sizeof(struct HttpHeader) );
				e->headers.transfer_encoding__chunked = 1 ;
			}
			else if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_TRANSFERENCODING)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_TRANSFERENCODING , sizeof(HTTP_HEADER_TRANSFERENCODING)-1 ) ) )
			{
				memcpy( &(e->headers.TRAILER) , p_header , sizeof(struct HttpHeader) );
			}
			else
			{
				e->headers.header_array_count++;
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
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 16 );
#endif
	
			if( e->headers.transfer_encoding__chunked == 1 && e->headers.content_length > 0 && UNLIKELY( p_header->name_len == e->headers.TRAILER.value_len && STRNICMP( p_header->name_ptr , == , e->headers.TRAILER.value_ptr , p_header->name_len ) ) )
			{
				b->process_ptr = fill_ptr ;
				e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
				return 0;
			}
			
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_BODY :
			
_GOTO_PARSESTEP_BODY :
			
			if( LIKELY( fill_ptr - e->body >= e->headers.content_length ) )
			{
				b->process_ptr = fill_ptr ;
				e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
				return 0;
			}
			else
			{
				b->process_ptr = fill_ptr ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			
		case FASTERHTTP_PARSESTEP_CHUNKED_SIZE :
			
_GOTO_PARSESTEP_CHUNKED_SIZE :
			
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
				if( e->headers.TRAILER.name_ptr )
				{
					e->parse_step = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
					goto _GOTO_PARSESTEP_HEADER_NAME0;
				}
				
				b->process_ptr = fill_ptr ;
				e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
				return 0;
			}
			
			e->chunked_body = p ;
			e->parse_step = FASTERHTTP_PARSESTEP_CHUNKED_DATA ;
			
		case FASTERHTTP_PARSESTEP_CHUNKED_DATA :
			
			if( LIKELY( fill_ptr - e->chunked_body >= e->chunked_length ) )
			{
				int		len = ( e->headers.content_length == 0 ? 0 : 2 ) ;
				memmove( e->chunked_body - e->chunked_length_length - len , e->chunked_body , fill_ptr - e->chunked_body );
				fill_ptr -= e->chunked_length_length - len ;
				p = e->chunked_body - e->chunked_length_length + e->chunked_length + 2 ;
				
				e->headers.content_length += e->chunked_length ;
				
				e->parse_step = FASTERHTTP_PARSESTEP_CHUNKED_SIZE ;
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
			
			return 0;

		default :
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
		return nret;
	
	nret = pfuncProcessHttpRequest( e , p ) ;
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

char *GetHttpHeaderPtr_TRANSFERENCODING( struct HttpEnv *e , int *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.TRANSFERENCODING.value_len ;
	return e->headers.TRANSFERENCODING.value_ptr;
}

int GetHttpHeaderLen_TRANSFERENCODING( struct HttpEnv *e )
{
	return e->headers.TRANSFERENCODING.value_len;
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
