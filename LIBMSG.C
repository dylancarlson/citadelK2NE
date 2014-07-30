/************************************************************************/
/*                              libmsg.c                                */
/*                                                                      */
/*      Message handling for Citadel bulletin board system              */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 87Feb11 HAW  Created.                                                */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      getMessage()            load message into RAM                   */
/*      getMsgStr()             reads a string out of message.buf       */
/*      getMsgChar()            returns successive chars off disk       */
/*      startAt()               setup to read a message off disk        */
/*      unGetMsgChar()          return a char to getMsgChar()           */
/************************************************************************/

/************************************************************************/
/*                 External variable declarations in MSG.C              */
/************************************************************************/
struct mBuf   mFile1, mFile2;
MSG_BUF       msgBuf;

static int    GMCCache;  /* To unGetMsgChar() into               */

extern CONFIG cfg;    /* configuration variables              */
extern FILE   *msgfl;


/************************************************************************/
/*      getMessage() reads a message off disk into RAM.                 */
/*      a previous call to setUp has specified the message.             */
/************************************************************************/
void getMessage()
{
    int  c;

    /* clear msgBuf out */
    zero_struct(msgBuf);

    do {
        c = getMsgChar();
    } while (c != 0xFF);     /* find start of msg    */

    msgBuf.mbheadChar   = mFile1.oldChar;        /* record location      */
    msgBuf.mbheadSector = mFile1.oldSector;

    getMsgStr(msgBuf.mbId, NAMESIZE);

    do  {
        c = getMsgChar();
        switch (c) {
        case 'A':       getMsgStr(msgBuf.mbauth,  NAMESIZE);    break;
        case 'D':       getMsgStr(msgBuf.mbdate,  NAMESIZE);    break;
        case 'C':       getMsgStr(msgBuf.mbtime,  NAMESIZE);    break;
        case 'M':       /* just exit -- we'll read off disk */  break;
        case 'N':       getMsgStr(msgBuf.mboname, NAMESIZE);    break;
        case 'O':       getMsgStr(msgBuf.mborig,  NAMESIZE);    break;
        case 'R':       getMsgStr(msgBuf.mbroom,  NAMESIZE);    break;
        case 'S':       getMsgStr(msgBuf.mbsrcId, NAMESIZE);    break;
        case 'T':       getMsgStr(msgBuf.mbto,    NAMESIZE);    break;
        case 'Q':       getMsgStr(msgBuf.mbaddr,  NAMESIZE);    break;
		case 'Y':       getMsgStr(msgBuf.mbcompnode, NAMESIZE);  break;
						/* AB - get mbcompnode */
		case 'X':		getMsgStr(msgBuf.mbmsgpath, 480);  break;
		case 'W':		getMsgStr(msgBuf.mbmsgreply, 480);
					    break;
        default:
            if (isAlpha(c)) {
                getMsgStr(msgBuf.mbtext, MAXTEXT);  /* discard unknown field  */
                msgBuf.mbtext[0]    = '\0';
            }
            else if (c == 0xFF) {        /* Damaged msgBase              */
                unGetMsgChar(c);
            }
            break;
        }
    } while (c != 'M'  &&  isAlpha(c));
}

/************************************************************************/
/*      getMsgStr() reads a string from message.buf                     */
/************************************************************************/
void getMsgStr(dest, lim)
char *dest;
int  lim;
{
    int  c;

    while (c = getMsgChar()) {          /* read the complete string     */
        if (lim) {                      /* if we have room then         */
            lim--;
            *dest++ = c;                /* copy char to buffer          */
        }
    }
    if (!lim) dest--;   /* Ensure not overwrite next door neighbor      */
    *dest = '\0';                       /* tie string off with null     */
}

/************************************************************************/
/*      getMsgChar() returns sequential chars from message on disk      */
/************************************************************************/
int getMsgChar()
{
    long work;
    int  toReturn;


	doRTS(FALSE);
    if (GMCCache) {     /* someone did an unGetMsgChar() --return it    */
        toReturn= GMCCache;
        GMCCache= '\0';
		doRTS(TRUE);
        return (toReturn & 0xFF);
    }

    mFile1.oldChar     = mFile1.thisChar;
    mFile1.oldSector   = mFile1.thisSector;

    toReturn = mFile1.sectBuf[mFile1.thisChar];
    toReturn &= 0xFF;   /* Only want the lower 8 bits */

    mFile1.thisChar    = ++mFile1.thisChar % MSG_SECT_SIZE;
    if (mFile1.thisChar == 0) {
        /* time to read next sector in: */
        mFile1.thisSector  = ++mFile1.thisSector % cfg.maxMSector;
        work = mFile1.thisSector;
        work *= MSG_SECT_SIZE;
        fseek(msgfl, work, 0);
        if (fread(mFile1.sectBuf, MSG_SECT_SIZE, 1, msgfl) != 1) {
            crashout("?nextMsgChar-read fail");
        }
#ifndef NO_CRYPT
        crypte(mFile1.sectBuf, MSG_SECT_SIZE, 0);
#endif
    }

	doRTS(TRUE);
    return(toReturn);
}

/************************************************************************/
/*      startAt() sets location to begin reading message from           */
/************************************************************************/
void startAt(whichmsg, mFile, sect, byt)
SECTOR_ID   sect;
int         byt;
FILE        *whichmsg;
struct mBuf *mFile;
{
    long temp;

    GMCCache  = '\0';   /* cache to unGetMsgChar() into */

    if (sect >= cfg.maxMSector) {
        printf("?startAt s=%u,b=%d", sect, byt);
     /*   crashout("?startAt crash"); */
        return ;   /* Don't crash anymore, just skip the msg */
    }
    mFile->thisChar    = byt;
    mFile->thisSector  = sect;

    temp = sect;
    temp *= MSG_SECT_SIZE;
    fseek(whichmsg, temp, 0);
    if (fread(mFile->sectBuf, MSG_SECT_SIZE, 1, whichmsg) != 1) {
        crashout("?startAt read fail");
    }
#ifndef NO_CRYPT
    crypte(mFile->sectBuf, MSG_SECT_SIZE, 0);
#endif
}

/************************************************************************/
/*      unGetMsgChar() returns (at most one) char to getMsgChar()       */
/************************************************************************/
void unGetMsgChar(c)
char c;
{
    GMCCache    = (int) c;
}

doRTS(int mode)
{
 f_rtsctrl(cfg.FOSSIL_PORT, mode);
}
