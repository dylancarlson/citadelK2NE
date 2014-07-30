/************************************************************************/
/*                              netmisc.c                               */
/*                                                                      */
/*      Networking functions of miscellaneous type.                     */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 88Jul11 VAQ  AnyTimeNet patched to properly handle active backbones  */
/* 86Aug20 HAW  History not maintained due to space problems.           */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/************************************************************************/

#define NET_DEBUG 1
/*
#define LEFT   1
#define TOP    1
#define RIGHT 80
#define BOT   25 */

#ifdef QTEST
#define WINLEFT 5
#define WINTOP  8    /* 10 */
#define WINRIGHT 61
#define WINBOTTOM 12  /* 14 */

int netwinbuff[80][25]; /* all this for screen save/restore */
int holdx, holdy, holdj;
#endif

/************************************************************************/
/*             External variable declarations in NETMISC.C              */
/************************************************************************/
char             *SR_Sent;
char             fail_str[3][5] =       /* kludge until next major release */
                        { "", "", "" };
char             ErrBuf[100];           /* General buffer for error messages */

int              AnyIndex = 0;  /* tracks who to call between net sessions */

FILE             *netLog, *netMisc, *netMsg;
static char      UsedNetMsg;
char             *nMsgTemplate = "netMsg.$$$";
char             logNetResults = FALSE, logFileFlag;
char             inNet = NON_NET;
char			 isShuttle;
AN_UNSIGNED      RecBuf[SECTSIZE + 5];
int              callSlot, netMessageTotal;
int              FinHour, FinMinute, endNotePointer, winTop;
label            normed, callerName, callerId;

char *pollCall;
int noKill, errCount = 0;

static char *SupportedBauds[] = {
        "300",
        "3/12",
        "3/12/24",
        "3/12/24/48",
        "3/12/24/48/96",
};

int oldx, oldy;
char dialThis[40];
char netMasterFlag;

/************************************************************************/
/*                 External variable definitions for NET.C              */
/************************************************************************/
extern CONFIG    cfg;            /* Lots an lots of variables    */
extern paintBrush colTable;      /* the ANSI rainbow             */
extern logBuffer logBuf;         /* Person buffer                */
extern aRoom     roomBuf;        /* Room buffer                  */
extern rTable    *roomTab;
extern MSG_BUF   msgBuf, tempMess;
extern NetBuffer netBuf;
extern NetTable  *netTab;
extern int       thisNet;
extern char      onConsole;
extern char      loggedIn;       /* Is we logged in?             */
extern char      outFlag;        /* Output flag                  */
extern char      haveCarrier;    /* Do we still got carrier?     */
extern char      modStat;        /* Needed so we don't die       */
extern char      WCError;
extern char roomLevelFlag;
extern int       thisRoom, SHUTTLE_POINTER;
extern int       thisLog, netLogCount, sessionMessageCount, holdx, holdy;
extern char      *confirm, callLogPosting[800];
extern char      heldMess;
extern char      netDebug;
extern char      fastModem; /* if we can do it, speeds up modem  */
extern char      specialBump, autoNet, human, frontEndNetFlag, frontEnd;
extern char      PrintBanner;    /* if TRUE then net node called */
extern char      answerGuard, justDidNet, windowReport, manualNet, ansi;
extern char      shuttleStyle[10];
extern char      fastModemNetPrefix[50];
extern char		 shuttleUser;

extern char      hostDomainName[80]; /* some internet stuff */

#ifdef ONLYTHENET
extern char      ExitToMsdos;    /* True to bring CitaNet down   */
#endif                           /* after end of net session.    */

/************************************************************************/
/*      called_stabilize() Attempts to stabilize communication on       */
/*                         receiver end.                                */
/************************************************************************/
char called_stabilize()
{
    char retVal;

    retVal = getNetBaud();

    if (!gotCarrier()) {
        killConnection();
        retVal = FALSE;
    }

    return retVal;
}

/************************************************************************/
/*      check_for_init() Looks for networking initialization sequence   */
/************************************************************************/
char check_for_init(mode)
char mode;      /* true if 7 detected already */
{
    int         count, timeOut;
    AN_UNSIGNED thisVal, lastVal;

    lastVal = (mode) ? 7 : 0;
    timeOut = (INTERVALS / 2) * (25);
    for (count = 0; count < timeOut; count++) {
        if (MIReady()) {
            thisVal = inp();
            switch (thisVal) {
                case 7:  lastVal = 7; break;
                case 13:
                         if (lastVal == 7) lastVal = 13;
                         else              lastVal = 0;
                         break;
                case 69:
                         if (lastVal == 13) {
                             lastVal = AckStabilize();
                             switch (lastVal) {
                                 case ACK: return TRUE;
                                 case 7:
                                 case 13: break;
                                 case 69: return (AckStabilize() == ACK);
                                 default: return FALSE;
                             }
                         }
                         else              lastVal = 0;
                         break;
                default:
                         lastVal = 0;
            }
        }
        else pause(1);
    }
    return FALSE;
}

/************************************************************************/
/*      AckStabilize() tries to stabilize with net caller               */
/************************************************************************/
int AckStabilize()
{
/*    while (MIReady()) inp(); */
    outMod(~7);
    outMod(~13);
    outMod(~69);
    return receive(1);
}

/************************************************************************/
/*      searchNet() Searches net for the given Id.                      */
/************************************************************************/
int searchNet(forId)
char *forId;
{
    int rover;
    label temp;

    for (rover = 0; rover < cfg.netSize; rover++) {
        if (netTab[rover].ntflags.in_use &&
            hash(forId) == netTab[rover].ntidhash) {
            getNet(rover);
            normId(netBuf.netId, temp);
            if (strCmpU(temp, forId) == SAMESTRING)
                return rover;
        }
    }
    return ERROR;
}

/************************************************************************/
/*      readMail() Integrates mail into the data base                   */
/************************************************************************/
void readMail(zap, procFn)
char zap;
void (*procFn)(void);
{
    SYS_FILE tempNm, temp2;
    extern char *READ_ANY;

    makeSysName(tempNm, "tempmail.$$$", &cfg.netArea);
    if ((netMisc = safeopen(tempNm, READ_ANY)) == NULL) {
        no_good("Couldn't open tempmail file from %s.", TRUE);
        return;
    }
    getRoom(MAILROOM);
    noKill = FALSE;
    while (getNetMessage(TRUE)) {
        if (strCmpU(cfg.nodeId + cfg.codeBuf, msgBuf.mborig) != SAMESTRING)
            (*procFn)();

    }
    fclose(netMisc);
    if (zap && !noKill) unlink(tempNm);
    else if (zap) {
        sPrintf(msgBuf.mbtext, "temp%d.$$$", errCount++);
        makeSysName(temp2, msgBuf.mbtext, &cfg.netArea);
        rename(tempNm, temp2);
    }
}

/************************************************************************/
/*      getNetMessage() gets a message from a global file               */
/************************************************************************/
char getNetMessage(all)
char all;
{
    int marker, c;

    zero_struct(msgBuf);
    marker = 0;
    while (marker != 'M') {
        while ((marker = getc(netMisc)) != -1 && marker == ' ')
            ;
        if (marker == -1) {
            return FALSE;
        }
        switch (marker) {
            case 'A':
                getNetStr(msgBuf.mbauth, NAMESIZE);
                break;
            case 'D':
                getNetStr(msgBuf.mbdate, NAMESIZE);
                break;
            case 'N':
                getNetStr(msgBuf.mboname, NAMESIZE);
                break;
            case 'O':  getNetStr(msgBuf.mborig, NAMESIZE);
                break;
            case 'R':
                getNetStr(msgBuf.mbroom, NAMESIZE);
                break;
            case 'S':
                getNetStr(msgBuf.mbsrcId, NAMESIZE);
                break;
            case 'T':
                getNetStr(msgBuf.mbto, NAMESIZE);
                break;
            case 'C':
                getNetStr(msgBuf.mbtime, NAMESIZE);
                break;
			case 'Y':
				getNetStr(msgBuf.mbcompnode, NAMESIZE);
				break;
			case 'X':
				getNetStr(msgBuf.mbmsgpath, 480);
				break;
			case 'W':
				getNetStr(msgBuf.mbmsgreply, 480);
				break;
            default:
                if (marker != 'M')
                    while ((c = getc(netMisc)) != -1 && c != '\0')
                        ;
                    break;
        }
    }
    if (all) getNetStr(msgBuf.mbtext, MAXTEXT);
    return TRUE;
}

/************************************************************************/
/*      getNetStr() gets a string from networked message                */
/************************************************************************/
void getNetStr(place, length)
char *place;
int length;
{
    int i, c;

    i = 0;

    do  {
        c = getc(netMisc);
        if (c == '\r') c = '\n';
        place[i++] = c;
    } while (c != -1 && c != '\0' && i < length);

    if (i >= length)
        place[length-1] = 0;
}

/************************************************************************/
/*      getNetChar() gets a character from a network temporary file     */
/************************************************************************/
int getNetChar()
{
     return getc(netMisc);
}

/************************************************************************/
/*      inMail() integrates mail into database                          */
/************************************************************************/
void inMail()
{
    int              logNo;
    logBuffer lBuf;
	long int here;
    extern FILE       *msgfl;
	extern char     inNet;
	 char tempNet;
	label temp;
	label author, context, target, node;
	char tempReply[480];

    initLogBuf(&lBuf);
    if (!msgBuf.mbto[0]) {
        splitF(netLog, "Presentation: 'to' error\n");
        noKill = TRUE;
        killLogBuf(&lBuf);
        return ;
    }
    logNo = findPerson(msgBuf.mbto, &lBuf);
	strcpy(temp, msgBuf.mbto);
    if (logNo == ERROR && hash(temp) != hash("Sysop")
		&& !msgBuf.mbmsgpath[0]) {
        splitF(netLog, "Presentation: Mail to '%s' can't be delivered.\n", msgBuf.mbto);
        killLogBuf(&lBuf);
		flushBuf();
        return;
    }

    if (  (strCmpU(msgBuf.mbto, "sysop") != 0) && (logNo !=ERROR) )
        strCpy(msgBuf.mbto, lBuf.lbname);      /* Make it look "good" */


    doMailForwardCheck(lBuf, logNo);
	killLogBuf(&lBuf);
	flushBuf();

}

/************************************************************************/
/*      netController() Handles the net stuff                           */
/************************************************************************/
#define unSetPoll()     free(pollCall)
/* extern char roomLevelFlag; */
void netController(NetStart, NetLength, whichNets, mode)
int NetStart, NetLength;
MULTI_NET_DATA whichNets;
char mode;
{
    int x;
    int searcher = 0, start, first;
    SYS_FILE AideMsg;
    long waitTime;
    int yr, hr, mins, dy;
    char *mn, netNote[60];
    extern char *WRITE_TEXT, *READ_TEXT, *APPEND_TEXT, *VERTAG;
    extern int noWorkFlag;

    inNet = mode;
	logFileFlag = FALSE;

    setPoll();
    if (mode == ANYTIME_NET) {
        if (!AnyCallsNeeded(whichNets)) {
            inNet = NON_NET;
            unSetPoll();
/*			getCdate(&yr, &mn, &dy, &hr, &mins);
			sPrintf(netNote,
			 " %s @ %d:%02d -- Net: no traffic.",
				formDate(), hr, mins); */
			roomLevelFlag = FALSE;
/*			SpecialMessage(netNote); */
			noWorkFlag = 1;
#ifdef ONLYTHENET
			ExitToMsdos=TRUE;
#endif
            return;
        }
		searcher = AnyIndex;    /* so we don't always start at front */
    }                           /* of list. */
    netLogCount++;
    SR_Sent = (char *) GetDynamic(SHARED_ROOMS);
    if (logNetResults) {
        makeSysName(AideMsg, "netlog.sys", &cfg.netArea);
        if ((netLog = safeopen(AideMsg, APPEND_TEXT)) == NULL)
            netResult("Couldn't open netLog");
    }
    else
        netLog = NULL;

    loggedIn = FALSE;                   /* Let's be VERY sure.          */
    thisLog = -1;
    getCdate(&yr, &mn, &dy, &hr, &mins);
dimColor(A_GREEN);
sessionMessageCount=0;
human=TRUE;
purgeFossilBuffs();
if (outFlag != NET_CALL) {
	fossilDTR(TRUE);
	recycleModem();
    }
/* splitF(netLog, "\nษอออออออออออออออออออออออออออออออออออออป"); */


if (ansi) netWindow(TRUE);
splitF(netLog,
 "*** In Networking Mode ***\n[CitaNet #%d. %s @ %d:%02d]\n\n",

/* "\nบ *** [CitaNet @ %2d:%02d] ***           บ\nบ Session #%02d for %-19s บ", */
/*           hr, mins,netLogCount, formDate()); */
             netLogCount, formDate(), hr, mins);

/* splitF(netLog, "\nศอออออออออออออออออออออออออออออออออออออผ\n\n"); */
#ifdef NEEDSIT
    sPrintf(netNote,
		"Citadel:K2NE V%s   Net started @ %d:%02d %s.",VERTAG, hr, mins, formDate());
	roomLevelFlag = TRUE;
    SpecialMessage(netNote); /* "Network Session" was where netNote is */
	roomLevelFlag = FALSE;
#endif

	window( 7, winTop+2, 73, 14);
    textcolor(YELLOW);


    logMessage(INTO_NET, "", 0);
    netMasterFlag=TRUE;
    modStat = haveCarrier = FALSE;
    setTime(NetStart, NetLength);
    initNetRooms();
    makeSysName(AideMsg, nMsgTemplate, &cfg.netArea);
    if ((netMsg = safeopen(AideMsg, WRITE_TEXT)) == NULL)
        crashout("Couldn't open netMsg");

    UsedNetMsg = FALSE;

    x = timeLeft();
    do {        /* force at least one time through loop */
        waitTime = (cfg.catChar % 5) + 1;
        while (waitTime > minimum(5, ((x/2)))) waitTime /= 2;
        if (inNet != ANYTIME_NET) {
	        for (startTimer(0); chkTimeSince(0) < (waitTime * 60) && !KBReady();) {
    	        if (gotCarrier()) break;
        	}
        }
		if (KBReady() && getCh() == 0x1b)
			break;
        /*
         * In case someone calls while we're doing after call processing
         */
        while (gotCarrier()) {
            modStat = haveCarrier = TRUE;
            called();
        }

        if (cfg.netSize != 0) {
            start = searcher;
            do {
				if (KBReady() && getCh() == 0x1b)
					break;
                if (needToCall(searcher, whichNets)) {
#ifdef NET_DEBUG
					if (netDebug) ExplainNeed(searcher, whichNets);
#endif
                    if (callOut(searcher)) {
                        caller();
                        getCdate(&yr, &mn, &dy, &hr, &mins);
                        splitF(netLog, "%d:%02d\n\n", hr, mins);
                    }
                    for (startTimer(0); !gotCarrier() &&
                        chkTimeSince(0) < ((inNet == ANYTIME_NET) ? 2l : 15l);)
                        ;
                    while (gotCarrier()) {
                        modStat = haveCarrier = TRUE;
                        called();
                    }
                }
                searcher = (searcher + 1) % cfg.netSize;
                if (mode == ANYTIME_NET && timeLeft < 0)
                    break;      /* maintain discipline */
			} while ((searcher+1) % cfg.netSize != start);
        }
        if ((mode == ANYTIME_NET) && !AnyCallsNeeded(whichNets))
        	break;
    } while ((x = timeLeft()) > 0);

    getCdate(&yr, &mn, &dy, &hr, &mins);
	splitF(netLog, "Presentation: Disabling session layer (%d:%02d).\n", hr, mins);
	dimColor(A_GREEN);
	netMasterFlag=FALSE;
    splitF(netLog,"*** Out of Networking Mode ***\n\n");

#ifdef ONLYTHENET
    ExitToMsdos = TRUE;     /* True to bring networker down  */
#endif

    netMessageTotal=netMessageTotal+sessionMessageCount;
    if (inNet == NORMAL_NET) {

        if (AnyCallsNeeded(whichNets)) {
            sPrintf(msgBuf.mbtext,
                    "Nodes missed: ");
            for (searcher = 0, first = 1; searcher < cfg.netSize; searcher++)
                if (needToCall(searcher, whichNets)) {
                    if (!first) strCat(msgBuf.mbtext,", ");
                    first = FALSE;
                    getNet(searcher);
                    strCat(msgBuf.mbtext, netBuf.netName);
                }
            strCat(msgBuf.mbtext, ".");
            netResult(msgBuf.mbtext);
        }
    }
    else if (inNet == ANYTIME_NET) {
				AnyIndex = searcher;    /* so we can start from here later */
    }

    fclose(netMsg);

        /* Make the error and status messages generated into an Aide> msg */
    makeSysName(AideMsg, nMsgTemplate, &cfg.netArea);
    if (UsedNetMsg) {
        if (access(AideMsg, 4) == -1) {
            sPrintf(msgBuf.mbtext, "Where did '%s' go???", AideMsg);
            aideMessage(FALSE);
        }
        else {
            heldMess = FALSE;
            ingestFile(AideMsg);
            strCpy(msgBuf.mbtext, tempMess.mbtext);
			logFileFlag = TRUE;
            aideMessage(FALSE);
        }
    }
    unlink(AideMsg);

    modStat = haveCarrier = FALSE;
    inNet = NON_NET;
    if (logNetResults) {
        fclose(netLog);
    }
    unSetPoll();
    ITL_DeInit();
    free(SR_Sent);
    logMessage(OUTOF_NET, "", 0);
    if (logFileFlag) {
		strCpy(callLogPosting, "Network functions noted in Aide>");
		logMessage(19,"",FALSE);
		logFileFlag = FALSE;
		}
	if (frontEnd) {
		frontEndNetFlag = TRUE;
		specialExit(); /* if Binkley then get me OUT of here! */
        }
	gate_keeper();
/*	if (manualNet==FALSE) doTotals(); */

/*  initFossil(); */
/*	VideoInit(); */

/*	fakeBanner(); */
	runHangup();                /* Click!                          */
    recycleModem();
	/* while (MIReady()) inp(); */
	purgeFossilBuffs();			/* Clear buffer of garbage         */

	getRoom(LOBBY);  /* Make sure user starts in lobby */
    getCdate(&yr, &mn, &dy, &hr, &mins);
	dimColor(A_GREEN);
	if (ansi==TRUE) netWindow(FALSE);
/*	printf(" [Returned from Net @ %d:%02d] ", hr, mins); */

	answerGuard=FALSE;
    justDidNet=TRUE;
    roomLevelFlag = FALSE;

    if (justDidNet==TRUE && manualNet==FALSE && autoNet==FALSE) {
		fakeBanner();
		greeting();
		doTotals();
		}

    if (autoNet==TRUE) {
		specialBump=TRUE;
		doTotals();
		autoNet=FALSE;
		gotoxy(holdx, holdy);
		specialBump=FALSE;
		}

    if (ansi==FALSE) {
		fakeBanner();
		greeting();
		shrtColor(colTable.level2 /* A_BLUE */);
		printf("%s> ",roomBuf.rbname);
		}
}


/************************************************************************/
/*      initNetRooms() Initializes buncha flags 'n things.  Kludgey.    */
/************************************************************************/
void initNetRooms()
{
    int rover, i;

    for (rover = 0; rover < cfg.netSize; rover++) {
        getNet(rover);
        if (netBuf.nbflags.in_use) {
            for (i = 0; i < SHARED_ROOMS; i++) {
                resetNeedsProcessing(i);
            }
            putNet(rover);
        }
    }
}

/************************************************************************/
/*      setTime() Sets up some global variables for the networker       */
/************************************************************************/
void setTime(NetStart, NetLength)
int NetStart, NetLength;
{
    FinHour = (NetStart + NetLength) / 60;
    FinMinute = (NetStart + NetLength) % 60;
}

/************************************************************************/
/*      timeLeft() Does a rough estimate of how much time left          */
/************************************************************************/
int timeLeft()
{
    int yr, hr, mins, dy, retVal;
    char *mn;

    getCdate(&yr, &mn, &dy, &hr, &mins);
    if (hr == FinHour && mins > FinMinute) retVal = -1;
    else if (hr > FinHour) retVal = -1;
    else retVal = (((FinHour - hr) * 60) + abs(mins - FinMinute));
    return retVal;
}

/************************************************************************/
/*      callOut() Attempts to call some other system.                   */
/************************************************************************/
char callOut(i)
int i;
{
    int   yr, hr, mins, dy;
    char  *mn;

    getNet(callSlot = i);
    if (!netBuf.nbflags.local && !cfg.BoolFlags.longHaul)
        return FALSE;
    getCdate(&yr, &mn, &dy, &hr, &mins);
    fossilDTR(TRUE);
	recycleModem();
    splitF(netLog, "Link: Calling %s @ %s (%d:%02d). ",
                     netBuf.netName, netBuf.netId, hr, mins);
    strCpy(normed, netBuf.netId);               /* Cosmetics */
    strCpy(callerId, netBuf.netId);
    strCpy(callerName, netBuf.netName);
    if (makeCall(FALSE))
		return modStat = haveCarrier = TRUE;
    killConnection();             /* Take SmartModem out of call mode   */
    splitF(netLog, "\nLink: No connection.\n\n");
    return FALSE;
}

/************************************************************************/
/*      moPuts() Put a string out to modem without carr check           */
/************************************************************************/
void moPuts(s)
char *s;
{
    while (*s) {
#ifdef TEST
        if (!fastModem) pause(5); /* speed it up if modem is fast enuff! */
#endif
        if (cfg.BoolFlags.debug) putchar(*s);
        outMod(*s++);
    }
}


/************************************************************************/
/*      moShuttlePuts() Put a string out to modem without carr check    */
/************************************************************************/
void moShuttlePuts(s)
char *s;
{
    while (*s) {
#ifdef TEST
        if (!fastModem) pause(5); /* speed it up if modem is fast enuff! */
#endif
        if (cfg.BoolFlags.debug) putchar(*s);
        outShuttleMod(*s++);
    }
}




/************************************************************************/
/*      netMessage() Send message via net                               */
/************************************************************************/
void netMessage(uploading)
char uploading;
{
    if (!cfg.BoolFlags.netParticipant) {
        mPrintf("Net disabled.\n ");
        return;
    }

    if (     (!loggedIn || !logBuf.lbflags.NET_PRIVS) &&
             (!roomBuf.rbflags.AUTO_NET || !roomBuf.rbflags.ALL_NET) ) {
        mPrintf("No net access.\n ");
        return;
    }

    zero_struct(msgBuf);

    if (!netInfo()) return ;
    procMessage(uploading);
}

/************************************************************************/
/*      writeNet() write up nodes on the net.                           */
/************************************************************************/
void writeNet(idsAlso)
char idsAlso;
{
    int            rover, i, first;
	MULTI_NET_DATA h;

    outFlag = OUTOK;
/* #ifndef NETSTATUS
    windowReport=FALSE;
#endif */
    if (!windowReport)
		 mPrintf("%s Nodes:\n \n ", shuttleUser ? "Telnet" : "Network");
	else cprintf(" Net Report:\n\n");

    for (rover = 0; outFlag != OUTSKIP && rover < cfg.netSize; rover++) {
        getNet(rover);
        if (netBuf.nbflags.in_use) {
            if (idsAlso || netBuf.MemberNets & ALL_NETS) {
                if (!windowReport)
					 switch (shuttleUser) {
                          case FALSE:
							 mPrintf("%-21s%s", netBuf.netName,
										netBuf.nbflags.telnet_ok ? "*" : " ");
                             break;
						  case TRUE:
							 if (netBuf.nbflags.telnet_ok) mPrintf("%s\n ",netBuf.netName);
							 break;
						  }
                else {
					 textcolor(RED+BLINK);
					 cprintf(needToCall(rover, ALL_NETS) ? "*" : " ");
					 textcolor(RED);
                     cprintf("%-22s   ", netBuf.netName);
                     }
#ifdef OLD_WAY
				else cprintf("%s%-22s   ",
					needToCall(rover, ALL_NETS) ? "*" : " ",
					netBuf.netName);
#endif
/* #ifdef QTEST */
				if (windowReport) {
					continue; /* special handling gets us out if needs be! */
					}
/* #endif */
                if (idsAlso) {
					mPrintf("%-18s%-14s",
						 netBuf.netId, SupportedBauds[netBuf.baudCode]);
					mPrintf((needToCall(rover, ALL_NETS)) ? "Work pending " : "");
                    if (needToCall(rover, ALL_NETS)) { /* show which nets! */
					    if (netBuf.MemberNets != 0l) {
					        mPrintf("[");
					        for (i = 0, first = 1, h = 1l; i < MAX_NET; i++) {
					            if (h & netBuf.MemberNets) {
				    	            if (!first) mPrintf(",");
					    	            else first = FALSE;
            					    mPrintf("%d", i+1);
					            	}
					            h <<= 1;
					        }
					        mPrintf("]");
						}
				    }
					if (!(netBuf.MemberNets & ALL_NETS))
						mPrintf("**DISABLED**");
                }
                if (!shuttleUser) mPrintf("\n ");
            }
        }
    }
}

/************************************************************************/
/*      searchNameNet() Search net for given node name                  */
/************************************************************************/
int searchNameNet(name)
label name;
{
    int rover;

    for (rover = 0; rover < cfg.netSize; rover++) {
        if (netTab[rover].ntflags.in_use &&
            hash(name) == netTab[rover].ntnmhash) {
            getNet(rover);
            if (strCmpU(netBuf.netName, name) == SAMESTRING)
                return rover;
        }
    }
    return ERROR;
}


/************************************************************************/
/*      netStuff() Handles networking for the sysop and telnet>         */
/************************************************************************/
void netStuff()
{
    extern char *who_str, MenuFlag;
    label       who;
    logBuffer   lBuf;
    int         logNo, netNum;
    long        Redials;

    if (!cfg.BoolFlags.netParticipant) {
        mPrintf("Net DISABLED.\n ");
        return ;
    }
    initLogBuf(&lBuf);
    do {
        outFlag = OUTOK;
		if (!gotCarrier() && MenuFlag) {
			printf("\n");
			doConsoleHelp("netkeys.blb");
			}
		doCR();
        mPrintf("network> ");
        switch (toUpper(iChar())) {
            case 'R':
    			if (shuttleUser) break;
	            mPrintf("equest File\n ");
                fileRequest();
                break;
            case 'S':
    			if (shuttleUser) break;
                mPrintf("end File(s)\n ");
                getSendFiles();
                break;
            case 'X':
                mPrintf("\bExit");
                killLogBuf(&lBuf);
                return;
            case 'C':
    			if (shuttleUser) break;
                mPrintf("redit setting\n ");
                getNormStr(who_str, who, NAMESIZE, ECHO);
                if (strLen(who) == 0) break;
                logNo   = findPerson(who, &lBuf);
                if (logNo == ERROR)   {
                    mPrintf("No record\n ");
                    break;
                }
                mPrintf("%s has %d credits.", who, lBuf.credit);
                lBuf.credit = (int) getNumber("How many now", 0l, 255l);
                if (loggedIn  &&  strCmpU(logBuf.lbname, who) == SAMESTRING)
                    logBuf.credit = lBuf.credit;

                putLog(&lBuf, logNo);

                break;
/*#ifdef QTEST*/
            case 'T':
				if (shuttleUser ? gotShuttleCarrier() : gotCarrier()) {
					mPrintf("\bAlready CONNECTED!");
					break;
					}
				isShuttle=shuttleUser;
/*                mPrintf("\bDial Listed Node\n Node name: "); */
				doCR();
				if (stricmp(shuttleStyle, "lan") != SAMESTRING) {  /* using MODEM  */
					shuttleDTR(FALSE); sleep(1); shuttleDTR(TRUE); /* DTR sequence */
					}

				strCpy(callLogPosting, "Telnet opened.");
				logMessage(19,"",FALSE);

				mPrintf("telnet> ");
   	            getNormStr("", who, NAMESIZE, ECHO);


                if (strcmp(who, "?")==SAMESTRING) {
                    writeNet(FALSE);
					doCR();
					mPrintf("telnet> ");
					getNormStr("", who, NAMESIZE, ECHO);
					}


       	        if (strLen(who) == 0) {
					endTelnet();
					break;
					}

           	    if ((netNum = searchNameNet(who)) == ERROR) {
					if (shuttleUser) {

						mPrintf("Translating \"%s\"... domain server (%s)", strupr(who),
							hostDomainName[0] ? hostDomainName : cfg.codeBuf+cfg.nodeName);

						doCR();
						mPrintf("% Unknown command or host name");
						}
               	    else mPrintf("%s unknown\n", who);
					endTelnet();
                   	break;
                	}
#ifdef OLDWAY
     	        if ((Redials = getNumber("# of redial trys", 0l, 65000l))
                               <= 0l)
#endif
   	            Redials = 1l;
           	    getNet(netNum);
               	if (shuttleUser) shuttleDTR(1);	else EnableModem();
                for (; Redials > 0l; Redials--) {
					isShuttle=TRUE;
					mPrintf("Trying %s", strupr(who));
   	                if (makeCall(TRUE)) {
						mPrintf("Connected to %s.", who);
/* returnToNet: */
			            sprintf(callLogPosting, "Telnet connection to %s. [", who);
						strcat(callLogPosting, logBuf.lbname);
						strcat(callLogPosting, "]");
						logMessage(19,"",FALSE);
                        SpecialMessage(callLogPosting);
       	                interact(FALSE);
						isShuttle=FALSE;
                        roomLevelFlag=TRUE;
					    ScrNewUser();
						doCR();
						shuttleDTR(0);
						mPrintf("[Connection to %s closed by foreign host]", who);
						break;
               	    	}
                    if (KBReady() && !shuttleUser) {
   	                    getCh();
       	                if (shuttleUser) shuttleDTR(0); else killConnection();
						isShuttle=FALSE;
						endTelnet();
           	            break;
                    	}
   	                /*cprintf("Failed\n");*/
       	            for (startTimer(0); chkTimeSince(0) < 3l; )
           	            ;
                }

				endTelnet();

   	            if (!gotCarrier()) {
       	            DisableModem();
           	        modStat = haveCarrier = FALSE;
                }
				isShuttle=FALSE;
                break;
/*#endif*/
            case 'V':
                mPrintf("iew list\n ");
                writeNet(shuttleUser ? FALSE : TRUE);
                break;
            case 'A':
    			if (shuttleUser) break;
                mPrintf("dd node\n ");
                addNetNode();
                break;
            case 'E':
    			if (shuttleUser) break;
                mPrintf("dit node\n ");
                editNode();
                break;
            case 'N':
    			if (shuttleUser) break;
                mPrintf("et privs toggle\n ");
                getNormStr(who_str, who, NAMESIZE, ECHO);
                if (strLen(who) == 0) break;
                logNo   = findPerson(who, &lBuf);
                if (logNo == ERROR)   {
                    mPrintf("No record\n ");
                    break;
                }
                mPrintf(
                    "%s has %snet privs\n ",
                    who,
                    (lBuf.lbflags.NET_PRIVS) ? "no " : ""
                );
                if (!getYesNo(confirm))   break;
                lBuf.lbflags.NET_PRIVS = !lBuf.lbflags.NET_PRIVS;
                if (strCmpU(lBuf.lbname, logBuf.lbname) == SAMESTRING)
                    logBuf.lbflags.NET_PRIVS = lBuf.lbflags.NET_PRIVS;

                putLog(&lBuf, logNo);

                break;
			case 'F':
				if (!shuttleUser) break;
				mPrintf("inger\n ");
				aideProfile();
				break;
            case '?':
				if (!gotCarrier() && !shuttleUser) {
					MenuFlag = TRUE;
					break;
					}
				if (shuttleUser) tutorial("network.mnu", TRUE);
				else tutorial("netopt.mnu", TRUE);
                break;
            default:
                mPrintf(" ?\n ");
        }
    } while (onLine());
    killLogBuf(&lBuf);
}

/************************************************************************/
/*      getSendFiles() get files to send to another system              */
/************************************************************************/
void getSendFiles()
{
    label          sysName;
    SYS_FILE       sysFile;
    struct fl_send sendWhat;
    int            place;
    FILE           *sendFd;
    char           temp[10];
    extern char    *APPEND_ANY;

    if (!getXString("system", sysName, NAMESIZE, NULL, "")) return;
    if ((place = searchNameNet(sysName)) == ERROR) {
        mPrintf("%s not found!\n ", sysName);
        return;
    }
    if (!sysGetSendFiles(&sendWhat)) return;
    sPrintf(temp, "%d.sfl", place);
    makeSysName(sysFile, temp, &cfg.netArea);
    if ((sendFd = safeopen(sysFile, APPEND_ANY)) == NULL) {
        mPrintf("Can't open %s for update?\n ");
        return ;
    }
    putSLNet(sendWhat, sendFd);
    fclose(sendFd);
    getNet(place);
    netBuf.nbflags.send_files = TRUE;
    putNet(place);
}

/************************************************************************/
/*      addNetNode() Add a node to the net listing                      */
/************************************************************************/
void addNetNode()
{
    int searcher, gen;
    char  goodAnswer, found;
    extern char *ALL_LOCALS;

    for (searcher = 0; searcher < cfg.netSize; searcher++)
        if (netTab[searcher].ntflags.in_use == FALSE) break;

    if (searcher != cfg.netSize) {
        getNet(searcher);
        found = TRUE;
        gen = (netBuf.nbGen + 1) % NET_GEN;
    }
    else {
        found = FALSE;
        gen = 0;
    }

    killNetBuf(&netBuf);
    zero_struct(netBuf);                /* Useful initialization       */
    initNetBuf(&netBuf);

    do {
        getNormStr("System name", netBuf.netName, NAMESIZE, ECHO);
        if (strLen(netBuf.netName) == 0) return;
        if ((goodAnswer = strCmpU(ALL_LOCALS, netBuf.netName)) == 0)
            mPrintf("Reserved!\n ");
    } while (!goodAnswer);

    getString("System ID", netBuf.netId, NAMESIZE, FALSE, ECHO);
    if (strLen(netBuf.netId) == 0) return;
    netBuf.baudCode = (int) getNumber(
     "Baud code (0=300, 1=1200, 2=2400, 3=4800, 4=9600)", 0l, 5l);
    netBuf.nbflags.local       = getYesNo("Is system local");
    netBuf.nbflags.uses_frontend =
		  getYesNo("Does system use \"front-end\" software\n [If in doubt, answer \"Y\"]");
    netBuf.nbflags.in_use      = TRUE;
    netBuf.MemberNets          = 1;     /* Default */
    netBuf.nbGen               = gen;   /* Update generation #  */
    netBuf.nbflags.telnet_ok   = getYesNo("Telnet ok");
    if (!found) {
        if (cfg.netSize != 0)
            netTab = (NetTable *)
                        realloc(netTab, sizeof (*netTab) * ++cfg.netSize);
        else
            netTab = (NetTable *)
                GetDynamic(sizeof(*netTab) * ++cfg.netSize);
        searcher = cfg.netSize - 1;
        netTab[searcher].netTRooms = (struct shared_room *) GetDynamic(SR_BULK);
        netTab[searcher].ntArchRooms =
                        (struct shared_room *) GetDynamic(NA_BULK);
    }
    putNet(searcher);
    InitVNode(searcher);
}

/************************************************************************/
/*      addNetMem() add nets to this system's list                      */
/************************************************************************/
int addNetMem(netnum)
char *netnum;
{
    int num;
    MULTI_NET_DATA temp;

    num = atoi(netnum);
    if (num < 1 || num > MAX_NET) {
        mPrintf("Only 32 allowed.");
        return TRUE;
    }
    temp = 1l;
    temp <<= (num-1);
    netBuf.MemberNets |= temp;
    return TRUE;
}

/************************************************************************/
/*      subNetMem() takes nets from a system's list                     */
/************************************************************************/
int subNetMem(netnum)
char *netnum;
{
    int num;
    MULTI_NET_DATA temp;

    num = atoi(netnum);
    if (num < 1 || num > MAX_NET) {
        mPrintf("Only 32 allowed.");
        return TRUE;
    }
    temp = 1l;
    temp <<= (num-1);
    temp = ~temp;
    netBuf.MemberNets &= temp;
    return TRUE;
}

/************************************************************************/
/*      editNode() Edit a net node                                      */
/************************************************************************/
void editNode()
{
    label          sysname, temp, temp2;
    int            place;

    getNormStr("System to edit", sysname, NAMESIZE, ECHO);
    if (strLen(sysname) == 0) return;
    if ((place = searchNameNet(sysname)) == ERROR) {
        mPrintf("%s not listed!\n ", sysname);
        return;
    }
    getNet(place);
    NodeValues();

    while (1) {
        outFlag = OUTOK;
        mPrintf("\n (%s) edit fn: ", netBuf.netName);
        switch (toUpper(iChar())) {
            case 'A':
                mPrintf("ccess setting\n ");
                getString("Access string (C/R deactivates)",
                                netBuf.access, 40, FALSE, ECHO);
                break;
            case 'R':
                mPrintf("ooms shared\n ");
#ifdef NET_BUG
                dumpNodeRoom(FALSE);
#else
                dumpNodeRoom();
#endif
                break;
            case 'X':
                mPrintf("\bExit");
                putNet(place);
                return;
            case 'B':
                mPrintf("ps change\n ");
                netBuf.baudCode = (int) getNumber(
                  "Baud code (0=3, 1=12, 2=24, 3=48, 4=96)", 0l, 4l);
                break;
            case 'N':
                mPrintf("ame change\n ");
                getNormStr("System name", temp, NAMESIZE, ECHO);
                if (strLen(temp) != 0) strCpy(netBuf.netName, temp);
                if (getYesNo("Is this a totally new system"))
                    netBuf.nbGen = (netBuf.nbGen + 1) % NET_GEN;
                break;
            case 'I':
                mPrintf("D change\n ");
                getString("System ID", temp, NAMESIZE, FALSE, ECHO);
                if (strLen(temp) != 0) strCpy(netBuf.netId, temp);
                if (getYesNo("Is this a totally new system"))
                    netBuf.nbGen = (netBuf.nbGen + 1) % NET_GEN;
                break;
            case 'K':
                mPrintf("ill node\n ");
                if (netBuf.nbflags.normal_mail)
                        mPrintf("MAIL remains.\n ");
                if (netBuf.nbflags.room_files)
                        mPrintf("FILE REQUESTS remain.\n ");
                if (getYesNo("Confirm")) {
                    netBuf.nbflags.in_use = FALSE;
                    putNet(place);
                    sPrintf(temp, "%d.ml", thisNet);
                    makeSysName(temp2, temp, &cfg.netArea);
                    unlink(temp2);
                    sPrintf(temp, "%d.rfl", thisNet);
                    makeSysName(temp2, temp, &cfg.netArea);
                    unlink(temp2);
                    return;
                }
                break;
            case 'L':
                mPrintf("ocal/LD\n ");
                netBuf.nbflags.local = getYesNo("Is system local");
                break;
			case 'F':
				mPrintf("ront-end\n ");
				netBuf.nbflags.uses_frontend =
					getYesNo("Does system use \"front-end\" software");
				break;
            case 'P':
                mPrintf("asswords\n Currently:\n ");
                mPrintf(" Our password: %s\n", netBuf.OurPwd);
                mPrintf(" Their password: %s\n", netBuf.TheirPwd);
                if (getXString("our new password",
                           temp2, NAMESIZE, "", ""))
                    strCpy(netBuf.OurPwd, temp2);
                if (getXString("their new password",
                           temp2, NAMESIZE, "", ""))
                    strCpy(netBuf.TheirPwd, temp2);
                break;
            case 'M':
                mPrintf("ember Nets\n ");
                getList(addNetMem, "Nets to add to this system's member list");
                getList(subNetMem,"Nets to take off this system's member list");
                break;
            case 'S':
                mPrintf("pine settings\n ");
                sPrintf(msgBuf.mbtext, "We will be a spine for %s",
                                                netBuf.netName);
                if (!(netBuf.nbflags.spine =
                        getYesNo(msgBuf.mbtext))) {
                    sPrintf(msgBuf.mbtext, "%s will be a spine",
                                                netBuf.netName);
                    netBuf.nbflags.is_spine =
                        getYesNo(msgBuf.mbtext);
                }
                else
                    netBuf.nbflags.is_spine = FALSE;
                break;
			case 'T':
				netBuf.nbflags.telnet_ok = getYesNo("Telnet ok");
				break;
            case 'V':
                NodeValues();
                break;
            case '?':
                tutorial("netedit.mnu", TRUE);
                break;
            default:
                mPrintf("?\n ");
        }
    }
}

/************************************************************************/
/*      NodeValues() prints out the values for the current node         */
/************************************************************************/
void NodeValues(void)
{
    int            i, first;
    MULTI_NET_DATA h;

    mPrintf("\n Node: %s\n Id: %s (%slocal @ %s)%s.\n %s",
                        netBuf.netName, netBuf.netId,
                        netBuf.nbflags.local ? "" : "non",
                        SupportedBauds[netBuf.baudCode],
						netBuf.nbflags.telnet_ok ? "T" : "",
						netBuf.nbflags.uses_frontend ?
								"Uses special front-end.\n " : "");
    if (netBuf.nbflags.spine)
        mPrintf("We are a spine.\n ");
    else if (netBuf.nbflags.is_spine)
        mPrintf("This node is a spine.\n ");

    if (strLen(netBuf.access) != 0)
        mPrintf("Access: %s\n ", netBuf.access);

    if (netBuf.nbflags.normal_mail)
        mPrintf("Outgoing Mail>.\n ");

    if (netBuf.nbflags.room_files)
        mPrintf("FILE REQUESTS remain.\n ");

    if (netBuf.nbflags.send_files)
        mPrintf("FILES to be SENT.\n ");

    if (netBuf.MemberNets != 0l) {
        mPrintf("Member of nets: ");
        for (i = 0, first = 1, h = 1l; i < MAX_NET; i++) {
            if (h & netBuf.MemberNets) {
                if (!first)
                    mPrintf(", ");
                else first = FALSE;
                mPrintf("%d", i+1); /* Yes - +1. Number the bits starting with 1 */
            }
            h <<= 1;
        }
        mPrintf(".\n ");
    }
    else mPrintf("System DISABLED.\n ");
}

/************************************************************************/
/*      fileRequest() For network requests of files.                    */
/************************************************************************/
void fileRequest()
{
    struct fl_req file_data;
    label    system, data;
    char     loc[100];
    SYS_FILE fn;
    char     abort;
    FILE     *temp;
    int      place;
    extern char *APPEND_ANY;
    char     ambiguous;

    getNormStr("system to request file from", system, NAMESIZE, ECHO);
    if (strLen(system) == 0) return;
    if ((place = searchNameNet(system)) == ERROR) {
        mPrintf("%s not listed!\n ", system);
        return;
    }

    getNet(place);

    sPrintf(loc, "roomname on %s that has desired file", system);;
    getNormStr(loc, file_data.room, NAMESIZE, ECHO);
    if (strLen(file_data.room) == 0) return;

    getNormStr("the file(s)'s name", file_data.roomfile, NAMESIZE, ECHO);
    if (strLen(file_data.roomfile) == 0) return;
    ambiguous = !(strchr(file_data.roomfile, '*') == NULL &&
                  strchr(file_data.roomfile, '?') == NULL);

    abort = !netGetArea(&file_data, ambiguous);

    if (!abort) {
        sPrintf(data, "%d.rfl", place);
        makeSysName(fn, data, &cfg.netArea);
        if ((temp = safeopen(fn, APPEND_ANY)) == NULL) {
            mPrintf("Couldn't append to '%s'????", fn);
        }
        fwrite(&file_data, sizeof (file_data), 1, temp);
        fclose(temp);
        netBuf.nbflags.room_files = TRUE;
        putNet(place);
    }
}

/************************************************************************/
/*      roomsShared() Returns true if this system has a room with new   */
/*                    data to share (orSomething)                       */
/************************************************************************/
char roomsShared(slot)
int slot;
{
    int i;

        /* We only want to make one "successful" call per
           voluntary net session */
    if ((inNet == NORMAL_NET || inNet == ANYTIME_NET ) &&
                                         !pollCall[slot])
        return FALSE;

        /* This isn't really a good idea.  Makes for too much disk
           usage.  Should clean it up someday. */
    if (slot != thisNet)
        getNet(slot);

        /* The following is for NORMAL NET sessions...
         * Rules:
         * We check each slot of the shared rooms list for this node.  For
         * each one that is in use, we do the following:
         * a) if we are regional host for the room and other system is
         *    a backbone, then don't assume we need to call.
         * b) if we are backboning the room, check to see what status of this
         *    room for other system is.
         *    1) If we are Passive Backbone, then we need not call.
         *    2) If we are Active Backbone, then do call.
         *    3) The Regional Host looks screwy.  This may be a bug.
         * c) If none of the above applies, implies we are a simple Peon, so
         *    we simply check to see if we have outgoing messages, and if so,
         *    return TRUE indicating that we need to call; otherwise, continue
         *    search.
         */
    for (i = 0; i < SHARED_ROOMS; i++) {
        if (isSharedRoom(slot, i) && roomValidate(slot, i)) {
            if (roomTab[netTabRoomSlot(slot, i)].rtShareType == REG_HOST)
                if (netTab[slot].netTRooms[i].mode == BACKBONE)
                    continue;
            if (roomTab[netTabRoomSlot(slot, i)].rtShareType == BACKBONE) {
                if (netTab[slot].netTRooms[i].mode == PASS_BACKBONE)
                    continue;
                if (netTab[slot].netTRooms[i].mode == REG_HOST ||
                    netTab[slot].netTRooms[i].mode == ACTIVE_BACKBONE) {
if (netDebug && inNet != NON_NET) splitF(netLog, "returning true at pt.1 (%s)\n",
roomTab[netTabRoomSlot(slot, i)].rtname);
                   if (inNet == NORMAL_NET)
					  return TRUE;
                }
            }
            if (roomTab[netTabRoomSlot(slot, i)].rtlastNet >
                                netTab[slot].netTRooms[i].lastMess) {
if (netDebug && inNet != NON_NET) splitF(netLog, "returning true at pt.2 (%s)\n",
roomTab[netTabRoomSlot(slot, i)].rtname);
                return TRUE;
            }
        }
    }
    return VirtualNeed(slot);
}

#ifdef NET_BUG
void dumpNodeRoom(file)
char file;
{
    label name;
    FILE *fopen();
    int rover;

    if (!inNet) {
        if (logNetResults && file) {
            sPrintf(name, "%c:netlog.sys", cfg.homeDisk + 'a');
            netLog = fopen(name, "a");
        }
        else
            netLog = NULL;
    }

    splitF(netLog, "-- DEBUG: dump of node %s (#%d) --\n", netBuf.netName,
                                                           thisNet);
    splitF(netLog, "Room list dump:\n");
    for (rover = 0; rover < SHARED_ROOMS; rover++) {
        splitF(netLog, "Slot %d: room#0x%-9xgen=0x%x", rover,
                netBuf.netRooms[rover].srslot,
                netBuf.netRooms[rover].srgen);
        if (netBuf.netRooms[rover].srgen & 0x8000) {
            splitF(netLog, " IN USE apparently\n");
            splitF(netLog, "%9croom is %s, gen is 0x%x\n", ' ',
                        roomTab[netRoomSlot(rover)].rtname,
                        roomTab[netRoomSlot(rover)].rtgen);
        }
        else splitF(netLog, "\n");
    }
    if (!inNet && logNetResults && file)
        fclose(netLog);
}
#else
void dumpNodeRoom()
{
    int rover;
    char *name;

    for (rover = 0; rover < SHARED_ROOMS; rover++) {
        if (isSharedRoom(thisNet, rover) && roomValidate(thisNet, rover)) {
            mPrintf("%-22sWe are a", roomTab[netRoomSlot(rover)].rtname);
            switch (roomTab[netRoomSlot(rover)].rtShareType) {
                case PEON:
                    name = " Peon";
                    break;
                case REG_HOST:
                    name = " Regional Host";
                    break;
                case BACKBONE:
                switch (netBuf.netRooms[rover].mode) {
                    case PEON:
                        name = " Backbone, they are a Peon";
                        break;
                    case ACTIVE_BACKBONE:
                    case REG_HOST:
                        name = "n Active Backbone";
                        break;
                    case PASS_BACKBONE:
                        name = " Passive Backbone";
                        break;
                }
                break;
            }
            mPrintf(name);
			mPrintf(" (last sent=%ld/%ld, netlast=%ld)\n ",
				netBuf.netRooms[rover].lastMess,
				netTab[thisNet].netTRooms[rover].lastMess,
				roomTab[netRoomSlot(rover)].rtlastNet);
        }
    }
    V_Listing();
}
#endif

/************************************************************************/
/*      netResult() Put a message to the net msg holder.                */
/************************************************************************/
void netResult(msg)
char *msg;
{
    int yr, hr, mins, dy;
    char *mn;

    getCdate(&yr, &mn, &dy, &hr, &mins);
    fprintf(netMsg, " (%d:%02d) %s\n \n", hr, mins, msg);
    fflush(netMsg);
    UsedNetMsg = TRUE;
}

/************************************************************************/
/*      netInfo() Acquires necessary info from the user                 */
/************************************************************************/
char netInfo()
{
    extern char *ALL_LOCALS;
    extern char *R_SH_MARK;
    char  notDone = TRUE;
    label sys;
    char  *address;
	label first, str1;
	char path[480];

    strcpy(first, '\0');
    if (thisRoom == MAILROOM) {
        do {
			setmem(path, strlen(path), '\0');
            getString("system to send to", sys, 20, TRUE, ECHO);
            if (strLen(sys) == 0) return FALSE;
            if (sys[0] == '?') showNetMailNodes();
/*			{
                writeNet(FALSE);
				writeNetPath();
                if (aide) mPrintf("'&L' == Local Nodes Announcement\n ");
            } */
            else
			{
               if (searchNameNet(sys) == ERROR && strCmpU(sys, ALL_LOCALS) != 0)
			   {
                  if (findPath(sys, first, path))
				  {
					 strcpy(sys, first);
 					 notDone=FALSE;
				   }
			       else mPrintf("Not listed\n ");
			    }
				else notDone=FALSE;
			 }

        } while (notDone);

        if (strCmpU(sys, ALL_LOCALS) != 0) {
            getNet(searchNameNet(sys));
            if (!cfg.BoolFlags.longHaul && !netBuf.nbflags.local) {
                mPrintf("LOCAL nodes only!\n ");
                return FALSE;
            }
/*            if (!netBuf.nbflags.local && logBuf.credit == 0) { */
			if (!netBuf.nbflags.local && checkLDcredit() == 0) {
                mPrintf("No LD credit.\n ");
                return FALSE;
            }
            address = netBuf.netName;
        }
        else {
            if (!logBuf.lbflags.AIDE) {
                mPrintf("Illegal.\n ");
                return FALSE;
            }
            address = ALL_LOCALS;
        }
    }
    else {
        if (!roomBuf.rbflags.SHARED) {
            mPrintf("Not a net room.\n ");
            return FALSE;
        }
        address = R_SH_MARK;
       	strCpy(msgBuf.mboname, cfg.codeBuf + cfg.nodeName);
    }
    strCpy(msgBuf.mbaddr, address);
	strCpy(msgBuf.mbmsgpath, path);
    strcpy(str1, cfg.codeBuf+cfg.nodeId);
	stripSpaces(str1);
	strCpy(msgBuf.mbmsgreply, str1);
	strcat(msgBuf.mbmsgreply, "|");
    return TRUE;
}


/************************************************************************/
/*      killConnection() zaps carrier for network                       */
/************************************************************************/
void killConnection()
{
    /* runHangup(); */

	fossilDTR(FALSE);
	recycleModem();
    modStat = FALSE;
	haveCarrier = FALSE;

    /* while (MIReady()) inp(); */    /* Clear buffer of garbage */
}

/************************************************************************/
/*      setPoll() Make sure of polling of hosts                         */
/************************************************************************/
void setPoll()
{
    int rover;

    pollCall = GetDynamic(cfg.netSize);
    for (rover = 0; rover < cfg.netSize; rover++) {
        pollCall[rover] = 1;
    }
}

/************************************************************************/
/*      makeCall() does dialing                                         */
/************************************************************************/
int makeCall(CheckKbd)
char CheckKbd;
{
    char  call[80];
    label blip1;
    int   bufc;
    char  buf[10], c, viable;

    /* while (MIReady()) inp(); */
	purgeFossilBuffs();

    if (!shuttleUser) recycleModem();

/* shuttleStyle[10] is MODEM or LAN */

    if (strnCmp(netBuf.access, "&P", 2) != SAMESTRING) {
        setNetCallBaud(netBuf.baudCode);
        normId(netBuf.netId, blip1);
        strCpy(call, shuttleUser ? "ATDT" :
			   netBuf.baudCode >3 ? fastModemNetPrefix :
			   cfg.codeBuf + cfg.netPrefix);
        if (strLen(netBuf.access) != 0) {
            strCat(call, netBuf.access);
        }
        else if (!netBuf.nbflags.local) {
            strCat(call, "1");
            strCat(call, blip1 + 2);
        }
        else {
            strCat(call, blip1 + 5);
        }
        if (isShuttle) mPrintf(" (%s)... ", netBuf.netId);
        strCat(call, cfg.codeBuf + cfg.netSuffix);

		if ( isShuttle && (!netBuf.nbflags.local && checkLDcredit() == 0
						||
			    !netBuf.nbflags.telnet_ok) ) {
			doCR();
        	mPrintf("% Connection to that host not permitted.");
			return FALSE;
			}

        if ((stricmp(shuttleStyle, "modem")==SAMESTRING) && shuttleUser)
           moShuttlePuts(call);
		else moPuts(call);
	    /* while (MIReady()) inp(); */

		purgeFossilBuffs();
        for (startTimer(0), bufc = 0, viable = TRUE;
            chkTimeSince(0) < ((netBuf.nbflags.local) ? 40l : 60l) && viable;) {
            if ((shuttleUser && gotShuttleCarrier() )
							 ||
				(!shuttleUser && gotCarrier()) )      break;
                /* Parse incoming string from modem -- call progress detection */
            if (CheckKbd && KBReady()) viable = FALSE;
            if (MIReady()) {
                if ((c = getMod()) == '\r') {
                    buf[bufc] = 0;
                    for (bufc = 0; bufc < 3; bufc++) {
                        if (strLen(fail_str[bufc]) != 0 &&
                            strCmpU(buf, fail_str[bufc]) == SAMESTRING) {
                            viable = FALSE;
							splitF(netLog, "(%s) ", buf);
                        }
                    }
                    bufc = 0;
               }
               else {
                   if (bufc > 8) bufc = 0;
                   else {
                       buf[bufc++] = c;
                   }
               }
           }
        }
        if (shuttleUser ? gotShuttleCarrier() : gotCarrier()) {
			if (isShuttle) {
				mPrintf("Open");
                doCR();
/*				mPrintf("Connected to "); */
				}
            return TRUE;
			}


#ifdef Q_TEST
            return /* modStat = haveCarrier = TRUE; */ TRUE;
#endif

    }
    else {
        runPCPdial();
    	}
	if (isShuttle) {
		doCR();
		mPrintf("%cTimeout: Remote host not responding.",'%');
		doCR();
		shuttleDTR(0);
		}
    return FALSE;
}

void parseBadRes(vals)
char *vals;
{
    char *v;
    int i, j;

    for (v = vals; *v != '='; v++)
        ;
    v++;

    for (j = 0; j < 3 && *v; j++) {
        for (i = 0; *v != ',' && *v; v++, i++)
            fail_str[j][i] = *v;
        fail_str[j][i] = 0;
        if (*v) v++;
    }
}


#ifdef NET_DEBUG
ExplainNeed(i, x)
int i;
MULTI_NET_DATA x;
{
    splitF(netLog, "Presentation: Data for Network layer follows\n              Data:  i%d sp%d MN%ld nm%d rf%d sf%d shared%d\n",
          netTab[i].ntflags.in_use,
          netTab[i].ntflags.is_spine,
          netTab[i].ntMemberNets & x, netTab[i].ntflags.normal_mail,
          netTab[i].ntflags.room_files,
          netTab[i].ntflags.send_files, roomsShared(i));
}
#endif

/************************************************************************/
/*      needToCall() do we need to call this system?                    */
/************************************************************************/
int needToCall(i, x)
int i;
MULTI_NET_DATA x;
{
                                /* first check for permission to call */
    if (netTab[i].ntflags.in_use &&     /* account in use */
       (netTab[i].ntMemberNets & x) &&  /* system is member of net */
       !netTab[i].ntflags.is_spine) {   /* system not spine */
                                /* check for requirement to call */
        if (netTab[i].ntflags.spine &&
           (inNet == NON_NET || (inNet == NORMAL_NET && pollCall[i] == 1)))
            return TRUE;
                                /* now check for need to call     */
        if (netTab[i].ntflags.normal_mail || /* normal outgoing mail ? */
            netTab[i].ntflags.room_files ||  /* request files ?        */
            netTab[i].ntflags.send_files ||  /* send files ?           */
            roomsShared(i))                  /* rooms to share?        */
            return TRUE;
    }
    return FALSE;
}

char AnyCallsNeeded(whichNets)
MULTI_NET_DATA whichNets;
{
    int searcher;

    for (searcher = 0; searcher < cfg.netSize; searcher++)
        if (needToCall(searcher, whichNets)) return TRUE;

    return FALSE;
}

/*
 *  manualDial() -- handles the CTRL-L D outdial stuff.
 *                  Function is called from doSysop() in CTDL.C
 */
#ifdef TERMOK
manualDial()
{
 char call[60], /* dialThis[40], */ stepFlag;
 int stepFlagCount;
 long Redials;

 getString("number to dial", dialThis, 40, FALSE, ECHO);

 sprintf(call, "%s%s%s", cfg.codeBuf + cfg.netPrefix,
              dialThis, cfg.codeBuf + cfg.netSuffix);

 if ((Redials = getNumber("# of redial attempts", 0l, 65000l)) <= 0l)
         Redials = 1l;

 EnableModem();
 for (; Redials > 0l; Redials--) {
     sleep(1);
     moPuts(call);
	 stepFlag=FALSE;
	 stepFlagCount=0;
	 while (stepFlag==FALSE) {
		if (gotCarrier()) {
	           interact(FALSE);
			   stepFlag=TRUE;
			   Redials=0l;
    		   }
		else stepFlagCount++;
		if (stepFlag==TRUE) break;
        if (KBReady()) {
			getCh();
			killConnection();
			break;
			}
		sleep(1);
		if (stepFlagCount>30) stepFlag=TRUE;
		}
     if (KBReady()) {
   	       getCh();
       	   killConnection();
           break;
           }
   	 printf(Redials > 1l ? "Redialing\n" : "Returning\n");
     if (Redials > 0l) {
		for (startTimer(0); chkTimeSince(0) < 3l; )
            ;
		}
     }
 if (!gotCarrier()) {
      DisableModem();
      modStat = haveCarrier = FALSE;
      }
}
#endif


netWindow(mode)
int mode;
{
 int skipper, inside;
 char *uleft="ษ";
 char *urite="ป";
 char *lleft="ศ";
 char *lrite="ผ";
 char *side="บ";
 char *border="อ";
 char *tmsg="ตCitadel:K2NE Network Sessionฦ";
 char *bmsg="ตPress ESC between calls to EXITฦ";


 skipper=3;
 if (mode==TRUE) {
	savetext(1,1,80,15);
#ifdef TOPSCREEN
    if (!onLine() ) doTopScreen(1, 2);
#endif
/* #ifdef NOISE */
    if (manualNet==TRUE) upWin();
/* #endif */
    window( 6, 4, 74, 15);
	textcolor(YELLOW);
	textbackground(BLUE);
	clrscr();
	cprintf("%s", uleft);
	skipper=1;
	while (skipper<18) {cprintf(border); skipper++;}
	cprintf(tmsg);
	skipper=1;
	while (skipper<21) {cprintf(border); skipper++;}
	cprintf(urite);
	skipper=3;
    while (skipper<13) {
		cprintf("%s", side);
		inside=1;
        while (inside<68) {
			cprintf(" ");
			inside++;
			}
		cprintf("%s", side);
        skipper++;
		}
    cprintf("%s", lleft);
	skipper=1;
	while (skipper<17) {cprintf(border); skipper++;}
	cprintf(bmsg);
	skipper=1;
	while (skipper<19) {cprintf(border); skipper++;}
    window(74,15, 75, 15);
	cprintf(lrite);
    winTop=5;
	window( 7, winTop, 73, 14);
	textcolor(WHITE);
	textbackground(RED);
	clrscr();
 	}
 else if (mode==FALSE) {
    sleep(2);
    window(6,4,74,15);
    textbackground(BLACK);
/* #ifdef NOISE */
    if (manualNet==TRUE) downWin();
/* #endif */
    clrscr();
    window(1,1,80,24);
	if (manualNet==FALSE && autoNet==FALSE) clrscr();
	else {
		restoretext(1,1,80,15);
/*		ScrNewUser(); */
		}
	}

 return;
}

endTelnet()
{
 strCpy(callLogPosting, "Telnet closed.");
 logMessage(19,"",FALSE);
}