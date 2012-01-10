
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

int
xface::Same(char *f, int wid, int hei)
{
	char val, *row;
	int x;

	val = *f;
	while (hei--) {
		row = f;
		x = wid;
		while (x--) {
			if (*(row++) != val) {
				return(0);
            }
        }
		f += WIDTH;
	}
	return(1);
}


int
xface::AllBlack(char *f, int wid, int hei)
{
	if (wid > 3) {
		wid /= 2;
		hei /= 2;
		return(AllBlack(f, wid, hei) && AllBlack(f + wid, wid, hei) &&
		       AllBlack(f + WIDTH * hei, wid, hei) &&
		       AllBlack(f + WIDTH * hei + wid, wid, hei));
	} else {
		return(*f || *(f + 1) || *(f + WIDTH) || *(f + WIDTH + 1));
    }
}


int
xface::AllWhite(char *f, int wid, int hei)
{
	return((*f == 0) && Same(f, wid, hei));
}


void
xface::PopGreys(char *f, int wid, int hei)
{
	if (wid > 3) {
		wid /= 2;
		hei /= 2;
		PopGreys(f, wid, hei);
		PopGreys(f + wid, wid, hei);
		PopGreys(f + WIDTH * hei, wid, hei);
		PopGreys(f + WIDTH * hei + wid, wid, hei);
	} else {
		wid = BigPop(freqs);
		if (wid & 1) {
			*f = 1;
        }
		if (wid & 2) {
			*(f + 1) = 1;
        }
		if (wid & 4) {
			*(f + WIDTH) = 1;
        }
		if (wid & 8) {
			*(f + WIDTH + 1) = 1;
        }
	}
}


void
xface::PushGreys(char *f, int wid, int hei)
{
	if (wid > 3) {
		wid /= 2;
		hei /= 2;
		PushGreys(f, wid, hei);
		PushGreys(f + wid, wid, hei);
		PushGreys(f + WIDTH * hei, wid, hei);
		PushGreys(f + WIDTH * hei + wid, wid, hei);
	} else {
		RevPush(freqs + *f + 2 * *(f + 1) +
                4 * *(f + WIDTH) + 8 * *(f + WIDTH + 1));
    }
}


void
xface::UnCompress(char *f, int wid, int hei, int lev)
{
	switch (BigPop(&levels[lev][0])) {
		case WHITE :
			return;
		case BLACK :
			PopGreys(f, wid, hei);
			return;
		default :
			wid /= 2;
			hei /= 2;
			lev++;
			UnCompress(f, wid, hei, lev);
			UnCompress(f + wid, wid, hei, lev);
			UnCompress(f + hei * WIDTH, wid, hei, lev);
			UnCompress(f + wid + hei * WIDTH, wid, hei, lev);
			return;
	}
}


void
xface::Compress(char *f, int wid, int hei, int lev)
{
	if (AllWhite(f, wid, hei)) {
		RevPush(&levels[lev][WHITE]);
		return;
	}
	if (AllBlack(f, wid, hei)) {
		RevPush(&levels[lev][BLACK]);
		PushGreys(f, wid, hei);
		return;
	}
	RevPush(&levels[lev][GREY]);
	wid /= 2;
	hei /= 2;
	lev++;
	Compress(f, wid, hei, lev);
	Compress(f + wid, wid, hei, lev);
	Compress(f + hei * WIDTH, wid, hei, lev);
	Compress(f + wid + hei * WIDTH, wid, hei, lev);
}

void
xface::UnCompAll(char *fbuf)
{
	char *p;

	BigClear();
	BigRead(fbuf);
	p = F;
	while (p < F + PIXELS) {
		*(p++) = 0;
    }
	UnCompress(F, 16, 16, 0);
	UnCompress(F + 16, 16, 16, 0);
	UnCompress(F + 32, 16, 16, 0);
	UnCompress(F + WIDTH * 16, 16, 16, 0);
	UnCompress(F + WIDTH * 16 + 16, 16, 16, 0);
	UnCompress(F + WIDTH * 16 + 32, 16, 16, 0);
	UnCompress(F + WIDTH * 32, 16, 16, 0);
	UnCompress(F + WIDTH * 32 + 16, 16, 16, 0);
	UnCompress(F + WIDTH * 32 + 32, 16, 16, 0);
}

void
xface::CompAll(char *fbuf)
{
	Compress(F, 16, 16, 0);
	Compress(F + 16, 16, 16, 0);
	Compress(F + 32, 16, 16, 0);
	Compress(F + WIDTH * 16, 16, 16, 0);
	Compress(F + WIDTH * 16 + 16, 16, 16, 0);
	Compress(F + WIDTH * 16 + 32, 16, 16, 0);
	Compress(F + WIDTH * 32, 16, 16, 0);
	Compress(F + WIDTH * 32 + 16, 16, 16, 0);
	Compress(F + WIDTH * 32 + 32, 16, 16, 0);
	BigClear();
	while (NumProbs > 0) {
		BigPush(ProbBuf[--NumProbs]);
    }
	BigWrite(fbuf);
}
