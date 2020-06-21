// perft.c

#include "stdio.h"
#include "../definitions/defs.h"
#include "stdlib.h"
#include "string.h"

long leafNodes;

typedef struct {
    char fen[256];
    long targetLeafNodes;
    long leafNodes;
    long nodes;
    long startTime;
    long endTime;
    long savedNodes;
} S_PERFTRES;

void ResetResult(S_PERFTRES *res) {
    memset(&res->fen[0], 256, 0);
    res->targetLeafNodes = 0;
    res->leafNodes = 0;
    res->nodes = 0;
    res->startTime = 0;
    res->endTime = 0;
    res->savedNodes = 0;
}

void PerftGo(int depth, S_BOARD *pos, S_PERFTRES *res) {
    ASSERT(CheckBoard(pos));

    if(depth == 0) {
        res->leafNodes++;
        return;
    }

    S_MOVELIST list[1];
    GenerateAllMoves(pos, list);

    int MoveNum = 0, move;
    for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        move = list->moves[MoveNum].move;
        printf("\t\tPerftGo() making move %s\n", PrMove(move));
        if(!MakeMove(pos, move)) {
            continue;
        }
        PerftGo(depth -1, pos, res);
        TakeMove(pos);
        res->nodes++;
    }

    return;
}

void PerftGoRoot(int depth, S_BOARD *pos, S_PERFTRES *res) {
    ASSERT(CheckBoard(pos));

    S_MOVELIST list[1];
    GenerateAllMoves(pos, list);

    int move;
    int MoveNum = 0;
    for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        move = list->moves[MoveNum].move;
        if(!MakeMove(pos, move)) {
            continue;
        }
        long cumnodes = res->leafNodes;

        PerftGo(depth-1, pos, res);
        TakeMove(pos);
        res->nodes++;
        long oldnodes = res->leafNodes - cumnodes;
        printf("Moves %d : %s : %ld\n", MoveNum+1, PrMove(move), oldnodes);
    }
    return;
}

void Perft(int depth, S_BOARD *pos) {
    ASSERT(CheckBoard(pos));

    if(depth == 0) {
        leafNodes++;
        return;
    }

    S_MOVELIST list[1];
    GenerateAllMoves(pos, list);

    int MoveNum = 0;
    for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        if( !MakeMove(pos, list->moves[MoveNum].move)) {
            continue;
        }
        Perft(depth - 1, pos);
        TakeMove(pos);
    }

    return;
}

void PerftTest(int depth, S_BOARD *pos) {
    ASSERT(CheckBoard(pos));

    PrintBoard(pos);
    printf("\nStarting Test To Depth:%d\n", depth);
    leafNodes = 0;
    int start = GetTimeMs();

    S_MOVELIST list[1];
    GenerateAllMoves(pos, list);

    int move;
    int MoveNum = 0;
    for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        move = list->moves[MoveNum].move;
        if( !MakeMove(pos, move)) {
            continue;
        }
        long cumnodes = leafNodes;
        Perft(depth - 1, pos);
        TakeMove(pos);
        long oldnodes = leafNodes - cumnodes;
        printf("Moves %d : %s : %ld\n", MoveNum+1, PrMove(move), oldnodes);
    }

    printf("\nTest Complete: %ld nodes visited in %dms.\n", leafNodes, (GetTimeMs() - start));

    return;
}

void PerftF(const int depth, S_BOARD *pos, S_PERFTRES *res) {
    printf("\n\n ***** Perft Depth %d ***** \n", depth);
    ParseFen(res->fen, pos);
    PerftGoRoot(depth, pos, res);
    printf("\nTotal Nodes: %ld\n", res->leafNodes);
}

void ParsePerftLine(char *line, S_PERFTRES *res, const int depth) {
    int i = 0;

    ResetResult(res);

    while(*line != ';') {
        res->fen[i++] = *line;
        line++;
    }

    while(*line) {
        if(*line == 'D') {
            line++;
            i = atoi(line);
            if(i == depth) {
                line += 2;
                res->targetLeafNodes = atoi(line);
                printf("Fen: %s : Target %ld\n", res->fen, res->targetLeafNodes);
                return;
            }
        }
        line++;
    }
}

void PerftOne(int depth, char *fenLineWithDepth) {
    printf("\n\n************** New Perft, Depth %d **************\n", depth);
    S_PERFTRES res[1];
    S_BOARD board[1];
    ParsePerftLine(fenLineWithDepth, res, depth);
    printf("After Parse: Target:%ld Actual:%ld", res->targetLeafNodes, res->leafNodes);
    int success = res->leafNodes == res->targetLeafNodes ? TRUE : FALSE;
    printf("\nResult: %s : %s leaf:%ld target:%ld\n", res->fen,(success) ? "OK":"**** FAILED ****", res->leafNodes, res->targetLeafNodes);
}

void PerftFile(const int depth) {
    FILE *perftFile;
    char lineIn [1024];
    S_PERFTRES results[512];
    int resCount = 0;
    int index = 0;
    int index2 = 0;

    perftFile = fopen("perftsuite.epd", "r");
    if(perftFile == NULL) {
        printf("File Not Found.\n");
        return;
    } else {
        while(fgets (lineIn, 1024, perftFile) != NULL) {
            ParsePerftLine(lineIn, &results[resCount++], depth);
            memset(&lineIn[0], 0, sizeof(lineIn));
        }
    }

    S_BOARD board[1];

    if(resCount != 0) {
        printf("\n\nResults:\n\n");
        int success = FALSE;
        for(index = 0; index < resCount; ++index) {
            PerftF(depth, board, &results[index]);
        }

        for(index2 = 0; index2 < resCount; ++index2) {
            success = results[index2].leafNodes == results[index2].targetLeafNodes ? TRUE : FALSE;
            printf("%s : %s\n",(success)?"OK":"FAILED", results[index2].fen);
        }
    }
}