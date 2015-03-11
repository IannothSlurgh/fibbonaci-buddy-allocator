#define _GNU_SOURCE

#include "ackerman.h"
#include "my_allocator.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void exit_handler( void )
{
	unsigned int released_mem = release_allocator();
}

int main(int argc, char ** argv) {

	atexit( exit_handler );

	extern char * optarg;
	extern int optind;
	int c;
	char * bsize="128";//Default of 128 b
	char * memsize="52488"; //Default of 512 kb
	while ( ( c = getopt( argc, argv, "b:s:" ) ) != -1 )
	{
		switch( c )
		{
			case 'b':
				bsize = optarg;
				break;
			case 's':
				memsize = optarg;
				break;
			case '?':
				printf( "Incorrect arguments.\n" );
				return 0;
				break;
		}
	}
	int basic_block_size = atoi( bsize );
	int memory_length = atoi( memsize );
	
  init_allocator( basic_block_size, memory_length );

  ackerman_main();
}
