#include "fasterhttp.h"

void test_SplitHttpUri( char *uri )
{
	char		*pathname_base = NULL ;
	int		pathname_len = 0 ;
	char		*filename_base = NULL ;
	int		filename_len = 0 ;
	char		*main_filename_base = NULL ;
	int		main_filename_len = 0 ;
	char		*ext_filename_base = NULL ;
	int		ext_filename_len = 0 ;
	char		*param_base = NULL ;
	int		param_len = 0 ;
	
	int		nret = 0 ;
	
	nret = SplitHttpUri( "." , uri , strlen(uri) , & pathname_base , & pathname_len , & filename_base , & filename_len , & main_filename_base , & main_filename_len , & ext_filename_base , & ext_filename_len , & param_base , & param_len ) ;
	if( nret )
	{
		printf( "SplitHttpUri[%s] failed[%d]\n" , uri , nret );
	}
	else
	{
		printf( "------------------------------\n" );
		printf( "          uri[%s]\n" , uri );
		printf( "     pathname[%.*s]\n" , pathname_len , pathname_base );
		printf( "     filename[%.*s]\n" , filename_len , filename_base );
		printf( "main_filename[%.*s]\n" , main_filename_len , main_filename_base );
		printf( " ext_filename[%.*s]\n" , ext_filename_len , ext_filename_base );
		printf( "        param[%.*s]\n" , param_len , param_base );
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
	
	test_SplitHttpUri( "?id=123" );
	test_SplitHttpUri( "/vc6/?id=123" );
	
	return 0;
}

