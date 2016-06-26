/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "internal.h"
#include "fasterhttp.h"

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
