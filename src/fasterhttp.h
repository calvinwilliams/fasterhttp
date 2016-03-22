/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_FASTERHTTP_
#define _H_FASTERHTTP_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/time.h>
#include <openssl/ssl.h>

#if ( defined _WIN32 )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC            _declspec(dllexport)
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC
#endif
#endif

#ifndef STRCMP
#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#endif

#ifndef STRICMP
#if ( defined _WIN32 )
#define STRICMP(_a_,_C_,_b_) ( stricmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strnicmp(_a_,_b_,_n_) _C_ 0 )
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ )
#define STRICMP(_a_,_C_,_b_) ( strcasecmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strncasecmp(_a_,_b_,_n_) _C_ 0 )
#endif
#endif

#ifndef MEMCMP
#define MEMCMP(_a_,_C_,_b_,_n_) ( memcmp(_a_,_b_,_n_) _C_ 0 )
#endif

#ifndef SNPRINTF
#if ( defined _WIN32 )
#define SNPRINTF        _snprintf
#define VSNPRINTF       _vsnprintf
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ )
#define SNPRINTF        snprintf
#define VSNPRINTF       vsnprintf
#endif
#endif

#define FASTERHTTP_ERROR_ALLOC				-11
#define FASTERHTTP_ERROR_PARAMTER			-12
#define FASTERHTTP_ERROR_INTERNAL			-14
#define FASTERHTTP_INFO_TCP_RECEIVE_WOULDBLOCK		30
#define FASTERHTTP_ERROR_TCP_SELECT_RECEIVE		-31
#define FASTERHTTP_ERROR_TCP_SELECT_RECEIVE_TIMEOUT	-32
#define FASTERHTTP_ERROR_TCP_RECEIVE			-33
#define FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK		40
#define FASTERHTTP_ERROR_TCP_SELECT_SEND		-41
#define FASTERHTTP_ERROR_TCP_SELECT_SEND_TIMEOUT	-42
#define FASTERHTTP_ERROR_TCP_SEND			-43
#define FASTERHTTP_ERROR_FD_SELECT_READ			-51
#define FASTERHTTP_ERROR_FD_SELECT_READ_TIMEOUT		-52
#define FASTERHTTP_ERROR_FD_READ			-53
#define FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER		100
#define FASTERHTTP_ERROR_HTTP_TRUNCATION		-101
#define FASTERHTTP_ERROR_HTTP_HEADERLINE_INVALID	-102
#define FASTERHTTP_ERROR_HTTP_PROCESSING		-104

#define FASTERHTTP_ERROR_NO_CONTENT			-204
#define FASTERHTTP_ERROR_BAD_REQUEST			-400
#define FASTERHTTP_ERROR_NOT_FOUND			-404
#define FASTERHTTP_ERROR_METHOD_INVALID			-405
#define FASTERHTTP_ERROR_URI_TOOLONG			-414
#define FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED		-505

#define FASTERHTTP_REQUEST_BUFSIZE_DEFAULT		400*1024
#define FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT		400*1024
#define FASTERHTTP_READBLOCK_SIZE_DEFAULT		4*1024
#define FASTERHTTP_HEADER_ITEM_ARRAYSIZE_DEFAULT	64

#define FASTERHTTP_REQUEST_BUFSIZE_MAX			10*1024*1024
#define FASTERHTTP_RESPONSE_BUFSIZE_MAX			10*1024*1024
#define FASTERHTTP_HEADER_ITEM_ARRAYSIZE_MAX		256

#define FASTERHTTP_PARSE_STEP_FIRSTLINE			0
#define FASTERHTTP_PARSE_STEP_HEADER			1
#define FASTERHTTP_PARSE_STEP_BODY			2
#define FASTERHTTP_PARSE_STEP_DONE			3

#define HTTP_METHOD_POST				"POST"
#define HTTP_METHOD_GET					"GET"

struct HttpBuffer ;
struct HttpEnv ;

_WINDLL_FUNC struct HttpEnv *CreateHttpEnv();
_WINDLL_FUNC void ResetHttpEnv( struct HttpEnv *e );
_WINDLL_FUNC void DestroyHttpEnv( struct HttpEnv *e );

_WINDLL_FUNC void SetHttpTimeout( struct HttpEnv *e , long timeout );

_WINDLL_FUNC struct HttpBuffer *GetHttpRequestBuffer( struct HttpEnv *e );
_WINDLL_FUNC struct HttpBuffer *GetHttpResponseBuffer( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpBufferBase( struct HttpBuffer *b );
_WINDLL_FUNC long GetHttpBufferLength( struct HttpBuffer *b );
_WINDLL_FUNC void CleanHttpBuffer( struct HttpBuffer *b );
_WINDLL_FUNC int ReallocHttpBuffer( struct HttpBuffer *b , long new_buf_size );
_WINDLL_FUNC int StrcatfHttpBuffer( struct HttpBuffer *b , char *format , ... );
_WINDLL_FUNC int MemcatHttpBuffer( struct HttpBuffer *b , char *base , long len );
_WINDLL_FUNC int WriteHttpBuffer( int sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b );
_WINDLL_FUNC int ReadHttpBuffer( int sock , SSL *ssl , struct HttpEnv *e , struct HttpBuffer *b );

_WINDLL_FUNC int SendHttpRequest( int sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ReceiveHttpResponse( int sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ParseHttpResponse( struct HttpEnv *e );

_WINDLL_FUNC int ReceiveHttpRequest( int sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ParseHttpRequest( struct HttpEnv *e );
_WINDLL_FUNC int SendHttpResponse( int sock , SSL *ssl , struct HttpEnv *e );

#endif

