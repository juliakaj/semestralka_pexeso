#include <unistd.h>     
#include <stdio.h>      
#include <string.h>     
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <stdlib.h>
#include "pexeso.h"

void handler( int signalNumber)
{
	exit( 0 );
}

void player( int fd )
{
	while ( true )
	{
		position pos;
		printf( "Put coordinations of first tile: \n" );
		scanf( " %d %d", &pos.x1, &pos.y1 );

		printf( "Put coordinations of second tile: \n" );
		scanf( " %d %d", &pos.x2, &pos.y2 );

		if ( write( fd, &pos, sizeof( position ) ) != sizeof( position ) )
		{
			perror( "write" );
			exit( -1 );
		}
	}
}

int main()
{
	if ( signal( SIGPIPE, handler ) == SIG_ERR )
	{
		perror( "signal" );
		exit( -1 );
	}

	int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( sock_fd == -1 )
	{
		perror( "socket" );
		exit( -1 );
	}

	struct sockaddr_un sa = { .sun_family = AF_UNIX, .sun_path = "game" };

	if ( connect( sock_fd, ( struct sockaddr * ) &sa, sizeof( sa ) ) == -1 )
	{
		perror( "connect" );
		exit( -1 );
	}

	player( sock_fd );
	exit( 0 );
}
