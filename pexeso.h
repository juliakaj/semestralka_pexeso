#include <stdbool.h>

typedef struct
{
    int rows;
    int cols;
    int score1;
    int score2;
    bool *found;
    char *board;
} GameBoard;

typedef struct
{
    int x1;
    int y1;
    int x2;
    int y2;
} position;