#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pexeso.h"

void handler( int signalNumber)
{
    printf( "Game has ended.\n" );
    exit( 0 );
}

bool is_valid( char *buffer )
{
    for ( int i = 0; i < 32 && buffer[ i ] != 0; ++i )
    {
        if ( buffer[ i ] != '\n' && ( buffer[ i ] < '0' || buffer[ i ] > '9' ) )
        {
            printf( "Invalid sign. Repeate.\n" );
            return false;
        }
    }

    return true;
}

void load_and_check( int *write_to )
{
    while ( true )
    {
        char buffer[ 32 ] = { 0 };
        scanf( "%s", buffer );

        if ( !is_valid( buffer ) )
            continue;

        *write_to = atoi( buffer );
        return;
    }
}

void player( int fd )
{
    while ( true )
    {
        position pos;

        printf( "Put x coordinate of first tile: \n" );
        load_and_check( &pos.x1 );

        printf( "Put y coordinate of first tile: \n" );
        load_and_check( &pos.y1 );

        printf( "Put x coordinate of second tile: \n" );
        load_and_check( &pos.x2 );

        printf( "Put y coordinate of second tile: \n" );
        load_and_check( &pos.y2 );

        if ( write( fd, &pos, sizeof( position ) ) != sizeof( position ) )
        {
            perror( "write" );
            exit( -1 );
        }
    }
}

int main() {
    if (signal(SIGPIPE, handler) == SIG_ERR) {
        perror("signal");
        return -1;
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(12345);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        perror("connect");
        return -1;
    }

    player(sock_fd);
    return 0;
}