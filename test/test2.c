#include "fasterhttp.h"

static int TestParseHttpRequest( struct HttpEnv *e , char *format , ... )
{
	struct HttpBuffer	*b = NULL ;
	va_list			valist ;
	
	int			nret = 0 ;
	
	ResetHttpEnv( e );
	
	b = GetHttpRequestBuffer( e ) ;
	
	va_start( valist , format );
	nret = StrcatvHttpBuffer( b , format , valist ) ;
	va_end( valist );
	if( nret )
	{
		printf( "StrcatvHttpBuffer failed[%d]\n" , nret );
		return -1;
	}
	
	nret = ParseHttpRequest( e ) ;
	if( nret == 0 )
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
	return nret;
}

static int test()
{
	struct HttpEnv		*e = NULL ;
	
	int			nret = 0 ;
	e = CreateHttpEnv() ;
	if( e == NULL )
	{
		printf( "CreateHttpEnv failed\n" );
		return -1;
	}
	
	nret = TestParseHttpRequest( e , "GET / HTTP/1.1\r\n\r\n" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "GET / HTTP/1.1\r\n"
					"Host: www.baidu.com\r\n"
					"User-Agent: Mozilla/5.0 (Windows NT 5.1; rv:45.0) Gecko/20100101 Firefox/45.0\r\n"
					"Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
					"Accept-Encoding: gzip, deflate, br\r\n"
					"Cookie: BAIDUID=0E27B789D33BF3C43C6022BD0182CF8D:SL=0:NR=10:FG=1; BIDUPSID=EE65333C3C1B7FB4807F6DC5DE576979; PSTM=1462883721; BD_UPN=13314152; ispeed_lsm=2; MCITY=-179%3A; BDUSS=t4TW1VRFNsMm91bGtTcUFHbVFqfnhiVFVYd2ZKZFc2c0dGaG12VmhZckZJbmxYQVFBQUFBJCQAAAAAAAAAAAEAAADIZsc0Y2FsdmlubGljaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMWVUVfFlVFXSG; pgv_pvi=56303616; BD_HOME=1; H_PS_PSSID=19290_1436_18240_20076_17001_15790_12201_20254; sug=3; sugstore=0; ORIGIN=2; bdime=0; __bsi=13900513390515515511_00_0_I_R_33_0303_C02F_N_I_I_0\r\n"
					"Connection: keep-alive\r\n"
					"Cache-Control: max-age=0\r\n"
					"\r\n" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "POST / HTTP/1.1\r\n"
					"Content-Length: 3\r\n"
					"\r\n"
					"xyz" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "GET /index.html HTTP/1.1\n\n" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "POST / HTTP/1.1\r\n"
					"Transfer-Encoding: chunked\r\n"
					"\r\n"
					"1\r\n"
					"x\r\n"
					"2\r\n"
					"yz\r\n"
					"0\r\n" );
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	nret = TestParseHttpRequest( e , "POST / HTTP/1.1\r\n"
					"Transfer-Encoding: chunked\r\n"
					"Trailer: Content-MD5\r\n"
					"\r\n"
					"1\r\n"
					"a\r\n"
					"2\r\n"
					"bc\r\n"
					"0\r\n"
					"Content-MD5: 1234567890ABCDEF\r\n" ) ;
	if( nret )
	{
		printf( "%s:%d | test failed[%d]\n" , __FILE__ , __LINE__ , nret );
		DestroyHttpEnv( e );
		return -1;
	}
	else
	{
		printf( "%s:%d | test ok[%d]\n" , __FILE__ , __LINE__ , nret );
	}
	
	DestroyHttpEnv( e );
	
	printf( "ALL test is ok!!!\n" );
	
	return 0;
}

int main()
{
	return -test();
}

