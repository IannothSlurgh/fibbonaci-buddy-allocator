/* 
    File: my_allocator.c

    Author: Ian DaCosta
            Department of Computer Science
            Texas A&M University
    Date  : 2/7/14

    Modified: 

    This file contains the implementation of the module "MY_ALLOCATOR".

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    #define KB_SIZE 1024 //Size of KB in bytes.
    #define SECURITY 0x1555 //13 bit alternating pattern beginning with 1.
    #define LR_ONLY 0x4
    #define INHERITANCE_ONLY 0x2
    #define FREE_ONLY 0x1
   
	#define ERR_SUCCESS 0x0
	#define ERR_BOOL_SIZE 0x1
	#define ERR_BROKEN_LINK 0x2
	#define ERR_DUPLICATE_LINK 0x3
	#define ERR_NO_FIRST_OR_NO_LAST 0x4
	#define ERR_NULL_POINTER 0x5
   
/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include<stdlib.h>
#include "my_allocator.h"
#include <stdio.h>

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

static Hdr ** free_list = 0;
static Addr mem_block = 0;
static Addr mem_block_tail = 0;
static unsigned int basic_block_size = 0;
static unsigned int num_free_list = 0;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FUNCTIONS FOR MODULE MY_ALLOCATOR */
/*--------------------------------------------------------------------------*/

unsigned int fib( unsigned int term ) //fib is unsafe given very large term
{
	switch( term )
	{
		case 0:
			return 1;
		break;
		case 1:
			return 2;
		break;
		default:
			return fib( term - 1 ) + fib( term - 2 );
	}
	return 0;
}

unsigned int has_valid_header( Hdr * h ) //Does h have special bit pattern?
{
	if( h == 0 ) //Null ptr
	{
		return 0;
	}
	unsigned int _info = h -> info;
	_info >>= 3;
	if( _info == SECURITY )
	{
		return 1;
	}
	return 0;
}

unsigned int configure_header( Hdr * h, unsigned int lr,
	unsigned int inheritance, unsigned int free, unsigned int _size )
{
	if(lr < 2 && inheritance < 2 && free < 2) //Single bits
	{
		h -> size = _size;
		unsigned int _info = SECURITY;
		_info <<= 3;
		_info |= ( lr << 2 );
		_info |= ( inheritance << 1 );
		_info |= free;
		h -> info = _info;
		return ERR_SUCCESS;
	}
	else
	{
		return ERR_BOOL_SIZE;
	}
}

unsigned int configure_header_long( Hdr * h, unsigned int lr,
	unsigned int inheritance, unsigned int free, unsigned int _size, 
	Hdr * _next, Hdr * _prev )
{
	unsigned int result = configure_header( h, lr, inheritance, free, _size );
	if( result == ERR_SUCCESS )
	{
		h -> next = _next;
		h -> prev = _prev;
	}
	return result;
}
	

unsigned int destroy_header( Hdr * h )
{
	h -> size = 0;
	h -> prev = 0;
	h -> next = 0;
	h -> info = 0;
}

unsigned int is_right_buddy( Hdr * h )
{
	unsigned _info = h -> info;
	_info &= LR_ONLY;
	_info >>= 2;
	return _info;
}

unsigned int is_free( Hdr * h )
{
	unsigned _info = h -> info;
	_info &= FREE_ONLY;
	return _info;
}

unsigned int get_inheritance( Hdr * h )
{
	unsigned _info = h -> info;
	_info &= INHERITANCE_ONLY;
	_info >>= 1;
	return _info;
}

unsigned int insert( Hdr * h )
{
	unsigned int size = h -> size;
	unsigned int free_list_index = 2 * size;
	unsigned int no_first = ( free_list[ free_list_index ] == 0 );
	unsigned int no_last = ( free_list[ free_list_index + 1 ] == 0 );
	Hdr * compare = 0;
	Hdr * last = 0;
	Hdr * first = 0;
	if( no_first && no_last )
	{
		free_list[ free_list_index ] = h;
		free_list[ free_list_index + 1 ] = h;
		h -> prev = 0;
		h -> next = 0;
	}
	else
	{
		if( !( no_first || no_last ) )
		{
			compare = free_list[ free_list_index ];
			last = free_list[ free_list_index + 1 ];
			first = free_list[ free_list_index ];
			while( h > compare && compare != last )
			{
				if( compare -> next == 0 )
				{
					return ERR_BROKEN_LINK;
				}
				else
				{
					compare = compare -> next;
				}
				
			}
			if( h == compare )
			{
				return ERR_DUPLICATE_LINK;
			}
			else
			{
				if( first == last )//Currently one item in particular list.
				{
					if( h > compare )
					{
						compare -> next = h;
						h -> next = 0;
						h -> prev = compare;
						free_list[ free_list_index + 1 ] = h;
					}
					else
					{
						compare -> prev = h;
						h -> next = compare;
						h -> prev = 0;
						free_list[ free_list_index ] = h;
					}
				}
				else
				{
					if( h > last ) //End of multi-item list
					{
						last -> next = h;
						h -> prev = last;
						h -> next = 0;
						free_list[ free_list_index + 1 ] = h;
						
					}
					else
					{
						if( h < first )
						{
							h -> next = first;
							h -> prev = 0;
							first -> prev = h;
							free_list[ free_list_index ] = h;
						}
						else /*connect list non single item list*/
						{
							h -> next = compare;
							h -> prev = compare -> prev;
							compare -> prev = h;
							h -> prev -> next = h;
						}
					}
				}
			}
		}
		else 
		{
			return ERR_NO_FIRST_OR_NO_LAST;
		}
	}
}

unsigned int eject( Hdr * h ) //Remove from linked list structure.
{
	unsigned int size = h -> size;
	unsigned int free_list_index = 2 * size;
	if( h -> next == 0 && h -> prev == 0 ) //Size 1 free_list
	{
		free_list[ free_list_index ] = 0; //No first free block.
		free_list[ free_list_index + 1 ] = 0; // No last free block.
	}
	else
	{
		if( h -> prev == 0 ) //The first available block of a free_list
		{
			h -> next -> prev = 0;
			free_list[ free_list_index ] = h -> next; //next is now first block.
		}
		else
		{
			if( h -> next == 0 ) //The last available block
			{
				h -> prev -> next = 0;
				free_list[ free_list_index + 1 ] = h -> prev; //prev is now last block.
			}
			else //A middle block
			{
				h -> prev -> next = h -> next; //Connect h_prev to next.
				h -> next -> prev = h -> prev; //Connect h_next to prev.
			}
		}
	}
	h -> next = 0;
	h -> prev = 0;	
}

/*By convention, right block size will always be greater than left block size*/
unsigned int split( Hdr * h, unsigned int left_block_size, unsigned int right_block_size )
{
	if( has_valid_header( h ) && is_free( h ) )
	{
		//If is not atomic (size 0) and if the fib terms represented by the sizes add up 
		if( h -> size > 0 && fib( left_block_size ) + fib( right_block_size ) == fib( h -> size ) )
		{
			eject( h );
			char * ptr_arithmetic = ( ( char * ) h ) + ( fib( left_block_size ) * basic_block_size );
			Hdr * h_right = ( Hdr * ) ptr_arithmetic;
			configure_header( ( h_right ), 1, get_inheritance( h ), 1, right_block_size );
			configure_header( ( h ), 0, is_right_buddy( h ), 1, left_block_size );
			insert( h_right );
			insert( h );
		}
	}
}

unsigned int coalesce( Hdr * h_left, Hdr * h_right )
{
	if( has_valid_header( h_left ) && has_valid_header( h_right ) )
	{
		eject( h_right );
		eject( h_left );
		unsigned int lr = get_inheritance( h_left );
		unsigned int inheritance = get_inheritance( h_right );
		unsigned int size = 0;
		if( h_right -> size == 0 && h_left -> size == 0 )
		{
			size = 1;
		}
		else
		{
			size = h_left -> size + 2;
		}
		configure_header( ( h_left), lr, inheritance, 1, size );
		destroy_header( h_right );
		insert( h_left );
	}
}

Hdr * find_buddy( Hdr * h )//SELF: finish.
{
	Hdr * buddy = 0;
	unsigned int size = 0;
	char * ptr_arithmetic = 0;
	if( has_valid_header( h ) )
	{
		if( is_right_buddy( h ) ) //Harder to find a left.
		{	
			for( int i = 0; i < num_free_list; ++i )//Go down each list
			{
				buddy = free_list[ i * 2 ];
				/*The buddy's size must lead to h and not exceed list end*/
				if( buddy != 0 )
				{
					while( buddy < h && buddy <= free_list[ i * 2 + 1 ] )
					{
						if( buddy != 0 )
						{
							size = fib( buddy -> size ) * basic_block_size;
							ptr_arithmetic = ( ( char * ) buddy ) + size;
							if( ( ( Hdr * ) ptr_arithmetic ) == h )
							{
								if( ! is_right_buddy( buddy ) )
								{
									return buddy;
								}
								else
								{
									return 0;
								}
							}
							else
							{	
								buddy = buddy -> next;
							}
						}
						else
						{	
							break;
						}
					}
				}
			}
		}
		else
		{	
			ptr_arithmetic = ( ( char * ) h ) + ( fib( h -> size ) * basic_block_size );
			buddy = ( Hdr * ) ptr_arithmetic;
			if( buddy < ( Hdr * ) mem_block_tail && buddy >= ( Hdr * ) mem_block && has_valid_header( buddy ) && is_right_buddy( buddy ) )
			{
				return buddy;
			}			
		}
	}
	return 0;
}

/*Gives the nth free list which is optimal for the required space*/
unsigned int get_sufficient_free_list( unsigned int _basic_block_size,
	unsigned int _length )
{
	unsigned int nth_free_list = 0;
	unsigned int current_fib = _basic_block_size;
	while( current_fib < _length )
	{
		++nth_free_list;
		current_fib = fib( nth_free_list ) * _basic_block_size;
	}
	return nth_free_list;
}

extern unsigned int init_allocator(unsigned int _basic_block_size, 
			    unsigned int _length)
{
	unsigned int nth_free_list = get_sufficient_free_list( _basic_block_size,
	_length );
	unsigned int mem_byte_size = fib( nth_free_list ) * _basic_block_size;
	//Allocate the Fibonacci value instead of inefficient request.
	mem_block = malloc( mem_byte_size );
	//2 slots per free list- first hdr ptr, last hdr ptr
	free_list = ( Hdr ** ) calloc( 2 * ( nth_free_list + 1 ), sizeof( header * ) );
	//If free_list fails to allocate, release allocator.
	if(free_list == 0)
	{
		if( mem_block != 0 )
		{
			free( mem_block );
			mem_block = 0;
		}
	}
	if( mem_block == 0 ) //Something failed.
	{
		return 0;
	}
	else //All allocations succeeded.
	{
		for(int i  = 0; i < mem_byte_size; ++i) //Sterilize of all unitialized values.
		{
			((char *) mem_block)[i] = 0;
		}
		configure_header_long( ( Hdr * ) mem_block, 0, 0, 1, nth_free_list, 0, 0 );
		basic_block_size = _basic_block_size;
		num_free_list = nth_free_list + 1;
		insert( (Hdr *) mem_block );
		mem_block_tail = ( ( char * ) mem_block ) + mem_byte_size; //The first byte that shouldn't be touched
		return _length; //Review this SELF.
	}
}

extern int release_allocator()
{
	if( ( Addr ) free_list != 0 )
	{
		free( (Addr) free_list );
	}
	if( mem_block != 0 )
	{
		free( mem_block );
	}
	free_list = 0;
	mem_block = 0;
	mem_block_tail = 0;
	basic_block_size = 0;
	num_free_list = 0;
 	return 0; //Fix this number later
}				

extern Addr my_malloc(unsigned int _length) 
{
	/*Get smallest free list which can fulfill request*/
	unsigned int nth_free_list = 
		get_sufficient_free_list( basic_block_size, _length + sizeof( Hdr ) );
	Hdr * left_h = 0;
	unsigned int original_fib = 0;
	if( mem_block == 0 ) //No one initialized allocator.
	{
		return 0;
	}
	if( nth_free_list + 1 >  num_free_list ) //Asking for too much capacity.
	{
		return 0;
	}
	original_fib = fib( nth_free_list ); //Testing 1
	/*Find the first block of fib memory that is same or larger.*/
	while( free_list[ 2 * ( nth_free_list ) ] == 0 && 2 * ( nth_free_list + 1 ) < 2 * ( num_free_list ) )
	{
		++nth_free_list; //Try next free list.
	}
	if( free_list[ 2 * ( nth_free_list ) ] == 0 )//Size currently unavailable.
	{
		return 0;
	}
	left_h = free_list[ 2 * ( nth_free_list ) ];
	unsigned int left_size = left_h -> size;
	unsigned int right_size = left_h -> size;
	unsigned int splittable_fib = fib( nth_free_list ); //The fib value of the block which must be split.
	Hdr * to_be_ejected = 0;
	if( original_fib != splittable_fib ) //The block is bigger than necessary, split.
	{
		while( splittable_fib > original_fib )
		{
			if( left_size > 1 )
			{
				right_size = left_size - 1;
				left_size -= 2;
			}
			else
			{
				if( left_size == 1 )
				{
					right_size = 0;
					left_size = 0;
				}
				else //Left size is 0, which shouldn't be possible.
				{
					return 0;
				}
			}
			split( left_h, left_size, right_size );
			splittable_fib = fib( left_size );
		}
	}
	if( original_fib == splittable_fib )
	{
		to_be_ejected = left_h;
	}
	else
	{
		Hdr * right_h = find_buddy( left_h );
		to_be_ejected = right_h;
	}
	eject( to_be_ejected );
	/*Set the header to not free, and give useable memory to user*/
	configure_header( to_be_ejected, is_right_buddy( to_be_ejected ), 
		get_inheritance( to_be_ejected ), 0, to_be_ejected -> size );
	char * ptr_arithmetic = ( ( char * ) to_be_ejected ) + sizeof( Hdr );
	Hdr * user_mem = ( Hdr * ) ptr_arithmetic;
	return ( void * ) user_mem;
}

extern int my_free(Addr _a) 
{
	char * ptr_arithmetic = ( ( char * ) _a );
	Hdr * h = 0;
	Hdr * buddy = 0;
	if( _a == 0 || mem_block == 0 || _a < mem_block || _a >= mem_block_tail )
	{
		return 0;
	}
	if( ptr_arithmetic - sizeof(Hdr) < (char *) mem_block || ptr_arithmetic - sizeof(Hdr) > (char *) mem_block_tail )
	{
		return 0;
	}
	ptr_arithmetic -= sizeof( Hdr );
	h = ( Hdr * ) ptr_arithmetic;
	unsigned int original_block_size = h -> size;
	unsigned int original_size = fib( original_block_size ) * basic_block_size;
	if( has_valid_header( h ) )
	{
		buddy = find_buddy( h );
		if( ! ( has_valid_header( buddy ) ) || ! ( is_free( buddy ) ) ) //No coalesce, just put this newly freed block on list.
		{
			configure_header( h, is_right_buddy( h ), //Make h free.
				get_inheritance( h ), 1, h -> size );
				insert( h ); //Put it on the list.			
		}
		else
		{
			while( has_valid_header( buddy ) && is_free( buddy ) )//Buddy free and OK.
			{
				if( is_right_buddy( buddy ) )
				{
					coalesce( h, buddy );//Will automatically set to free.
				}
				else
				{
					coalesce( buddy, h );
					h = buddy;
				}
				buddy = find_buddy( h );
			}
		}
	}
	return original_size;
}

