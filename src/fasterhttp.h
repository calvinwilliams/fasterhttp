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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
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

#if __GNUC__ >= 3
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
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

#if ( defined _WIN32 )
#define SOCKLEN_T	int
#define CLOSESOCKET	closesocket
#elif ( defined __unix ) || ( defined __linux__ )
#define SOCKET		int
#define SOCKLEN_T	socklen_t
#define CLOSESOCKET	close
#endif

#define FASTERHTTP_ERROR_ALLOC				-11
#define FASTERHTTP_ERROR_PARAMTER			-12
#define FASTERHTTP_ERROR_USING				-13
#define FASTERHTTP_ERROR_INTERNAL			-14
#define FASTERHTTP_ERROR_TCP_SELECT_RECEIVE		-31
#define FASTERHTTP_ERROR_TCP_SELECT_RECEIVE_TIMEOUT	-32
#define FASTERHTTP_ERROR_TCP_RECEIVE			-33
#define FASTERHTTP_INFO_TCP_SEND_WOULDBLOCK		40
#define FASTERHTTP_ERROR_TCP_SELECT_SEND		-41
#define FASTERHTTP_ERROR_TCP_SELECT_SEND_TIMEOUT	-42
#define FASTERHTTP_ERROR_TCP_SEND			-43
#define FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER		100
#define FASTERHTTP_ERROR_HTTP_HEADERFIRSTLINE_INVALID	-101
#define FASTERHTTP_ERROR_HTTP_HEADER_INVALID		-102
#define FASTERHTTP_ERROR_HTTP_TRUNCATION		-103

#define FASTERHTTP_ERROR_NO_CONTENT			-204
#define FASTERHTTP_ERROR_BAD_REQUEST			-400
#define FASTERHTTP_ERROR_NOT_FOUND			-404
#define FASTERHTTP_ERROR_METHOD_INVALID			-405
#define FASTERHTTP_ERROR_URI_TOOLONG			-414
#define FASTERHTTP_ERROR_METHOD_NOT_SUPPORTED		-504
#define FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED		-505

#define FASTERHTTP_TIMEOUT_DEFAULT			60

#define FASTERHTTP_REQUEST_BUFSIZE_DEFAULT		40*1024
#define FASTERHTTP_RESPONSE_BUFSIZE_DEFAULT		400*1024
#define FASTERHTTP_HEADER_ARRAYSIZE_DEFAULT		64

#define FASTERHTTP_REQUEST_BUFSIZE_MAX			10*1024*1024
#define FASTERHTTP_RESPONSE_BUFSIZE_MAX			10*1024*1024
#define FASTERHTTP_HEADER_ARRAYSIZE_MAX			1024

#define FASTERHTTP_PARSESTEP_BEGIN				0
#define FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_METHOD0		110
#define FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_METHOD		111
#define FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_URI0		120
#define FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_URI		121
#define FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_VERSION0		130
#define FASTERHTTP_PARSESTEP_REQUESTFIRSTLINE_VERSION		131
#define FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_VERSION0		150
#define FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_VERSION		151
#define FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_STATUSCODE0	160
#define FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_STATUSCODE	161
#define FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_REASONPHRASE0	170
#define FASTERHTTP_PARSESTEP_RESPONSEFIRSTLINE_REASONPHRASE	171
#define FASTERHTTP_PARSESTEP_HEADER_NAME0			210
#define FASTERHTTP_PARSESTEP_HEADER_NAME			211
#define FASTERHTTP_PARSESTEP_HEADER_VALUE0			220
#define FASTERHTTP_PARSESTEP_HEADER_VALUE			221
#define FASTERHTTP_PARSESTEP_BODY				300
#define FASTERHTTP_PARSESTEP_CHUNKED_SIZE			311
#define FASTERHTTP_PARSESTEP_CHUNKED_DATA			312
#define FASTERHTTP_PARSESTEP_DONE				400

#define HTTP_RETURN					'\r'
#define HTTP_NEWLINE					'\n'

#define HTTP_HEADER_METHOD				"METHOD"
#define HTTP_HEADER_URI					"URI"
#define HTTP_HEADER_VERSION				"VERSION"
#define HTTP_HEADER_STATUSCODE				"STATUSCODE"
#define HTTP_HEADER_REASONPHRASE			"REASONPHRASE"

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
#define HTTP_HEADER_TRANSFERENCODING			"Transfer-Encoding"
#define HTTP_HEADER_TRANSFERENCODING__CHUNKED		"chunked"
#define HTTP_HEADER_TRAILER				"Trailer"

struct HttpBuffer ;
struct HttpEnv ;

/* http env */
_WINDLL_FUNC struct HttpEnv *CreateHttpEnv();
_WINDLL_FUNC void ResetHttpEnv( struct HttpEnv *e );
_WINDLL_FUNC void DestroyHttpEnv( struct HttpEnv *e );

/* properties */
_WINDLL_FUNC void SetHttpTimeout( struct HttpEnv *e , long timeout );
_WINDLL_FUNC struct timeval *GetHttpElapse( struct HttpEnv *e );

/* buffer operations */
_WINDLL_FUNC struct HttpBuffer *GetHttpRequestBuffer( struct HttpEnv *e );
_WINDLL_FUNC struct HttpBuffer *GetHttpResponseBuffer( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpBufferBase( struct HttpBuffer *b );
_WINDLL_FUNC long GetHttpBufferLength( struct HttpBuffer *b );
_WINDLL_FUNC void CleanHttpBuffer( struct HttpBuffer *b );
_WINDLL_FUNC int ReallocHttpBuffer( struct HttpBuffer *b , long new_buf_size );
_WINDLL_FUNC int StrcatHttpBuffer( struct HttpBuffer *b , char *str );
_WINDLL_FUNC int StrcatfHttpBuffer( struct HttpBuffer *b , char *format , ... );
_WINDLL_FUNC int StrcatvHttpBuffer( struct HttpBuffer *b , char *format , va_list valist );
_WINDLL_FUNC int MemcatHttpBuffer( struct HttpBuffer *b , char *base , long len );
_WINDLL_FUNC void SetHttpBufferPtr( struct HttpBuffer *b , char *ptr , long len );

/* http client advance api */
_WINDLL_FUNC int RequestHttp( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http server advance api */
typedef int funcProcessHttpRequest( struct HttpEnv *e , void *p );
_WINDLL_FUNC int ResponseHttp( SOCKET sock , SSL *ssl , struct HttpEnv *e , funcProcessHttpRequest *pfuncProcessHttpRequest , void *p );

/* http client api */
_WINDLL_FUNC int SendHttpRequest( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ReceiveHttpResponse( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http server api */
_WINDLL_FUNC int ReceiveHttpRequest( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int SendHttpResponse( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http client api with nonblock */
_WINDLL_FUNC int SendHttpRequestNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int ReceiveHttpResponseNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http server api with nonblock */
_WINDLL_FUNC int ReceiveHttpRequestNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );
_WINDLL_FUNC int SendHttpResponseNonblock( SOCKET sock , SSL *ssl , struct HttpEnv *e );

/* http test api */
_WINDLL_FUNC int ParseHttpResponse( struct HttpEnv *e );
_WINDLL_FUNC int ParseHttpRequest( struct HttpEnv *e );

/* http data */
_WINDLL_FUNC char *GetHttpHeaderPtr_METHOD( struct HttpEnv *e , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen_METHOD( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpHeaderPtr_URI( struct HttpEnv *e , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen_URI( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpHeaderPtr_VERSION( struct HttpEnv *e , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen_VERSION( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpHeaderPtr_STATUSCODE( struct HttpEnv *e , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen_STATUSCODE( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpHeaderPtr_REASONPHRASE( struct HttpEnv *e , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen_REASONPHRASE( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpHeaderPtr_TRANSFERENCODING( struct HttpEnv *e , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen_TRANSFERENCODING( struct HttpEnv *e );
_WINDLL_FUNC char *GetHttpHeaderPtr_TRAILER( struct HttpEnv *e , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen_TRAILER( struct HttpEnv *e );

_WINDLL_FUNC struct HttpHeader *QueryHttpHeader( struct HttpEnv *e , char *name );
_WINDLL_FUNC char *GetHttpHeaderPtr( struct HttpEnv *e , char *name , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderLen( struct HttpEnv *e , char *name );
_WINDLL_FUNC int GetHttpHeaderCount( struct HttpEnv *e );
_WINDLL_FUNC struct HttpHeader *TravelHttpHeaderPtr( struct HttpEnv *e , struct HttpHeader *p_header );
_WINDLL_FUNC char *GetHttpHeaderNamePtr( struct HttpHeader *p_header , int *p_key_len );
_WINDLL_FUNC int GetHttpHeaderNameLen( struct HttpHeader *p_header );
_WINDLL_FUNC char *GetHttpHeaderValuePtr( struct HttpHeader *p_header , int *p_value_len );
_WINDLL_FUNC int GetHttpHeaderValueLen( struct HttpHeader *p_header );

_WINDLL_FUNC char *GetHttpBodyPtr( struct HttpEnv *e , int *p_body_len );
_WINDLL_FUNC int GetHttpBodyLen( struct HttpEnv *e );

#endif

