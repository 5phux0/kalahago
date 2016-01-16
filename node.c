#include "node.h"
#include "_cgo_export.h"

//Private functions

void sortmNodes(struct moveNode** a, int len){
    int i, j, p;
    struct moveNode* t;
    if (len < 2)
        return;
    if (len > 2) {
        int x=a[0]->b->opponent, y=a[len/2]->b->opponent, z=a[len-1]->b->opponent;
        if (x < y) {
            if (y < z)
                p = y;
            else if (x < z)
                p = z;
            else
                p = x;
        }else{
            if (y < z)
                p = z;
            else if (x < z)
                p = x;
            else
                p = z;
        }
    }else
        p = a[len-1]->b->opponent;
    
    for (i = 0, j = len - 1;; i++, j--) {
        while (a[i]->b->opponent > p)
            i++;
        while (p > a[j]->b->opponent)
            j--;
        if (i >= j)
            break;
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    sortmNodes(a, i);
    sortmNodes(a + i, len - i);
}

//Genaral node Functions

void node_print(void* node){
    struct evaluatedNode* enode = node;
    switch (enode->state.outlook) {
        case WINNING:
            printf("Winning: ");
            break;
        case LOSING:
            printf("Losing: ");
            break;
        default:
            printf("Unevaluated: ");
            break;
    }
    for (int i=0; i<enode->state.pathLength; i++) {
        printf("%d ", enode->path[i]);
    }
    printf("\n");
}

void node_printPath(void* node){
    struct evaluatedNode* enode = node;
    for (int i=0; i<enode->state.pathLength; i++) {
        printf("%d ", enode->path[i]);
    }
    printf("\n");
}

//moveNode Functions

struct moveNode* mnode_new(struct board* b, struct boardContext* ctx){
    struct moveNode* node = malloc(sizeof(struct moveNode));
    node->b = b;
    node->state = ((struct nodeState){UNEVALUATED, 0});
    node->path = NULL;
    node->children = calloc(ctx->nHoles, sizeof(void *));
    return node;
}

void mnode_free(struct moveNode* node){
    free(node->path);
    board_free(node->b);
    free(node->children);
    free(node);
}

void mnode_freeChildren(struct moveNode* node, int f, int l){
    for (int i=f; i<l; i++) {
        if (node->children[i] != NULL) {
            mnode_free(node->children[i]);
        }
    }
}

//evaluatedNode Functions

void enode_free(struct evaluatedNode *node){
    free(node->path);
    for (int i=0; i<node->childrenLen; i++) {
        enode_free(node->children[i]);
    }
    free(node->children);
    free(node);
}

struct evaluatedNode* enode_newFromMnode(struct moveNode* mnode, nodeOutlook eNodeOutlook, struct evaluatedNode** children, int childrenLen){
    struct evaluatedNode* enode = malloc(sizeof(struct evaluatedNode));
    enode->state = (struct nodeState){eNodeOutlook, mnode->state.pathLength};
    enode->path = calloc(enode->state.pathLength, sizeof(unsigned int));
    memcpy(enode->path, mnode->path, enode->state.pathLength*sizeof(unsigned int));
    enode->children = children;
    enode->childrenLen = childrenLen;
    return enode;
}

//Container functions

void ccont_init(struct childContainer* cont, int cap){
    cont->lenght = 0;
    cont->capacity = cap;
    cont->children = calloc(cap, sizeof(void*));
}

void ccont_shrink(struct childContainer* cont){
    cont->capacity = cont->lenght;
    cont->children = realloc(cont->children, cont->lenght*sizeof(void*));
}

void ccont_free(struct childContainer* cont){
    for (int i=0; i<cont->lenght; i++) {
        mnode_free((struct moveNode*) cont->children[i]);
    }
    free(cont->children);
}

void ccont_addElement(struct childContainer* cont, struct moveNode* node){
    if (cont->lenght >= cont->capacity) {
        if (cont->capacity == 0)
            cont->capacity = 1;
        else
            cont->capacity *= 2;
        cont->children = realloc(cont->children, cont->capacity*sizeof(void*));
    }
    cont->children[cont->lenght] = node;
    cont->lenght++;
}

//Main algotithm functions

//returns a new move- or evaluated-node that is the result of picking up hole-"ind" fron rnode
void* mnode_newFromMove(struct moveNode* rootNode, int ind, struct boardContext* ctx){
    ctx->moveCount++;
    struct board* b = board_copy(rootNode->b, ctx);
    struct moveNode* newNode = mnode_new(b, ctx);
    
    newNode->state.pathLength = rootNode->state.pathLength+1;
    newNode->path = calloc(newNode->state.pathLength, sizeof(unsigned int));
    memcpy(newNode->path, rootNode->path, rootNode->state.pathLength*sizeof(unsigned int));
    newNode->path[rootNode->state.pathLength] = ind;
    
    board_pickup(b, ind, ctx);
    
    if ((b->mover && b->player > ctx->halfStones) || (!b->mover && b->opponent > ctx->halfStones)){
        struct evaluatedNode* evaluated = enode_newFromMnode(newNode, WINNING, NULL, 0);
        mnode_free(newNode);
        return evaluated;
    } else if ((b->mover && b->opponent >= ctx->halfStones) || (!b->mover && b->player >= ctx->halfStones)){
        struct evaluatedNode* evaluated = enode_newFromMnode(newNode, LOSING, NULL, 0);
        mnode_free(newNode);
        return evaluated;
    } else{
        int ind = -1;
        for (int i=0; i<ctx->nHoles; i++) {
            if(b->pholes[i] > 0){
                if (ind < 0)
                    ind = i;
                else
                    return newNode;
            }
        }
        if(ind >= 0){
            struct moveNode* secNewNode = mnode_newFromMove(newNode, ind, ctx);
            mnode_free(newNode);
            return secNewNode;
        }else{
            return newNode;
        }
    }
}

struct evaluatedNode* mnode_evaluateTurn(struct moveNode* rootNode, struct childContainer* container, struct boardContext* ctx){
    for (int i=ctx->nHoles-1; i>=0; i--) {
        if (rootNode->b->pholes[i] < 1) {
            rootNode->children[i] = NULL;
            continue;
        }
        struct moveNode* child = mnode_newFromMove(rootNode, i, ctx);
        if (child->state.outlook != UNEVALUATED) {
            if ((rootNode->b->mover && child->state.outlook == WINNING) || (!rootNode->b->mover && child->state.outlook == LOSING)) {
                mnode_freeChildren(rootNode, i, ctx->nHoles);
                return (struct evaluatedNode*) child;
            }else{
                enode_free((struct evaluatedNode*) child);
                rootNode->children[i] = NULL;
            }
        }else if (rootNode->b->mover != child->b->mover){
            ccont_addElement(container, child);
            rootNode->children[i] = NULL;
        }else{
            rootNode->children[i] = child;
        }
    }
    
    for (int i=ctx->nHoles-1; i>=0; i--) {
        if (rootNode->children[i] != NULL) {
            struct evaluatedNode* result = mnode_evaluateTurn(rootNode->children[i], container, ctx);
            if (result != NULL) {
                mnode_freeChildren(rootNode, i, ctx->nHoles);
                return result;
            }
        }
    }
    mnode_freeChildren(rootNode, 0, ctx->nHoles);
    return NULL;
}

//Returns an evaluatedNode* with children describing a tree with paths to assured victory,
//where possible opponent moves are branches. Returns NULL if no such path can be found.
struct evaluatedNode* searchWinPath(struct moveNode* rootNode, struct boardContext* ctx){
    if(!goOverseer(ctx)){
        return NULL;
    }
    if((rootNode->b->mover && rootNode->b->player > ctx->halfStones)||
        (!rootNode->b->mover && rootNode->b->opponent > ctx->halfStones)){
        return enode_newFromMnode(rootNode, WINNING, NULL, 0);
    }
    if((!rootNode->b->mover && rootNode->b->player >= ctx->halfStones)||
        (rootNode->b->mover && rootNode->b->player >= ctx->halfStones)){
        return enode_newFromMnode(rootNode, LOSING, NULL, 0);
    }


    struct childContainer cont;
    ccont_init(&cont, 100);
    struct evaluatedNode* result = mnode_evaluateTurn(rootNode, &cont, ctx);
    
    if (result != NULL) {
        ccont_free(&cont);
        if (result->state.outlook == WINNING) {
            struct evaluatedNode** children = malloc(sizeof(void*));
            children[0] = result;
            return enode_newFromMnode(rootNode, WINNING, children, 1);
        }else{
            enode_free(result);
            return NULL;
        }
    }
    ccont_shrink(&cont);
    sortmNodes(cont.children, cont.lenght);
    
    if (rootNode->b->mover) {
        for (int i=0; i<cont.lenght; i++) {
            struct evaluatedNode* enode = searchWinPath(cont.children[i], ctx);
            if (enode != NULL) {
                ccont_free(&cont);
                struct evaluatedNode** children = malloc(sizeof(void*));
                children[0] = enode;
                return enode_newFromMnode(rootNode, WINNING, children, 1);
            }
        }
        ccont_free(&cont);
        return NULL;
    }else{
        struct evaluatedNode** children = calloc(cont.lenght, sizeof(void*));
        int i = 0;
        for (; i<cont.lenght; i++) {
            struct evaluatedNode* enode = searchWinPath(cont.children[i], ctx);
            if (enode == NULL) {
                ccont_free(&cont);
                for (int j=0; j<i; j++) {
                    enode_free(children[j]);
                }
                free(children);
                return NULL;
            }else{
                children[i] = enode;
            }
        }
        ccont_free(&cont);
        return enode_newFromMnode(rootNode, WINNING, realloc(children, i*sizeof(void*)), i);
    }
}

//Utillity

void newBoardAndContext(struct board** bPointer, struct boardContext** ctxPointer,
    unsigned int* holes, unsigned int n){
    struct board* b = malloc(sizeof(struct board));
    struct boardContext* ctx = malloc(sizeof(struct boardContext));
    initBoardAndContextFromHoles(b, ctx, holes, n);
    *bPointer = b;
    *ctxPointer = ctx;
}

int enode_getPathLen(struct evaluatedNode* node){
    return (int) node->state.pathLength;
}

void printEnodeInTree(struct evaluatedNode* node){
    switch (node->childrenLen) {
        case 0:
            printf("You have won!\n");
            return;
            break;
        case 1:
            printf("Your turn.\n");
            break;
        default:
            printf("Opponent turn.\n");
        break;
    }
    for (int i=0; i<node->childrenLen; i++) {
        printf("%d:", i);
        node_printPath(node->children[i]);
    }
}
