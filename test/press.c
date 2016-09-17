#include "fasterhttp.h"

#define DEBUG		0

#define REQ                                                                                                                        \
    "GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg HTTP/1.1\r\n"                                                \
    "Host: www.kittyhell.com\r\n"                                                                                                  \
    "User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "             \
    "Pathtraq/0.9\r\n"                                                                                                             \
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"                                                  \
    "Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"                                                                                 \
    "Accept-Encoding: gzip,deflate\r\n"                                                                                            \
    "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"                                                                            \
    "Keep-Alive: 115\r\n"                                                                                                          \
    "Connection: keep-alive\r\n"                                                                                                   \
    "Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "                                                          \
    "__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "                                                             \
    "__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor.com|utmcct=/reader/|utmcmd=referral\r\n"             \
    "\r\n"

static int press( int count )
{
	struct HttpEnv		*e = NULL ;
	int			i ;
	struct HttpBuffer	*b = NULL ;
	int			nret = 0 ;
	
	e = CreateHttpEnv() ;
	if( e == NULL )
	{
		printf( "CreateHttpEnv failed\n" );
		return -1;
	}
	
#if DEBUG
	b = GetHttpRequestBuffer( e ) ;
	SetHttpBufferPtr( b , REQ , sizeof(REQ) );
	
	nret = ParseHttpRequest( e ) ;
	if( nret )
	{
		printf( "%s:%d | press failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		if( GetHttpHeaderCount( e ) > 0 )
		{
			struct HttpHeader *p_header = NULL ;
			
			p_header = TravelHttpHeaderPtr( e , NULL ) ;
			while( p_header )
			{
				printf( "HTTP HREADER [%.*s] [%.*s]\n" , GetHttpHeaderNameLen(p_header) , GetHttpHeaderNamePtr(p_header,NULL) , GetHttpHeaderValueLen(p_header) , GetHttpHeaderValuePtr(p_header,NULL) );
				p_header = TravelHttpHeaderPtr( e , p_header ) ;
			}
		}
		
		if( GetHttpBodyLen( e ) > 0 )
		{
			printf( "HTTP BODY [%.*s]\n" , GetHttpBodyLen(e) , GetHttpBodyPtr(e,NULL) );
		}
	}
#endif
	
	for( i = 0 ; i < count ; i++ )
	{
		ResetHttpEnv( e );
		
		b = GetHttpRequestBuffer( e ) ;
		SetHttpBufferPtr( b , sizeof(REQ) , REQ ) ;
		
		nret = ParseHttpRequest( e ) ;
		if( UNLIKELY(nret) )
		{
			printf( "ParseHttpRequest failed[%d]\n" , nret );
			DestroyHttpEnv( e );
			return -1;
		}
	}
	
	DestroyHttpEnv( e );
	
	return 0;
}

int main( int argc , char *argv[] )
{
	if( argc == 1 + 1 )
	{
		return -press( atoi(argv[1]) );
	}
	else
	{
		printf( "USAGE : press count\n" );
		exit(9);
	}
}

