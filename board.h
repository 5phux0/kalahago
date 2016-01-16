#ifndef board_h
#define board_h

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct board{
    unsigned int* pholes;
    unsigned int* oholes;
    unsigned int player;
    unsigned int opponent;
    bool mover;
};

struct boardContext{
    unsigned int nHoles;
    unsigned int totalStones;
    unsigned int totalHoles;
    unsigned int halfStones;
    unsigned int moveCount;
};

struct board* board_copy(struct board*, struct boardContext*);
void board_free(struct board*);
void board_pickup(struct board*, int, struct boardContext*);
void board_print(struct board*, struct boardContext*);
void board_flip(struct board*, struct boardContext*);

struct boardContext* context_new(unsigned int, unsigned int);
void initBoardAndContextFromHoles(struct board*, struct boardContext*, unsigned int*, unsigned int);

#endif /* board_h */
