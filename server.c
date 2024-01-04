#include "pexeso.h"
#include <unistd.h>     
#include <stdio.h>      
#include <string.h>     
#include <sys/socket.h> 
#include <sys/un.h>     
#include <errno.h>      
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

GameBoard board;

void print_sep()
{
	for ( int i = 0; i < board.cols; ++i )
		printf( "+---" );

	printf( "+\n" );
}

void print_board()
{
	print_sep();
	for ( int row = 0; row < board.rows; ++row )
	{
		for ( int col = 0; col < board.cols; ++col )
		{
			if ( board.found[ row * board.cols + col ] )
				printf( "| %c ", board.board[ row * board.cols + col ] );
			else
				printf( "|...");
		}
		
		printf( "|\n" );
		print_sep();
	}
}

bool is_invalid( int x, int y )
{
	return x < 0 || x >= board.cols || y < 0 || y >= board.rows;
}

bool is_finished()
{
	return ( board.score1 + board.score2 ) == ( ( board.rows * board.cols ) / 2 );
}

void game( int fds[ 2 ] )
{
	position pos;
	while ( true )
	{
		for ( int i = 0; i < 2; )
		{		
			print_board();
			printf( "Player %d is on turn.\n", i + 1 );
			if ( read( fds[ i ], &pos, sizeof( pos ) ) != sizeof( pos ) )
			{
				perror( "read" );
				return;
			}

			if ( is_invalid( pos.x1, pos.y1 ) || is_invalid( pos.x2, pos.y2 ) )
			{
				printf( "Player %d chose invalid position.\n", i );
				continue;
			}

			if ( board.board[ pos.y1 * board.cols + pos.x1 ] ==
				 board.board[ pos.y2 * board.cols + pos.x2 ] )
			{
				board.found[ pos.y1 * board.cols + pos.x1 ] = true;
				board.found[ pos.y2 * board.cols + pos.x2 ] = true;
				if ( i == 0 )
					++board.score1;
				else
					++board.score2;
			}

			if ( is_finished() )
				break;

			++i;
		}
	}

	if ( board.score1 == board.score2 )
		printf( "Draw!\n" );
	else
		printf( "Player %d won. CONGRATULATIONS!!\n", board.score1 > board.score2 ? 0 : 1 );
}

void server()
{
	int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( sock_fd == -1 )
	{
		perror( "socket" );
		exit( -1 );
	}

	struct sockaddr_un sa = { .sun_family = AF_UNIX, .sun_path = "game" };
	if ( unlink( sa.sun_path ) == -1 && errno != ENOENT )
	{
		perror( "unlink" );
		close( sock_fd );
		exit( -1 );
	}

	if ( bind( sock_fd, ( struct sockaddr * ) &sa, sizeof( sa ) ) == -1 )
	{
		perror( "bind" );
		close( sock_fd );
		exit( -1 );
	}

	if ( listen( sock_fd, 2 ) == -1 )
	{
		perror( "listen" );
		close( sock_fd );
		exit( -1 );
	}
	
	int client_fd[ 2 ] = { -1 };
	for ( int i = 0; i < 2; ++i )
	{
		client_fd[ i ] = accept( sock_fd, NULL, NULL );
		if ( client_fd[ i ] == -1 )
		{
			perror( "accept" );
			close( client_fd[ 0 ] );
			close( client_fd[ 1 ] );
			close( sock_fd );
			exit( -1 );
		}
	}

	game( client_fd );
	close( client_fd[ 0 ] );
	close( client_fd[ 1 ] );
	close( sock_fd );
	exit( 0 );
}

int init_board( int rows, int cols )
{
	board.rows = rows;
	board.cols = cols;

	board.board = calloc( rows * cols, sizeof( char ) );
	if ( board.board == NULL )
	{
		perror( "calloc" );
		return -1;
	}
	
	board.found = calloc( rows * cols, sizeof( char ) );
	if ( board.found == NULL )
	{
		perror( "calloc" );
		free( board.board );
		return -1;
	}

	return 0;
}

int set_board()
{
	int size = board.rows * board.cols;

	int *positions = malloc( size * sizeof( int ) );
	if ( positions == NULL )
		return -1;

	for ( int i = 0; i < size; ++i )
		positions[ i ] = i;

	srand( time( NULL ) );

	int last = size - 1;
	char chr = '!';

	while ( last > 0 )
	{
		for ( int i = 0; i < 2; ++i )
		{
			int id = rand() % ( last + 1 );	
			board.board[ positions[ id ] ] = chr;
			memmove( positions + id, positions + id + 1, sizeof( int ) * ( size - id ) );
			--last;
		}
		++chr;
	}
	return 0;
}

int main( int argc, char **argv )
{
	if ( argc != 3 )
	{
		fprintf( stderr, "usage: ./pexeso <rows> <cols>\n" );
		return -1;
	}

	int rows = atoi( argv[ 1 ] );
	int cols = atoi( argv[ 2 ] );

	if ( ( rows * cols ) % 2 == 1 )
	{
		fprintf( stderr, "Amount of pairs has to be even.\n" );
		return -1;
	}

	if ( init_board( rows, cols ) != 0 )
		return -1;

	if ( set_board() != 0 )
	{
		free( board.board );
		return -1;
	}
	
	server();
	free( board.board );
	free( board.found );
	return 0;
}
