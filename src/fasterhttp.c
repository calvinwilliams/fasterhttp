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
	
	struct HttpHeader	STATUS_CODE ;
	struct HttpHeader	REASON_PHRASE ;
	
	struct HttpHeader	CONTENT_LENGTH ;
	int			content_length ;
	
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
	
	e->parse_step = FASTERHTTP_PARSE_STEP_FIRSTLINE ;
	
	return e;
}

void ResetHttpEnv( struct HttpEnv *e )
{
	SetHttpTimeout( e , FASTERHTTP_TIMEOUT_DEFAULT );
	
	if( e->request_buffer.buf_size > FASTERHTTP_REQUEST_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->request_buffer) , FASTERHTTP_REQUEST_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->request_buffer) );
	if( e->response_buffer.buf_size > FASTERHTTP_RESPONSE_BUFSIZE_MAX )
		ReallocHttpBuffer( &(e->response_buffer) , FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT );
	CleanHttpBuffer( &(e->response_buffer) );
	
	e->parse_step = FASTERHTTP_PARSE_STEP_FIRSTLINE ;
	
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
		if( e->request_buffer.base )
			free( e->request_buffer.base );
		if( e->response_buffer.base )
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
	/* memset( b->base , 0x00 , b->buf_size ); */
	b->fill_ptr = b->base ;
	b->process_ptr = b->base ;
	
	return;
}

int ReallocHttpBuffer( struct HttpBuffer *b , long new_buf_size )
{
	char	*new_base = NULL ;
	int	fill_len = b->fill_ptr - b->base ;
	int	process_len = b->process_ptr - b->base ;
	
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

#define HEADERFIRSTLINE_TOKEN(_header_,_name_,_last_flag_) \
	(_header_).name_ptr = (_name_) ; \
	(_header_).name_len = strlen(_name_) ; \
	for( ; (*p) == ' ' ; p++ ) \
		; \
	if( (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID; \
	(_header_).value_ptr = p ; \
	for( ; (*p) != ' ' && (*p) != HTTP_NEWLINE ; p++ ) \
		; \
	if( (_last_flag_) == 0 && (*p) == HTTP_NEWLINE ) \
		return FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID; \
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
		HEADERFIRSTLINE_TOKEN( e->headers.METHOD , "METHOD" , 0 )
		/*
		if( e->headers.METHOD.value_len == sizeof(HTTP_METHOD_GET)-1 && MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_GET  , sizeof(HTTP_METHOD_GET)-1 ) )
			;
		else if( e->headers.METHOD.value_len == sizeof(HTTP_METHOD_POST)-1 && MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_POST  , sizeof(HTTP_METHOD_POST)-1 ) )
			;
		else if( e->headers.METHOD.value_len == sizeof(HTTP_METHOD_HEAD)-1 && MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_HEAD  , sizeof(HTTP_METHOD_HEAD)-1 ) )
			;
		else if( e->headers.METHOD.value_len == sizeof(HTTP_METHOD_TRACE)-1 && MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_TRACE  , sizeof(HTTP_METHOD_TRACE)-1 ) )
			;
		else if( e->headers.METHOD.value_len == sizeof(HTTP_METHOD_OPTIONS)-1 && MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_OPTIONS  , sizeof(HTTP_METHOD_OPTIONS)-1 ) )
			;
		else if( e->headers.METHOD.value_len == sizeof(HTTP_METHOD_PUT)-1 && MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_PUT  , sizeof(HTTP_METHOD_PUT)-1 ) )
			;
		else if( e->headers.METHOD.value_len == sizeof(HTTP_METHOD_DELETE)-1 && MEMCMP( e->headers.METHOD.value_ptr , == , HTTP_METHOD_DELETE  , sizeof(HTTP_METHOD_DELETE)-1 ) )
			;
		else
			return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
		*/
		
		/* Parse URI */
		HEADERFIRSTLINE_TOKEN( e->headers.URI , "URI" , 0 )
		
		/* Parse VERSION */
		HEADERFIRSTLINE_TOKEN( e->headers.VERSION , "VERSION" , 1 )
		/*
		if( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 && MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_0  , sizeof(HTTP_VERSION_1_0)-1 ) )
			;
		else if( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_1)-1 && MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_1  , sizeof(HTTP_VERSION_1_1)-1 ) )
			;
		else
			return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
		*/
		
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
		HEADERFIRSTLINE_TOKEN( e->headers.VERSION , "VERSION" , 0 )
		if( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_0)-1 && MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_0  , sizeof(HTTP_VERSION_1_0)-1 ) )
			;
		else if( e->headers.VERSION.value_len == sizeof(HTTP_VERSION_1_1)-1 && MEMCMP( e->headers.VERSION.value_ptr , == , HTTP_VERSION_1_1  , sizeof(HTTP_VERSION_1_1)-1 ) )
			;
		else
			return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
		
		/* Parse STATUS_CODE */
		HEADERFIRSTLINE_TOKEN( e->headers.STATUS_CODE , "STATUS_CODE" , 0 )
		
		/* Parse REASON_PHRASE */
		HEADERFIRSTLINE_TOKEN( e->headers.REASON_PHRASE , "REASON_PHRASE" , 1 )
		
		b->process_ptr = p + 1 ;
		
		return 0;
	}
	
	return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
}

#define HEADERKEY_TOKEN(_p_,_header_) \
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
		
		/* Parse header 'KEY' */
		HEADERKEY_TOKEN( p , header )
		
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
			case FASTERHTTP_PARSE_STEP_FIRSTLINE :
				
				if( b == &(e->request_buffer) )
				{
					nret = ParseHttpRequestHeaderFirstLine( e , b ) ;
					if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
						break;
					else if( nret )
						return nret;
					else
						e->parse_step = FASTERHTTP_PARSE_STEP_HEADER ;
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
				
				nret = ParseHttpHeader( e , b ) ;
				if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
					break;
				else if( nret )
					return nret;
				else if( e->headers.content_length > 0 )
					e->body = b->process_ptr , e->parse_step = FASTERHTTP_PARSE_STEP_BODY ;
				else
					e->parse_step = FASTERHTTP_PARSE_STEP_DONE ;
				
				break;
			
			case FASTERHTTP_PARSE_STEP_BODY :
				
				nret = ParseHttpBody( e , b ) ;
				if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
					break;
				else if( nret )
					return nret;
				else
					e->parse_step = FASTERHTTP_PARSE_STEP_DONE ;
				
				break;
			
			case FASTERHTTP_PARSE_STEP_DONE :
				
				return 0;
			
			default :
				return FASTERHTTP_ERROR_INTERNAL;
		}
		if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
			break;
	}
	
	return nret;
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
		(*p_value_len) = e->headers.STATUS_CODE.value_len ;
	return e->headers.STATUS_CODE.value_ptr;
}

int GetHttpHeaderLen_STATUS_CODE( struct HttpEnv *e )
{
	return e->headers.STATUS_CODE.value_len;
}

char *GetHttpHeaderPtr_REASON_PHRASE( struct HttpEnv *e , long *p_value_len )
{
	if( p_value_len )
		(*p_value_len) = e->headers.REASON_PHRASE.value_len ;
	return e->headers.REASON_PHRASE.value_ptr;
}

int GetHttpHeaderLen_REASON_PHRASE( struct HttpEnv *e )
{
	return e->headers.REASON_PHRASE.value_len;
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
