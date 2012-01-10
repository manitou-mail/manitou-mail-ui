
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
xface::BigRead(char *fbuf)
{
    int c;

    while (*fbuf != '\0') {
        c = *(fbuf++);
        if ((c < FIRSTPRINT) || (c > LASTPRINT)) {
            continue;
        }
        BigMul(NUMPRINTS);
        BigAdd((WORD)(c - FIRSTPRINT));
    }
}


void
xface::BigWrite(char *fbuf)
{
    static WORD tmp;
    static char buf[DIGITS];
    char *s;
    int i;

    s = buf;
    while (B.b_words > 0) {
        BigDiv(NUMPRINTS, &tmp);
        *(s++) = tmp + FIRSTPRINT;
    }
    i = 7;              /* Leave room for the field name on the first line */
    *(fbuf++) = ' ';
    while (s-- > buf) {
        if (i == 0) {
            *(fbuf++) = ' ';
        }
        *(fbuf++) = *s;
        if (++i >= MAXLINELEN) {
            *(fbuf++) = '\n';
            i = 0;
        }
    }
    if (i > 0) {
        *(fbuf++) = '\n';
    }
    *(fbuf++) = '\0';
}


void
xface::ReadFace(char *fbuf)
{
    int c, i;
    char *s, *t;

    t = s = fbuf;
    for (i = strlen(s); i > 0; i--) {
        c = (int) *(s++);
        if ((c >= '0') && (c <= '9')) {
            if (t >= fbuf + DIGITS) {
                status = ERR_EXCESS;
                break;
            }
            *(t++) = c - '0';
        } else if ((c >= 'A') && (c <= 'F')) {
            if (t >= fbuf + DIGITS) {
                status = ERR_EXCESS;
                break;
            }
            *(t++) = c - 'A' + 10;
        } else if ((c >= 'a') && (c <= 'f')) {
            if (t >= fbuf + DIGITS) {
                status = ERR_EXCESS;
                break;
            }
            *(t++) = c - 'a' + 10;
        } else if (((c == 'x') || (c == 'X')) && (t > fbuf) && (*(t-1) == 0)) {
            t--;
        }
    }
    if (t < fbuf + DIGITS) {
      throw ERR_INSUFF;
    }
    s = fbuf;
    t = F;
    c = 1 << (BITSPERDIG - 1);
    while (t < F + PIXELS) {
        *(t++) = (*s & c) ? 1 : 0;
        if ((c >>= 1) == 0) {
            s++;
            c = 1 << (BITSPERDIG - 1);
        }
    }
}


void
xface::WriteFace(char *fbuf)
{
    char *s, *t;
    int i, bits, digits, words;

    s = F;
    t = fbuf;
    bits = digits = words = i = 0;
    while (s < F + PIXELS) {
        if ((bits == 0) && (digits == 0)) {
            *(t++) = '0';
            *(t++) = 'x';
        }
        if (*(s++)) {
            i = i * 2 + 1;
        } else {
            i *= 2;
        }
        if (++bits == BITSPERDIG) {
            *(t++) = *(i + HexDigits);
            bits = i = 0;
            if (++digits == DIGSPERWORD) {
                *(t++) = ',';
                digits = 0;
                if (++words == WORDSPERLINE) {
                    *(t++) = '\n';
                    words = 0;
                }
            }
        }
    }
    *(t++) = '\0';
}
