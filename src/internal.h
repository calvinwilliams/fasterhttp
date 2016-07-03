/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_INTERNAL_
#define _H_INTERNAL_

#ifdef __cplusplus
extern "C" {
#endif

#include "fasterhttp.h"

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
	char			enable_response_compressing ;
	
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

int SendHttpBuffer( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b );
int ReceiveHttpBuffer( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b );
int ReceiveHttpBuffer1( SOCKET sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b );

int ParseHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b );

void _DumpHexBuffer( FILE *fp , char *buf , long buflen );

#ifdef __cplusplus
}
#endif

#endif
