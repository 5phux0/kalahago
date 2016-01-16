#include "board.h"

//Board
//Private functions

void board_placeStone(struct board* b, int holei, int n, struct boardContext* ctx){
    if(holei < ctx->nHoles)
        b->pholes[holei] += n;
    else if(holei == ctx->nHoles)
        b->player += n;
    else
        b->oholes[holei-ctx->nHoles-1] += n;
}

void board_stealOholes(struct board* b, struct boardContext* ctx){
    for (int i=0; i<ctx->nHoles; i++) {
        b->player += b->oholes[i];
        b->oholes[i] = 0;
    }
}

//Public functions

struct board* board_copy(struct board* b, struct boardContext* ctx){
    struct board* nb = malloc(sizeof(struct board));
    nb->pholes = calloc(ctx->nHoles, sizeof(unsigned int));
    memcpy(nb->pholes, b->pholes, ctx->nHoles*sizeof(unsigned int));
    nb->oholes = calloc(ctx->nHoles, sizeof(unsigned int));
    memcpy(nb->oholes, b->oholes, ctx->nHoles*sizeof(unsigned int));
    nb->player = b->player;
    nb->opponent = b->opponent;
    nb->mover = b->mover;
    return nb;
}

void board_free(struct board* b){
    free(b->pholes);
    free(b->oholes);
    free(b);
}

void board_pickup(struct board* b, int holei, struct boardContext* ctx){
    int n;
    if (holei < ctx->nHoles){
        n = b->pholes[holei];
        b->pholes[holei] = 0;
    } else if (holei > ctx->nHoles){
        n = b->oholes[holei-ctx->nHoles-1];
        b->oholes[holei-ctx->nHoles-1] = 0;
    }else{
        printf("board_pickup called with invalid hole");
        exit(1);
    }
    
    int all = n / ctx->totalHoles;
    int sec = n % ctx->totalHoles;
    if(all == 0){
        for (int i=holei+1; i<holei+1+sec; i++) {
            board_placeStone(b, i%ctx->totalHoles, 1, ctx);
        }
    }else{
        for (int i=holei+1; i<holei+1+sec; i++) {
            board_placeStone(b, i%ctx->totalHoles, all+1, ctx);
        }
        for (int i=holei+1+sec; i<holei+1+ctx->totalHoles; i++) {
            board_placeStone(b, i%ctx->totalHoles, all, ctx);
        }
    }
    
    int c = (holei+n)%ctx->totalHoles;
    if(c < ctx->nHoles){
        if(b->pholes[c] == 1){
            board_flip(b, ctx);
            return;
        }else{
            board_pickup(b, c, ctx);
            return;
        }
    }else if (c == ctx->nHoles){
        if (holei == ctx->nHoles-1) {
            for (int i=0; i<ctx->nHoles-1; i++){
                if (b->pholes[i] != 0)
                    goto End;
            }
            board_stealOholes(b, ctx);
        }
    }else{
        if(b->oholes[c-ctx->nHoles-1] == 1){
            for (int i=0; i<ctx->nHoles; i++){
                if (b->pholes[i] != 0)
                    goto FlipAndEnd;
            }
            board_stealOholes(b, ctx);
        FlipAndEnd:
            board_flip(b, ctx);
            return;
        }else{
            board_pickup(b, c, ctx);
            return;
        }
    }
End: return;
}

void board_flip(struct board* b, struct boardContext* ctx){
    int btemp[ctx->nHoles];
    memcpy(btemp, b->pholes, sizeof(int)*ctx->nHoles);
    int temp = b->player;
    
    memcpy(b->pholes, b->oholes, sizeof(int)*ctx->nHoles);
    b->player = b->opponent;
    
    memcpy(b->oholes, btemp, sizeof(int)*ctx->nHoles);
    b->opponent = temp;
    
    b->mover = !b->mover;
}

void board_print(struct board* b, struct boardContext* ctx){
    char ps[100] = "";
    char os[100] = "";
    char temp[10];
    for (int i=0; i<ctx->nHoles; i++) {
        sprintf(temp, "%d\t", b->pholes[i]);
        strcat(ps, temp);
        sprintf(temp, "%d\t", b->oholes[ctx->nHoles-i-1]);
        strcat(os, temp);
    }
    const char *ts;
    if(b->mover)
        ts = "Player turn  ";
    else
        ts = "Opponent turn";
    printf("%s\n%d\t%s\t%d\n%s\n", ps, b->opponent, ts, b->player, os);
}

//Context functions

struct boardContext* context_new(unsigned int nHoles, unsigned int totalStones){
    struct boardContext* ctx = malloc(sizeof(struct boardContext));
    ctx->nHoles = nHoles;
    ctx->totalStones = totalStones;
    ctx->halfStones = totalStones/2;
    ctx->totalHoles = nHoles*2+1;
    ctx->moveCount = 0;
    return ctx;
}

void initBoardAndContextFromHoles(struct board* b, struct boardContext* ctx, unsigned int* holes, unsigned int len){
    if (len%2 != 0) {
        return;
    }
    ctx->nHoles = len/2-1;
    ctx->totalHoles = len-1;
    ctx->totalStones = 0;
    for (int i=0; i<len; i++) {
        ctx->totalStones += holes[i];
    }
    ctx->halfStones = ctx->totalStones/2;
    ctx->moveCount = 0;
    
    b->pholes = calloc(ctx->nHoles, sizeof(unsigned int));
    memcpy(b->pholes, holes, ctx->nHoles*sizeof(unsigned int));
    b->player = holes[ctx->nHoles];
    
    b->oholes = calloc(ctx->nHoles, sizeof(unsigned int));
    memcpy(b->oholes, holes+ctx->nHoles+1, ctx->nHoles*sizeof(unsigned int));
    b->opponent = holes[len-1];
    
    b->mover = true;
}
