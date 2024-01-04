#define SEMESTRALKA_PEXESO_H
#define SEMESTRALKA_PEXESO_H
#include <stdbool.h>

typedef struct {
    bool finished;
    int rows;
    int cols;
    int score1;
    int score2;
    bool found;
    charboard;
}GameBoard;

typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
}position;

void handler(int) {
    exit(0);
}

void player(int fd) {
    while (true) {
        position pos;
        printf("Zadaj prvu suradnicu: \n");
        scanf("%d %d", &pos.x1, &pos.y1);

        printf("Zadaj druhu suradnicu: \n");
        scanf("%d %d", &pos.x2, &pos.y2);

        if (write(fd, &pos, sizeof(position)) != sizeof(position)) {
            perror("write");
            exit(-1);
        }
    }
}

int main () {
    if(signal(SIGPIPE, handler)==SIG_ERR){
        perror("signal");
        exit(-1);
    }

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock_fd == -1) {
        perror("socket");
        exit(-1);
    }

    struct sockaddr_un sa = {.sun_family = AF_UNIX, .sun_path = "game"};

    if(connect(sock_fd, (struct sockaddr * )&sa, sizeof(sa)) == -1){
        perror("connect");
        exit(-1);
    }

    player(sock_fd);
}