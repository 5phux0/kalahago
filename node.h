#ifndef node_h
#define node_h

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

typedef enum {UNEVALUATED=0, WINNING, LOSING} nodeOutlook; //Unevaluated->moveNode else evaluatedNode

struct nodeState{
    nodeOutlook outlook: 2;
    unsigned int pathLength: 30;
};

struct moveNode{
    struct nodeState state;
    unsigned int* path;
    struct board* b;
    struct moveNode** children;
};

struct evaluatedNode{
    struct nodeState state;
    unsigned int* path;
    int childrenLen;
    struct evaluatedNode** children;
};

struct childContainer{
    unsigned int lenght;
    unsigned int capacity;
    struct moveNode** children;
};

void ccont_init(struct childContainer*, int);

void node_print(void*);
void node_printPath(void*);

struct moveNode* mnode_new(struct board*, struct boardContext*);
void* mnode_newFromMove(struct moveNode*, int, struct boardContext*);
struct evaluatedNode* mnode_evaluateTurn(struct moveNode*, struct childContainer*, struct boardContext*);
struct evaluatedNode* searchWinPath(struct moveNode*, struct boardContext*);
struct evaluatedNode* enode_newFromMnode(struct moveNode*, nodeOutlook, struct evaluatedNode**, int);
void enode_free(struct evaluatedNode*);
void mnode_free(struct moveNode*);

void newBoardAndContext(struct board**, struct boardContext**, unsigned int*, unsigned int);
int enode_getPathLen(struct evaluatedNode*);
void printEnodeInTree(struct evaluatedNode*);

#endif /* node_h */
