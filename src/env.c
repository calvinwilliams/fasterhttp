/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "internal.h"
#include "fasterhttp.h"

int _FASTERHTTP_VERSION_1_0_0 = 0 ;

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
	if( e->enable_response_compressing == 2 )
		e->enable_response_compressing = 1 ;
	else
		e->enable_response_compressing = 0 ;
	
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
	
	memset( p_headers->header_array , 0x00 , sizeof(struct HttpHeader) * p_headers->header_array_size );
	p_headers->header_array_count = 0 ;
	
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

void EnableHttpResponseCompressing( struct HttpEnv *e , char enable_response_compressing )
{
	e->enable_response_compressing = enable_response_compressing ;
	return;
}

