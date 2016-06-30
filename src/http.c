/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "internal.h"
#include "fasterhttp.h"

static char *TokenHttpHeaderValue( char *str , char **pp_token , int *p_token_len )
{
	char	*p = str ;
	
	while( (*p) == ' ' )
		p++;
	if( (*p) == '\r' || (*p) == '\n' || (*p) == '\0' )
	{
		(*pp_token) = NULL ;
		(*p_token_len) = 0 ;
		return NULL;
	}
	
	(*pp_token) = p ;
	while( (*p) != ' ' && (*p) != ',' && (*p) != '\r' && (*p) != '\n' && (*p) != '\0' )
		p++;
	if( p == (*pp_token) )
	{
		(*pp_token) = NULL ;
		(*p_token_len) = 0 ;
		return NULL;
	}
	else
	{
		(*p_token_len) = p - (*pp_token) ;
		return p;
	}
}

static int CompressHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b , char *p_compress_algorithm , int compress_algorithm_len , int compress_algorithm )
{
	char		*p_CONTENT_LENGTH = NULL ;
	char		*p_CONTENT_LENGTH_end = NULL ;
	char		*p_headers_end = NULL ;
	char		*body = NULL ;
	
	uLong		in_len ;
	uLong		out_len ;
	Bytef		*out_base = NULL ;
	z_stream	stream ;
	
	int		new_buf_size ;
	
	int		nret = 0 ;
	
printf( "enter CompressHttpBuffer\n" );
_DumpHexBuffer( stdout , b->base , b->fill_ptr-b->base );
	p_CONTENT_LENGTH = STRISTR( b->base , HTTP_HEADER_CONTENT_LENGTH ) ;
	if( p_CONTENT_LENGTH == NULL || p_CONTENT_LENGTH >= b->fill_ptr )
		return 0;
	p_CONTENT_LENGTH_end = strstr( p_CONTENT_LENGTH+1 , "\r\n" ) ;
	if( p_CONTENT_LENGTH_end == NULL || p_CONTENT_LENGTH_end >= b->fill_ptr )
		return 0;
	p_CONTENT_LENGTH_end += 2 ;
	p_headers_end = strstr( p_CONTENT_LENGTH+1 , "\r\n\r\n" ) ;
	if( p_headers_end == NULL || p_headers_end >= b->fill_ptr )
		return 0;
	p_headers_end += 2 ;
	body = p_headers_end + 2 ;
	
	stream.zalloc = NULL ;
	stream.zfree = NULL ;
	stream.opaque = NULL ;
	nret = deflateInit2( &stream , Z_DEFAULT_COMPRESSION , Z_DEFLATED , compress_algorithm , MAX_MEM_LEVEL , Z_DEFAULT_STRATEGY ) ;
	if( nret != Z_OK )
	{
		return FASTERHTTP_ERROR_INTERNAL;
	}
	
	in_len = b->fill_ptr - body ;
	out_len = deflateBound( &stream , in_len ) ;
	out_base = (unsigned char *)malloc( out_len+1 ) ;
	if( out_base == NULL )
	{
		deflateEnd( &stream );
		return FASTERHTTP_ERROR_INTERNAL;
	}
	
	stream.next_in = (Bytef*)body ;
	stream.avail_in = in_len ;
	stream.next_out = out_base ;
	stream.avail_out = out_len ;
	nret = deflate( &stream , Z_FINISH ) ;
	if( nret != Z_OK )
	{
		free( out_base );
		deflateEnd( &stream );
		return FASTERHTTP_ERROR_INTERNAL;
	}
printf( "deflate\n" );
_DumpHexBuffer( stdout , (char*)out_base , (int)(stream.total_out) );
	
	new_buf_size = (body-b->base) + strlen(HTTP_HEADER_CONTENT_LENGTH)+2+10+2 + strlen(HTTP_HEADER_CONTENTENCODING)+2+10+2 + 10+2 + stream.total_out + 1 ;
	if( new_buf_size > b->buf_size )
	{
		nret = ReallocHttpBuffer( b , new_buf_size ) ;
		if( nret )
		{
			free( out_base );
			deflateEnd( &stream );
			return nret;
		}
		
		p_CONTENT_LENGTH = STRISTR( b->base , HTTP_HEADER_CONTENT_LENGTH ) ;
		if( p_CONTENT_LENGTH == NULL || p_CONTENT_LENGTH >= b->fill_ptr )
			return 0;
		p_CONTENT_LENGTH_end = strstr( p_CONTENT_LENGTH+1 , "\r\n" ) ;
		if( p_CONTENT_LENGTH_end == NULL || p_CONTENT_LENGTH_end >= b->fill_ptr )
			return 0;
		p_CONTENT_LENGTH_end += 2 ;
		p_headers_end = strstr( p_CONTENT_LENGTH+1 , "\r\n\r\n" ) ;
		if( p_headers_end == NULL || p_headers_end >= b->fill_ptr )
			return 0;
		p_headers_end += 2 ;
		body = p_headers_end + 2 ;
	}
	
	memmove( p_CONTENT_LENGTH , p_CONTENT_LENGTH_end , p_headers_end - p_CONTENT_LENGTH_end );
	b->fill_ptr = p_CONTENT_LENGTH + ( p_headers_end - p_CONTENT_LENGTH_end ) ;
	nret = StrcatfHttpBuffer( b , HTTP_HEADER_CONTENTENCODING ": %.*s" HTTP_RETURN_NEWLINE
					HTTP_HEADER_CONTENT_LENGTH ": %d" HTTP_RETURN_NEWLINE
					HTTP_RETURN_NEWLINE
					, compress_algorithm_len , p_compress_algorithm
					, stream.total_out ) ;
	if( nret )
	{
		free( out_base );
		deflateEnd( &stream );
		return nret;
	}
	nret = MemcatHttpBuffer( b , (char*)out_base , (int)(stream.total_out) ) ;
	if( nret )
	{
		free( out_base );
		deflateEnd( &stream );
		return nret;
	}
	
	free( out_base );
	deflateEnd( &stream );
printf( "leave CompressHttpBuffer\n" );
_DumpHexBuffer( stdout , b->base , b->fill_ptr-b->base );
	
	return 0;
}

static int CompressHttpResponse( struct HttpEnv *e )
{
	char	*base = NULL ;
	char	*p_compress_algorithm = NULL ;
	int	compress_algorithm_len ;
	
	base = GetHttpHeaderPtr( e , HTTP_HEADER_CONTENTENCODING , NULL ) ;
	while( base )
	{
		base = TokenHttpHeaderValue( base , & p_compress_algorithm , & compress_algorithm_len ) ;
		if( p_compress_algorithm )
		{
			if( compress_algorithm_len == 4 && STRNICMP( p_compress_algorithm , == , "gzip" , compress_algorithm_len ) )
			{
				return CompressHttpBuffer( e , GetHttpResponseBuffer(e) , p_compress_algorithm , compress_algorithm_len , HTTP_COMPRESSALGORITHM_GZIP ) ;
			}
			else if( compress_algorithm_len == 7 && STRNICMP( p_compress_algorithm , == , "deflate" , compress_algorithm_len ) )
			{
				return CompressHttpBuffer( e , GetHttpResponseBuffer(e) , p_compress_algorithm , compress_algorithm_len , HTTP_COMPRESSALGORITHM_DEFLATE ) ;
			}
		}
	}
	
	return 0;
}

int SendHttpResponse( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
printf( "e->enable_response_compressing[%d]\n" , e->enable_response_compressing );
	if( e->enable_response_compressing == 1 )
	{
printf( "CompressHttpResponse\n" );
		nret = CompressHttpResponse( e ) ;
		if( nret )
			return nret;
		
		e->enable_response_compressing = 2 ;
	}
	
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

static int UncompressHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b , int compress_algorithm )
{
	uLong		in_len ;
	uLong		out_len ;
	Bytef		*out_base = NULL ;
	z_stream	stream ;
	
	int		new_buf_size ;
	
	int		nret = 0 ;
	
	memset( & stream , 0x00 , sizeof(z_stream) );
printf( "enter UncompressHttpBuffer\n" );
_DumpHexBuffer( stdout , b->base , b->fill_ptr-b->base );
	stream.zalloc = NULL ;
	stream.zfree = NULL ;
	stream.opaque = NULL ;
printf( "compress_algorithm[%d]\n" , compress_algorithm );
	nret = inflateInit2( &stream , compress_algorithm ) ;
	if( nret != Z_OK )
	{
		return FASTERHTTP_ERROR_INTERNAL;
	}
	
	in_len = e->headers.content_length ;
	//out_len = in_len * 10 ;
	out_len = 10 ;
	out_base = (Bytef*)malloc( out_len+1 ) ;
	if( out_base == NULL )
	{
		inflateEnd( &stream );
		return FASTERHTTP_ERROR_INTERNAL;
	}
	memset( out_base , 0x00 , out_len+1 );
	
printf( "111\n" );
	while(1)
	{
printf( "222\n" );
		stream.total_out = 0 ;
		stream.next_in = (Bytef*)(e->body) ;
		stream.avail_in = in_len ;
		stream.next_out = out_base ;
		stream.avail_out = out_len ;
printf( "111 - stream.avail_in[%d] stream.avail_out[%d] stream.total_out[%d]\n" , (int)(stream.avail_in) , (int)(stream.avail_out) , (int)(stream.total_out) );
		nret = inflate( &stream , Z_NO_FLUSH ) ;
printf( "222 - stream.avail_in[%d] stream.avail_out[%d] stream.total_out[%d]\n" , (int)(stream.avail_in) , (int)(stream.avail_out) , (int)(stream.total_out) );
		if( nret != Z_OK )
		{
printf( "!=Z_OK\n" );
			free( out_base );
			inflateEnd( &stream );
			return FASTERHTTP_ERROR_INTERNAL;
		}
		else
		{
			uLong		new_out_len ;
			Bytef		*new_out_base = NULL ;
printf( "Z_OK\n" );
			if( stream.avail_in == 0 )
				break;
			
			new_out_len = out_len * 2 ;
			new_out_base = (Bytef*)realloc( new_out_base , new_out_len+1 ) ;
			if( new_out_base == NULL )
			{
				free( out_base );
				inflateEnd( &stream );
				return FASTERHTTP_ERROR_INTERNAL;
			}
			out_len = new_out_len ;
			out_base = new_out_base ;
		}
	}
printf( "999\n" );
	
	new_buf_size = (e->body-b->base) + stream.total_out + 1 ;
	if( new_buf_size > b->buf_size )
	{
		nret = ReallocHttpBuffer( b , new_buf_size ) ;
		if( nret )
		{
			free( out_base );
			inflateEnd( &stream );
			return nret;
		}
	}
	
	memmove( e->body , out_base , stream.total_out );
	b->fill_ptr = e->body + stream.total_out ;
	e->headers.content_length = stream.total_out ;
	
	free( out_base );
	inflateEnd( &stream );
	
printf( "leave UncompressHttpBuffer\n" );
_DumpHexBuffer( stdout , b->base , b->fill_ptr-b->base );
	return 0;
}

static int UncompressHttpResponse( struct HttpEnv *e )
{
	char	*base = NULL ;
	char	*p_compress_algorithm = NULL ;
	int	compress_algorithm_len ;
	
	base = GetHttpHeaderPtr( e , HTTP_HEADER_CONTENTENCODING , NULL ) ;
	while( base )
	{
		base = TokenHttpHeaderValue( base , & p_compress_algorithm , & compress_algorithm_len ) ;
		if( p_compress_algorithm )
		{
			if( compress_algorithm_len == 4 && STRNICMP( p_compress_algorithm , == , "gzip" , compress_algorithm_len ) )
			{
				return UncompressHttpBuffer( e , GetHttpResponseBuffer(e) , HTTP_COMPRESSALGORITHM_GZIP ) ;
			}
			else if( compress_algorithm_len == 7 && STRNICMP( p_compress_algorithm , == , "deflate" , compress_algorithm_len ) )
			{
				return UncompressHttpBuffer( e , GetHttpResponseBuffer(e) , HTTP_COMPRESSALGORITHM_DEFLATE ) ;
			}
		}
	}
	
	return 0;
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
		goto _GOTO_ON_ERROR;
	
	nret = pfuncProcessHttpRequest( e , p ) ;
	if( nret )
	{
		nret = HTTP_SERVICE_UNAVAILABLE ;
		goto _GOTO_ON_ERROR;
	}
	
	nret = SendHttpResponse( sock , ssl , e ) ;
	if( nret )
		return nret;
	
_GOTO_ON_ERROR :
	
	nret = FormatHttpResponseStartLine( abs(nret)/1000 , e ) ;
	if( nret )
		return nret;
	
	nret = StrcatHttpBuffer( GetHttpResponseBuffer(e) , HTTP_RETURN_NEWLINE ) ;
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
			return UncompressHttpResponse(e);
	}
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

int FormatHttpResponseStartLine( int status_code , struct HttpEnv *e )
{
	struct HttpBuffer	*b = GetHttpResponseBuffer(e) ;
	
	int			nret = 0 ;
	
	switch( status_code )
	{
		case HTTP_CONTINUE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_CONTINUE_S " Continue" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_SWITCHING_PROTOCOL :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_SWITCHING_PROTOCOL_S " Switching Protocol" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_OK :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_OK_S " OK" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_CREATED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_CREATED_S " Created" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_ACCEPTED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_ACCEPTED_S " Accepted" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NON_AUTHORITATIVE_INFORMATION :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NON_AUTHORITATIVE_INFORMATION_S " Non Authoritative Information" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NO_CONTENT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NO_CONTENT_S " No Content" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_RESET_CONTENT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_RESET_CONTENT_S " Reset Content" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PARTIAL_CONTENT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PARTIAL_CONTENT_S " Partial Content" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_MULTIPLE_CHOICES :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_MULTIPLE_CHOICES_S " Multiple Choices" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_MOVED_PERMANNETLY :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_MOVED_PERMANNETLY_S " Moved Permannetly" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_FOUND :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_FOUND_S " Found" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_SEE_OTHER :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_SEE_OTHER_S " See Other" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_MODIFIED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_MODIFIED_S " Not Modified" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_USE_PROXY :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_USE_PROXY_S " Use Proxy" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_TEMPORARY_REDIRECT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_TEMPORARY_REDIRECT_S " Temporary Redirect" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_BAD_REQUEST :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_BAD_REQUEST_S " Bad Request" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_UNAUTHORIZED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_UNAUTHORIZED_S " Unauthorized" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PAYMENT_REQUIRED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PAYMENT_REQUIRED_S " Payment Required" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_FORBIDDEN :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_FORBIDDEN_S " Forbidden" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_FOUND :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_FOUND_S " Not Found" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_METHOD_NOT_ALLOWED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_METHOD_NOT_ALLOWED_S " Method Not Allowed" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_ACCEPTABLE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_ACCEPTABLE_S " Not Acceptable" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PROXY_AUTHENTICATION_REQUIRED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PROXY_AUTHENTICATION_REQUIRED_S " Proxy Authentication Required" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_REQUEST_TIMEOUT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_REQUEST_TIMEOUT_S " Request Timeout" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_CONFLICT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_CONFLICT_S " Conflict" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_GONE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_GONE_S " Gone" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_LENGTH_REQUIRED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_LENGTH_REQUIRED_S " Length Request" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_PRECONDITION_FAILED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_PRECONDITION_FAILED_S " Precondition Failed" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_REQUEST_ENTITY_TOO_LARGE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_REQUEST_ENTITY_TOO_LARGE_S " Request Entity Too Large" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_URI_TOO_LONG :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_URI_TOO_LONG_S " Request URI Too Long" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_UNSUPPORTED_MEDIA_TYPE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_UNSUPPORTED_MEDIA_TYPE_S " Unsupported Media Type" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_REQUESTED_RANGE_NOT_SATISFIABLE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_REQUESTED_RANGE_NOT_SATISFIABLE_S " Requested Range Not Satisfiable" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_EXPECTATION_FAILED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_EXPECTATION_FAILED_S " Expectation Failed" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_INTERNAL_SERVER_ERROR :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_INTERNAL_SERVER_ERROR_S " Internal Server Error" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_NOT_IMPLEMENTED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_NOT_IMPLEMENTED_S " Not Implemented" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_BAD_GATEWAY :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_BAD_GATEWAY_S " Bad Gateway" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_SERVICE_UNAVAILABLE :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_SERVICE_UNAVAILABLE_S " Service Unavailable" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_GATEWAY_TIMEOUT :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_GATEWAY_TIMEOUT_S " Gateway Timeout" HTTP_RETURN_NEWLINE ) ;
			break;
		case HTTP_HTTP_VERSION_NOT_SUPPORTED :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_HTTP_VERSION_NOT_SUPPORTED_S " HTTP Version Not Supported" HTTP_RETURN_NEWLINE ) ;
			break;
		default :
			nret = StrcpyHttpBuffer( b , HTTP_VERSION_1_1 " " HTTP_INTERNAL_SERVER_ERROR_S " Internal Server Error" HTTP_RETURN_NEWLINE ) ;
			break;
	}
	if( nret )
		return nret;
	
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
		return UncompressHttpResponse(e);
}

int ReceiveHttpResponseNonblock1( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ReceiveHttpBuffer1( sock , ssl , e , &(e->response_buffer) ) ;
	if( nret )
		return nret;
	
	nret = ParseHttpBuffer( e , &(e->response_buffer) ) ;
	if( nret == FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER )
		return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
	else if( nret )
		return nret;
	else
		return UncompressHttpResponse(e);
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

int ReceiveHttpRequestNonblock1( SOCKET sock , SSL *ssl , struct HttpEnv *e )
{
	int		nret = 0 ;
	
	nret = ReceiveHttpBuffer1( sock , ssl , e , &(e->request_buffer) ) ;
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
	 
	if( e->enable_response_compressing == 1 )
	{
		nret = CompressHttpResponse( e ) ;
		if( nret )
			return nret;
		
		e->enable_response_compressing = 2 ;
	}
	
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

