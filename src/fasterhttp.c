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
	
	struct HttpHeader	*header_array ;
	int			header_array_size ;
	int			header_array_count ;
	
	struct HttpHeader	header ;
} ;

struct HttpEnv
{
	struct timeval		timeout ;
	
	struct HttpBuffer	request_buffer ;
	struct HttpBuffer	response_buffer ;
	
	int			parse_step ;
	struct HttpHeaders	headers ;
	char			*body ;
} ;

static long natol( char *str , long len )
{
	char	buf[ 40 + 1 ] ;
	
	if( len > 40 )
		len = 40 ;
	strncpy( buf , str , len );
	buf[len] = '\0' ;
	
	return atol(buf);
}

static unsigned long HashCalc( unsigned char *name , int len )
{
	unsigned long	val = 5381 ;
	int		i ;
	
	for( i = 0 ; i < len && (*name) ; i++ , name++ )
	{
		val = ( ( val << 5 ) + val ) + (*name) ;
	}
	
	return val;
}

static int PushHttpHeader( struct HttpHeaders *p_headers , struct HttpHeader *p_header )
{
	unsigned long		val ;
	int			i ;
	struct HttpHeader	*p = NULL ;
	int			nret = 0 ;
	
	if( p_headers->header_array_count+1 > p_headers->header_array_size/2 )
	{
		struct HttpHeaders	new_headers ;
		int			i ;
		
		new_headers.header_array_size = p_headers->header_array_size * 2 ;
		new_headers.header_array = (struct HttpHeader *)malloc( sizeof(struct HttpHeader) * new_headers.header_array_size ) ;
		if( new_headers.header_array == NULL )
			return -1;
		memset( new_headers.header_array , 0x00 , sizeof(struct HttpHeader) * new_headers.header_array_size );
		
		for( i = 0 ; i < p_headers->header_array_count ; i++ )
		{
			if( p_headers->header_array[i].name_ptr )
			{
				nret = PushHttpHeader( & new_headers , p_headers->header_array+i ) ;
				if( nret )
				{
					free( new_headers.header_array );
					return -2;
				}
			}
		}
		
		p_headers->header_array = new_headers.header_array ;
		p_headers->header_array_size = new_headers.header_array_size ;
	}
	
	val = HashCalc( (unsigned char *)(p_header->name_ptr) , p_header->name_len ) % p_headers->header_array_size ;
	for( i = 0 , p = p_headers->header_array+val ; i < p_headers->header_array_size ; i++ , p++ )
	{
		if( p > p_headers->header_array + p_headers->header_array_size )
			p = p_headers->header_array;
		
		if( p->name_ptr == NULL )
		{
			memcpy( p , p_header , sizeof(struct HttpHeader) );
			p_headers->header_array_count++;
			break;
		}
	}
	if( i >= p_headers->header_array_size )
		return FASTERHTTP_ERROR_INTERNAL;
	
	return 0;
}

static struct HttpHeader *QueryHttpHeader( struct HttpHeaders *p_headers , char *name )
{
	long			name_len ;
	unsigned long		val ;
	int			i ;
	struct HttpHeader	*p = NULL ;
	
	name_len = strlen(name) ;
	
	val = HashCalc( (unsigned char *)name , strlen(name) ) % p_headers->header_array_size ;
	for( i = 0 , p = p_headers->header_array+val ; i < p_headers->header_array_size ; i++ , p++ )
	{
		if( p > p_headers->header_array + p_headers->header_array_size )
			p = p_headers->header_array;
		
		if( p->name_ptr && p->name_len == name_len && MEMCMP( p->name_ptr , == , name , name_len ) )
		{
			return p;
		}
	}
	
	return NULL;
}

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
	
	e->parse_step = FASTERHTTP_PARSESTEP_BEGIN ;
	
	return e;
}

void ResetHttpEnv( struct HttpEnv *e )
{
	SetHttpTimeout( e , FASTERHTTP_TIMEOUT_DEFAULT );
	
	if( e->request_buffer.ref_flag == 0 && e->request_buffer.buf_size > FASTERHTTP_REQUEST_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->request_buffer) , FASTERHTTP_REQUEST_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->request_buffer) );
	if( e->response_buffer.ref_flag == 0 && e->response_buffer.buf_size > FASTERHTTP_RESPONSE_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->response_buffer) , FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->response_buffer) );
	
	e->parse_step = FASTERHTTP_PARSESTEP_BEGIN ;
	
	e->headers.content_length = 0 ;
	
	if( e->headers.header_array_size > FASTERHTTP_HEADER_ARRAYSIZE_MAX )
	{
		struct HttpHeader	*p = NULL ;
		p = (struct HttpHeader *)malloc( sizeof(struct HttpHeader) * FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT ) ;
		if( p )
		{
			memset( p , 0x00 , sizeof(struct HttpHeader) * e->headers.header_array_size );
			e->headers.header_array = p ;
			e->headers.header_array_size = FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT ;
		}
	}
	CleanHttpHeader( &(e->headers) );
	
	e->body = NULL ;
	
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

#if 0
#define HEADERFIRSTLINE_TOKEN(_header_,_name_) \
	(_header_).name_ptr = (_name_) ; \
	(_header_).name_len = strlen(_name_) ; \
	for( ; (*p) == ' ' ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID; \
	(_header_).value_ptr = p ; \
	for( ; (*p) != ' ' && (*p) != HTTP_NEWLINE ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID; \
	(_header_).value_len = p - (_header_).value_ptr ; \
	if( (_header_).value_ptr[(_header_).value_len-1] == HTTP_RETURN ) \
		(_header_).value_len--; \

#define HEADERFIRSTLINE_TOKEN_LAST(_header_,_name_) \
	(_header_).name_ptr = (_name_) ; \
	(_header_).name_len = strlen(_name_) ; \
	for( ; (*p) == ' ' ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID; \
	(_header_).value_ptr = p ; \
	for( ; (*p) != ' ' && (*p) != HTTP_NEWLINE ; p++ ) \
		; \
	(_header_).value_len = p - (_header_).value_ptr ; \
	if( (_header_).value_ptr[(_header_).value_len-1] == HTTP_RETURN ) \
		(_header_).value_len--; \

/*
GET / HTTP/1.1
*/

static int ParseHttpRequestHeaderFirstLine( struct HttpEnv *e , struct HttpBuffer *b )
{
	char	*line_end = NULL ;
	
	line_end = strchr( b->process_ptr , HTTP_NEWLINE ) ;
	if( line_end )
	{
		char	*p = b->process_ptr ;
		
		/* Parse METHOD */
		HEADERFIRSTLINE_TOKEN( e->headers.METHOD , "METHOD" )
		if( e->headers.METHOD.value_len == 3 )
		{
			if( MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_GET  , sizeof(HTTP_METHOD_GET)-1 ) )
				;
			else if( MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_PUT  , sizeof(HTTP_METHOD_PUT)-1 ) )
				;
			else
				return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
		}
		else if( e->headers.METHOD.value_len == 4 )
		{
			if( MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_POST  , sizeof(HTTP_METHOD_POST)-1 ) )
				;
			else if( MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_HEAD  , sizeof(HTTP_METHOD_HEAD)-1 ) )
				;
			else
				return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
		}
		else if( e->headers.METHOD.value_len == 5 )
		{
			if( MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_TRACE  , sizeof(HTTP_METHOD_TRACE)-1 ) )
				;
			else
				return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
		}
		else if( e->headers.METHOD.value_len == 6 )
		{
			if( MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_DELETE  , sizeof(HTTP_METHOD_DELETE)-1 ) )
				;
			else
				return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
		}
		else if( e->headers.METHOD.value_len == 7 )
		{
			if( MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_OPTIONS  , sizeof(HTTP_METHOD_OPTIONS)-1 ) )
				;
			else
				return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
		}
		else
		{
			return FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED;
		}
		
		/* Parse URI */
		HEADERFIRSTLINE_TOKEN( e->headers.URI , "URI" )
		
		/* Parse VERSION */
		HEADERFIRSTLINE_TOKEN_LAST( e->headers.VERSION , "VERSION" )
		if( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 )
		{
			if( MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_0  , sizeof(HTTP_VERSION_1_0)-1 ) )
				;
			else if( MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_1  , sizeof(HTTP_VERSION_1_1)-1 ) )
				;
			else
				return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
		}
		else
		{
			return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
		}
		
		b->process_ptr = p + 1 ;
		
		return 0;
	}
	
	return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
}

/*
HTTP/1.1 200 OK
*/

static int ParseHttpResponseHeaderFirstLine( struct HttpEnv *e , struct HttpBuffer *b )
{
	char	*line_end = NULL ;
	char	*p = NULL ;
	
	line_end = strchr( b->process_ptr , HTTP_NEWLINE ) ;
	if( line_end )
	{
		p = b->process_ptr ;
		
		/* Parse VERSION */
		HEADERFIRSTLINE_TOKEN( e->headers.VERSION , "VERSION" )
		if( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 && MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_0  , sizeof(HTTP_VERSION_1_0)-1 ) )
			;
		else if( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_1)-1 && MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_1  , sizeof(HTTP_VERSION_1_1)-1 ) )
			;
		else
			return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
		
		/* Parse STATUSCODE */
		HEADERFIRSTLINE_TOKEN( e->headers.STATUSCODE , "STATUSCODE" )
		
		/* Parse REASONPHRASE */
		HEADERFIRSTLINE_TOKEN_LAST( e->headers.REASONPHRASE , "REASONPHRASE" )
		
		b->process_ptr = p + 1 ;
		
		return 0;
	}
	
	return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
}

#define HEADERNAME_TOKEN(_p_,_header_) \
	for( ; (*p) == ' ' ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADER_INVALID; \
	(_header_).name_ptr = p ; \
	for( ; (*p) != ' ' && (*p) != ':' && (*p) != HTTP_NEWLINE ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADER_INVALID; \
	(_header_).name_len = p - (_header_).name_ptr ; \
	for( ; (*p) != ':' && (*p) != HTTP_NEWLINE ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADER_INVALID; \
	p++; \

#define HEADERVALUE_TOKEN(_p_,_header_) \
	for( ; (*p) == ' ' ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADER_INVALID; \
	(_header_).value_ptr = p ; \
	for( p = line_end-1 ; (*p) == ' ' || (*p) == HTTP_RETURN ; p-- ) \
		; \
	(_header_).value_len = p+1 - (_header_).value_ptr ; \

/*
Host: www.baidu.com
User-Agent: Mozilla/5.0 (Windows NT 5.1; rv:45.0) Gecko/20100101 Firefox/45.0
Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3
Accept-Encoding: gzip, deflate, br
Cookie: BAIDUID=0E27B789D33BF3C43C6022BD0182CF8D:SL=0:NR=10:FG=1; BIDUPSID=EE65333C3C1B7FB4807F6DC5DE576979; PSTM=1462883721; BD_UPN=13314152; ispeed_lsm=2; MCITY=-179%3A; BDUSS=t4TW1VRFNsMm91bGtTcUFHbVFqfnhiVFVYd2ZKZFc2c0dGaG12VmhZckZJbmxYQVFBQUFBJCQAAAAAAAAAAAEAAADIZsc0Y2FsdmlubGljaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMWVUVfFlVFXSG; pgv_pvi=56303616; BD_HOME=1; H_PS_PSSID=19290_1436_18240_20076_17001_15790_12201_20254; sug=3; sugstore=0; ORIGIN=2; bdime=0; __bsi=13900513390515515511_00_0_I_R_33_0303_C02F_N_I_I_0
Connection: keep-alive
Cache-Control: max-age=0
*/

/*
Server: bfe/1.0.8.14
Date: Tue, 07 Jun 2016 11:45:33 GMT
Content-Type: text/html;charset=utf-8
Transfer-Encoding: chunked
Connection: keep-alive
Cache-Control: private
Expires: Tue, 07 Jun 2016 11:45:33 GMT
Content-Encoding: gzip
X-UA-Compatible: IE=Edge,chrome=1
BDPAGETYPE: 2
BDQID: 0xf7f86d0a0009c7c3
BDUSERID: 885483208
Set-Cookie: BDSVRTM=120; path=/
BD_HOME=1; path=/
H_PS_PSSID=19290_1436_18240_20076_17001_15790_12201_20254; path=/; domain=.baidu.com
__bsi=13514842734789003845_00_0_I_R_122_0303_C02F_N_I_I_0; expires=Tue, 07-Jun-16 11:45:38 GMT; domain=www.baidu.com; path=/
*/

static int ParseHttpHeader( struct HttpEnv *e , struct HttpBuffer *b )
{
	char			*line_end = NULL ;
	char			*p = NULL ;
	struct HttpHeader	header ;
	int			nret = 0 ;
	
	line_end = strchr( b->process_ptr , HTTP_NEWLINE ) ;
	while( line_end )
	{
		if( *(b->process_ptr) == HTTP_RETURN && b->process_ptr+1 == line_end )
		{
			b->process_ptr+=2;
			return 0;
		}
		if( line_end == b->process_ptr )
		{
			b->process_ptr++;
			return 0;
		}
		p = b->process_ptr ;
		
		/* Parse header 'NAME' */
		HEADERNAME_TOKEN( p , header )
		
		/* Parse header 'VALUE' */
		HEADERVALUE_TOKEN( p , header )
		
		nret = PushHttpHeader( &(e->headers) , &header ) ;
		if( nret )
			return nret;
		
		if( header.name_len == sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 && STRNICMP( header.name_ptr , == , HTTP_HEADER_CONTENT_LENGTH , sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 ) )
			e->headers.content_length = natol( header.value_ptr , header.value_len ) ;
		
		b->process_ptr = line_end + 1 ;
		
		line_end = strchr( b->process_ptr , HTTP_NEWLINE ) ;
	}
	
	return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
}

static int ParseHttpBody( struct HttpEnv *e , struct HttpBuffer *b )
{
	if( b->fill_ptr - e->body >= e->headers.content_length )
		return 0;
	else
		return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
}

int ParseHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b )
{
	int		nret = 0 ;
	
	while(1)
	{
		switch( e->parse_step )
		{
			case FASTERHTTP_PARSESTEP_FIRSTLINE :
				
				if( b == &(e->request_buffer) )
				{
					nret = ParseHttpRequestHeaderFirstLine( e , b ) ;
					if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
						break;
					else if( nret )
						return nret;
					else
						e->parse_step = FASTERHTTP_PARSESTEP_HEADER ;
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
			
			case FASTERHTTP_PARSESTEP_HEADER :
				
				nret = ParseHttpHeader( e , b ) ;
				if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
					break;
				else if( nret )
					return nret;
				else if( e->headers.content_length > 0 )
					e->body = b->process_ptr , e->parse_step = FASTERHTTP_PARSESTEP_BODY ;
				else
					e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
				
				break;
			
			case FASTERHTTP_PARSESTEP_BODY :
				
				nret = ParseHttpBody( e , b ) ;
				if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
					break;
				else if( nret )
					return nret;
				else
					e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
				
				break;
			
			case FASTERHTTP_PARSESTEP_DONE :
				
				return 0;
			
			default :
				return FASTERHTTP_ERROR_INTERNAL;
		}
		if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
			break;
	}
	
	return nret;
}
#endif

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
	register char	*p = b->process_ptr ;
	char		*p2 = NULL ;
	int		nret = 0 ;
	
#if DEBUG_TIMEVAL
	SetTimeval( 0 );
#endif
	
	if( unlikely( e->parse_step == FASTERHTTP_PARSESTEP_BEGIN ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; likely( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; likely( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; likely( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			e->headers.VERSION.value_len = p - e->headers.VERSION.value_ptr ;
			for( ; likely( (*p) != HTTP_NEWLINE ) ; p++ )
				;
			if( likely( *(p-1) == HTTP_RETURN ) )
				e->headers.VERSION.value_len--;
			p++;
			
			p2 = e->headers.VERSION.value_ptr ;
			if( likely( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
			{
				if( likely( *(p2) == HTTP_VERSION_1_0[0] && *(p2+1) == HTTP_VERSION_1_0[1] && *(p2+2) == HTTP_VERSION_1_0[2]
					&& *(p2+3) == HTTP_VERSION_1_0[3] && *(p2+4) == HTTP_VERSION_1_0[4] && *(p2+5) == HTTP_VERSION_1_0[5]
					&& *(p2+6) == HTTP_VERSION_1_0[6] ) )
				{
					if( unlikely( *(p2+7) == HTTP_VERSION_1_0[7] ) )
						;
					else if( likely( *(p2+7) == HTTP_VERSION_1_1[7] ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; likely( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID;
			e->headers.VERSION.value_len = p - e->headers.VERSION.value_ptr ;
			p++;
			
			p2 = e->headers.VERSION.value_ptr ;
			if( likely( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
			{
				if( likely( *(p2) == HTTP_VERSION_1_0[0] && *(p2+1) == HTTP_VERSION_1_0[1] && *(p2+2) == HTTP_VERSION_1_0[2]
					&& *(p2+3) == HTTP_VERSION_1_0[3] && *(p2+4) == HTTP_VERSION_1_0[4] && *(p2+5) == HTTP_VERSION_1_0[5]
					&& *(p2+6) == HTTP_VERSION_1_0[6] ) )
				{
					if( unlikely( *(p2+7) == HTTP_VERSION_1_0[7] ) )
						;
					else if( likely( *(p2+7) == HTTP_VERSION_1_1[7] ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; likely( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			
			for( ; likely( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			e->headers.REASONPHRASE.value_len = p - e->headers.REASONPHRASE.value_ptr ;
			for( ; (*p) != HTTP_NEWLINE ; p++ )
				;
			if( likely( *(p-1) == HTTP_RETURN ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
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
			else if( unlikely( (*p) == HTTP_RETURN ) )
			{
				if( p+1 >= b->fill_ptr )
				{
					return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
				}
				else if( *(p+1) == HTTP_NEWLINE )
				{
					if( e->headers.content_length > 0 )
					{
						e->body = p + 2 ;
						e->parse_step = FASTERHTTP_PARSESTEP_BODY ;
						goto _GOTO_PARSESTEP_BODY;
					}
					else
					{
						b->process_ptr = p + 2 ;
						e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
						return 0;
					}
				}
			}
			e->headers.header.name_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 13 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_NAME ;
			
		case FASTERHTTP_PARSESTEP_HEADER_NAME :
			
#if DEBUG_TIMEVAL
			SetTimeval( 14 );
#endif
			
			for( ; likely( (*p) != ' ' && (*p) != ':' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			e->headers.header.name_len = p - e->headers.header.name_ptr ;
			
			for( ; likely( (*p) != ':' && (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			else if( unlikely( p >= b->fill_ptr ) )
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
			
			for( ; unlikely( (*p) == ' ' && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( unlikely( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			e->headers.header.value_ptr = p ;
			p++;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 15 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_VALUE ;
			
		case FASTERHTTP_PARSESTEP_HEADER_VALUE :
			
#if DEBUG_TIMEVAL
			SetTimeval( 16 );
#endif
			
			for( ; likely( (*p) != HTTP_NEWLINE && p < b->fill_ptr ) ; p++ )
				;
			if( unlikely( p >= b->fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			e->headers.header.value_len = p - e->headers.header.value_ptr ;
			for( p2 = p - 1 ; unlikely( (*p2) == ' ' || (*p2) == HTTP_RETURN ) ; p2-- )
				e->headers.header.value_len--;
			/*
			for( ; (*p) != HTTP_NEWLINE ; p++ )
				;
			*/
			p++;
			
			nret = PushHttpHeader( &(e->headers) , &(e->headers.header) ) ;
			if( nret )
				return nret;
			
			if( unlikely( e->headers.header.name_len == sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 && STRNICMP( e->headers.header.name_ptr , == , HTTP_HEADER_CONTENT_LENGTH , sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 ) ) )
				e->headers.content_length = natol( e->headers.header.value_ptr , e->headers.header.value_len ) ;
			
#if DEBUG_TIMEVAL
			SetTimevalAndDiff( 16 );
#endif
	
			e->parse_step = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_BODY :
			
_GOTO_PARSESTEP_BODY :
			
			if( likely( b->fill_ptr - e->body >= e->headers.content_length ) )
			{
				b->process_ptr = b->fill_ptr ;
				e->parse_step = FASTERHTTP_PARSESTEP_DONE ;
				return 0;
			}
			else
			{
				b->process_ptr = b->fill_ptr ;
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

char *GetHttpHeaderPtr_METHOD( struct HttpEnv *e , long *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.METHOD.value_len ;
	return e->headers.METHOD.value_ptr;
}

int GetHttpHeaderLen_METHOD( struct HttpEnv *e )
{
	return e->headers.METHOD.value_len;
}

char *GetHttpHeaderPtr_URI( struct HttpEnv *e , long *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.URI.value_len ;
	return e->headers.URI.value_ptr;
}

int GetHttpHeaderLen_URI( struct HttpEnv *e )
{
	return e->headers.URI.value_len;
}

char *GetHttpHeaderPtr_VERSION( struct HttpEnv *e , long *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.VERSION.value_len ;
	return e->headers.VERSION.value_ptr;
}

int GetHttpHeaderLen_VERSION( struct HttpEnv *e )
{
	return e->headers.VERSION.value_len;
}

char *GetHttpHeaderPtr_STATUS_CODE( struct HttpEnv *e , long *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.STATUSCODE.value_len ;
	return e->headers.STATUSCODE.value_ptr;
}

int GetHttpHeaderLen_STATUS_CODE( struct HttpEnv *e )
{
	return e->headers.STATUSCODE.value_len;
}

char *GetHttpHeaderPtr_REASON_PHRASE( struct HttpEnv *e , long *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.REASONPHRASE.value_len ;
	return e->headers.REASONPHRASE.value_ptr;
}

int GetHttpHeaderLen_REASON_PHRASE( struct HttpEnv *e )
{
	return e->headers.REASONPHRASE.value_len;
}

char *GetHttpHeaderPtr( struct HttpEnv *e , char *name , long *p_value_len )
{
	struct HttpHeader	*p_header = NULL ;
	
	p_header = QueryHttpHeader( &(e->headers) , name ) ;
	if( p_header == NULL )
		return NULL;
	
	if( p_value_len )
		(*p_value_len) = p_header->value_len ;
	return p_header->value_ptr;
}

int GetHttpHeaderLen( struct HttpEnv *e , char *name )
{
	struct HttpHeader	*p_header = NULL ;
	
	p_header = QueryHttpHeader( &(e->headers) , name ) ;
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

char *GetHttpHeaderNamePtr( struct HttpHeader *p_header , long *p_name_len )
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

char *GetHttpHeaderValuePtr( struct HttpHeader *p_header , long *p_value_len )
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

char *GetHttpBodyPtr( struct HttpEnv *e , long *p_body_len )
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
