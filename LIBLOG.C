/************************************************************************/
/*                              liblog.c                                */
/*                                                                      */
/*                  Citadel log code for the library                    */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 85Nov15 HAW  File created.                                           */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      getLog()                loads requested CTDLLOG record          */
/*      putLog()                stores a logBuffer into citadel.log     */
/************************************************************************/

logBuffer logBuf;                /* Log buffer of a person       */
int              thisLog;               /* entry currently in logBuf    */
FILE             *logfl;                /* log file descriptor          */

extern CONFIG cfg;               /* Configuration variables      */

/************************************************************************/
/*      getLog() loads requested log record into RAM buffer             */
/************************************************************************/
void getLog(lBuf, n)
logBuffer *lBuf;
int              n;
{
    long int s, r;

    if (lBuf == &logBuf)   thisLog      = n;

    r = LB_TOTAL_SIZE;                  /* To get away from overflows   */
    s = n * r;                          /* This should be offset        */
    n *= 3;

    if (cfg.weAre != CONFIGUR)
        fseek(logfl, s, 0);

    if (fread(lBuf, LB_SIZE, 1, logfl) != 1) {
        crashout("?getLog-read fail//EOF detected (1)!");
    }
#ifndef NO_CRYPT
    crypte(lBuf, LB_SIZE, n);           /* decode buffer    */
#endif
    if (fread(lBuf->lbgen, GEN_BULK, 1, logfl) != 1) {
        crashout("?getLog-read fail//EOF detected (2)!");
    }

    if (fread(lBuf->lbMail, MAIL_BULK, 1, logfl) != 1) {
        crashout("?getLog-read fail//EOF detected (3)!");
    }
}

/************************************************************************/
/*      putLog() stores given log record into ctdllog.sys               */
/************************************************************************/
void putLog(lBuf, n)
logBuffer *lBuf;
int              n;
{
    long int s, r;

    r = LB_TOTAL_SIZE;
    s = n * r;
    n   *= 3;
#ifndef NO_CRYPT
    crypte(lBuf, LB_SIZE, n);         /* encode buffer        */
#endif
    if (cfg.weAre != CONFIGUR)        /* No need if configuring         */
        fseek(logfl, s, 0);

    if (fwrite(lBuf, LB_SIZE, 1, logfl) != 1) {
        crashout("?putLog-write fail (1)!");
    }

    if (fwrite(lBuf->lbgen, GEN_BULK, 1, logfl) != 1) {
        crashout("?putLog-write fail (2)!");
    }

    if (fwrite(lBuf->lbMail, MAIL_BULK, 1, logfl) != 1) {
        crashout("?putLog-write fail (3)!");
    }
#ifndef NO_CRYPT
    crypte(lBuf, LB_SIZE, n);         /* encode buffer        */
#endif
    fflush(logfl);
}
