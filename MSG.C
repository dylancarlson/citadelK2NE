/*
 *      msg.c
 *
 *      Message handling for Citadel bulletin board system
 */

/*
 * history
 *
 * 86Aug15 HAW  Large chunk of History deleted due to space problems.
 * 84Mar29 HAW Start upgrade to BDS C 1.50a, identify _spr problem.
 * 83Mar03 CrT & SB   Various bug fixes...
 * 83Feb27 CrT  Save private mail for sender as well as recipient.
 * 83Feb23      Various.  transmitFile() won't drop first char on WC...
 * 82Dec06 CrT  2.00 release.
 * 82Nov05 CrT  Stream retrieval.  Handles messages longer than MAXTEXT.
 * 82Nov04 CrT  Revised disk format implemented.
 * 82Nov03 CrT  Individual history begun.  General cleanup.
 */

#include "ctdl.h"

 #define TEST_SYS

/*
 * contents
 *
 * aideMessage()      saves auto message in Aide>
 * CheckForwarding()  forward mail to another system?
 * dGetWord()         reads a word off disk
 * doActualWrite()    to allow two message files for bkp
 * doFlush()          writes out to specified msg file
 * fakeFullCase()     converts uppercase message to mixed case
 * flushMsgBuf()      wraps up message-to-disk store
 * getRecipient()     get recipient for the message
 * getWord()          gets one word from message buffer
 * mAbort()           checks for user abort of typeout
 * makeMessage()      menu-level message-entry routine
 * mFormat()          formats a string to modem and console
 * mPeek()            sysop debugging tool--shows ctdlmsg.sys
 * noteLogMessage()   enter message into log record
 * noteMessage()      enter message into current room
 * note2Message()     noteMessage() local
 * printMessage()     prints a message on modem & console
 * pullIt()           sysop special message-removal routine
 * putLong()          puts a long integer to file
 * putMessage()       write message to disk
 * putMsgChar()       writes successive message chars to disk
 * putNetMessage()    write net message to disk
 * putWord()          writes one word to modem & console
 * showMessages()     menu-level show-roomful-of-messages fn
 */

/*
 * External variable declarations in MSG.C
 */
MSG_BUF          tempMess;  /* For held messages                    */

FILE             *msgfl;        /* file descriptor for the msg file */
FILE             *msgfl2;       /* disk based backup msg file       */
FILE             *alterNetFile; /* is it FIDO, UseNet, Other?       */

AN_UNSIGNED          crtColumn; /* current position on screen       */

static char          pullMessage = FALSE;/* true to pull current message*/
char                 journalMessage = FALSE;

SECTOR_ID            pulledMLoc;/* loc of pulled message                */

MSG_NUMBER           pulledMId = 0l;    /* id of message to be pulled   */

label                oldTarget;
char                 jrnlFile[100] = "";
char                 *NoNetRoomPrivs = "You don't have net privileges";
char                 outFlag = OUTOK;   /* will be one of the above     */
char                 heldMess;
char                 PrintBanner = FALSE;

char                 Forwarded = FALSE;
char				 replyToThis = FALSE;
char				 flipDirection = FALSE;
int                  msgInCount, currLine, thisMsgNumber;
char                 Net_Monitor; /* = FALSE;  default GATEWAY to OFF!    */
char                 /* messageCalledThis,*/ mailTag;
char				 pausePromptFlag; /* turn "--More--" off/on           */

/* char *CLRHOME = "2J[H"; */
/*
 * External variable definitions for MSG.C
 */
extern MSG_BUF      msgBuf;         /* The -sole- message buffer    */
extern struct mBuf      mFile1, mFile2;
extern CONFIG    cfg;            /* Configuration variables      */
extern paintBrush colTable;      /* the ANSI rainbow             */
extern LogTable    *logTab;        /* The people                   */
extern logBuffer logBuf;         /* Buffer for the pippuls       */
extern aRoom     roomBuf;        /* Room buffer                  */
extern rTable    *roomTab;
extern NetTable  *netTab;
extern NetBuffer netBuf;

extern FILE             *upfd;

extern int              thisRoom;       /* Current room                 */
extern int              thisNet;        /* Current node in use          */
extern int              thisLog;        /* Current log position         */
extern int              outPut;

extern char             *strFile;

extern char             exChar;
extern char             echo;           /* Output flag                  */
extern char             echoChar;
extern char             loggedIn;       /* Logged in flag               */
extern char             whichIO;        /* Who gets output?             */
extern char             prevChar;       /* Output's evil purposes       */
extern char             inNet;
extern int              TransProtocol;  /* Flag                         */
extern char             haveCarrier;    /* Flag                         */
extern char             onConsole;      /* Flag                         */
extern char             remoteSysop, lastFiveFlag;
extern int				fieldSuppress;  /* Net_Switch stuff             */
extern int				uniqueMsgNr, anchorCounter, howMany;
extern char				uniqueFlag, jiggleFlag, headerFlag, jumpOut;
extern char             alterName[NAMESIZE];  /* used for alterNet grabs */
extern char             alterNet; /* ditto */
extern char             lastHowMany, longMessage, userBioSpecial;

/*
 *      aideMessage() saves auto message in Aide>                       *
 */
void aideMessage(char noteDeletedMessage)
{
    int ourRoom;

    /* Ensures not a net message    */
    msgBuf.mboname[0] = 0;
    msgBuf.mborig[0]  = 0;
    msgBuf.mbsrcId[0] = 0;
    msgBuf.mbdate[0]  = 0;
    msgBuf.mbtime[0]  = 0;

    /* message is already set up in msgBuf.mbtext */
    putRoom(ourRoom = thisRoom);
    getRoom(AIDEROOM);

    strCpy(msgBuf.mbauth, "Citadel");
    msgBuf.mbto[0] = '\0';
    if (putMessage())   noteMessage(NULL, ERROR);

    if (noteDeletedMessage)   {
        note2Message(pulledMId, pulledMLoc);
    }

    putRoom(AIDEROOM);
    noteRoom();
    getRoom(ourRoom);
}

int forwardingMessage=FALSE;

/*
/*      CheckForwarding() send mail to another system?
 */
void CheckForwarding(logBuffer *lbuf)
{
    if (thisRoom != MAILROOM) return;   /* Only forward in Mail. */

    if (msgBuf.mbaddr[0]) return;       /* this is net anyways.  */

    if (strCmpU(msgBuf.mbto, "Sysop") == SAMESTRING ||
        strCmpU(msgBuf.mbto, "Citadel") == SAMESTRING)
        return;

    if (lbuf->lbfwd != -1) {             /* Has forwarding address? */
        getNet(lbuf->lbfwd);
        /* Check to make sure all is still valid */
        if (    netBuf.nbflags.in_use &&
                lbuf->lbNetGen == netBuf.nbGen &&
                (netBuf.nbflags.local || lbuf->credit != 0)) {
            strCpy(msgBuf.mbaddr, netBuf.netName);
            Forwarded = TRUE;                   /* So goes local, too.   */
			forwardingMessage=TRUE;
            if (!netBuf.nbflags.local)
                lbuf->credit--;
        }
		else if (lbuf->lbNetGen != netBuf.nbGen)
            lbuf->lbfwd = -1;    /* Cancel */
				                                   /* check to make sure */
												   /* user's address != incoming */
												   /* message fromnode  */

    }
}

/*
 *      deleteMessage() deletes message for pullIt()
 */
char deleteMessage(int m)
{
    int i;

    /* record vital statistics for possible insertion elsewhere: */
    mPrintf("elete msg\n ");
    pulledMLoc = roomBuf.msg[m].rbmsgLoc;
    pulledMId  = roomBuf.msg[m].rbmsgNo ;

    if (thisRoom == AIDEROOM)   return TRUE;

    /* return emptied slot: */
    for (i = m;  i > 0;  i--) {
        roomBuf.msg[i].rbmsgLoc      = roomBuf.msg[i - 1].rbmsgLoc;
        roomBuf.msg[i].rbmsgNo       = roomBuf.msg[i - 1].rbmsgNo ;
    }
    roomBuf.msg[0].rbmsgNo   = 0l;      /* mark new slot at end as free */
    roomBuf.msg[0].rbmsgLoc  = 0;      /* mark new slot at end as free */

    /* store revised room to disk before we forget...   */
    noteRoom();
    putRoom(thisRoom);

    /* note in Aide>: */
    sPrintf(msgBuf.mbtext, "Following msg. from %s deleted by %s:",
#ifdef BRIAN
    msgBuf.mbauth , logBuf.lbname);
#else
   (msgBuf.mbauth[0]) ? msgBuf.mbauth : "<anonymous>", logBuf.lbname);
#endif
    aideMessage( /* noteDeletedMessage== */ TRUE);
    return TRUE;
}

/*
 *      canRespond() can we set up an auto-response on the net?
 */
char canRespond()
{
    label temp;

    if (msgBuf.mborig[0] == 0)   /* i.e. is local mail           */
        return TRUE;

    normId(msgBuf.mborig, temp);
    if (searchNet(temp) == ERROR)       /* No such node */
        return FALSE;

    if (netBuf.nbflags.local)
        return TRUE;

    if (logBuf.credit != 0)
        return TRUE;

    return FALSE;
}

/*
 *      dGetWord() fetches one word from current message, off disk
 *      returns TRUE if more words follow, else FALSE
 */
char dGetWord(char *dest, int lim)
{
    int  c;

    --lim;      /* play it safe */

    /* pick up any leading blanks: */
    for (c = getMsgChar();   c == ' '  &&  c && lim;   c = getMsgChar()) {
        if (lim) { *dest++ = c;   lim--; }
    }

    /* step through word: */
    for (                ;  c != ' ' && c && lim;   c = getMsgChar()) {
        if (lim) { *dest++ = c;   lim--; }
    }

    /* trailing blanks: */
    for (                ;   c == ' ' && c && lim;   c = getMsgChar()) {
        if (lim) { *dest++ = c;   lim--; }
    }

    if (c)  unGetMsgChar(c);    /* took one too many    */

    *dest = '\0';               /* tie off string       */

    return  c;
}

/*
 *      doActualWrite() to allow automatic bkp of msg file from RAM.
 */
char doActualWrite(whichmsg, mFile, c)
FILE        *whichmsg;
struct mBuf *mFile;
char        c;
{
    MSG_NUMBER temp;
    int        toReturn = 0;

    if (mFile->sectBuf[mFile->thisChar] == 0xFF)  {
        /* obliterating a msg   */
        toReturn = 1;
    }

    mFile->sectBuf[mFile->thisChar]   = c;

    mFile->thisChar    = ++mFile->thisChar % MSG_SECT_SIZE;

    if (mFile->thisChar == 0) { /* time to write sector out and get next: */
        temp = mFile->thisSector;
        temp *= MSG_SECT_SIZE;
        fseek(whichmsg, temp, 0);
#ifndef NO_CRYPT
        crypte(mFile->sectBuf, MSG_SECT_SIZE, 0);
#endif
        if (fwrite(mFile->sectBuf, MSG_SECT_SIZE, 1, whichmsg) != 1) {
            crashout("?putMsgChar-Wfail");
        }

        mFile->thisSector = ++mFile->thisSector % cfg.maxMSector;
        temp = mFile->thisSector;
        temp *= MSG_SECT_SIZE;
        fseek(whichmsg, temp, 0);
        if (fread(mFile->sectBuf, MSG_SECT_SIZE, 1, whichmsg) != 1) {
            crashout("?putMsgChar-Rfail");
        }
#ifndef NO_CRYPT
        crypte(mFile->sectBuf, MSG_SECT_SIZE, 0);
#endif
    }
    return  toReturn;
}

/*
 *      doFlush() do actual writeup for specified msg file
 */
void doFlush(whichmsg, mFile)
FILE *whichmsg;
struct mBuf *mFile;
{
    long int s;

    s = mFile->thisSector;
    s *= MSG_SECT_SIZE;
    fseek(whichmsg, s, 0);
#ifndef NO_CRYPT
    crypte(mFile->sectBuf, MSG_SECT_SIZE, 0);
#endif
    if (fwrite(mFile->sectBuf, MSG_SECT_SIZE, 1, whichmsg) != 1) {
        crashout("?ctdlmsg.sys Wfail");
    }
#ifndef NO_CRYPT
    crypte(mFile->sectBuf, MSG_SECT_SIZE, 0);
#endif
    fflush(whichmsg);
}

/*
 *      fakeFullCase() converts a message in uppercase-only to a
 *      reasonable mix.  It can't possibly make matters worse...
 *      Algorithm: First alphabetic after a period is uppercase, all
 *      others are lowercase, excepting pronoun "I" is a special case.
 *      We assume an imaginary period preceding the text.
 */
#ifdef FAKEFULLCASE
void fakeFullCase(char *text)
{
    char *c;
    char lastWasPeriod;
    char state;

    for(lastWasPeriod=TRUE, c=text;   *c;  c++) {
        if (
            *c != '.'
            &&
            *c != '?'
            &&
            *c != '!'
        ) {
            if (isAlpha(*c)) {
                if (lastWasPeriod)      *c      = toUpper(*c);
                else                    *c      = toLower(*c);
                lastWasPeriod   = FALSE;
            }
        } else {
            lastWasPeriod       = TRUE ;
        }
    }

    /* little state machine to search for ' i ': */
#define NUTHIN          0
#define FIRSTBLANK      1
#define BLANKI          2
    for (state=NUTHIN, c=text;  *c;  c++) {
        switch (state) {
        case NUTHIN:
            if (isSpace(*c))    state   = FIRSTBLANK;
            else                state   = NUTHIN    ;
            break;
        case FIRSTBLANK:
            if (*c == 'i')      state   = BLANKI    ;
            else                state   = NUTHIN    ;
            break;
        case BLANKI:
            if (isSpace(*c))    state   = FIRSTBLANK;
            else                state   = NUTHIN    ;

            if (!isAlpha(*c))   *(c-1)  = 'I';
            break;
        }
    }
}
#endif

/*
 * flushMsgBuf() wraps up writing a message to disk, takes into
 *               account 2nd msg file if necessary
 */
void flushMsgBuf()
{
    doFlush(msgfl, &mFile1);
    if (cfg.BoolFlags.mirror)
        doFlush(msgfl2, &mFile2);
}

/*
 *      getWord() fetches one word from current message
 */
int getWord(char *dest, char *source, int offset, int lim)
{
    int i, j;

    /* skip leading blanks if any */
    for (i = 0;  source[offset+i] ==' ' && i < lim;  i++);

    /* step over word */
    for (;

         source[offset+i]   != ' '     &&
         i                  <  lim - 1 &&
         source[offset+i]   != 0;

         i++
    );

    /* pick up any trailing blanks */
    for (;  source[offset+i]==' ' && i<lim;  i++);

    /* copy word over */
    for (j = 0; j < i; j++)  dest[j] = source[offset+j];
    dest[j] = 0;        /* null to tie off string */

    return(offset+i);
}

/*
 *      mAbort() returns TRUE if the user has aborted typeout
 *      Globals modified:       outFlag
 */
char mAbort()
{
    char c, toReturn, oldEcho;
    /* Check for abort/pause from user */
    if (outFlag == IMPERVIOUS || outFlag == NET_CALL) {
        toReturn        = FALSE;
    }
    else if (!BBSCharReady()) {
        if (haveCarrier && !gotCarrier())
            modIn();            /* Let modIn() report the problem       */
        toReturn        = FALSE;
    } else {
        oldEcho  = echo;
        echo     = NEITHER;
        echoChar = 0;
        	/* allows SYSOP to hit ESC and simulate user doing 'S'<br> */
		if (!PrintBanner && whichIO == MODEM && KBReady() && getCh() == 0x1b)
			c = 'S';
		else
        	c = toUpper(modIn());   /* avoid the filter */
        switch (c) {
        case XOFF:
		case ' ':                                   /* pause on space */
        case 'P':                                   /* pause:         */
            c = iChar();                            /* wait to resume */
            if (     toLower(c) == 'd' &&
                     (aide ||
                     (strCmpU(logBuf.lbname, roomBuf.rbmoderator) == SAMESTRING
                     && strLen(logBuf.lbname) != 0)))
                pullMessage = TRUE;
            else if (toLower(c) == 'j' && SomeSysop())
                journalMessage = TRUE;
			else if (toLower(c) == 'e' && loggedIn && thisRoom != MAILROOM) {
                replyToThis = TRUE;
                outFlag = OUTSKIP;
				}
			else if (toLower(c) == 'r'
						&& thisRoom != MAILROOM) {
				flipDirection = TRUE;
				outFlag = OUTSKIP;
				}
            toReturn      = FALSE;
            break;
		case 'R':
			if (!PrintBanner) flipDirection = TRUE;
			outFlag = OUTSKIP;
			toReturn = FALSE;
			break;
        case 'J':                                   /* jump paragraph:*/
            outFlag     = OUTPARAGRAPH;
            toReturn    = FALSE;
            break;
        case 'N':                                   /* next:          */
            outFlag     = OUTNEXT;
            toReturn    = TRUE;
            break;
		case 'A':									/* a convenience! */
        case 'S':                                   /* skip:          */
		case '':
            outFlag     = OUTSKIP;
            toReturn    = TRUE;
            break;

        case 7: /* anytime net indicator */
            if (PrintBanner && cfg.BoolFlags.netParticipant) {
#ifdef IDEAL
                if (check_for_init(TRUE)) {      /* Check for confirmation */
                    outFlag = NET_CALL;
                    toReturn = TRUE;
                }
#else
                outFlag = NET_CALL;
                toReturn = TRUE;
#endif
            }
        default:
            toReturn    = FALSE;
            break;
        }
        echo    = oldEcho;
    }
    return toReturn;
}

/*
 * getRecipient() get recipient for the message
 */
char getRecipient(logBuffer *lBuf, int *logNo)
{
    if (thisRoom != MAILROOM)  {
        msgBuf.mbto[0] = 0;             /* Zero recipient       */
        return TRUE;
    }

    if (msgBuf.mbto[0] == 0) {
        if (!loggedIn || (!aide && cfg.BoolFlags.noMail)) {
            strCpy(msgBuf.mbto, "Sysop");
            mPrintf(" (private mail to 'sysop')\n ");
            return TRUE;
        }

        getNormStr("recipient", msgBuf.mbto, NAMESIZE, ECHO);
        if (strLen(msgBuf.mbto) == 0) return FALSE;
    }

    if (!msgBuf.mbaddr[0]) {
		mailTag=TRUE;
		if (strCmpU(msgBuf.mbto, "Sysop") == SAMESTRING)
               *logNo   = findPerson(msgBuf.mbto, lBuf);
        else   *logNo   = findPartPerson(msgBuf.mbto, lBuf);
        if ((*logNo == ERROR &&
                  (strCmpU(msgBuf.mbto, "Citadel") != SAMESTRING ||
                   !aide    ||    !onConsole) &&
             hash(msgBuf.mbto) != hash("Sysop")) ||
             strLen(msgBuf.mbto) == 0) {
            if (inNet == NON_NET) mPrintf("No '%s' known", msgBuf.mbto);
			mailTag=FALSE;
            return FALSE;
        }
        if (hash(msgBuf.mbto) != hash("Sysop") &&
            strCmpU(msgBuf.mbto, "Citadel") != SAMESTRING)
            strCpy(msgBuf.mbto, lBuf->lbname);  /* Get "true" rep.  */
        if (strCmp(msgBuf.mbto, logBuf.lbname) == SAMESTRING) {
            if (inNet == NON_NET) mPrintf("Mail to yourself?");
			mailTag=FALSE;
            return FALSE;
        }
    }
	mailTag=FALSE;
    return TRUE;
}

/*
 * replyMessage() reply to a Mail> message
 */
char replyMessage()
{
    label who;
    label node;
	label us;
	char system[20], temp[20], str2[20];
	char tempReply[480];

    strCpy(who, msgBuf.mbauth);
    strCpy(node, msgBuf.mborig[0] == 0 ? "" : netBuf.netName);
	strCpy(tempReply, msgBuf.mbmsgreply);

    setmem(&msgBuf, sizeof msgBuf, 0);      /* Egad! */

    if (thisRoom==MAILROOM) { /* fixes a small bug  -  K2NE  */
	    strCpy(msgBuf.mbto, who);
    	strCpy(msgBuf.mbaddr, node);
    	strCpy(msgBuf.mbmsgpath, tempReply);
		strCpy(us, cfg.codeBuf +cfg.nodeId);
		stripSpaces(us);
		strCpy(msgBuf.mbmsgreply, us);
        }

	/* Theoretically, this is all we need to send a msg back through the net.
	   When a system receives this message, it doesn't know whether it is a
	   reply or not, and it doesn't care.  The message is received and all
	   the regular checks are made.  There is a path so the msg will follow
	   the path if it meets those requirement.  Simple!
	 */

    return procMessage(ASCII);
}

/*
 * hldMessage() handles held messages
 */
char hldMessage()
{
    if (!heldMess) {
        mPrintf(" \n No held msg.\007\n ");
        return FALSE;
    }
    heldMess = FALSE;
    copy_struct(tempMess, msgBuf);
    if (roomTab[thisRoom].rtflags.SHARED == 0)
        msgBuf.mboname[0] = 0;
    else if ( (loggedIn
				||
			  (alterNet==TRUE && roomBuf.rbflags.ALT_LINKED) ) &&
			 roomBuf.rbflags.SHARED &&
             roomBuf.rbflags.AUTO_NET &&
         	  (roomBuf.rbflags.ALL_NET
				||
			  logBuf.lbflags.NET_PRIVS
				||
			  alterNet==TRUE) )
        netInfo();
    zero_struct(tempMess);
    procMessage(ASCII);
    return TRUE;
}

/*
 * makeMessage is menu-level routine to enter a message
 * Return: TRUE if message saved else FALSE
 */
void makeMessage(char uploading)
{
    zero_struct(msgBuf);
    procMessage(uploading);
}

/*
 *      idiotMessage()  checks for idiocy
 */
char idiotMessage()
{
    int base, rover;

    if (loggedIn || thisRoom != MAILROOM)
        return FALSE;

    for (base = 0; msgBuf.mbtext[base]; base++) {
        for (rover = 0; rover < IDIOT_TRIGGER; rover++) {
            if (msgBuf.mbtext[base] != msgBuf.mbtext[base + rover] ||
                        msgBuf.mbtext[base] == ' ')
                break;
        }
        if (rover == IDIOT_TRIGGER) return TRUE;        /* Jackass caught! */
    }
    return FALSE;
}

/*
 * procMessage is menu-level routine to enter a message
 */
char procMessage(char uploading)
{
    char      *pc, allUpper;
    logBuffer lBuf;
    int       logNo;

    initLogBuf(&lBuf);
#ifdef BRIAN
    if (loggedIn)
        strCpy(msgBuf.mbauth, logBuf.lbname);

    strCpy(msgBuf.mbroom, roomBuf.rbname);
    strCpy(msgBuf.mbdate, formDate());
#else
    if (alterNet==TRUE && !loggedIn) strCpy(msgBuf.mbauth, alterName);
    if (loggedIn)
        strCpy(msgBuf.mbauth, (roomBuf.rbflags.ANON) ? "" : logBuf.lbname);
    strCpy(msgBuf.mbroom, roomBuf.rbname);
    strCpy(msgBuf.mbdate, (roomBuf.rbflags.ANON) ? "****" :  formDate());
#endif
    if (!getRecipient(&lBuf, &logNo)) {
        killLogBuf(&lBuf);
        flushBuf();
        return FALSE;
    }

    if (getText(uploading) == TRUE) {
                        /* Asshole check */
        if (idiotMessage()) {
            strCpy(msgBuf.mbtext, "Vandalism attempt.");
            aideMessage(FALSE);
            flushBuf();
            killLogBuf(&lBuf);
            return TRUE;
        }
        for (pc=msgBuf.mbtext, allUpper=TRUE;   *pc && allUpper;  pc++) {
            if (toUpper(*pc) != *pc)   allUpper = FALSE;
        }
#ifdef FAKEFULLCASE
        if (allUpper)   fakeFullCase(msgBuf.mbtext);
#endif
        CheckForwarding(&lBuf);
		if (msgBuf.mbaddr[0]) strcpy(msgBuf.mbcompnode, cfg.codeBuf+cfg.nodeId);
		 /* AB - sets up this systems node id in field mbcompnode, only if */
		 /* there is mbaddr */


        if (putMessage()) noteMessage(&lBuf, logNo);
        flushBuf();

        killLogBuf(&lBuf);
        return TRUE;
    }
    return FALSE;
}

/*
 * mFormat() formats a string to modem and console
 */
void mFormat(char *string)
{
    char wordBuf[MAXWORD];
    int  i;

    for (i = 0;  string[i] && (outFlag == OUTOK      ||
                             outFlag == IMPERVIOUS ||
                             outFlag == OUTPARAGRAPH);  ) {
        i = getWord(wordBuf, string, i, MAXWORD);
        putWord(wordBuf);
        if (mAbort()) {
			currLine=0;
			return;
			}
    }
}

/*
 * moveMessage() Moves a message for pullIt()
 */
char moveMessage(int m)
{
    label blah;
    int   i, roomTarg, ourRoom;
    int   curRoom;

    curRoom = thisRoom;
    mPrintf("ove msg.\n ");

    if (!getXString("where", blah, 20, oldTarget, oldTarget))
        return FALSE;

    if ((roomTarg = roomCheck(roomExists, blah)) == ERROR) {
        if ((roomTarg = roomCheck(partialExist, blah)) == ERROR) {
            mPrintf("No '%s' exists!", blah);
            return FALSE;
        }
        else {
            thisRoom = roomTarg;
            if (roomCheck(partialExist, blah) != ERROR) {
                thisRoom = curRoom;
                mPrintf("'%s' is not a unique string.", blah);
                return FALSE;
            }
            thisRoom = curRoom;
        }
    }

    strCpy(oldTarget, roomTab[roomTarg].rtname);

    pulledMLoc = roomBuf.msg[m].rbmsgLoc;
    pulledMId  = roomBuf.msg[m].rbmsgNo ;

    for (i = m;  i > 0;  i--) {
        roomBuf.msg[i].rbmsgLoc      = roomBuf.msg[i - 1].rbmsgLoc;
        roomBuf.msg[i].rbmsgNo       = roomBuf.msg[i - 1].rbmsgNo ;
    }
    roomBuf.msg[0].rbmsgNo   = 0l;      /* mark new slot at end as free */
    roomBuf.msg[0].rbmsgLoc  = 0 ;      /* mark new slot at end as free */

    noteRoom();
    putRoom(ourRoom = thisRoom);
    getRoom(roomTarg);

    note2Message(pulledMId, pulledMLoc);
    putRoom(thisRoom);
    noteRoom();
    getRoom(ourRoom);
    sPrintf(
        msgBuf.mbtext,"Following msg. from %s moved from %s",
#ifdef BRIAN
         msgBuf.mbauth,
#else
        (msgBuf.mbauth[0]) ? msgBuf.mbauth : "<anonymous>",
#endif
        formRoom(thisRoom, FALSE, FALSE));
    sPrintf(lbyte(msgBuf.mbtext), " to %s by %s",
    	formRoom(roomTarg, FALSE, FALSE), logBuf.lbname);
    aideMessage( /* noteDeletedMessage == */ TRUE);
    return TRUE;
}

#ifdef DEBUG_MSGS
/*
 * mPeek() dumps a sector in message.buf.  sysop debugging tool
 */
void mPeek()
{
    char visible();
    char blup[50];
    DATA_BLOCK peekBuf;
    int  col, row;
    MSG_NUMBER r, s;

    sPrintf(blup, " sector to dump (from 0 - %d): ", cfg.maxMSector);
    s = getNumber(blup, 0l, (MSG_NUMBER) (cfg.maxMSector-1));
    r = s * MSG_SECT_SIZE;
    fseek(msgfl, r, 0);
    fread(peekBuf, MSG_SECT_SIZE, 1, msgfl);
#ifndef NO_CRYPT
    crypte(peekBuf, MSG_SECT_SIZE, 0);
#endif
    for (row = 0;  row < 2;  row++) {
        mPrintf("\n ");
        for (col = 0;  col < 64;  col++) {
            mPrintf("%c", visible(peekBuf[row*64 +col]));
        }
    }
}
#endif

/*
 * msgToDisk() Puts a message to the given disk file
 */
void msgToDisk(filename, id, loc)
char       *filename;
MSG_NUMBER id;
SECTOR_ID  loc;
{
    extern char *APPEND_TEXT;

    if ((upfd = safeopen(filename, APPEND_TEXT)) == NULL) {
        mPrintf("Can't open '%s'\n ", filename);
    }
    else {
        outPut = DISK;
        printMessage(loc, id, FALSE);
        outPut = NORMAL;
        fclose(upfd);
    }
}

/*
 * noteLogMessage() slots message into log record
 */
void noteLogMessage(logBuffer *lBuf, int logNo)
{
    int i;

    /* store into recipient's log record: */
    /* slide message pointers down to make room for this one: */
    for (i = 0;  i < MAILSLOTS - 1;  i++) {
        lBuf->lbMail[i].rbmsgLoc = lBuf->lbMail[i + 1].rbmsgLoc;
        lBuf->lbMail[i].rbmsgNo  = lBuf->lbMail[i + 1].rbmsgNo;
    }

    /* slot this message in:    */
    lBuf->lbMail[MAILSLOTS-1].rbmsgNo  = cfg.newest;
    lBuf->lbMail[MAILSLOTS-1].rbmsgLoc = cfg.catSector;

    putLog(lBuf, logNo);
}

/*
 * noteMessage() slots message into current room
 */
void noteMessage(logBuffer *lBuf, int logNo)
{
    logBuffer lBuf2;
    int logRover;
    char *fn;

    logBuf.lbvisit[0]   = ++cfg.newest;

    if (thisRoom != MAILROOM) {
        note2Message(cfg.newest, cfg.catSector);

        /* write it to disk:            */
        putRoom(thisRoom);
        noteRoom();
    } else {            /* when in Mail... */
        if (strCmpU(msgBuf.mbto, "Sysop") == SAMESTRING)  {
            if (!msgBuf.mbaddr[0]) {     /* Not Net Mail to sysop? */
                initLogBuf(&lBuf2);      /* Then save in local system */
                if ((logRover = findPerson(cfg.SysopName, &lBuf2)) == ERROR) {
                    getRoom(AIDEROOM);

                    /* enter in Aide> room -- 'sysop' is special */
                    note2Message(cfg.newest, cfg.catSector);

                    /* write it to disk:            */
                    putRoom(AIDEROOM);
                    noteRoom();

                    getRoom(MAILROOM);
                }
                note3message(logRover, &lBuf2);
                killLogBuf(&lBuf2);
            }
            /* note in ourself if logged in: */
            else
            if (loggedIn) {
                noteLogMessage(&logBuf, thisLog);
                fillMailRoom();
            }
        } else if (strCmpU(msgBuf.mbto, "Citadel") == SAMESTRING &&
                   !msgBuf.mbaddr[0]) {
            initLogBuf(&lBuf2);
            for (logRover = 0; logRover < cfg.MAXLOGTAB; logRover++) {
                printf("Log %d\r", logRover);       /* Notify sysop     */
                getLog(&lBuf2, logRover);

                if (lBuf2.lbflags.L_INUSE) {
                    noteLogMessage(&lBuf2, logRover);
                }
            }
            killLogBuf(&lBuf2);
            noteLogMessage(&logBuf, thisLog);       /* note in ourself  */
            fillMailRoom();                         /* update room also */
        } else {
            note3message(logNo, lBuf);
        }
    }

    msgBuf.mbaddr[0] = 0;
    msgBuf.mbto[0]   = 0;
    Forwarded        = FALSE;

    /* make message official:   */
    cfg.catSector   = mFile1.thisSector;
    cfg.catChar     = mFile1.thisChar;
    setUp(FALSE);
    if (roomBuf.rbflags.ARCHIVE == 1) {
        if ((fn = findArchiveName(thisRoom)) == NULL) {
            sPrintf(msgBuf.mbtext, "Archive problem: %s.",
                                                roomBuf.rbname);
            aideMessage(FALSE);
        }
        else {
            msgToDisk(fn, roomBuf.msg[MSGSPERRM - 1].rbmsgNo,
                          roomBuf.msg[MSGSPERRM - 1].rbmsgLoc);
        }
    }
}

/*
 * note2Message() makes slot in current room... called by noteMess
 */
void note2Message(id, loc)
MSG_NUMBER id;
SECTOR_ID  loc;
{
    int  i;

    /* store into current room: */
    /* slide message pointers down to make room for this one:       */
    for (i = 0;  i < MSGSPERRM - 1;  i++) {
        roomBuf.msg[i].rbmsgLoc  = roomBuf.msg[i+1].rbmsgLoc;
        roomBuf.msg[i].rbmsgNo   = roomBuf.msg[i+1].rbmsgNo ;
    }

    /* slot this message in:        */
    roomBuf.msg[MSGSPERRM-1].rbmsgNo     = id ;
    roomBuf.msg[MSGSPERRM-1].rbmsgLoc    = loc;
}

/*
 * note3Message() makes slot in current room... called by noteMess
 */
void note3message(int logNo, logBuffer *lBuf)
{
    if (logNo != thisLog && (!msgBuf.mbaddr[0] || Forwarded) && logNo != ERROR) {
        noteLogMessage(lBuf, logNo);
    }
    if (loggedIn) {
        noteLogMessage(&logBuf, thisLog);
        fillMailRoom();
    }
}

/*
 * findMessage() gets all set up to do something with a message
 */
char findMessage(loc, id)
SECTOR_ID  loc;         /* sector in message.buf        */
MSG_NUMBER id;          /* unique-for-some-time ID#     */
{
    MSG_NUMBER here;

    startAt(msgfl, &mFile1, loc, 0);

    do {
        getMessage();
        here = atol(msgBuf.mbId);
    } while (here != id &&  mFile1.thisSector == loc);

    return ((here == id));
}

/*
 * printMessage() prints indicated message on modem & console
 */
void printMessage(loc, id, net_format)
SECTOR_ID  loc;         /* sector in message.buf        */
MSG_NUMBER id;          /* unique-for-some-time ID#     */
char       net_format;  /* output in net format?        */
{
    int  moreFollows;
    int  oldTermWidth, oldTermNulls;
    int  strip;

    if (!findMessage(loc, id) && TransProtocol == ASCII && !net_format) {
/*
 *      mPrintf("?Can't find message at %lu in sector %u!\n ",
 *                       id, loc);
 *      if (cfg.BoolFlags.debug)
 *           printf(" loc=%u, id=%lu, mbIds=%s\n",
 *               loc, id, msgBuf.mbId);
 */
        mPrintf("? Message missing.\n ");
        return;
    }

    if (!net_format) {
        oldTermWidth = termWidth;
        oldTermNulls = termNulls;
        if (outPut == DISK) {
            termWidth = 80;
			termNulls = 0;
        }
        doCR();

        shrtColor(colTable.level1 /* A_RED */);
        mPrintf("%s", formHeader());
        shrtColor(colTable.level0 /* A_GREEN */);
        doCR();
		currLine=1;
        if (headerFlag==TRUE) return;

        while (1) {
			pausePromptFlag=TRUE;
            moreFollows     = dGetWord(msgBuf.mbtext, 150);
                /* strip control Ls out of the output                   */
            for (strip = 0; msgBuf.mbtext[strip] != 0; strip++)
                if (msgBuf.mbtext[strip] == 0x0c ||
                    msgBuf.mbtext[strip] == SPECIAL)
                    msgBuf.mbtext[strip] = ' ';
            putWord(msgBuf.mbtext);
            if (!(moreFollows  &&  !mAbort())) {
                if (outFlag == OUTNEXT)         /* If <N>ext, extra line */
                    doCR();
				pausePromptFlag=FALSE;
                break;
            }
        }
        doCR();
        termWidth = oldTermWidth;
	    termNulls = oldTermNulls;
		pausePromptFlag=FALSE;
    }
    else {
        prNetStyle(getMsgChar);
    }
    msgBuf.mbaddr[0] = 0;
}

/*
 * prNetStyle() send message in network format, any source
 */
void prNetStyle(SourceFn)
int (*SourceFn)(void);
{
    long val;
    int  c;
    extern PROTO_TABLE Table[];
#ifdef NET_K2NE
	extern char testFileMake;
	extern FILE *testFileptr;

	if (testFileMake) {
		fprintf(testFileptr, "A%s", msgBuf.mbauth);
		fprintf(testFileptr, "D%s", msgBuf.mbdate);
		fprintf(testFileptr, "C%s", msgBuf.mbtime);
		fprintf(testFileptr, "N%s", msgBuf.mboname);
		fprintf(testFileptr, "O%s", msgBuf.mborig );
		fprintf(testFileptr, "R%s", msgBuf.mbroom);
		fprintf(testFileptr, "S%s", msgBuf.mbsrcId);
		fprintf(testFileptr, "T%s", msgBuf.mbto   );
		fprintf(testFileptr, "Y%s", msgBuf.mbcompnode);
		fprintf(testFileptr, "X%s", msgBuf.mbmsgpath );
		fprintf(testFileptr, "W%s", msgBuf.mbmsgreply);
		fprintf(testFileptr, "M");

	    do  {
		        c = (*SourceFn)();
    		    if (c=='\n')  c='\r';
				fprintf(testFileptr, "%c", c); */
		    } while (c);
        }
#else

        /* fill in local node in origin fields if local message: */
    if (!msgBuf.mborig[ 0])
        strCpy(msgBuf.mborig,  cfg.nodeId + cfg.codeBuf  );
    if (!msgBuf.mboname[0])
        strCpy(msgBuf.mboname, cfg.nodeName + cfg.codeBuf);

    /* Convert # to 8-bit Citadel style for compatibility   */
    if (!msgBuf.mbsrcId[0]) {
        val = atol(msgBuf.mbId);
        sPrintf(msgBuf.mbsrcId, "%ld %ld",
                      val & 0xFFFF0000l,val & 0xFFFFl);
    }

    /* send header fields out: */
    if (msgBuf.mbauth[ 0])
			mTrPrintf("A%s", msgBuf.mbauth );
    if (msgBuf.mbdate[ 0])
			mTrPrintf("D%s", msgBuf.mbdate );
    if (msgBuf.mbtime[ 0])
			mTrPrintf("C%s", msgBuf.mbtime );
    if (msgBuf.mboname[0])
			mTrPrintf("N%s", msgBuf.mboname);
    if (msgBuf.mborig[ 0])
			mTrPrintf("O%s", msgBuf.mborig );
    if (msgBuf.mbroom[ 0])
			mTrPrintf("R%s", msgBuf.mbroom );
    if (msgBuf.mbsrcId[0])
			mTrPrintf("S%s", msgBuf.mbsrcId);
    if (msgBuf.mbto[   0])
			mTrPrintf("T%s", msgBuf.mbto   );
/*
 * 89Mar28
 * Unfortunately, political bullshit never fails to louse up what should
 * be a fascinating hobby.  Although we have been using and documenting
 * the following 3 field-designators since July/1988, Hue, Jr. has seen
 * fit to attempt to usurp one or more of them for his software in
 * Jan/1989.  We offered to modify Citadel:K2NE to avoid
 * this field conflict and requested three fields from Hue, Jr. on
 * Feb. 22, 1989.  He has not seen fit to reply to us.  Therefore, we
 * will continue to use these fields since the only potential problem
 * would be if a system were running Hue Jr software.  After consulting
 * with Dave Parsons (Orc), developer/maintainer of STadel/STadel-PC, it
 * will now our policy to consider the W, X and Y fields to be
 * reserved for K2NE usage and Hue-be-damned.  (VAQ/BBR/Orc)
 *
 * The 'fieldSuppress' variable has been added to ensure that the
 * Net_Switch fields are ONLY processed for MAIL functions and
 * not for shared rooms.  89Apr04 (VAQ)
 */

	if (msgBuf.mbcompnode[0] && !fieldSuppress)
			mTrPrintf("Y%s", msgBuf.mbcompnode);
	if (msgBuf.mbmsgpath[0] && !fieldSuppress)
			mTrPrintf("X%s", msgBuf.mbmsgpath);
	if (msgBuf.mbmsgreply[0] && !fieldSuppress)
			mTrPrintf("W%s", msgBuf.mbmsgreply);
    /* send message text proper: */
    (*Table[TransProtocol].method)('M');
    do  {
        c = (*SourceFn)();
        if (c=='\n')  c='\r';
        if (!(*Table[TransProtocol].method)(c)) break;
    } while (c);
#endif
}

/*
 * pullIt() is a sysop special to remove a message from a room
 */
char pullIt(int m)
{
    char  finished;
    char  answer;

    /* confirm that we're removing the right one:       */
    outFlag = OUTOK;

#ifdef OLDWAY (we used to print the message all over again before prompting)
    printMessage(roomBuf.msg[m].rbmsgLoc, roomBuf.msg[m].rbmsgNo, FALSE);
#endif

    for (finished = FALSE; !finished;)  {
        do {
            outFlag = IMPERVIOUS;
            mPrintf("\n <D>elete <M>ove <A>bort? (D/M/A) ");
            answer = toUpper(iChar());
            if (answer == 'A' ||
                answer == 'D' ||
                answer == 'M')
                break;
        } while (onLine());

        outFlag = OUTOK;
        if (answer == 'A' || !onLine()) return FALSE;

        if (answer == 'D')      finished = deleteMessage(m);
        else if (answer == 'M') finished = moveMessage(m);
    }
    return TRUE;
}

/*
 * putMessage() stores a message to disk
 * Always called before noteMessage() -- newest not ++ed yet.
 * Returns: TRUE on successful save, else FALSE
 */
char putMessage()
{
    extern char *ALL_LOCALS, *WRITE_LOCALS;
    extern char *R_SH_MARK, *LOC_NET, *NON_LOC_NET;
    char  *s, *month, *ml;
    int   year, day, h, m, netPlace;
    char alternativeNetFile[20];
	char storeTemp[20];
/*  char reserveK[30] = "***cit/k2ne*reserved*field***"; */
    char FIDOrecipient[30], FIDOtopic[80];
#ifdef QTEST
    *alternativeNetFile = malloc(20);
	*storeTemp =          malloc(20);
#endif

    startAt(msgfl, &mFile1, cfg.catSector, cfg.catChar);
                                    /* tell putMsgChar where to write   */
    if (cfg.BoolFlags.mirror)
        startAt(msgfl2, &mFile2, cfg.catSector, cfg.catChar);
    doRTS(FALSE);
    putMsgChar(0xFF);               /* start-of-message                 */
    doRTS(TRUE);
    /* write message ID */
    dPrintf("%lu", cfg.newest + 1);

    if (inNet != NON_NET || !roomBuf.rbflags.ANON) {
        /* write date:      */
        if (msgBuf.mbdate[0]) {
            dPrintf("D%s", msgBuf.mbdate);
        }
        else {
            dPrintf("D%s", formDate());
        }

        /* write time:      */
        if (msgBuf.mbtime[0]) {
            dPrintf("C%s", msgBuf.mbtime);
        }
        else {
            getCdate(&year, &month, &day, &h, &m);
            civTime(&h, &ml);
            dPrintf("C%d:%02d %s", h, m, ml);
        }

        if (inNet != NON_NET || loggedIn || alterNet==TRUE ||
                        strCmpU(msgBuf.mbauth, "Citadel") == SAMESTRING) {
                /* write author's name out:         */
	        if (msgBuf.mbauth[0]) {
                dPrintf("A%s", msgBuf.mbauth);
            }
        }
    }
    else {
        dPrintf("D****");
    }

    /* write room name out:             */
    dPrintf("R%s", roomBuf.rbname);

    if (msgBuf.mbto[0]) {       /* private message -- write addressee   */
        dPrintf("T%s", strCmpU(msgBuf.mbto,"Citadel")==SAMESTRING ?
					"All System Users" : msgBuf.mbto);
    }

    if (msgBuf.mbaddr[0]) {     /* net message routing                  */
        if (strCmpU(msgBuf.mbaddr, ALL_LOCALS) == SAMESTRING) {
            for (netPlace = 0; netPlace < cfg.netSize; netPlace++) {
                getNet(netPlace);
				if (msgBuf.mbcompnode[0]) dPrintf("Y%s", msgBuf.mbcompnode);
				if (msgBuf.mbmsgpath[0]) dPrintf("X%s", msgBuf.mbmsgpath);
				if (msgBuf.mbmsgreply[0]) dPrintf("W%s", msgBuf.mbmsgreply);
				/* AB - put our system in msg */
                if (netBuf.nbflags.in_use && netBuf.nbflags.local)
                    netMailProcess(netPlace);
            }
        }
        else if (strCmpU(msgBuf.mbaddr, R_SH_MARK  ) == SAMESTRING ||
                 strCmpU(msgBuf.mbaddr, NON_LOC_NET) == SAMESTRING) {
            dPrintf("N%s", msgBuf.mboname);
			if (msgBuf.mbcompnode[0]) dPrintf("Y%s", msgBuf.mbcompnode);
			if (msgBuf.mbmsgpath[0]) dPrintf("X%s", msgBuf.mbmsgpath);
			if (msgBuf.mbmsgreply[0]) dPrintf("W%s", msgBuf.mbmsgreply);
			   /* AB - write mbcompnode */
            if (msgBuf.mborig[0])   {
                dPrintf("O%s", msgBuf.mborig);
            }
            roomTab[thisRoom].rtlastNet = cfg.newest + 1;
        }
        else if (strCmpU(msgBuf.mbaddr, LOC_NET    ) == SAMESTRING) {
            dPrintf("N%s", msgBuf.mboname);
			if (msgBuf.mbcompnode[0]) dPrintf("Y%s", msgBuf.mbcompnode);
			if (msgBuf.mbmsgpath[0]) dPrintf("X%s", msgBuf.mbmsgpath);
			if (msgBuf.mbmsgreply[0]) dPrintf("W%s", msgBuf.mbmsgreply);
				/* AB - write mbcompnode */
            if (msgBuf.mborig[0])   {
                dPrintf("O%s", msgBuf.mborig);
            }
        }
        else {
            netPlace = searchNameNet(msgBuf.mbaddr);
			if (msgBuf.mbcompnode[0]) dPrintf("Y%s", msgBuf.mbcompnode);
			if (msgBuf.mbmsgpath[0]) dPrintf("X%s", msgBuf.mbmsgpath);
			if (msgBuf.mbmsgreply[0]) dPrintf("W%s", msgBuf.mbmsgreply);
			/* put our node in msg */
            getNet(netPlace);
            netMailProcess(netPlace);
        }
        dPrintf("Q%s", wrNetId(msgBuf.mbaddr));
    }
    else {
        if (msgBuf.mboname[0]) {
            dPrintf("N%s", msgBuf.mboname);
        }
        if (msgBuf.mborig[0])   {
            dPrintf("O%s", msgBuf.mborig);
        }
        if (msgBuf.mbsrcId[0])   {
            dPrintf("S%s", msgBuf.mbsrcId);
        }
		if (msgBuf.mbcompnode[0]) dPrintf("Y%s", msgBuf.mbcompnode);
		if (msgBuf.mbmsgpath[0]) dPrintf("X%s", msgBuf.mbmsgpath);
		if (msgBuf.mbmsgreply[0]) dPrintf("W%s", msgBuf.mbmsgreply);
		/* AB - put our system in msg */
    }


    /* write message text by hand because it would overrun dPrintf buffer: */
	doRTS(FALSE);
    putMsgChar('M');    /* M-for-message.       */
    for (s = msgBuf.mbtext;  *s;  s++) putMsgChar(*s);

    putMsgChar(0);      /* null to end text         */
	doRTS(TRUE);
	if (roomBuf.rbflags.ALTER_NET    /* Check to see if room is linked...   */
		 &&                          /*           AND                       */
		alterNet==FALSE              /* Message is NOT coming from FIDO...  */
		 &&                          /*           AND                       */
	   (logBuf.lbflags.RUGGIE==FALSE /* {User is NOT banned from FIDOnet... */
		 ||                          /* {          OR                       */
	    Net_Monitor==TRUE) ) {       /* {Message is coming in via CitaNet.  */


		if (Net_Monitor==FALSE) {
	        getNormStr("FIDOnet recipient", FIDOrecipient, 25, ECHO);
			if (strlen(FIDOrecipient)<1) sprintf(FIDOrecipient, "%s", "All");
			getNormStr("FIDOnet TOPIC", FIDOtopic, 80, ECHO);
        	if (strlen(FIDOtopic)<1) sprintf(FIDOtopic, "%s", "...");
			}
		if (Net_Monitor==TRUE) { /* this msg coming from other Cit node */
			sprintf(FIDOrecipient, "%s", "All");
			sprintf(FIDOtopic, "%s", "...");
			}
		itoa(thisRoom ,storeTemp, 10);
		sprintf(alternativeNetFile, "%s%s", "alternet.", storeTemp);
		doRTS(FALSE);
        alterNetFile = fopen(alternativeNetFile, "a");
		fprintf(alterNetFile, "%s\n",msgBuf.mbdate);
        getCdate(&year, &month, &day, &h, &m);
		fprintf(alterNetFile, "%02d:%02d:00\n", h, m);
		fprintf(alterNetFile, "%s%s%s\n%s\n%s\n%s\n", msgBuf.mbauth,

/* We only do this for*/ (msgBuf.mboname[0] && Net_Monitor==TRUE)
/* msgs from other Cit*/					? " @" : "",
/* nodes that have to */ (msgBuf.mboname[0] && Net_Monitor==TRUE)
/* go out via FIDOnet */					? msgBuf.mboname : "",
/* next line is ALWAYS active */
			 FIDOrecipient, FIDOtopic, msgBuf.mbtext);
        if (Net_Monitor==TRUE) fprintf(alterNetFile,
				"# Origin: %s via Citadel_Gate at %s\n",
					msgBuf.mboname, cfg.nodeName + cfg.codeBuf);
		fprintf(alterNetFile, "%s\n", "<<CIT_END>>");
		fclose(alterNetFile);
		doRTS(TRUE);
		}

    flushMsgBuf();

    return  TRUE;
}

/************************************************************************/
/*      netMailProcess() Process net mail message                       */
/************************************************************************/
void netMailProcess(int netPlace)
{
    if (!netBuf.nbflags.local) {
        if (!Forwarded && logBuf.credit == 0) {
            crashout("Illegal l-d net privs!");
        }
        if (!Forwarded) {
            logBuf.credit--;
            storeLog();
        }
    }
    netMailOut();
    putNet(netPlace);
}

/************************************************************************/
/*      netMailOut() put mail pointer and number into temp file         */
/************************************************************************/
void netMailOut()
{
    FILE        *fd;
    label       temp;
    SYS_FILE    fn;
    extern char *APPEND_ANY;
    struct netMLstruct buf;

    sPrintf(temp, "%d.ml", thisNet);
    makeSysName(fn, temp, &cfg.netArea);
    if ((fd = safeopen(fn, APPEND_ANY)) == NULL) {
        crashout("putMessage - mail router error!");
    }
    buf.ML_id  = cfg.newest + 1;
    buf.ML_loc = cfg.catSector;
    putMLNet(fd, buf);
    fclose(fd);
    netBuf.nbflags.normal_mail = TRUE;
}

/************************************************************************/
/*      putMsgChar() writes successive message chars to disk            */
/*      Globals:        thisChar=       thisSector=                     */
/*      Returns:        ERROR if problems else TRUE                     */
/************************************************************************/
int putMsgChar(char c)
{
    int  toReturn;
    int  count1, count2;

    toReturn = TRUE;
    count1 = doActualWrite(msgfl, &mFile1, c);
    if (cfg.BoolFlags.mirror) {
        count2 = doActualWrite(msgfl2, &mFile2, c);
        if (count1 != count2) printf("MirrorMsg discrepancy!");
    }
    if (count1)
        logBuf.lbvisit[(MAXVISIT-1)]    = ++cfg.oldest;
    return toReturn;
}

/************************************************************************/
/*      putWord() writes one word to modem & console                    */
/************************************************************************/
void putWord(char *st)
{
    char *s;
    int  newColumn, fillSpaces;
	extern char callLogFlag;
	extern int formIndentedLine;

    for (newColumn = crtColumn, s = st;  *s; s++)   {
        if (*s != TAB)  ++newColumn;
        else            while (++newColumn % 8);
    }
    if (callLogFlag && (newColumn > termWidth) && prevChar!=NEWLINE) {
        doCR();
		if (pausePromptFlag==TRUE) testPausePrompt();
				else currLine=0;
		newColumn = 1;
		crtColumn = 1;
		for (fillSpaces = 0; fillSpaces < formIndentedLine; fillSpaces++) {
			newColumn++;
			crtColumn++;
			oChar(' ');
			}
		}
    else if (newColumn > termWidth)   {
		doCR();
		if (pausePromptFlag==TRUE) testPausePrompt();
				else currLine=0;
		}
    for (;  *st;  st++) {

        if (*st != TAB) ++crtColumn;
        else            while (++crtColumn % 8);

        /* worry about words longer than a line:        */
        if (crtColumn > termWidth) {
			doCR();
			}
        if (prevChar!=NEWLINE  ||  (*st > ' '))   oChar(*st);
        else {
            /* end of paragraph: */
            if (outFlag == OUTPARAGRAPH)   {
                outFlag = OUTOK;
            }
            doCR();
            if (pausePromptFlag==TRUE) {
				if (testPausePrompt()) crtColumn-=6;
				else;
				}
			else   currLine=0;
   	        oChar(*st);
        }
    }
}

/************************************************************************/
/*      showMessages() is routine to print roomful of msgs              */
/************************************************************************/
void showMessages(whichMess, revOrder, allOrLocal, date, user, phrase)
label date, user;
char  whichMess, revOrder, allOrLocal, *phrase;
{
    int        i, rover;
    int        start, finish, increment, oldStart, oldFinish, oldIncrement;
    int        flipOut; /* a "lid catcher" for the PAUSE prompt */
    MSG_NUMBER lowLim, highLim, msgNo;
    char       pulled;
    char       fullFileName[100];
    long       MsgTime, DesTime;
    char       UseDate, userTag, conditionFlag, helpFileGone;
    char       *replyReminder="\n ** Your current reply **";
	extern char     notThisUser;

    setUp(FALSE);
    helpFileGone=FALSE;
	conditionFlag=TRUE;

    if (thisRoom == MAILROOM && !loggedIn) {
        tutorial("POLICY.HLP", TRUE);
        return ;
    }

    if (!expert && TransProtocol == ASCII)
        mPrintf("\n <J>ump <N>ext <P>ause <S>top");

    if ((whichIO != CONSOLE && thisRoom == MAILROOM) &&
        (cfg.SeeMail != TRUE))
        echo = CALLER;

    /* Allow for reverse retrieval: */
    if (!revOrder) {
		if (uniqueFlag==TRUE) {
			if (jumpOut == TRUE) return;
			start       = MSGSPERRM - anchorCounter + uniqueMsgNr - 1;
			finish      = jiggleFlag ? MSGSPERRM : start+1;
			increment   = 1;
			uniqueFlag  = FALSE;
			jiggleFlag  = FALSE;
			}
		else {
        	start       = (thisRoom!=MAILROOM && lastFiveFlag) ? MSGSPERRM-5 :
               (thisRoom!=MAILROOM && lastHowMany) ? MSGSPERRM-howMany :  0;
        	finish      = (thisRoom == MAILROOM) ? MAILSLOTS : MSGSPERRM;
        	increment   = 1;
			lastFiveFlag=FALSE;
        	}
		}
	else {
        start       = (((thisRoom == MAILROOM) ? MAILSLOTS :
						jiggleFlag ? MSGSPERRM-anchorCounter+uniqueMsgNr
						: MSGSPERRM) -1);
        finish      = -1;
        increment   = -1;
		jiggleFlag=FALSE;
        }

    if ((UseDate = ReadDate(date, &DesTime)) == ERROR) {
        mPrintf("\n Bad date\n ");
        return;
        }

    switch (whichMess)   {
    case NEWoNLY:
        lowLim  = logBuf.lbvisit[ logBuf.lbgen[thisRoom] & CALLMASK]+1;
        highLim = cfg.newest;
        if (!revOrder && TransProtocol == ASCII &&
             thisRoom != MAILROOM && oldToo && !allOrLocal &&
             strLen(user) == 0 && strLen(phrase) == 0 && strLen(date) == 0) {
            for (i = MSGSPERRM - 1; i != -1; i--)
                if (lowLim > roomBuf.msg[i].rbmsgNo &&
                    roomBuf.msg[i].rbmsgNo >= cfg.oldest)
                    break;
            if (i != -1)
                printMessage(roomBuf.msg[i].rbmsgLoc, roomBuf.msg[i].rbmsgNo,
                                                                FALSE);
            if (whichIO == MODEM && !gotCarrier() ) {
				echo = BOTH;
            	break;
				}
        }
        break;
    case OLDaNDnEW:
        lowLim  = cfg.oldest;
        highLim = cfg.newest;
        break;
    case OLDoNLY:
        lowLim  = cfg.oldest;
        highLim = logBuf.lbvisit[ logBuf.lbgen[thisRoom] & CALLMASK];
        break;
    }

    /* stuff may have scrolled off system unseen, so: */
    if (cfg.oldest  > lowLim) {
        lowLim = cfg.oldest;
    }

    for (i = start; i != finish && onLine(); i += increment) {
        if (outFlag != OUTOK) {
            if (outFlag == OUTNEXT || outFlag == OUTPARAGRAPH)
                outFlag = OUTOK;
            else if (outFlag == OUTSKIP)   {
                echo = BOTH;
                return;
            }
        }

        msgNo   = roomBuf.msg[i].rbmsgNo;
        if (msgNo >= lowLim  &&  highLim >= msgNo) {

            if (allOrLocal == LOCAL_ONLY || UseDate
					|| strLen(user) || strLen(phrase)) {
                findMessage(roomBuf.msg[i].rbmsgLoc, msgNo);
                if (allOrLocal == LOCAL_ONLY && msgBuf.mboname[0] &&
         strCmpU(msgBuf.mboname, cfg.codeBuf + cfg.nodeName) != SAMESTRING)
                    continue;

                if (UseDate) {
                    if (ReadDate(msgBuf.mbdate, &MsgTime) == TRUE) {
                        if ((!revOrder && MsgTime < DesTime) ||
                            (revOrder && MsgTime > DesTime))
                            continue;
                   }
                }

				if (strLen(user) != 0 && notThisUser) {
					if (strCmpU(msgBuf.mbauth, user) == SAMESTRING) continue;
				}
                if (strLen(user) != 0 && !notThisUser) {
                    if (matchString(msgBuf.mbauth, user, lbyte(msgBuf.mbauth))
                                                                == NULL)
                        continue;
                }
                if (strLen(phrase) != 0) {
                    getMsgStr(msgBuf.mbtext, MAXTEXT);
                        /* Kill extraneous line breaks */
                    for (rover = 0; msgBuf.mbtext[rover]; rover++)
                        if (msgBuf.mbtext[rover] == NEWLINE &&
                            msgBuf.mbtext[rover+1] != ' ' &&
                            msgBuf.mbtext[rover+1] != NEWLINE)
                            msgBuf.mbtext[rover] = ' ';

                    if (matchString(msgBuf.mbtext, phrase,
                                        lbyte(msgBuf.mbtext)) == NULL)
                        continue;
                }
            }

			if (thisRoom != MAILROOM) msgInCount=anchorCounter-MSGSPERRM+i;
			else msgInCount=anchorCounter-MAILSLOTS+i;
            thisMsgNumber=anchorCounter-i;
            printMessage(roomBuf.msg[i].rbmsgLoc, msgNo, FALSE);

/*** let's put in a "Next msg <y/n>?" to keep the screaming yahoos happy! */

            if (thisRoom != MAILROOM && userPause )  {
                flipOut=1;
				if (increment<0 && i==(MSGSPERRM-anchorCounter -1) ) {
					if (heldMess) {
						mPrintf(replyReminder);
						hldMessage();
						}
					return;
					}
				if (increment>0 && i==(MSGSPERRM /* -1 */ ) ) {
					if (heldMess) {
                        mPrintf(replyReminder);
						hldMessage();
						}
					return;
					}
				if ( (start-finish)*(start-finish)== /* 1 */ 0 ) {
					if (heldMess) {
						mPrintf(replyReminder);
						hldMessage();
						}
					return;
					}
				do {
					if (flipOut>10) return;
/*					mPrintf("\n %s<A>gain <S>top <R>eply <+/-> [N]ext: ", */
					mPrintf("\n %s<A>gain <S>top <B>ack <P>rofile [N]ext (%d): ",
						(aide ||
                         (strCmpU(logBuf.lbname, roomBuf.rbmoderator)==SAMESTRING
						 && strLen(logBuf.lbname) != 0)) ? "<D>elete/move " : "",
/*							  helpFileGone==FALSE ? "<H>elp " : "", */
							  thisRoom==MAILROOM ? MAILSLOTS-anchorCounter+thisMsgNumber-1
							  					 : MSGSPERRM - anchorCounter + thisMsgNumber - 1 );
/*							  thisRoom==MAILROOM ? (MAILSLOTS-thisMsgNumber) : (MSGSPERRM-thisMsgNumber)); */
/*							  increment==-1 ? "+" : "-"); */
        			userTag = toUpper(iChar());
                	switch(userTag) {
						case 10 :
						case 13 : conditionFlag=TRUE;
/* the addition of */	   /* doWipeCheck(); */
/* the code for the*/      break;
/* 'Back' option is*/   case '?':

#ifdef OLDWAY

/* added at the    */	case 'H': if (helpFileGone==FALSE) mPrintf("\bHelp");
/* nagging of a    */			  else {
/* PatriotWhiner!  */				  mPrintf(" ?");
									  flipOut++;
									  conditionFlag=FALSE;
                                      break;
									  }
								  if (!HelpIfPresent("pausemsg.blb")) {
									  helpFileGone=TRUE;
									  mPrintf("-file missing");
									  }
								  conditionFlag=FALSE;
                                  break;
#endif

						case 'B': mPrintf("ack");
								  flipDirection=TRUE;
								  conditionFlag=TRUE;
								  break;
						case 'N': mPrintf("ext");
                                  conditionFlag=TRUE;
								  /* if (!doWipeCheck()) */
								  doCR();
								  break;
						case 'S': mPrintf("top");
                                  conditionFlag=TRUE;
								  doCR();
                                  return;
						case 'A': mPrintf("gain");
                                  conditionFlag=TRUE;
								  /* if (!doWipeCheck()) */
								  doCR();
								  i-=increment;
								  break;
/*						case 'R': mPrintf("eply");
                                  replyToThis = TRUE;
								  conditionFlag=TRUE;
								  break; */
						case 'P': userBioSpecial=TRUE;
								  /* messageCalledThis=TRUE; */
                                  doUserInfo();
								  doCR();
                                  /* aideProfile(); */
								  /* messageCalledThis=FALSE; */
								  userBioSpecial=FALSE;
								  conditionFlag=FALSE;
                                  break;
						case 'D': if (aide ||
                                  (strCmpU(logBuf.lbname, roomBuf.rbmoderator)==SAMESTRING
												 && strLen(logBuf.lbname) != 0))
                                                       pullMessage = TRUE;
								  break;
                	    default : flipOut++;
								  conditionFlag=FALSE;
								  mPrintf(" ?");
								  break;
						}
				   } while (conditionFlag==FALSE);
                }

            if ( whichIO == MODEM && !gotCarrier() ) {
				echo = BOTH;
            	return;
				}

            /*  Pull current message from room if flag set */
            if (pullMessage) {
                pullMessage = FALSE;
                pulled = pullIt(i);
                outFlag = OUTOK;
                if (revOrder && pulled)   i++;
            }
            else
                pulled = FALSE;

            if (journalMessage) {
                if (getXString(strFile, fullFileName, 100,
                      (strLen(jrnlFile) == 0) ? NULL : jrnlFile, jrnlFile)) {
                    msgToDisk(fullFileName, msgNo, roomBuf.msg[i].rbmsgLoc);
                    strCpy(jrnlFile, fullFileName);
                }
                outFlag = OUTOK;
                journalMessage = FALSE;
            }
/* #ifdef READY */
            if (replyToThis) {
				heldMess ? hldMessage() : replyMessage();
				outFlag = OUTOK;
				replyToThis = FALSE;
                revOrder ? i++ : i--;
				}
/* #endif */
			if (flipDirection) {
			   if (increment == 1) {
			    	start       = MSGSPERRM -1;
			        finish      = -1;
                    increment   = -1;
                    }
               else {
                    start       = 0;
                    finish      = MSGSPERRM;
                    increment   = 1;
                    }
			   outFlag = OUTOK;
			   flipDirection = FALSE;
			   }
            if (
                TransProtocol == ASCII
                &&
                !pulled
                &&
                thisRoom  == MAILROOM
/*                &&
                whichMess == NEWoNLY */
                &&
                canRespond()
                &&
                (strCmpU(msgBuf.mbauth, logBuf.lbname) != SAMESTRING
                ||
                msgBuf.mborig[0] != 0)  /* i.e. is not local mail           */
                &&
                strCmpU(msgBuf.mbauth, "Citadel") != SAMESTRING
				&&
				strCmpU(msgBuf.mbauth, "FlashNET") != SAMESTRING
                &&
#ifndef BRIAN
                msgBuf.mbauth[0] != 0   /* not anonymous mail> */
                &&
#endif
                getYesNo("Respond")
            ) {

                if (replyMessage())
                    i--;
                if (whichIO != CONSOLE && thisRoom == MAILROOM &&
                    (cfg.SeeMail != TRUE))
                    echo = CALLER;      /* Restore privacy zapped by make... */
                outFlag = OUTOK;
            }

            else if (TransProtocol==ASCII && !pulled && thisRoom==MAILROOM
					 && (whichMess==OLDoNLY || whichMess==OLDaNDnEW)
					 && canRespond() &&
					 ( (strCmpU(msgBuf.mbauth, logBuf.lbname) == SAMESTRING
                                    &&
						msgBuf.mborig[0]==0 )
                        || strCmpU(msgBuf.mbauth, "Citadel")==SAMESTRING
                        || strCmpU(msgBuf.mbauth, "Flashnet")==SAMESTRING
						|| msgBuf.mbauth[0]==0 ) ) {

                        flipOut=1;
						conditionFlag=FALSE;
                    	do {
							if (flipOut>10) return;
							doCR();
							dimColor(colTable.level0 /* A_GREEN */);
							mPrintf("<S>top [N]ext: ");
    	    	    		userTag = toUpper(iChar());
        	    	   		switch(userTag) {
								case 'S': conditionFlag=TRUE;
										  mPrintf("top");
										  doCR();
										  return;
	                            case  10:
								case  13: conditionFlag=TRUE;
										  doCR();
										  break;
								case 'N': conditionFlag=TRUE;
										  mPrintf("ext");
										  doCR();
										  break;
								default : conditionFlag=FALSE;
										  flipOut++;
										  break;
								}
        	                }  while (conditionFlag==FALSE);
					}

        }
    }
    echo = BOTH;
}

char redirect(int noprompt)
{
    extern char *APPEND_TEXT, *strFile;
    char fullFileName[100];

    if (noprompt)
    	strcpy(fullFileName,jrnlFile);
    else
    	if (!getXString(strFile, fullFileName, 100,
          (strLen(jrnlFile) == 0) ? NULL : jrnlFile, jrnlFile)) {
    		return FALSE;
    	}
    if ((upfd = safeopen(fullFileName, APPEND_TEXT)) == NULL) {
    	mPrintf("Couldn't open '%s'\n ", fullFileName);
    } else {
        outPut = DISK;
        strCpy(jrnlFile, fullFileName);
        return TRUE;
    }
    return FALSE;
}

#ifdef CLEARSCREEN
doWipeCheck()
{
  int returnResult=FALSE;

  window(1,2,80,24);
  if (longMessage==TRUE) {
  	clrscr();
    returnResult=TRUE;
  	longMessage=FALSE;
    }
/*  printf("\027[H"); */
  window(1,2,80,25);
  return returnResult;
}
#endif