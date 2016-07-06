/*
 * fasterhttp
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "internal.h"
#include "fasterhttp.h"

struct HttpBuffer *GetHttpRequestBuffer( struct HttpEnv *e )
{
	return &(e->request_buffer);
}

struct HttpBuffer *GetHttpResponseBuffer( struct HttpEnv *e )
{
	return &(e->response_buffer);
}

char *GetHttpBufferBase( struct HttpBuffer *b )
{
	return b->base;
}

int GetHttpBufferLength( struct HttpBuffer *b )
{
	return b->fill_ptr-b->base;
}

char *GetHttpBufferFillPtr( struct HttpBuffer *b )
{
	return b->fill_ptr;
}

int GetHttpBufferRemainLength( struct HttpBuffer *b )
{
	return b->buf_size-1-(b->fill_ptr-b->base);
}

void OffsetHttpBufferFillPtr( struct HttpBuffer *b , int len )
{
	b->fill_ptr += len ;
	return;
}

void ReformingHttpBuffer( struct HttpBuffer *b )
{
	int	len = b->fill_ptr - b->process_ptr ;
	memmove( b->base , b->process_ptr , len );
	b->process_ptr = b->base ;
	b->fill_ptr = b->base + len ;
	
	return;
}

void CleanHttpBuffer( struct HttpBuffer *b )
{
	b->fill_ptr = b->base ;
	b->process_ptr = b->base ;
	
	return;
}

int ReallocHttpBuffer( struct HttpBuffer *b , long new_buf_size )
{
	char	*new_base = NULL ;
	int	fill_len = b->fill_ptr - b->base ;
	int	process_len = b->process_ptr - b->base ;
	
	if( b->ref_flag == 1 )
		return FASTERHTTP_ERROR_USING;
	
	if( new_buf_size == -1 )
	{
		if( b->buf_size <= 100*1024*1024 )
			new_buf_size = b->buf_size * 2 ;
		else
			new_buf_size = 100*1024*1024 ;
	}
	new_base = (char *)realloc( b->base , new_buf_size ) ;
	if( new_base == NULL )
		return FASTERHTTP_ERROR_ALLOC;
	memset( new_base + fill_len , 0x00 , new_buf_size - fill_len );
	b->buf_size = new_buf_size ;
	b->base = new_base ;
	b->fill_ptr = b->base + fill_len ;
	b->process_ptr = b->base + process_len ;
	
	return 0;
}

int StrcpyHttpBuffer( struct HttpBuffer *b , char *str )
{
	int		len ;
	
	int		nret = 0 ;
	
	len = strlen(str) ;
	while( len > b->buf_size-1 )
	{
		nret = ReallocHttpBuffer( b , -1 ) ;
		if( nret )
			return nret;
	}
	
	memcpy( b->base , str , len );
	b->fill_ptr = b->base + len ;
	
	return 0;
}

int StrcpyfHttpBuffer( struct HttpBuffer *b , char *format , ... )
{
	va_list		valist ;
	int		len ;
	
	int		nret = 0 ;
	
	while(1)
	{
		va_start( valist , format );
		len = VSNPRINTF( b->base , b->buf_size-1 , format , valist ) ;
		va_end( valist );
		if( len == -1 || len == b->buf_size-1 )
		{
			nret = ReallocHttpBuffer( b , -1 ) ;
			if( nret )
				return nret;
		}
		else
		{
			b->fill_ptr = b->base + len ;
			break;
		}
	}
	
	return 0;
}

int StrcpyvHttpBuffer( struct HttpBuffer *b , char *format , va_list valist )
{
	int		len ;
	
	int		nret = 0 ;
	
	while(1)
	{
		len = VSNPRINTF( b->base , b->buf_size-1 , format , valist ) ;
		if( len == -1 || len == b->buf_size-1 )
		{
			nret = ReallocHttpBuffer( b , -1 ) ;
			if( nret )
				return nret;
		}
		else
		{
			b->fill_ptr = b->base + len ;
			break;
		}
	}
	
	return 0;
}

int StrcatHttpBuffer( struct HttpBuffer *b , char *str )
{
	long		len ;
	
	int		nret = 0 ;
	
	len = strlen(str) ;
	while( (b->fill_ptr-b->base) + len > b->buf_size-1 )
	{
		nret = ReallocHttpBuffer( b , -1 ) ;
		if( nret )
			return nret;
	}
	
	memcpy( b->fill_ptr , str , len );
	b->fill_ptr += len ;
	
	return 0;
}

int StrcatfHttpBuffer( struct HttpBuffer *b , char *format , ... )
{
	va_list		valist ;
	long		len ;
	
	int		nret = 0 ;
	
	while(1)
	{
		va_start( valist , format );
		len = VSNPRINTF( b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) , format , valist ) ;
		va_end( valist );
		if( len == -1 || len == b->buf_size-1 - (b->fill_ptr-b->base) )
		{
			nret = ReallocHttpBuffer( b , -1 ) ;
			if( nret )
				return nret;
		}
		else
		{
			b->fill_ptr += len ;
			break;
		}
	}
	
	return 0;
}

int StrcatvHttpBuffer( struct HttpBuffer *b , char *format , va_list valist )
{
	int		len ;
	
	int		nret = 0 ;
	
	while(1)
	{
		len = VSNPRINTF( b->fill_ptr , b->buf_size-1 - (b->fill_ptr-b->base) , format , valist ) ;
		if( len == -1 || len == b->buf_size-1 - (b->fill_ptr-b->base) )
		{
			nret = ReallocHttpBuffer( b , -1 ) ;
			if( nret )
				return nret;
		}
		else
		{
			b->fill_ptr += len ;
			break;
		}
	}
	
	return 0;
}

int StrcatHttpBufferFromFile( struct HttpBuffer *b , char *pathfilename , int *p_filesize )
{
	int		filesize = -1 ;
	int		new_buf_size ;
	FILE		*fp = NULL ;
	
	int		nret = 0 ;
	
	if( p_filesize )
		filesize = (*p_filesize) ;
		
	if( filesize == -1 )
	{
		struct stat	st ;
		
		nret = stat( pathfilename , & st ) ;
		if( nret == -1 )
			return FASTERHTTP_ERROR_FILE_NOT_FOUND;
		filesize = st.st_size ;
	}
	
	if( filesize > 0 )
	{
		new_buf_size = (b->fill_ptr-b->base) + filesize + 1 ;
		if( new_buf_size > b->buf_size )
		{
			nret = ReallocHttpBuffer( b , new_buf_size ) ;
			if( nret )
				return nret;
		}
		
		fp = fopen( pathfilename , "r" ) ;
		if( fp == NULL )
			return FASTERHTTP_ERROR_FILE_NOT_FOUND;
		
		nret = fread( b->fill_ptr , filesize , 1 , fp ) ;
		if( nret == -1 )
			return FASTERHTTP_ERROR_FILE_NOT_FOUND;
		
		fclose( fp );
		
		b->fill_ptr += filesize ;
	}
	
	if( p_filesize )
		(*p_filesize) = filesize ;
	
	return 0;
}

int MemcatHttpBuffer( struct HttpBuffer *b , char *base , long len )
{
	int		nret = 0 ;
	
	while( (b->fill_ptr-b->base) + len > b->buf_size-1 )
	{
		nret = ReallocHttpBuffer( b , -1 ) ;
		if( nret )
			return nret;
	}
	
	memcpy( b->fill_ptr , base , len );
	b->fill_ptr += len ;
	
	return 0;
}

void SetHttpBufferPtr( struct HttpBuffer *b , char *ptr , long size )
{
	if( b->ref_flag == 0 && b->buf_size > 0 && b->base )
	{
		free( b->base );
	}
	
	b->base = ptr ;
	b->buf_size = size ;
	b->fill_ptr = b->base + size-1 ;
	b->process_ptr = b->base ;
	
	b->ref_flag = 1 ;
	
	return;
}

