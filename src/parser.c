/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "internal.h"
#include "fasterhttp.h"

#define DEBUG_PARSE	1

int ParseHttpBuffer( struct HttpEnv *e , struct HttpBuffer *b )
{
	register char		*p = b->process_ptr ;
	char			*p2 = NULL ;
	
	int			*p_parse_step = &(e->parse_step) ;
	struct HttpHeader	*p_METHOD = &(e->headers.METHOD) ;
	struct HttpHeader	*p_URI = &(e->headers.URI) ;
	struct HttpHeader	*p_VERSION = &(e->headers.VERSION) ;
	struct HttpHeader	*p_STATUSCODE = &(e->headers.STATUSCODE) ;
	struct HttpHeader	*p_REASONPHRASE = &(e->headers.REASONPHRASE) ;
	struct HttpHeader	*p_TRAILER = &(e->headers.TRAILER) ;
	
	struct HttpHeader	*p_header = &(e->headers.header_array[e->headers.header_array_count]) ;
	char			*fill_ptr = b->fill_ptr ;
	
	char			*p_version = &(e->headers.version) ;
	int			*p_content_length = &(e->headers.content_length) ;
	char			*p_transfer_encoding__chunked = &(e->headers.transfer_encoding__chunked) ;
	char			*p_connection__keepalive = &(e->headers.connection__keepalive) ;
	
#if DEBUG_PARSE
	printf( "DEBUG_PARSE >>>>>>>>> ParseHttpBuffer - b->process_ptr[0x%02X...][%.*s]\n" , b->process_ptr[0] , (int)(b->fill_ptr-b->process_ptr) , b->process_ptr );
#endif
	if( UNLIKELY( *(p_parse_step) == FASTERHTTP_PARSESTEP_BEGIN ) )
	{
		if( b == &(e->request_buffer) )
		{
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD0 ;
		}
		else if( b == &(e->response_buffer) )
		{
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION0 ;
		}
		else
		{
			return FASTERHTTP_ERROR_PARAMTER;
		}
	}
	
	switch( *(p_parse_step) )
	{
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_METHOD->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_METHOD\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_METHOD->value_len = p - p_METHOD->value_ptr ;
			p++;
			
			p2 = p_METHOD->value_ptr ;
			if( LIKELY( p_METHOD->value_len == 3 ) )
			{
				if( LIKELY( *(p2) == HTTP_METHOD_GET[0] && *(p2+1) == HTTP_METHOD_GET[1] && *(p2+2) == HTTP_METHOD_GET[2] ) )
					;
				else if( *(p2) == HTTP_METHOD_PUT[0] && *(p2+1) == HTTP_METHOD_PUT[1] && *(p2+2) == HTTP_METHOD_PUT[2] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( LIKELY( p_METHOD->value_len == 4 ) )
			{
				if( LIKELY( *(p2) == HTTP_METHOD_POST[0] && *(p2+1) == HTTP_METHOD_POST[1] && *(p2+2) == HTTP_METHOD_POST[2]
					&& *(p2+3) == HTTP_METHOD_POST[3] ) )
					;
				else if( *(p2) == HTTP_METHOD_HEAD[0] && *(p2+1) == HTTP_METHOD_HEAD[1] && *(p2+2) == HTTP_METHOD_HEAD[2]
					&& *(p2+3) == HTTP_METHOD_HEAD[3] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( p_METHOD->value_len == 5 )
			{
				if( *(p2) == HTTP_METHOD_TRACE[0] && *(p2+1) == HTTP_METHOD_TRACE[1] && *(p2+2) == HTTP_METHOD_TRACE[2]
					&& *(p2+3) == HTTP_METHOD_TRACE[3] && *(p2+4) == HTTP_METHOD_TRACE[4] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( p_METHOD->value_len == 6 )
			{
				if( *(p2) == HTTP_METHOD_DELETE[0] && *(p2+1) == HTTP_METHOD_DELETE[1] && *(p2+2) == HTTP_METHOD_DELETE[2]
					&& *(p2+3) == HTTP_METHOD_DELETE[3] && *(p2+4) == HTTP_METHOD_DELETE[4] && *(p2+5) == HTTP_METHOD_DELETE[5] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else if( p_METHOD->value_len == 7 )
			{
				if( *(p2) == HTTP_METHOD_OPTIONS[0] && *(p2+1) == HTTP_METHOD_OPTIONS[1] && *(p2+2) == HTTP_METHOD_OPTIONS[2]
					&& *(p2+3) == HTTP_METHOD_OPTIONS[3] && *(p2+4) == HTTP_METHOD_OPTIONS[4] && *(p2+5) == HTTP_METHOD_OPTIONS[5]
					&& *(p2+6) == HTTP_METHOD_OPTIONS[6] )
					;
				else
					return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			else
			{
				return FASTERHTTP_ERROR_METHOD_INVALID;
			}
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI0 ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_URI->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_URI\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_URI->value_len = p - p_URI->value_ptr ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION0 ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_VERSION->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION ;
			
		case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_REQUESTSTARTLINE_VERSION\n" );
#endif
			
			for( ; LIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p_VERSION->value_len = p - p_VERSION->value_ptr ;
			if( LIKELY( *(p-1) == HTTP_RETURN ) )
				p_VERSION->value_len--;
			p++;
			
			p2 = p_VERSION->value_ptr ;
			if( LIKELY( p_VERSION->value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
			{
				if( LIKELY( *(p2) == HTTP_VERSION_1_0[0] && *(p2+1) == HTTP_VERSION_1_0[1] && *(p2+2) == HTTP_VERSION_1_0[2]
					&& *(p2+3) == HTTP_VERSION_1_0[3] && *(p2+4) == HTTP_VERSION_1_0[4] && *(p2+5) == HTTP_VERSION_1_0[5]
					&& *(p2+6) == HTTP_VERSION_1_0[6] ) )
				{
					if( UNLIKELY( *(p2+7) == HTTP_VERSION_1_0[7] ) )
						*(p_version) = HTTP_VERSION_1_0_N ;
					else if( LIKELY( *(p2+7) == HTTP_VERSION_1_1[7] ) )
						*(p_version) = HTTP_VERSION_1_1_N ;
					else
						return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
				}
			}
			else
			{
				return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
			}
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_VERSION->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_VERSION\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_VERSION->value_len = p - p_VERSION->value_ptr ;
			p++;
			
			p2 = p_VERSION->value_ptr ;
			if( LIKELY( p_VERSION->value_len == sizeof(HTTP_VERSION_1_0)-1 ) )
			{
				if( LIKELY( *(p2) == HTTP_VERSION_1_0[0] && *(p2+1) == HTTP_VERSION_1_0[1] && *(p2+2) == HTTP_VERSION_1_0[2]
					&& *(p2+3) == HTTP_VERSION_1_0[3] && *(p2+4) == HTTP_VERSION_1_0[4] && *(p2+5) == HTTP_VERSION_1_0[5]
					&& *(p2+6) == HTTP_VERSION_1_0[6] ) )
				{
					if( UNLIKELY( *(p2+7) == HTTP_VERSION_1_0[7] ) )
						*(p_version) = HTTP_VERSION_1_0_N ;
					else if( LIKELY( *(p2+7) == HTTP_VERSION_1_1[7] ) )
						*(p_version) = HTTP_VERSION_1_1_N ;
					else
						return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
				}
			}
			else
			{
				return FASTERHTTP_ERROR_VERSION_NOT_SUPPORTED;
			}
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE0 ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_STATUSCODE->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_STATUSCODE\n" );
#endif
			
			for( ; LIKELY( (*p) != ' ' && (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_STATUSCODE->value_len = p - p_STATUSCODE->value_ptr ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE0 ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE0\n" );
#endif
			
			for( ; UNLIKELY( (*p) == ' ' && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADERSTARTLINE_INVALID;
			p_REASONPHRASE->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE ;
			
		case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_RESPONSESTARTLINE_REASONPHRASE\n" );
#endif
			
			for( ; LIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
				;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p_REASONPHRASE->value_len = p - p_REASONPHRASE->value_ptr ;
			if( LIKELY( *(p-1) == HTTP_RETURN ) )
				p_REASONPHRASE->value_len--;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_HEADER_NAME0 :
			
_GOTO_PARSESTEP_HEADER_NAME0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_NAME0\n" );
#endif
			
			/*
			while( UNLIKELY( (*p) == ' ' && p < fill_ptr ) )
				p++;
			*/
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			else if( LIKELY( (*p) == HTTP_RETURN ) )
			{
				if( UNLIKELY( p+1 >= fill_ptr ) )
				{
					return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
				}
				else if( LIKELY( *(p+1) == HTTP_NEWLINE ) )
				{
					p += 2 ;
				}
				else
				{
					p++;
				}
				

#define _IF_THEN_GO_PARSING_BODY \
				if( *(p_content_length) > 0 ) \
				{ \
					e->body = p ; \
					*(p_parse_step) = FASTERHTTP_PARSESTEP_BODY ; \
					goto _GOTO_PARSESTEP_BODY; \
				} \
				else if( *(p_transfer_encoding__chunked) == 1 ) \
				{ \
					e->body = p ; \
					e->chunked_body_end = p ; \
					*(p_parse_step) = FASTERHTTP_PARSESTEP_CHUNKED_SIZE ; \
					goto _GOTO_PARSESTEP_CHUNKED_SIZE; \
				} \
				else \
				{ \
					b->process_ptr = p ; \
					*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ; \
					return 0; \
				}
				
				_IF_THEN_GO_PARSING_BODY
			}
			else if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
			{
				p++;
				
				_IF_THEN_GO_PARSING_BODY
			}
			else
			{
				p_header->name_ptr = p ;
				p++;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME ;
			}
			
		case FASTERHTTP_PARSESTEP_HEADER_NAME :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_NAME\n" );
#endif
			
			while( LIKELY( (*p) != ' ' && (*p) != ':' && (*p) != HTTP_NEWLINE && p < fill_ptr ) )
				p++;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			p_header->name_len = p - p_header->name_ptr ;
			
			while( UNLIKELY( (*p) != ':' && (*p) != HTTP_NEWLINE && p < fill_ptr ) )
				p++;
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			else if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_VALUE0 ;
			
		case FASTERHTTP_PARSESTEP_HEADER_VALUE0 :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_VALUE0\n" );
#endif
			
			while( LIKELY( (*p) == ' ' && p < fill_ptr ) )
				p++;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			if( UNLIKELY( (*p) == HTTP_NEWLINE ) )
				return FASTERHTTP_ERROR_HTTP_HEADER_INVALID;
			p_header->value_ptr = p ;
			p++;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_VALUE ;
			
		case FASTERHTTP_PARSESTEP_HEADER_VALUE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_HEADER_VALUE\n" );
#endif
			
			while( LIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) )
				p++;
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			p_header->value_len = p - p_header->value_ptr ;
			p2 = p - 1 ;
			while( LIKELY( (*p2) == ' ' || (*p2) == HTTP_RETURN ) )
			{
				p_header->value_len--;
				p2--;
			}
			p++;
			
			if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_CONTENT_LENGTH , sizeof(HTTP_HEADER_CONTENT_LENGTH)-1 ) ) )
			{
				*(p_content_length) = strtol( p_header->value_ptr , NULL , 10 ) ;
			}
			else if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_TRANSFERENCODING)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_TRANSFERENCODING , sizeof(HTTP_HEADER_TRANSFERENCODING)-1 ) ) && UNLIKELY( p_header->value_len == sizeof(HTTP_HEADER_TRANSFERENCODING__CHUNKED)-1 && STRNICMP( p_header->value_ptr , == , HTTP_HEADER_TRANSFERENCODING__CHUNKED , sizeof(HTTP_HEADER_TRANSFERENCODING__CHUNKED)-1 ) ) )
			{
				*(p_transfer_encoding__chunked) = 1 ;
			}
			else if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_TRAILER)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_TRAILER , sizeof(HTTP_HEADER_TRAILER)-1 ) ) )
			{
				p_TRAILER->value_ptr = p_header->value_ptr ;
				p_TRAILER->value_len = p_header->value_len ;
			}
			else if( UNLIKELY( p_header->name_len == sizeof(HTTP_HEADER_CONNECTION)-1 && STRNICMP( p_header->name_ptr , == , HTTP_HEADER_CONNECTION , sizeof(HTTP_HEADER_CONNECTION)-1 ) ) )
			{
				if( LIKELY( p_header->value_len == sizeof(HTTP_HEADER_CONNECTION__KEEPALIVE)-1 && STRNICMP( p_header->value_ptr , == , HTTP_HEADER_CONNECTION__KEEPALIVE , sizeof(HTTP_HEADER_CONNECTION__KEEPALIVE)-1 ) ) )
					*(p_connection__keepalive) = 1 ;
				else if( LIKELY( p_header->value_len == sizeof(HTTP_HEADER_CONNECTION__CLOSE)-1 && STRNICMP( p_header->value_ptr , == , HTTP_HEADER_CONNECTION__CLOSE , sizeof(HTTP_HEADER_CONNECTION__CLOSE)-1 ) ) )
					*(p_connection__keepalive) = -1 ;
				else
					*(p_connection__keepalive) = 0 ;
			}
			e->headers.header_array_count++;
			
			if( *(p_transfer_encoding__chunked) == 1 && *(p_content_length) > 0 && UNLIKELY( p_header->name_len == p_TRAILER->value_len && STRNICMP( p_header->name_ptr , == , p_TRAILER->value_ptr , p_header->name_len ) ) )
			{
				b->process_ptr = p ;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ;
				if( b->fill_ptr > b->process_ptr )
					e->reforming_flag = 1 ;
				return 0;
			}
			
			if( UNLIKELY( e->headers.header_array_count+1 > e->headers.header_array_size ) )
			{
				int			new_header_array_size ;
				struct HttpHeader	*new_header_array ;
				
				new_header_array_size = e->headers.header_array_size * 2 ;
				new_header_array = (struct HttpHeader *)realloc( e->headers.header_array , sizeof(struct HttpHeader) * new_header_array_size ) ;
				if( new_header_array == NULL )
					return FASTERHTTP_ERROR_ALLOC;
				memset( new_header_array + e->headers.header_array_count-1 , 0x00 , sizeof(struct HttpHeader) * (new_header_array_size-e->headers.header_array_count+1) );
				e->headers.header_array_size = new_header_array_size ;
				e->headers.header_array = new_header_array ;
			}
			p_header = & (e->headers.header_array[e->headers.header_array_count]) ;
			
			*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
			goto _GOTO_PARSESTEP_HEADER_NAME0;
			
		case FASTERHTTP_PARSESTEP_BODY :
			
_GOTO_PARSESTEP_BODY :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_BODY\n" );
#endif
			
			if( LIKELY( fill_ptr - e->body >= *(p_content_length) ) )
			{
				b->process_ptr = e->body + *(p_content_length) ;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ;
				if( b->fill_ptr > b->process_ptr )
					e->reforming_flag = 1 ;
printf( "e->reforming_flag[%d]\n" , e->reforming_flag );
				return 0;
			}
			else
			{
				b->process_ptr = fill_ptr ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			
		case FASTERHTTP_PARSESTEP_CHUNKED_SIZE :
			
_GOTO_PARSESTEP_CHUNKED_SIZE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_CHUNKED_SIZE\n" );
#endif
			
			for( ; UNLIKELY( (*p) != HTTP_NEWLINE && p < fill_ptr ) ; p++ )
			{
				if( LIKELY( '0' <= (*p) && (*p) <= '9' ) )
					e->chunked_length = e->chunked_length * 10 + (*p) - '0' ;
				else if( 'a' <= (*p) && (*p) <= 'f' )
					e->chunked_length = e->chunked_length * 10 + (*p) - 'a' + 10 ;
				else if( 'A' <= (*p) && (*p) <= 'F' )
					e->chunked_length = e->chunked_length * 10 + (*p) - 'A' + 10 ;
				e->chunked_length_length++;
			}
			if( UNLIKELY( p >= fill_ptr ) )
			{
				b->process_ptr = p ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			e->chunked_length_length++;
			p++;
			if( e->chunked_length == 0 )
			{
				if( p_TRAILER->value_ptr )
				{
					*(p_parse_step) = FASTERHTTP_PARSESTEP_HEADER_NAME0 ;
					goto _GOTO_PARSESTEP_HEADER_NAME0;
				}
				
				b->process_ptr = p ;
				*(p_parse_step) = FASTERHTTP_PARSESTEP_DONE ;
				if( b->fill_ptr > b->process_ptr )
					e->reforming_flag = 1 ;
				return 0;
			}
			
			e->chunked_body = p ;
			*(p_parse_step) = FASTERHTTP_PARSESTEP_CHUNKED_DATA ;
			
		case FASTERHTTP_PARSESTEP_CHUNKED_DATA :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_CHUNKED_DATA\n" );
#endif
			
			if( LIKELY( fill_ptr - e->chunked_body >= e->chunked_length + 2 ) )
			{
				memmove( e->chunked_body_end , e->chunked_body , e->chunked_length );
				p = e->chunked_body + e->chunked_length ;
				e->chunked_body_end = e->chunked_body_end + e->chunked_length ;
				if( (*p) == '\r' && *(p+1) == '\n' )
					p+=2;
				else if( (*p) == '\n' )
					p++;
				
				*(p_content_length) += e->chunked_length ;
				
				*(p_parse_step) = FASTERHTTP_PARSESTEP_CHUNKED_SIZE ;
				e->chunked_length = 0 ;
				e->chunked_length_length = 0 ;
				goto _GOTO_PARSESTEP_CHUNKED_SIZE;
			}
			else
			{
				b->process_ptr = fill_ptr ;
				return FASTERHTTP_INFO_NEED_MORE_HTTP_BUFFER;
			}
			
		case FASTERHTTP_PARSESTEP_DONE :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> case FASTERHTTP_PARSESTEP_DONE\n" );
#endif
			
			return 0;

		default :
#if DEBUG_PARSE
			printf( "DEBUG_PARSE >>> default\n" );
#endif
			return FASTERHTTP_ERROR_INTERNAL;
	}
}

