// search.c

#include "stdio.h"
#include "../definitions/defs.h"

int rootDepth;

static void CheckUp(S_SEARCHINFO *info)
{
    // .. check if time up, or interrupt from GUI
    if (info->timeset == TRUE && GetTimeMs() > info->stoptime)
    {
        info->stopped = TRUE;
    }

    ReadInput(info);
}

static void PickNextMove(int moveNum, S_MOVELIST *list)
{
    S_MOVE temp;
    int index = 0;
    int bestScore = 0;
    int bestNum = moveNum;

    for (index = moveNum; index < list->count; ++index)
    {
        if (list->moves[index].score > bestScore)
        {
            bestScore = list->moves[index].score;
            bestNum = index;
        }
    }

    ASSERT(moveNum >= 0 && moveNum < list->count);
    ASSERT(bestNum >= 0 && bestNum < list->count);
    ASSERT(bestNum >= moveNum);

    temp = list->moves[moveNum];
    list->moves[moveNum] = list->moves[bestNum];
    list->moves[bestNum] = temp;
}

static int IsRepitition(const S_BOARD *pos)
{

    int index = 0;

    for (index = (pos->hisPly - pos->fiftyMove); index < pos->hisPly - 1; ++index)
    {
        ASSERT(index >= 0 && index < MAXGAMEMOVES);
        if (pos->posKey == pos->history[index].posKey)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info)
{

    int index = 0;
    int index2 = 0;

    for (index = 0; index < 13; ++index)
    {
        for (index2 = 0; index2 < BRD_SQ_NUM; ++index2)
        {
            pos->searchHistory[index][index2] = 0;
        }
    }

    for (index = 0; index < 2; ++index)
    {
        for (index2 = 0; index2 < MAXDEPTH; ++index2)
        {
            pos->searchKillers[index][index2] = 0;
        }
    }

    pos->HashTable->overWrite = 0;
    pos->HashTable->hit = 0;
    pos->HashTable->cut = 0;
    pos->ply = 0;

    info->stopped = 0;
    info->ponderHit = 0;
    info->ponderStop = 0;
    info->nodes = 0;
    info->fh = 0;
    info->fhf = 0;
    return;
}

static int Quiescence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info)
{

    ASSERT(CheckBoard(pos));
    ASSERT(beta > alpha);

    if ((info->nodes & 2047) == 0)
    {
        CheckUp(info);
    }

    info->nodes++;

    if (IsRepitition(pos) || pos->fiftyMove >= 100)
    {
        return 0;
    }

    if (pos->ply > MAXDEPTH - 1)
    {
        return EvalPosition(pos);
    }

    int Score = EvalPosition(pos);
    ASSERT(Score > -INFINITE && Score < INFINITE);

    if (Score >= beta)
    {
        return beta;
    }

    if (Score > alpha)
    {
        alpha = Score;
    }

    S_MOVELIST list[1];
    GenerateAllCaps(pos, list);

    int MoveNum = 0;
    int Legal = 0;
    int OldAlpha = alpha;
    Score = -INFINITE;

    for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
    {
        PickNextMove(MoveNum, list);
        if (!MakeMove(pos, list->moves[MoveNum].move))
        {
            continue;
        }

        Legal++;
        Score = -Quiescence(-beta, -alpha, pos, info);

        TakeMove(pos);

        if (info->stopped == TRUE)
        {
            return 0;
        }

        if (Score > alpha)
        {
            if (Score >= beta)
            {
                if (Legal == 1)
                {
                    info->fhf++;
                }
                info->fh++;
                return beta;
            }
            alpha = Score;
        }
    }

    ASSERT(alpha >= OldAlpha);

    return alpha;
}

static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int DoNull)
{
    ASSERT(CheckBoard(pos));
    ASSERT(beta > alpha);
    ASSERT(depth >= 0);

    if (depth <= 0)
    {
        return Quiescence(alpha, beta, pos, info);
    }

    if ((info->nodes & 2047) == 0)
    {
        CheckUp(info);
    }

    info->nodes++;

    if ((IsRepitition(pos) == TRUE) || (pos->fiftyMove >= 100) && pos->ply)
    {
        return 0;
    }

    if (pos->ply > (MAXDEPTH - 1))
    {
        return EvalPosition(pos);
    }

    int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);

    if (InCheck == TRUE)
    {
        depth++;
    }

    int Score = -INFINITE;
    int PvMove = NOMOVE;

    if (ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth) == TRUE)
    {
        pos->HashTable->cut++;
        return Score;
    }

    if (DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 0) && depth >= 4)
    {
        MakeNullMove(pos);
        Score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, FALSE);
        TakeNullMove(pos);
        if (info->stopped == TRUE)
        {
            return 0;
        }
        if (Score >= beta && abs(Score) < ISMATE)
        {
            info->nullCut++;
            return beta;
        }
    }

    S_MOVELIST list[1];
    GenerateAllMoves(pos, list);

    int MoveNum = 0;
    int Legal = 0;
    int OldAlpha = alpha;
    int bestMove = NOMOVE;

    int BestScore = -INFINITE;

    Score = -INFINITE;

    if (PvMove != NOMOVE)
    {
        for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
        {
            if (list->moves[MoveNum].move == PvMove)
            {
                list->moves[MoveNum].score = 2000000;
                break;
            }
        }
    }

    for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
    {
        PickNextMove(MoveNum, list);
        if (!MakeMove(pos, list->moves[MoveNum].move))
        {
            continue;
        }

        Legal++;
        Score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE);

        TakeMove(pos);

        if (info->stopped == TRUE)
        {
            return 0;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            bestMove = list->moves[MoveNum].move;
            if (Score > alpha)
            {
                if (Score >= beta)
                {
                    if (Legal == 1)
                    {
                        info->fhf++;
                    }
                    info->fh++;
                    if (!(list->moves[MoveNum].move & MFLAGCAP))
                    {
                        pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
                        pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
                    }

                    StoreHashEntry(pos, bestMove, beta, HFBETA, depth);

                    return beta;
                }
                alpha = Score;

                if (!(list->moves[MoveNum].move & MFLAGCAP))
                {
                    pos->searchHistory[pos->pieces[FROMSQ(bestMove)]][TOSQ(bestMove)] += depth;
                }
            }
        }
    }

    if (Legal == 0)
    {
        if (InCheck)
        {
            return -INFINITE + pos->ply;
        }
        else
        {
            return 0;
        }
    }

    ASSERT(alpha >= OldAlpha);

    if (alpha != OldAlpha)
    {
        StoreHashEntry(pos, bestMove, BestScore, HFEXACT, depth);
    }
    else
    {
        StoreHashEntry(pos, bestMove, alpha, HFALPHA, depth);
    }

    return alpha;
}

void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info)
{
    int bestMove = NOMOVE;
    int bestScore = -INFINITE;
    int currentDepth = 0;
    int pvMoves = 0;
    int pvNum = 0;
    int tPonder = 0;

    ClearForSearch(pos, info);

    if (EngineOptions->UseBook == TRUE)
    {
        bestMove = GetBookMove(pos);
    }

    // iterative deepening
    if (bestMove == NOMOVE)
    {
        for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth)
        {
            // alpha	 beta
            rootDepth = currentDepth;
            bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE);

            if (info->stopped == TRUE)
            {
                break;
            }

            pvMoves = GetPvLine(currentDepth, pos);
            bestMove = pos->PvArray[0];
            tPonder = pos->PvArray[1];
            if (info->GAME_MODE == UCIMODE)
            {
                printf("info score cp %d depth %d nodes %ld time %d ", bestScore, currentDepth, info->nodes, GetTimeMs() - info->starttime);
            }
            else if (info->GAME_MODE == XBOARDMODE && info->POST_THINKING == TRUE)
            {
                printf("%d %d %d %ld ",
                       currentDepth, bestScore, (GetTimeMs() - info->starttime) / 10, info->nodes);
            }
            else if (info->POST_THINKING == TRUE)
            {
                printf("score:%d depth:%d nodes:%ld time:%d(ms) ",
                       bestScore, currentDepth, info->nodes, GetTimeMs() - info->starttime);
            }
            if (info->GAME_MODE == UCIMODE || info->POST_THINKING == TRUE)
            {
                pvMoves = GetPvLine(currentDepth, pos);
                if (!info->GAME_MODE == XBOARDMODE)
                {
                    printf("pv");
                }
                for (pvNum = 0; pvNum < pvMoves; ++pvNum)
                {
                    printf(" %s", PrMove(pos->PvArray[pvNum]));
                }
                printf("\n");
            }
        }
    }

    if (info->GAME_MODE == UCIMODE)
    {
        printf("bestmove %s", PrMove(bestMove));
        printf(" ponder %s\n", PrMove(tPonder));
    }
    else if (info->GAME_MODE == XBOARDMODE)
    {
        printf("move %s\n", PrMove(bestMove));
        MakeMove(pos, bestMove);
    }
    else
    {
        printf("\n\n***!! Jarvaang makes move %s !!***\n\n", PrMove(bestMove));
        MakeMove(pos, bestMove);
        PrintBoard(pos);
    }
}

static int PonderQuiescence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info)
{
    ASSERT(CheckBoard(pos));
    ASSERT(beta > alpha);

    if ((info->nodes & 1023) == 0)
    {
        CheckUp(info);
    }

    info->nodes++;

    if (IsRepitition(pos) || pos->fiftyMove >= 100)
    {
        return 0;
    }

    if (pos->ply > MAXDEPTH - 1)
    {
        return EvalPosition(pos);
    }

    int Score = EvalPosition(pos);

    if (Score >= beta)
    {
        return beta;
    }

    if (Score > alpha)
    {
        alpha = Score;
    }

    S_MOVELIST list[1];
    GenerateAllCaps(pos, list);

    int MoveNum = 0;
    int Legal = 0;
    int OldAlpha = alpha;
    Score = -INFINITE;

    for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
    {
        PickNextMove(MoveNum, list);
        if (!MakeMove(pos, list->moves[MoveNum].move))
        {
            continue;
        }

        Legal++;
        Score = -PonderQuiescence(-beta, -alpha, pos, info);

        TakeMove(pos);

        if (info->ponderHit == TRUE)
        {
            return EvalPosition(pos);
        }
        else if (info->ponderStop == TRUE)
        {
            return 0;
        }

        if (info->stopped == TRUE)
        {
            return 0;
        }

        if (Score > alpha)
        {
            if (Score >= beta)
            {
                if (Legal == 1)
                {
                    info->fhf++;
                }
                info->fh++;
                return beta;
            }

            alpha = Score;
        }
    }

    ASSERT(alpha >= OldAlpha);

    return alpha;
}

static int PonderAlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int DoNull)
{
    ASSERT(CheckBoard(pos));
    ASSERT(beta > alpha);
    ASSERT(depth >= 0);

    if (depth <= 0)
    {
        return PonderQuiescence(alpha, beta, pos, info);
    }

    if ((info->nodes & 2047) == 0)
    {
        CheckUp(info);
    }

    info->nodes++;

    if ((IsRepitition(pos) == TRUE) || (pos->fiftyMove >= 100) && pos->ply)
    {
        return 0;
    }

    if (pos->ply > (MAXDEPTH - 1))
    {
        return EvalPosition(pos);
    }

    int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);
    if (InCheck == TRUE)
    {
        depth++;
    }

    int Score = -INFINITE;
    int PvMove = NOMOVE;

    if (ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth) == TRUE)
    {
        pos->HashTable->cut++;
        return Score;
    }

    if (DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 0) && depth >= 4)
    {
        MakeNullMove(pos);
        Score = -PonderAlphaBeta(-beta, -beta + 1, depth - 4, pos, info, FALSE);
        TakeNullMove(pos);
        if (info->stopped == TRUE)
        {
            return 0;
        }
        if (Score >= beta && abs(Score) < ISMATE)
        {
            info->nullCut++;
            return beta;
        }
    }

    S_MOVELIST list[1];
    GenerateAllMoves(pos, list);

    int MoveNum = 0;
    int Legal = 0;
    int OldAlpha = alpha;
    int bestMove = NOMOVE;
    Score = -INFINITE;
    int bestScore = -INFINITE;

    if (PvMove != NOMOVE)
    {
        for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
        {
            if (list->moves[MoveNum].move == PvMove)
            {
                list->moves[MoveNum].score = 2000000;
                break;
            }
        }
    }

    for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
    {
        PickNextMove(MoveNum, list);
        if (!MakeMove(pos, list->moves[MoveNum].move))
        {
            continue;
        }

        Legal++;
        Score = -PonderAlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE);

        TakeMove(pos);

        if (info->ponderHit == TRUE)
        {
            return EvalPosition(pos);
        }
        else if (info->ponderStop == TRUE)
        {
            return 0;
        }

        if (info->stopped == TRUE)
        {
            return 0;
        }

        if (Score > bestScore)
        {
            bestScore = Score;
            bestMove = list->moves[MoveNum].move;
            if (Score > alpha)
            {
                if (Score >= beta)
                {
                    if (Legal == 1)
                    {
                        info->fhf++;
                    }
                    info->fh++;
                    if (!(list->moves[MoveNum].move & MFLAGCAP))
                    {
                        pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
                        pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
                    }

                    StoreHashEntry(pos, bestMove, beta, HFBETA, depth);

                    return beta;
                }

                alpha = Score;

                if (!(list->moves[MoveNum].move & MFLAGCAP))
                {
                    pos->searchHistory[pos->pieces[FROMSQ(bestMove)]][TOSQ(bestMove)] += depth;
                }
            }
        }
    }

    if (Legal == 0)
    {
        if (INCHECK == TRUE)
        {
            return -INFINITE + pos->ply;
        }
        else
        {
            return 0;
        }
    }

    ASSERT(alpha >= OldAlpha);

    if (alpha != OldAlpha)
    {
        StoreHashEntry(pos, bestMove, bestScore, HFEXACT, depth);
    }
    else
    {
        StoreHashEntry(pos, bestMove, alpha, HFALPHA, depth);
    }

    return alpha;
}

void PonderSearchPosition(S_BOARD *pos, S_SEARCHINFO *info)
{
    printf("\n\nPondering  \n\n");
    int bestMove = NOMOVE;
    int bMove = NOMOVE;
    int bestScore = -INFINITE;
    int currentDepth = 0;
    int pvMoves = 0;
    int pvNum = 0;
    int temp_ponder;

    ClearForSearch(pos, info);

    // iterative deepening
    for (currentDepth = 1; currentDepth <= MAXDEPTH; ++currentDepth)
    {

        rootDepth = currentDepth;
        bestScore = PonderAlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE);
        if (info->ponderStop == TRUE)
        {
            break;
        }
        /**
         * If a ponderhit is obtained 
         * Update stop time
         * Drop into normal search and proceed with Normal search
         */
        if (info->ponderHit == TRUE)
        {
            info->stoptime = info->starttime + info->ponderDrop;
            info->stopped = FALSE;
            SearchPosition(pos, info);
            break;
        }

        if (info->stopped == TRUE)
        {
            break;
        }

        pvMoves = GetPvLine(currentDepth, pos);
        bestMove = pos->PvArray[0];
        temp_ponder = pos->PvArray[1];

        printf("info score cp %d depth %d ", bestScore, currentDepth);
        printf("nodes %ld time %d pv", info->nodes, GetTimeMs() - info->starttime);
        pvMoves = GetPvLine(currentDepth, pos);
        for (pvNum = 0; pvNum < pvMoves; ++pvNum)
        {
            printf(" %s", PrMove(pos->PvArray[pvNum]));
        }
        printf("\n");
    }

    printf("bestmove %s", PrMove(bestMove));
    printf(" ponder %s\n", PrMove(temp_ponder));
}
