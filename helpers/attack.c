// attack.c

#include "stdio.h"
#include "../definitions/defs.h"

const int KnDir[8] = {-8, -19, -21, -12, 8, 19, 21, 12};
const int RkDir[4] = {-1, -10, 1, 10};
const int BiDir[4] = {-9, -11, 11, 9};
const int KiDir[8] = {-1, -10, 1, 10, -9, -11, 11, 9};

int SqAttacked(const int sq, const int side, const S_BOARD *pos)
{

    int pce, index, t_sq, dir;

    ASSERT(SqOnBoard(sq));
    ASSERT(SideValid(side));
    ASSERT(CheckBoard(pos));

    // pawns
    if (side == WHITE)
    {
        if (pos->pieces[sq - 11] == wP || pos->pieces[sq - 9] == wP)
        {
            return TRUE;
        }
    }
    else
    {
        if (pos->pieces[sq + 11] == bP || pos->pieces[sq + 9] == bP)
        {
            return TRUE;
        }
    }

    // knights
    for (index = 0; index < 8; ++index)
    {
        pce = pos->pieces[sq + KnDir[index]];
        ASSERT(PceValidEmptyOffbrd(pce));
        if (pce != OFFBOARD && (IsKn(pce) == TRUE) && (PieceCol[pce] == side))
        {
            return TRUE;
        }
    }

    // Rooks and Queens
    for (index = 0; index < 4; ++index)
    {
        dir = RkDir[index];
        t_sq = sq + dir;
        ASSERT(SqIs120(t_sq));
        pce = pos->pieces[t_sq];
        ASSERT(PceValidEmptyOffbrd(pce));
        while (pce != OFFBOARD)
        {
            if (pce != EMPTY)
            {
                if ((IsRQ(pce) == TRUE) && PieceCol[pce] == side)
                {
                    return TRUE;
                }
                break;
            }
            t_sq += dir;
            ASSERT(SqIs120(t_sq));
            pce = pos->pieces[t_sq];
        }
    }

    // Bishops and Queens
    for (index = 0; index < 4; ++index)
    {
        dir = BiDir[index];
        t_sq = sq + dir;
            ASSERT(SqIs120(t_sq));
        pce = pos->pieces[t_sq];
        ASSERT(PceValidEmptyOffbrd(pce));
        while (pce != OFFBOARD)
        {
            if (pce != EMPTY)
            {
                if ((IsBQ(pce) == TRUE) && PieceCol[pce] == side)
                {
                    return TRUE;
                }
                break;
            }
            t_sq += dir;
            ASSERT(SqIs120(t_sq));
            pce = pos->pieces[t_sq];
        }
    }

    // Kings
    for (index = 0; index < 8; ++index)
    {
        pce = pos->pieces[sq + KiDir[index]];
        ASSERT(PceValidEmptyOffbrd(pce));
        if (pce != OFFBOARD && (IsKi(pce) == TRUE) && PieceCol[pce] == side)
        {
            return TRUE;
        }
    }

    return FALSE;
}