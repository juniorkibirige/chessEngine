// uci.c

#include "stdio.h"
#include "../definitions/defs.h"
#include "string.h"

#define INPUTBUFFER 400 * 6

void ParseGo(char *line, S_SEARCHINFO *info, S_BOARD *pos)
{

    int depth = -1, movestogo = 30, movetime = -1;
    int time = -1, inc = 0;
    char *ptr = NULL;
    int timeForPonder = 0;
    info->timeset = FALSE;

    if ((ptr = strstr(line, "infinite")))
    {
        ;
    }

    if ((ptr = strstr(line, "binc")) && pos->side == BLACK)
    {
        inc = atoi(ptr + 5);
    }

    if ((ptr = strstr(line, "winc")) && pos->side == WHITE)
    {
        inc = atoi(ptr + 5);
    }

    if ((ptr = strstr(line, "btime")) && pos->side == BLACK)
    {
        time = atoi(ptr + 6);
        timeForPonder = time;
    }

    if ((ptr = strstr(line, "wtime")) && pos->side == WHITE)
    {
        time = atoi(ptr + 6);
        timeForPonder = time;
    }

    if ((ptr = strstr(line, "movestogo")))
    {
        movestogo = atoi(ptr + 10);
    }

    if ((ptr = strstr(line, "movetime")))
    {
        movetime = atoi(ptr + 9);
        timeForPonder = movetime;
    }

    if ((ptr = strstr(line, "depth")))
    {
        depth = atoi(ptr + 6);
    }

    if (movetime != -1)
    {
        time = movetime;
        movestogo = 1;
    }

    info->starttime = GetTimeMs();
    info->depth = depth;

    if (time != -1)
    {
        info->timeset = TRUE;
        time /= movestogo;
        time -= 100;
        info->ponderDrop = time + inc;
        info->stoptime = info->starttime + time + inc;
    }

    if (depth == -1)
    {
        info->depth = MAXDEPTH;
    }

    if ((ptr = strstr(line, "ponder")))
    {
        info->stoptime = info->starttime + timeForPonder;
        PonderSearchPosition(pos, info);
    }
    else
    {
        printf("Normal Search\n");
        SearchPosition(pos, info);
    }
}
void ParsePosition(char *lineIn, S_BOARD *pos)
{

    lineIn += 9;
    char *ptrChar = lineIn;

    if (strncmp(lineIn, "startpos", 8) == 0)
    {
        ParseFen(START_FEN, pos);
    }
    else
    {
        ptrChar = strstr(lineIn, "fen");
        if (ptrChar == NULL)
        {
            ParseFen(START_FEN, pos);
        }
        else
        {
            ptrChar += 4;
            ParseFen(ptrChar, pos);
        }
    }

    ptrChar = strstr(lineIn, "moves");
    int move;
    if (ptrChar != NULL)
    {
        ptrChar += 6;
        while (*ptrChar)
        {
            move = ParseMove(ptrChar, pos);
            if (move == NOMOVE)
                break;
            MakeMove(pos, move);
            pos->ply = 0;
            while (*ptrChar && *ptrChar != ' ')
                ptrChar++;
            ptrChar++;
        }
    }
    // PrintBoard(pos);
}
void ParseOption(char *line, S_BOARD *pos)
{
    char *ptrChar = NULL;
    if (ptrChar = strstr(line, "name"))
    {
        ptrChar += 5;
        if (strncmp(ptrChar, "Ponder", 6) == 0)
        {
            ptrChar = strstr(line, "value");
            ptrChar += 6;
            if (strncmp(ptrChar, "true", 4) == 0)
            {
                pos->ponder = TRUE;
            }
            else if (strncmp(ptrChar, "false", 5) == 0)
            {
                pos->ponder = FALSE;
            }
        }
        else if (strncmp(ptrChar, "Hash value ", 11) == 0)
        {
            int MB = 64;
            sscanf(ptrChar, "%*s %*s %d", &MB);
            if (MB < 4)
                MB = 4;
            if (MB > MAX_HASH)
                MB = MAX_HASH;
            printf("Set Hash to %d MB\n", MB);
            InitHashTable(pos->HashTable, MB);
        }
        else if (strncmp(ptrChar, "Book value", 10) == 0)
        {
            char *ptrTrue = strstr(line, "true");
            if (ptrTrue != NULL)
            {
                EngineOptions->UseBook = TRUE;
            }
            else
                EngineOptions->UseBook = FALSE;
        }
    }
}
void Uci_Loop(S_BOARD *pos, S_SEARCHINFO *info)
{
    info->GAME_MODE = UCIMODE;
    info->POST_THINKING = TRUE;
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char line[INPUTBUFFER];
    printf("id name %s\n", NAME);
    printf("id author Jarvaang Enterprises\n");
    printf("option name Ponder type check default true\n");
    printf("option name Hash type spin default 64 min 4 max %d\n", MAX_HASH);
    printf("option name Book type check default true\n");
    printf("uciok\n");

    int MB = 64;
    while (TRUE)
    {
        memset(&line[0], 0, sizeof(line));
        fflush(stdout);
        if (!fgets(line, INPUTBUFFER, stdin))
            continue;

        if (line[0] == '\n')
            continue;

        if (!strncmp(line, "isready", 7))
        {
            printf("readyok\n");
            continue;
        }
        else if (!strncmp(line, "setoption", 9))
        {
            ParseOption(line, pos);
        }
        else if (!strncmp(line, "position", 8))
        {
            ParsePosition(line, pos);
        }
        else if (!strncmp(line, "ucinewgame", 10))
        {
            ParsePosition("position startpos\n", pos);
        }
        else if (!strncmp(line, "go", 2))
        {
            ParseGo(line, info, pos);
        }
        else if (!strncmp(line, "quit", 4))
        {
            info->quit = TRUE;
            break;
        }
        else if (!strncmp(line, "uci", 3))
        {
            printf("id name %s\n", NAME);
            printf("id author Jarvaang Enterprises\n");
            printf("option name Ponder type check default true\n");
            printf("option name Hash type spin default 64 min 4 max %d\n", MAX_HASH);
            printf("option name Book type check default true\n");
            printf("uciok\n");
        }
        else if (!strncmp(line, "debug", 4))
        {
            DebugAnalysisTest(pos, info);
            break;
        }
        if (info->quit)
            break;
    }
}
