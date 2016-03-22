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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#if ( defined _WIN32 )
#include <winsock2.h>
#include <windows.h>
#elif ( defined __unix ) || ( defined __linux__ )
#include <unistd.h>
#include <sys/time.h>
#endif

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

#if ( defined __unix ) || ( defined __linux__ )
#define SOCKET		int
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
#define FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID	-101
#define FASTERHTTP_ERROR_HTTP_HEADER_INVALID		-102
#define FASTERHTTP_ERROR_HTTP_TRUNCATION		-103
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
#define FASTERHTTP_HEADER_ITEM_ARRAYSIZE_MAX		1024

#define FASTERHTTP_PARSE_STEP_FIRSTLINE			0
#define FASTERHTTP_PARSE_STEP_HEADER			1
#define FASTERHTTP_PARSE_STEP_BODY			2
#define FASTERHTTP_PARSE_STEP_DONE			3

#define HTTP_NEWLINE					"\r\n"
#define HTTP_NEWLINE_RETURN				'\r'

#define HTTP_METHOD_GET					"GET"
#define HTTP_METHOD_POST				"POST"
#define HTTP_METHOD_HEAD				"HEAD"
#define HTTP_METHOD_TRACE				"TRACE"
#define HTTP_METHOD_OPTIONS				"OPTIONS"
#define HTTP_METHOD_PUT					"PUT"
#define HTTP_METHOD_DELETE				"DELETE"

#define HTTP_VERSION_1_0				"HTTP/1.0"
#define HTTP_VERSION_1_1				"HTTP/1.1"

#define HTTP_HEADER_CONTENT_LENGTH			"Content-Length"

struct HttpBuffer ;
struct HttpEnv ;

/* http env */
_WINDLL_FUNC struct HttpEnv *CreateHttpEnv();
_WINDLL_FUNC void ResetHttpEnv( struct HttpEnv *e );
_WINDLL_FUNC void DestroyHttpEnv( struct HttpEnv *e );

/* properties */
_WINDLL_FUNC void SetHttpTimeout( struct HttpEnv *e , long timeout );

/* buffer operations */
_WINDLL_FUNC struct HttpBuffer *GetHttpRequestBuffer( struct HttpEnv *e );
_WINDLL_FUNC struct HttpBuffer *GetHttpResponseBuffer( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpBufferBase( struct HttpBuffer *b );
_WINDLL_FUNC long GetHttpBufferLength( struct HttpBuffer *b );
_WINDLL_FUNC void CleanHttpBuffer( struct HttpBuffer *b );
_WINDLL_FUNC int ReallocHttpBuffer( struct HttpBuffer *b , long new_buf_size );
_WINDLL_FUNC int StrcatfHttpBuffer( struct HttpBuffer *b , char *format , ... );
_WINDLL_FUNC int MemcatHttpBuffer( struct HttpBuffer *b , char *base , long len );

/* http client advance api */
_WINDLL_FUNC int RequestHttp( int sock , SSL *ssl , struct HttpEnv *e );

/* http server advance api */
typedef int funcProcessHttpRequest( struct HttpEnv *e , void *p );
_WINDLL_FUNC int ResponseHttp( int sock , SSL *ssl , struct HttpEnv *e , funcProcessHttpRequest *pfuncProcessHttpRequest , void *p );

/* http client api */
_WINDLL_FUNC int SendHttpRequest( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ReceiveHttpResponse( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ParseHttpResponse( struct HttpEnv *e );

/* http server api */
_WINDLL_FUNC int ReceiveHttpRequest( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ParseHttpRequest( struct HttpEnv *e );
_WINDLL_FUNC int SendHttpResponse( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http client api with nonblock */
_WINDLL_FUNC int SendHttpRequestNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ReceiveHttpResponseNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http server api with nonblock */
_WINDLL_FUNC int ReceiveHttpRequestNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int SendHttpResponseNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http headers */
_WINDLL_FUNC char *GetHttpHeaderPtr( struct HttpEnv *e , char *key , long *p_value_len );

#endif

