#include "fasterhttp.h"

void test_SplitHttpUri( char *uri )
{
	struct HttpUri	httpuri ;
	
	int		nret = 0 ;
	
	memset( & httpuri , 0x00 , sizeof(struct HttpUri) );
	nret = SplitHttpUri( "." , uri , strlen(uri) , & httpuri ) ;
	if( nret )
	{
		printf( "SplitHttpUri[%s] failed[%d]\n" , uri , nret );
	}
	else
	{
		printf( "------------------------------\n" );
		printf( "          uri[%s]\n" , uri );
		printf( "     pathname[%.*s]\n" , httpuri.dirname_len , httpuri.dirname_base );
		printf( "     filename[%.*s]\n" , httpuri.filename_len , httpuri.filename_base );
		printf( "main_filename[%.*s]\n" , httpuri.main_filename_len , httpuri.main_filename_base );
		printf( " ext_filename[%.*s]\n" , httpuri.ext_filename_len , httpuri.ext_filename_base );
		printf( "        param[%.*s]\n" , httpuri.param_len , httpuri.param_base );
	}
	
	return;
}

int main()
{
	test_SplitHttpUri( "vc6" );
	test_SplitHttpUri( "test_SplitHttpUri.c" );
	test_SplitHttpUri( "test_SplitHttpUri.c?id=123" );
	test_SplitHttpUri( "makefile" );
	test_SplitHttpUri( "makefile?id=123" );
	
	test_SplitHttpUri( "/vc6" );
	test_SplitHttpUri( "/test_SplitHttpUri.c" );
	test_SplitHttpUri( "/test_SplitHttpUri.c?id=123" );
	
	test_SplitHttpUri( "/vc6/vc6.dsw" );
	test_SplitHttpUri( "/vc5/vc6.dsw" );
	test_SplitHttpUri( "/vc5/vc6.dsw?id=123" );
	
	test_SplitHttpUri( "/" );
	test_SplitHttpUri( "?id=123" );
	test_SplitHttpUri( "/vc6/?id=123" );
	
	return 0;
}

