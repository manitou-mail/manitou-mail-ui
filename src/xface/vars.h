
/*
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

#ifndef _COMPFACE_VARS_H_
#define _COMPFACE_VARS_H_

//BigInt B;

/* Internal face representation - 1 char per pixel is faster */
//char F[PIXELS];

/* data.h was established by sampling over 1000 faces and icons */
xface::Guesses xface::G =
#include "data.h"
;

//int status;

//jmp_buf comp_env;

/* At the bottom of the octree 2x2 elements are considered black if any
 * pixel is black.  The probabilities below give the distribution of the
 * 16 possible 2x2 patterns.  All white is not really a possibility and
 * has a probability range of zero.  Again, experimentally derived data.
 */
xface::Prob xface::freqs[16] = {
    { 0,   0}, {38,   0}, {38,  38}, {13, 152},
    {38,  76}, {13, 165}, {13, 178}, { 6, 230},
    {38, 114}, {13, 191}, {13, 204}, { 6, 236},
    {13, 217}, { 6, 242}, { 5, 248}, { 3, 253}
};

char xface::HexDigits[] = "0123456789ABCDEF";

/* A stack of probability values */
//Prob *ProbBuf[PIXELS * 2];
//int NumProbs = 0;

xface::Prob xface::levels[4][3] = {
    {{  1, 255}, {251, 0}, {  4, 251}},   /* Top of tree almost always grey */
    {{  1, 255}, {200, 0}, { 55, 200}},
    {{ 33, 223}, {159, 0}, { 64, 159}},
    {{131,   0}, {  0, 0}, {125, 131}}    /* Grey disallowed at bottom */
};

#endif /* _COMPFACE_VARS_H_ */
