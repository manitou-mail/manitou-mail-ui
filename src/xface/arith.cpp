
/*
 *
 *  Compface - 48x48x1 image compression and decompression
 *
 *  Copyright (c) 1990-2002 James Ashton - Sydney University
 *
 *  Permission is given to distribute these sources, as long as the
 *  copyright messages are not removed.
 *
 *  No responsibility is taken for any errors on inaccuracies inherent
 *  either to the comments or the code of this program, but if reported
 *  to me, then an attempt will be made to fix them.
 */

#include "compface.h"

void
xface::RevPush(Prob *p)
{
    if (NumProbs >= PIXELS * 2 - 1) {
      throw ERR_INTERNAL;
    }
    ProbBuf[NumProbs++] = p;
}


void
xface::BigPush(Prob *p)
{
    XFACE_WORD tmp;

    BigDiv(p->p_range, &tmp);
    BigMul(0);
    BigAdd(tmp + p->p_offset);
}


int
xface::BigPop(Prob *p)
{
  XFACE_WORD tmp;
    int i;

    BigDiv(0, &tmp);
    i = 0;
    while ((tmp < p->p_offset) || (tmp >= p->p_range + p->p_offset)) {
        p++;
        i++;
    }
    BigMul(p->p_range);
    BigAdd(tmp - p->p_offset);
    return(i);
}


#ifdef DEBUG_XFACE
void
xface::BigPrint()              /* Print a BigInt in HexaDecimal. */
{
    int i, c, count;
    XFACE_WORD *w;

    count = 0;
    w = B.b_word + (i = B.b_words);
    while (i--) {
        w--;
        c = *((*w >> 4) + HexDigits);
        putc(c, stderr);
        c = *((*w & 0xf) + HexDigits);
        putc(c, stderr);
        if (++count >= 36) {
            putc('\\', stderr);
            putc('\n', stderr);
            count = 0;
        }
    }
    putc('\n', stderr);
}
#endif /*DEBUG_XFACE*/


/* Divide B by a storing the result in B and the remainder in the word
 * pointer to by r.
 */

void
xface::BigDiv(XFACE_WORD a, XFACE_WORD *r)
{
    int i;
    XFACE_WORD *w;
    COMP c, d;

    a &= WORDMASK;
    if ((a == 1) || (B.b_words == 0)) {
        *r = 0;
        return;
    }

/* Treat this as a == WORDCARRY and just shift everything right a XFACE_WORD */

    if (a == 0)    {
        i = --B.b_words;
        w = B.b_word;
        *r = *w;
        while (i--) {
            *w = *(w + 1);
            w++;
        }
        *w = 0;
        return;
    }
    w = B.b_word + (i = B.b_words);
    c = 0;
    while (i--) {
        c <<= BITSPERWORD;
        c += (COMP) *--w;
        d = c / (COMP) a;
        c = c % (COMP) a;
        *w = (XFACE_WORD) (d & WORDMASK);
    }
    *r = c;
    if (B.b_word[B.b_words - 1] == 0) {
        B.b_words--;
    }
}


/* Multiply a by B storing the result in B. */

void
xface::BigMul(XFACE_WORD a)
{
    int i;
    XFACE_WORD *w;
    COMP c;

    a &= WORDMASK;
    if ((a == 1) || (B.b_words == 0)) {
        return;
    }

/* Treat this as a == WORDCARRY and just shift everything left a XFACE_WORD */

    if (a == 0) {
        if ((i = B.b_words++) >= MAXWORDS - 1) {
	  throw ERR_INTERNAL;
        }
        w = B.b_word + i;
        while (i--) {
            *w = *(w - 1);
            w--;
        }
        *w = 0;
        return;
    }
    i = B.b_words;
    w = B.b_word;
    c = 0;
    while (i--) {
        c += (COMP)*w * (COMP)a;
        *(w++) = (XFACE_WORD)(c & WORDMASK);
        c >>= BITSPERWORD;
    }
    if (c) {
        if (B.b_words++ >= MAXWORDS) {
            throw ERR_INTERNAL;
        }
        *w = (COMP)(c & WORDMASK);
    }
}


/* Subtract a from B storing the result in B. */

void
xface::BigSub(XFACE_WORD a)
{
    int i;
    XFACE_WORD *w;
    COMP c;

    a &= WORDMASK;
    if (a == 0) {
        return;
    }
    i = 1;
    w = B.b_word;
    c = (COMP) *w - (COMP) a;
    *w = (XFACE_WORD) (c & WORDMASK);
    while (c & WORDCARRY) {
        if (i >= B.b_words) {
	  throw ERR_INTERNAL;
        }
        c = (COMP) *++w - 1;
        *w = (XFACE_WORD) (c & WORDMASK);
        i++;
    }
    if ((i == B.b_words) && (*w == 0) && (i > 0)) {
        B.b_words--;
    }
}


/* Add to a to B storing the result in B. */

void
xface::BigAdd(XFACE_WORD a)
{
    int i;
    XFACE_WORD *w;
    COMP c;

    a &= WORDMASK;
    if (a == 0) {
        return;
    }
    i = 0;
    w = B.b_word;
    c = a;
    while ((i < B.b_words) && c) {
        c += (COMP) *w;
        *w++ = (XFACE_WORD) (c & WORDMASK);
        c >>= BITSPERWORD;
        i++;
    }
    if ((i == B.b_words) && c) {
        if (B.b_words++ >= MAXWORDS) {
	  throw ERR_INTERNAL;
        }
        *w = (COMP) (c & WORDMASK);
    }
}


void
xface::BigClear()
{
    B.b_words = 0;
}
