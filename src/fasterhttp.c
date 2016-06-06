/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

struct HttpBuffer
{
	long		buf_size ;
	long		len ;
	char		*base ;
	char		*ptr ;
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
	struct HttpHeaderItem	*item_array ;
	long			item_array_size ;
	
	long			content_length ;
} ;

struct HttpEnv
{
	struct timeval		timeout ;
	
	struct HttpBuffer	request_buffer ;
	struct HttpBuffer	response_buffer ;
	
	struct HttpHeader	header ;
} ;

struct HttpEnv *CreateHttpEnv()
{
	struct HttpEnv	*e = NULL ;
	
	e = (struct HttpEnv *)malloc( sizeof(struct HttpEnv) ) ;
	if( e == NULL )
	{
		CleanHttpEnv( e );
		return FASTERHTTP_ERROR_ALLOC;
	}
	memset( e , 0x00 , sizeof(struct HttpEnv) );
	
	SetHttpTimeout( e , 60 );
	
	e->request_buffer.buf_size = FASTERHTTP_REQUEST_BUFSIZE_DEFAULT ;
	e->request_buffer.base = (char*)malloc( e->request_buffer.buf_size ) ;
	if( e->request_buffer.base == NULL )
	{
		CleanHttpEnv( e );
		return NULL;
	}
	memset( e->request_buffer.base , 0x00 , sizeof(e->request_buffer.buf_size) );
	e->request_buffer.ptr = e->request_buffer.base ;
	e->request_buffer.len = 0 ;
	
	e->response_buffer.buf_size = FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT ;
	e->response_buffer.base = (char*)malloc( e->response_buffer.buf_size ) ;
	if( e->response_buffer.base == NULL )
	{
		CleanHttpEnv( e );
		return NULL;
	}
	memset( e->response_buffer.base , 0x00 , sizeof(e->response_buffer.buf_size) );
	e->response_buffer.ptr = e->response_buffer.base ;
	e->response_buffer.len = 0 ;
	
	e->header.item_array_size = FASTERHTTP_HEADER_ITEM_ARRAYSIZE_DEFAULT ;
	e->header.item_array = (struct HttpHeaderItem *)malloc( sizeof(struct HttpHeaderItem) * e->header.item_array_size ) ;
	if( e->header.item_array == NULL )
	{
		CleanHttpEnv( e );
		return NULL;
	}
	memset( e->header.item_array , 0x00 , sizeof(struct HttpHeaderItem) * e->header.item_array_size );
	
	return e;
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
	return e->request_buffer;
}

struct HttpBuffer *GetHttpResponseBuffer( struct HttpEnv *e )
{
	return e->response_buffer;
}

char *GetHttpBufferBase( struct HttpBuffer *b )
{
	return b->base;
}

long GetHttpBufferLength( struct HttpBuffer *b )
{
	return b->len;
}

void CleanHttpBuffer( struct HttpBuffer *b )
{
	memset( b->base , 0x00 , b->buf_size );
	b->len = 0 ;
	b->ptr = b->base ;
	
	return;
}

#define DECLARE_REALLOC_VARS \
	long	new_buf_size ; \
	char	*new_base = NULL ; \

#define REALLOC_BUFFER(_b_) \
	if( (_b_)->buf_size ) <= 100*1024*1024 ) \
		new_buf_size = (_b_)->buf_size * 2 ; \
	else \
		new_buf_size = 100*1024*1024 ; \
	new_base = (char *)realloc( (_b_)->base , new_buf_size ) ; \
	if( new_base == NULL ) \
		return FASTERHTTP_ERROR_ALLOC; \
	memset( new_base + (_b_)->len , 0x00 , new_buf_size - (_b_)->len ); \
	(_b_)->buf_size = new_buf_size ; \
	(_b_)->ptr = (_b_)->ptr - (_b_)->base + new_base ; \
	(_b_)->base = new_base ; \

int StrcatfHttpBuffer( struct HttpBuffer *b , char *format , ... )
{
	va_list		valist ;
	long		len ;
	DECLARE_REALLOC_VARS
	
	while(1)
	{
		va_start( valist , format );
		len = VSNPRINTF( b->ptr , b->buf_size-1 - b->len , format , valist ) ;
		va_end( valist );
		if( len == -1 || len == b->buf_size-1 - b->len )
		{
			REALLOC_BUFFER( b );
		}
		else
		{
			break;
		}
	}
	
	return 0;
}

int MemcatHttpBuffer( struct HttpBuffer *b , char *base , long len )
{
	DECLARE_REALLOC_VARS
	
	while( b->len + len > b->buf_size-1 )
	{
		REALLOC_BUFFER( b , len );
	}
	
	memcpy( b->ptr , base , len );
	b->len += len ;
	
	return 0;
}

