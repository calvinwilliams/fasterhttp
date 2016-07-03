/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "internal.h"
#include "fasterhttp.h"

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

