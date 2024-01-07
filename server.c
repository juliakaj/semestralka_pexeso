#include "pexeso.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <netinet/in.h>

GameBoard board;
pthread_mutex_t mtx;




struct thread_info
{
    int id;
    int fd;
};

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
    printf( "\n" );
}

bool is_invalid( int x, int y )
{
    return x < 0 || x >= board.cols || y < 0 || y >= board.rows ||
           board.found[ y * board.cols + x ];
}

bool is_finished()
{
    return ( board.score1 + board.score2 ) == ( ( board.rows * board.cols ) / 2 );
}

void *client( void *args )
{
    struct thread_info *ti = args;
    position pos;
    while ( true )
    {
        if ( pthread_mutex_lock( &mtx ) != 0 )
        {
            perror( "pthread_mutex_lock" );
            exit( -1 );
        }

        if ( is_finished() )
        {
            if ( pthread_mutex_unlock( &mtx ) != 0 )
            {
                perror( "pthread_mutex_unlock" );
                exit( -1 );
            }
            return NULL;
        }

        printf( "Player %d is on turn.\n", ti->id );
        print_board();

        while ( true )
        {
            if ( read( ti->fd, &pos, sizeof( pos ) ) != sizeof( pos ) )
            {
                perror( "read" );
                exit( -1 );
            }

            if ( is_invalid( pos.x1, pos.y1 ) || is_invalid( pos.x2, pos.y2 ) )
                printf( "Player %d chose invalid position %d %d and %d %d.\n",
                        ti->id, pos.x1, pos.y1, pos.x2, pos.y2 );
            else
                break;
        }

        board.found[ pos.y1 * board.cols + pos.x1 ] = true;
        board.found[ pos.y2 * board.cols + pos.x2 ] = true;

        printf( "Player %d chose position %d %d and %d %d\nGame state is:\n",
                ti->id, pos.x1, pos.y1, pos.x2, pos.y2 );

        print_board();

        if ( board.board[ pos.y1 * board.cols + pos.x1 ] ==
             board.board[ pos.y2 * board.cols + pos.x2 ] )
        {
            if ( ti->id == 1 )
                ++board.score1;
            else
                ++board.score2;
        }
        else
        {
            board.found[ pos.y1 * board.cols + pos.x1 ] = false;
            board.found[ pos.y2 * board.cols + pos.x2 ] = false;
        }

        bool finished = is_finished();

        if ( pthread_mutex_unlock( &mtx ) != 0 )
        {
            perror( "pthread_mutex_unlock" );
            exit( -1 );
        }

        if ( finished )
            break;

        sleep( 1 );
    }

    return NULL;
}

void server()
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(12345);
    sa.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        perror("bind");
        close(sock_fd);
        exit(-1);
    }

    if (listen(sock_fd, 2) == -1)
    {
        perror("listen");
        close(sock_fd);
        exit(-1);
    }

    struct thread_info ti[ 2 ];

    ti[ 0 ].fd = -1;
    ti[ 1 ].fd = -1;

    pthread_t threads[ 2 ];

    for ( int i = 0; i < 2; ++i )
    {
        ti[ i ].fd = accept( sock_fd, NULL, NULL );
        if ( ti[ i ].fd == -1 )
        {
            perror( "accept" );
            close( ti[ i ].fd );
            close( ti[ ( i + 1 ) % 2 ].fd );
            close( sock_fd );
            exit( -1 );
        }

        ti[ i ].id = i + 1;
        if ( pthread_create( &threads[ i ], NULL, client, ( void * ) &ti[ i ] ) != 0 )
        {
            perror( "pthread_create" );
            close( ti[ i ].fd );
            close( ti[ ( i + 1 ) % 2 ].fd );
            close( sock_fd );
            exit( -1 );
        }
    }

    if ( pthread_join( threads[ 0 ], NULL ) != 0 ||
         pthread_join( threads[ 1 ], NULL ) != 0 )
    {
        perror( "pthread_join" );
        close( ti[ 0 ].fd );
        close( ti[ 1 ].fd );
        close( sock_fd );
        exit( -1 );
    }

    if ( board.score1 == board.score2 )
        printf( "Draw!\n" );
    else
        printf( "Player %d won. CONGRATULATIONS!!\n", board.score1 > board.score2 ? 1 : 2 );

    close( ti[ 0 ].fd );
    close( ti[ 1 ].fd );
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
        fprintf( stderr, "usage: ./server <rows> <cols>\n" );
        return -1;
    }

    if ( pthread_mutex_init( &mtx, NULL ) != 0 )
    {
        perror( "pthread_mutex_init" );
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

    if ( pthread_mutex_destroy( &mtx ) != 0 )
    {
        perror( "pthread_mutex_destroy" );
        return -1;
    }

    return 0;
}