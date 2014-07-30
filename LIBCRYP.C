/************************************************************************/
/*                              libCryp.c                               */
/*                                                                      */
/*                    Library of encryption for Citadel                 */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 87Aug06 HAW  Hash added.                                             */
/* 85Nov15 HAW  Created.                                                */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      crypte()                encrypts/decrypts data blocks           */
/*      hash()                  hashes a string to an integer           */
/************************************************************************/

extern CONFIG cfg;               /* Configuration variables      */

#ifndef NO_CRYPT

/************************************************************************/
/*      crypte() encrypts/decrypts data blocks                          */
/*                                                                      */
/*  This was at first using a full multiply/add pseudo-random sequence  */
/*  generator, but 8080s don't like to multiply.  Slowed down I/O       */
/*  noticably.  Rewrote for speed.                                      */
/*      84Sep04 HAW  I'll just use it......                             */
/************************************************************************/
void crypte(buf, len, seed)
AN_UNSIGNED   *buf;
unsigned      len, seed;
{
    static AN_UNSIGNED *b;      /* Make this static for speed (I guess),*/
    static  int c, s;           /* since register variables not around  */

    seed        = (seed + cfg.cryptSeed) & 0xFF;
    b           = buf;
    c           = len;
    s           = seed;
    for (;  c;  c--) {
        *b++   ^= s;
        s       = (s + CRYPTADD)  &  0xFF;
    }
}
#endif

/************************************************************************/
/*      hash() hashes a string to an integer                            */
/************************************************************************/
int hash(str)
char *str;
{
    int  h, shift;

    for (h=shift=0;  *str;  shift=(shift+1)&7, str++) {
        h ^= (toUpper(*str)) << shift;
    }
    return h;
}
