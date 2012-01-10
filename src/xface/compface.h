
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

#include <string.h>
//#include <fcntl.h>
//#include <setjmp.h>

class xface {
public:
/* Need to know how many bits per hexadecimal digit for io */
#define BITSPERDIG 4
 static char HexDigits[];

/* Define the face size - 48x48x1 */
#define WIDTH  48
#define HEIGHT WIDTH

/* Total number of pixels and digits */
#define PIXELS (WIDTH * HEIGHT)
#define DIGITS (PIXELS / BITSPERDIG)

 char F[PIXELS];

 const char* buffer() const {
   return F;
 }

/* Output formatting word lengths and line lengths */
#define DIGSPERWORD  4
#define WORDSPERLINE (WIDTH / DIGSPERWORD / BITSPERDIG)

/* Compressed output uses the full range of printable characters.
 * in ascii these are in a contiguous block so we just need to know
 * the first and last.  The total number of printables is needed too.
 */
#define FIRSTPRINT '!'
#define LASTPRINT  '~'
#define NUMPRINTS  (LASTPRINT - FIRSTPRINT + 1)

/* output line length for compressed data */
#define MAXLINELEN 78

/* Portable, very large unsigned integer arithmetic is needed.
 * Implementation uses arrays of WORDs.  COMPs must have at least
 * twice as many bits as WORDs to handle intermediate results.
 */
#define WORD        unsigned char
#define COMP        unsigned long
#define BITSPERWORD 8
#define WORDCARRY   (1 << BITSPERWORD)
#define WORDMASK    (WORDCARRY - 1)
#define MAXWORDS    ((PIXELS * 2 + BITSPERWORD - 1) / BITSPERWORD)

typedef struct bigint
{
    int b_words;
    WORD b_word[MAXWORDS];
} BigInt;

 BigInt B;

/* This is the guess the next pixel table.  Normally there are 12 neighbour
 * pixels used to give 1<<12 cases but in the upper left corner lesser
 * numbers of neighbours are available, leading to 6231 different guesses.
 */
typedef struct guesses {
    char g_00[1<<12];
    char g_01[1<<7];
    char g_02[1<<2];
    char g_10[1<<9];
    char g_20[1<<6];
    char g_30[1<<8];
    char g_40[1<<10];
    char g_11[1<<5];
    char g_21[1<<3];
    char g_31[1<<5];
    char g_41[1<<6];
    char g_12[1<<1];
    char g_22[1<<0];
    char g_32[1<<2];
    char g_42[1<<2];
} Guesses;

static Guesses G;

/* Data of varying probabilities are encoded by a value in the range 0 - 255.
 * The probability of the data determines the range of possible encodings.
 * Offset gives the first possible encoding of the range.
 */
typedef struct prob {
    WORD p_range;
    WORD p_offset;
} Prob;

 Prob *ProbBuf[PIXELS * 2];
 int NumProbs;

/* Each face is encoded using 9 octrees of 16x16 each.  Each level of the
 * trees has varying probabilities of being white, grey or black.
 * The table below is based on sampling many faces.
 */

#define BLACK 0
#define GREY  1
#define WHITE 2

 static Prob levels[4][3];
 static Prob freqs[16];

#define ERR_OK        0     /* successful completion */
#define ERR_EXCESS    1     /* completed OK but some input was ignored */
#define ERR_INSUFF    -1    /* insufficient input.  Bad face format? */
#define ERR_INTERNAL  -2    /* Arithmetic overflow or buffer overflow */

 int status;

//extern jmp_buf comp_env;

int AllBlack(char *, int, int);
int AllWhite(char *, int, int);
int BigPop(Prob *);
int compface(char *);
int ReadBuf();
int Same(char *, int, int);
int uncompface(char *);
int WriteBuf();

void BigAdd(WORD);
void BigClear();
void BigDiv(WORD, WORD *);
void BigMul(WORD);
void BigPrint();
void BigPush(Prob *);
void BigRead(char *);
void BigSub(WORD);
void BigWrite(char *);
void CompAll(char *);
void Compress(char *, int, int, int);
void GenFace();
void PopGreys(char *, int, int);
void PushGreys(char *, int, int);
void ReadFace(char *);
void RevPush(Prob *);
void UnCompAll(char *);
void UnCompress(char *, int, int, int);
void UnGenFace();
void WriteFace(char *);
void Gen(char *f);
};
