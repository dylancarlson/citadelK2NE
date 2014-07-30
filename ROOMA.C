/************************************************************************/
/*                              rooma.c                                 */
/*              room code for Citadel bulletin board system             */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/*      SEE THE INCREM.* FILES FOR FURTHER HISTORICAL NOTES             */
/* 84Jul12 JLS & HAW  gotoRoom() and dumpRoom() modified for <S>kip.    */
/* 84Apr04 HAW  Start 1.50a update                                      */
/* 83Feb24      Insert check for insufficient RAM, externs too low.     */
/* 82Dec06 CrT  2.00 release.                                           */
/* 82Nov05 CrT  main() splits off to become citadel.c                   */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      dumpRoom()              tells us # new messages etc             */
/*      fDir()                  prints out a filename for a dir listing */
/*      fillMailRoom()          set up Mail> from log record            */
/*      gotoRoom()              handles "g(oto)" command for menu       */
/*      initCitadel()           system startup initialization           */
/*      listRooms()             lists known rooms                       */
/*      retRoom()               handle Ungoto command                   */
/*      roomExists()            returns slot# of named room else ERROR  */
/*      searchRooms()           searches room list for matching string  */
/*      setUp()                                                         */
/*      systat()                shows current system status             */
/*      wildCard()              expands ambiguous filenames             */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*             external variable declarations in ROOMA.C                */
/************************************************************************/
char          *baseRoom;
char          shuttleUser, userBioSpecial;
int           UngotoStack[UN_STACK];
char          remoteSysop = FALSE;      /* Is current user a sysop      */
char          shownHidden, justVisiting;
long          BDSizeCount;
int           InitColumns = 40;

#ifdef QTEST
char          sysProt1[30], sysProt2[30], sysProt3[30], sysProt4[30];
#endif

static int    *lPtrTab;                 /* For .Ungoto                  */

/************************************************************************/
/*             external variable definitions for ROOMA.C                */
/************************************************************************/
extern CONFIG    cfg;            /* A buncha variables           */
extern paintBrush colTable;      /* the ANSI rainbow             */
extern LogTable  *logTab;        /* RAM index of pippuls         */
extern MSG_BUF   msgBuf;
extern logBuffer logBuf;         /* Pippul buffer                */
extern NetBuffer netBuf;
extern struct floor     *FloorTab;
extern FILE      *logfl;         /* log file descriptor          */
extern FILE      *msgfl;         /* Message file descriptor      */
extern FILE      *msgfl2;        /* Another for auto backup      */
extern FILE      *netfl;         /* Net file                     */
extern rTable    *roomTab;       /* RAM index of rooms        */
extern aRoom     roomBuf;        /* room buffer                  */
extern FILE      *roomfl;        /* file descriptor for rooms    */
extern int       thisRoom;       /* room currently in roomBuf    */
extern char      loggedIn;       /* Are we logged in?            */
extern char      inNet;
extern char      echo;           /* output flag                  */
extern char      prevChar;       /* Last char out                */
extern char      onConsole;      /* on console?                  */
extern char      whichIO;        /* where is the I/O?            */
extern int       thisSlot;       /* Current log slot             */
extern char      outFlag;
extern char      nextDay;        /* System up before bailout?    */
extern char      heldMess;
extern label     oldTarget;      /* Room to move messages to     */
extern char      ShowNew;
extern char      fastModem, ansi, shortSession, JustChecking;
extern int DispDirectoryRoom(), DispPrivateRoom(), DispNetworkRoom(),
           DispAnonRoom(), DispExterLinkedRoom(), DispCrossLinkedRoom();
extern int DispGatewayRoom();
extern int       SYSproto, anchorCounter;
extern char      siegeResult;    /* keep things neat for prying eyes! */
extern char      alterNet, infoBannerCut, globalAllFinished;
extern char      shortWay, shuttleActive;
extern int       ourBBSport;
extern char shuttleStyle[10];
extern char	noAudibleRing;
extern char	noNoises;
extern char	autoNet;
extern char justOut;
extern char	unixStyle;
extern char	locFIDO;
extern char    Net_Monitor;
extern char	mailTag;
extern char	notDoorUser;
extern char	queryDenied;
extern char	runningAsDoor;
extern char	siege;
extern char    privateStatus;
extern char	justDidNet;
extern char	frontEnd;
extern int	timeInDoors;
extern int	userMessages;
extern char    archiveLogFlag;
extern char    frontEndNetFlag;
extern char	specialPrompt;
extern char onlyAsNet;
extern char	AideNetTrigger;
extern char	shortSession;
extern char	sentNotice;
extern char	ansi;
extern char    sysopColors;
extern char    chatFlag;
extern char	uploadingFlag;
extern char	uniqueFlag;
extern char	jiggleFlag;
extern int  anchorCounter;
extern char	fastOut;
extern char	jumpOut;
extern char	shortStuff;
extern int	tryToLogIn;
extern int  InitColumns; /* modernization! */
extern char    autoFIDOlink;
extern char	stickingAround;
extern int	linkerNumber;
extern int  ringsToAnswer;
extern int  numberOfRings;
extern char    answerGuard;
extern char	manualNet;
extern int SHUTTLE_POINTER;
extern int      SHUTTLE_POINTER;
extern char   hostDomainName[80];
extern char killScreen;










/************************************************************************/
/*      dumpRoom() tells us # new messages etc                          */
/************************************************************************/
void dumpRoom(ShowFloor)
char ShowFloor;
{
#ifndef NOTONLYTHENET
    extern char HasSkipped;
    int         i, count, newCount;

    for (newCount = count = i = 0;
             i < ((thisRoom == MAILROOM) ? MAILSLOTS : MSGSPERRM);   i++) {
                                /* Msg is still in system?  Count it.   */
        if (roomBuf.msg[i].rbmsgNo >= cfg.oldest) {
            count++;

            /* don't boggle -- just checking against newest as of */
            /* the last time we were  in this room:               */
            if (roomBuf.msg[i].rbmsgNo >
                          logBuf.lbvisit[ logBuf.lbgen[thisRoom] & CALLMASK ] &&
                                  roomBuf.msg[i].rbmsgNo <= cfg.newest) {
                newCount++;
            }
        }
    }

    if (!loggedIn && thisRoom == MAILROOM)      /* Kludge for new users */
        newCount = count = 1;                   /* So they see intro.   */


#ifdef OLD_FLOOR_DISPLAY

    if (ShowFloor)

/* No longer showing floors at transition - put floorname in front of
   roomname as in "[Floor] Room> " for those in floor-mode. */

/*        mPrintf(" Moving to \"%s\" floor.\n ", FloorTab[thisFloor].FlName); */
/*        mPrintf(" [Floor: %s]\n ", FloorTab[thisFloor].FlName); */

#endif

   if (newCount==count && loggedIn) { /* show room description if new = all */
		justVisiting=TRUE;
        readRoomInfoFile();
		justVisiting=FALSE;
		}


/*    mPrintf(" %d message%s\n ", count, count == 1 ? "" : "s"); */
/*   doCR();  IS THIS REALLY NEEDED?? */

/* ORIGINAL METHOD
    mPrintf(" %d message%s", count, count == 1 ? "" : "s");
*/
    mPrintf(" Total: %d", count);

    anchorCounter=count;
    if ((thisRoom == MAILROOM || loggedIn) && newCount > 0)
/*        mPrintf(" %d new\n", newCount); */

/* ORIGINAL METHOD
        mPrintf("\n  %d new", newCount);
*/
        mPrintf(", %d new", newCount);

    if (roomBuf.rbflags.READ_ONLY) {
		/* doCR(); */
		mPrintf(" [Read Only]");
		}



    infoBannerCut=TRUE;
	doInfobanner();
/*    doCR(); */

    if (thisRoom == LOBBY) {
        HasSkipped = FALSE;
        if (tableRunner(NSRoomHasNew, TRUE) != ERROR)
            return ;

        if (tableRunner(RoomHasNew, TRUE) == ERROR)
            return ;

        if (HasSkipped) {
            if (FloorMode)
                FSkipped();
            else {
                mPrintf("\n Skipped rooms: \n ");
                ShowNew = TRUE;
                JustChecking = FALSE;
                tableRunner(SkippedNewRoom, TRUE);
            }
        }
    }
#endif
}

int SkippedNewRoom(i)
int i;
{
#ifndef ONLYTHENET
    if (roomTab[i].rtflags.SKIP == 1 && RoomHasNew(i)) {
        roomTab[i].rtflags.SKIP = 0;                   /* Clear. */
        if (ShowNew) mPrintf(" %s ", formRoom(i, TRUE, TRUE));
        if (JustChecking) return TRUE;
    }
    return FALSE;
#endif
}

/************************************************************************/
/*      fDir() prints out one filename and size, for a dir listing      */
/************************************************************************/
int fDir(fileName)
char *fileName;
{
#ifndef ONLYTHENET
    long size, Sectors;
    extern char *READ_ANY;
    extern DIR_EXTRA GblOther;
	char sizestr[6];

    outFlag = OUTOK;
    unopenSize(&size, fileName);
    BDSizeCount += size;
	if (size > 99999)
		sprintf(sizestr,"%4dK",size/1024);
	else
		sprintf(sizestr,"%5ld",size);
/*    shrtColor(colTable.level2); BLUE */
    mPrintf("%-15s%s  ", fileName, sizestr, ' ');
    mAbort();       /* chance to next(!)/pause/skip */
/* 	shrtColor(colTable.level0); GREEN */
    return TRUE;
#endif
}

/************************************************************************/
/*      fillMailRoom()                                                  */
/************************************************************************/
void fillMailRoom()
{
#ifndef ONLYTHENET
    int i;

    /* mail room -- copy messages in logBuf to room: */
    for (i = 0;  i < MAILSLOTS;  i++) {
        roomBuf.msg[i].rbmsgNo   = 0l;  /* Marks "no" msg */
        roomBuf.msg[i].rbmsgLoc  = 0 ;  /* Jest fer fun   */
    }
    for (i = 0;  i < MAILSLOTS;  i++) {
        roomBuf.msg[i].rbmsgLoc  = logBuf.lbMail[i].rbmsgLoc;
        roomBuf.msg[i].rbmsgNo   = logBuf.lbMail[i].rbmsgNo ;
    }
    noteRoom();
#endif
}

/************************************************************************/
/*      gotoRoom() is the menu fn to travel to a new room               */
/*      returns TRUE if room is Lobby>, else FALSE                      */
/************************************************************************/
int gotoRoom(nam, mode)
char *nam, mode;
{
#ifndef NOTONLYTHENET
    int  i, foundit, roomNo;
    int  lRoom;
    static char *noRoom = " Not found\n"; /* " ?no %s room\n"; */
    int  oldFloor;
    char NewFloor = FALSE;

    lRoom = thisRoom;

    if (strLen(nam) == 0) {

        foundit = FALSE;        /* leaves us in Lobby> if nothing found */
        if (mode != 'S') {
            if (loggedIn)
                logBuf.lbgen[thisRoom] = roomBuf.rbgen << GENSHIFT;
            roomTab[thisRoom].rtflags.SKIP = 0;
        }
        if (!FloorMode) {

            for (i = 0; i<MAXROOMS  &&  !foundit; i++) {
                if (
                    roomTab[i].rtflags.INUSE == 1
                    &&
                    ( roomTab[i].rtgen == (logBuf.lbgen[i] >> GENSHIFT) ||
                      (aide &&
                      (cfg.BoolFlags.aideSeeAll || onConsole) &&
                      (!roomTab[i].rtflags.INVITE || onConsole)) )
                    &&
                    !roomTab[i].rtflags.SKIP
                ) {
                    if (roomTab[i].rtlastMessage >
                            logBuf.lbvisit[logBuf.lbgen[i] & CALLMASK] &&
                        roomTab[i].rtlastMessage >= cfg.oldest) {

                        if (i != thisRoom) {
                            foundit  = i;
                        }
                    }
                }
            }

            getRoom(foundit);
            mPrintf("\"%s\"\n ", roomBuf.rbname);
        }
        else {
            NewFloor = NewRoom();
            foundit = thisRoom;
        }
        UngotoMaintain(lRoom);
    } else {
        oldFloor = thisFloor;
        foundit = 0;
        /* non-empty room name, so now we look for it: */
        if (
            (roomNo = roomCheck(roomExists, nam)) == ERROR &&
            (roomNo = roomCheck(partialExist, nam)) == ERROR
        ) {
            mPrintf(noRoom /*, nam */ );
        } else if (roomTab[roomNo].rtflags.INVITE
						&&
				  (!aide || !onConsole)
						&&
                  roomTab[roomNo].rtgen != logBuf.lbgen[roomNo] >> GENSHIFT
						&&
				  alterNet==FALSE) {
            mPrintf(noRoom /* , nam */);
        } else {
            foundit = roomNo;
            if (mode != 'S') {
                if (loggedIn)
                    logBuf.lbgen[thisRoom] = roomBuf.rbgen << GENSHIFT;
                roomTab[thisRoom].rtflags.SKIP = 0;
            }
            UngotoMaintain(lRoom);
            getRoom(roomNo);

            /* if may have been unknown... if so, note it:      */
            if ((logBuf.lbgen[thisRoom] >> GENSHIFT) != roomBuf.rbgen) {
                logBuf.lbgen[thisRoom] = (
                    (roomBuf.rbgen << GENSHIFT) +
                    (MAXVISIT -1)
                );
/*			justVisiting=TRUE; */

            }
            if (FloorMode) NewFloor = !(oldFloor == thisFloor);
        }
    }

    setUp(FALSE);
	globalAllFinished=FALSE;
    if (foundit==0 && !FloorMode) globalAllFinished=TRUE;
    if (!siegeResult && thisRoom == MAILROOM) {
		ScrNewUser();
	    return foundit;
		}
/*	if (justVisiting) readRoomInfoFile();
	justVisiting=FALSE; */
    dumpRoom(NewFloor);
	ScrNewUser();
    return foundit;
#endif
}

/************************************************************************/
/*      initCitadel() does not reformat data files                      */
/************************************************************************/
void initCitadel()
{
    SYS_FILE tempName;
    extern char *READ_TEXT, *VERSION, *netVersion, *FLASH_MAIL_VERSION;
	extern char *NET_SWITCH_VERSION;
	extern char noInit;
    echo = BOTH;

    if (!readSysTab(TRUE, TRUE))
        exit(CRASH_EXIT);/* No system table? Tacky, tacky*/
#ifdef NOANSI
	ansi=cfg.Ansi;
#else
	ansi=TRUE;
#endif
	fastModem=cfg.modemSpeed;
    cfg.weAre = CITADEL;
    systemInit();
#ifndef NOFOSSIL
	if (!noInit)	{
			modemInit();
			}
	else initFossil();
	if (stricmp(shuttleStyle, "modem")==SAMESTRING
					||
		stricmp(shuttleStyle, "lan")==SAMESTRING) {
	        SHUTTLE_POINTER=1-cfg.FOSSIL_PORT;
 			initShuttle();
			shuttleActive=TRUE;
        	}
#endif
#ifdef QTEST
    SYSproto = protocolInit(); /* for sysop-defined external protocols
   					              if present as PROTOTBL.SYS in homeArea */
#endif

	whichIO = CONSOLE;
	termWidth = 79;

	if (!noInit) {
/* #ifdef OLDWAY */
			doCredits();
/* #endif */
			if (ansi==FALSE) doInfobanner();
        }
    if (access(LOCKFILE, 0) != -1) {
        printf("LOCKed!\n");
        writeSysTab();          /* Save it out just in case */
        systemShutdown();
        exit(RECURSE_EXIT);
    }

    InitEvents(TRUE);
    initLogBuf(&logBuf);
    initRoomBuf(&roomBuf);
    initNetBuf(&netBuf);
    initTransfers();
    lPtrTab = (int *) GetDynamic(MAXROOMS * sizeof (int));

    strCpy(oldTarget, "Aide");

/* why bother???? #### someone is killing it and I am working around it <br>
   either form works.
    baseRoom = &cfg.codeBuf[cfg.bRoom];
	baseRoom = cfg.codeBuf + cfg.bRoom;
*/
#ifdef NOFOSSIL
	if (!noInit)	{
			modemInit();
			}
	else initFossil();
#endif
    setUp(TRUE);
    VideoInit();
    SpecialMessage("Opening files");
    /* open message files: */
    makeSysName(tempName, "ctdlmsg.sys",  &cfg.msgArea);
    openFile(tempName, &msgfl );
    makeSysName(tempName, "ctdllog.sys",  &cfg.logArea);
    openFile(tempName, &logfl );
    makeSysName(tempName, "ctdlroom.sys", &cfg.roomArea);
    openFile(tempName, &roomfl);

    if (cfg.BoolFlags.netParticipant) {
        makeSysName(tempName, "ctdlnet.sys", &cfg.netArea);
        openFile(tempName, &netfl);
        VirtInit();
    }

    if (cfg.BoolFlags.mirror) {
        makeSysName(tempName, "ctdlmsg.sys", &cfg.msg2Area);
        openFile(tempName, &msgfl2);
    }

	ScrNewUser();
    initArchiveList();
	    getRoom(LOBBY);     /* load Lobby>  */
    	whichIO = MODEM;
	    purgeFossilBuffs();
    	setUp(FALSE);
}

/************************************************************************/
/*      legalMatch() Looks for partial matches, checks legalities       */
/************************************************************************/
char legalMatch(i, target)
label target;
int i;
{
#ifndef NOTONLYTHENET
    char  Equal, *endbuf;

    Equal = (roomTab[i].rtgen == (logBuf.lbgen[i] >> GENSHIFT));

    if ((roomTab[i].rtflags.INUSE == 1) &&
           ((aide && cfg.BoolFlags.aideSeeAll &&
            !roomTab[i].rtflags.INVITE)
                                       || Equal)) {
        for (endbuf = roomTab[i].rtname; *endbuf; endbuf++);
        return (matchString(roomTab[i].rtname, target, endbuf) != NULL);
    }
    return FALSE;
#endif
}

#ifndef NOTONLYTHENET
/************************************************************************/
/*      listRooms() lists known rooms                                   */
/************************************************************************/
void listRooms(mode)
int mode;
{
    shownHidden = FALSE;


    if (FloorMode || mode == FORGOTTEN) {

        FKnown(mode);
        return;
    }

    if (mode==5)   /* directory */
	{
       mPrintf("\n ");
	   tableRunner(DispDirectoryRoom, TRUE);
 	   mPrintf("\n ");
	   return;
    }

    if (mode==6)   /* private */
	{
       mPrintf("\n ");
	   tableRunner(DispPrivateRoom, TRUE);
       mPrintf("\n ");
	   return;
    }
#ifndef BRIAN
    if (mode==7)   /* anonymous */
	{
       mPrintf("\n ");
	   tableRunner(DispAnonRoom, TRUE);
       mPrintf("\n ");
	   return;
    }
#endif
    if (mode==8)   /* networked */
	{
       mPrintf("\n ");
	   tableRunner(DispNetworkRoom, TRUE);
       mPrintf("\n ");
	   return;
    }

    if (mode==9)   /* external-net linked */
	{
       mPrintf("\n ");
	   tableRunner(DispExterLinkedRoom, TRUE);
       mPrintf("\n ");
	   return;
    }

    if (mode==10)   /* cross-linked shared rooms*/
	{
       mPrintf("\n ");
	   tableRunner(DispCrossLinkedRoom, TRUE);
       mPrintf("\n ");
	   return;
    }
/* #ifdef QTEST */
    if (mode==11)   /* gateway rooms*/
	{
       mPrintf("\n ");
	   tableRunner(DispGatewayRoom, TRUE);
       mPrintf("\n ");
	   return;
    }
/* #endif */

        /* Else */
    shrtColor(colTable.level0 /* A_GREEN */);
    mPrintf("\n Rooms with unread messages:\n ");
    ShowNew = TRUE;
    shrtColor(colTable.level1 /* A_RED */);
    tableRunner(DispRoom, TRUE);
    if (mode == INT_EXPERT) return;         /* Sneak out backdoor */
	shrtColor(colTable.level0 /* A_GREEN */);
    mPrintf("\n No unseen msgs in:\n ");
    shrtColor(colTable.level2 /* A_BLUE */);
    ShowNew = FALSE;
    tableRunner(DispRoom, TRUE);
}
#endif /* NOTONLYTHENET */

int tableRunner(func, OnlyKnown)
int (*func)(int rover);
char OnlyKnown;
{
    int rover;

    for (rover = 0; rover < MAXROOMS; rover++) {
        if (  roomTab[rover].rtflags.INUSE &&
              (!OnlyKnown ||
               roomTab[rover].rtgen - (logBuf.lbgen[rover] >> GENSHIFT) == 0
            ))
            if ((*func)(rover)) return rover;
    }
    return ERROR;
}

/************************************************************************/
/*      knowRoom() check to see if current person knows given room      */
/*      Return 0 if not know room, 2 if forgot room, 1 otherwise        */
/************************************************************************/
char knowRoom(i)
int i;
{
#ifndef NOTONLYTHENET
    int difference;

    difference = abs(roomTab[i].rtgen - (logBuf.lbgen[i] >> GENSHIFT));
    return ((difference == 0) ? 1 : ((difference == FORGET_OFFSET) ? 2 : 0));
#endif
}

/************************************************************************/
/*      partialExist() roams the list looking for a partial match       */
/************************************************************************/
int partialExist(target)
label target;
{
#ifndef ONLYTHENET
    int rover;

    for (rover = (thisRoom + 1) % MAXROOMS; rover != thisRoom;
                                            rover = (rover + 1) % MAXROOMS)
        if (legalMatch(rover, target)) return rover;
    return ERROR;
#endif
}

/************************************************************************/
/*      retRoom() Ungoto command                                        */
/************************************************************************/
void retRoom(roomName)
char *roomName;
{
#ifndef ONLYTHENET
    int slot, OldFloor;

    OldFloor = thisFloor;
    if (strLen(roomName) == 0) {
        if (UngotoStack[0] == -1) {
            mPrintf("\n Nope!\n ");
            return;
        }
        getRoom(UngotoStack[0]);
        mPrintf("\"%s\"\n ", roomBuf.rbname);
        logBuf.lbgen[thisRoom] = lPtrTab[thisRoom];
                        /* Now pop that top element off the stack */
        movmem(UngotoStack + sizeof(int), UngotoStack,
                                        (UN_STACK - 1) * sizeof(int));
        UngotoStack[UN_STACK-1] = -1;   /* bottom of stack */
    }
    else {
        if (
            ((slot = roomCheck(roomExists, roomName)) == ERROR &&
            (slot = roomCheck(partialExist, roomName)) == ERROR) ||
            (roomTab[slot].rtflags.INVITE && (!aide || !onConsole) &&
            roomTab[slot].rtgen != logBuf.lbgen[slot] >> GENSHIFT)
        ) {
            mPrintf(" ?no \"%s\" room\n", roomName);
            return;
        }
        UngotoMaintain(thisRoom);
        getRoom(slot);
        logBuf.lbgen[thisRoom] = lPtrTab[thisRoom];
    }
    setUp(FALSE);
    dumpRoom(FloorMode ? !(OldFloor == thisFloor) : FALSE);
#endif
}

/************************************************************************/
/*      roomCheck() returns slot# of named room else ERROR              */
/************************************************************************/
int roomCheck(checker, nam)
int (*checker)(char *name);
char *nam;
{
#ifndef NOTONLYTHENET
    int roomNo;

    if (
        (roomNo = (*checker)(nam)) == ERROR
        ||
        (roomNo==AIDEROOM  &&  !aide)
        ||
        (roomTab[roomNo].rtflags.PUBLIC == 0 && !loggedIn && !alterNet)
    )
        return ERROR;
    return roomNo;
#endif
}

/************************************************************************/
/*      roomExists() returns slot# of named room else ERROR             */
/************************************************************************/
int roomExists(room)
char *room;
{
#ifndef NOTONLYTHENET
    int i;

    for (i = 0;  i < MAXROOMS;  i++) {
        if (
            roomTab[i].rtflags.INUSE == 1   &&
            strCmpU(room, roomTab[i].rtname) == SAMESTRING
        ) {
            return(i);
        }
    }
    return(ERROR);
#endif
}

/************************************************************************/
/*      searchRooms() searches for user string in list of rooms         */
/************************************************************************/
void searchRooms()
{
#ifndef ONLYTHENET
    label target;
    int   i;

    getNormStr("", target, NAMESIZE, ECHO);

    mPrintf("Matches:\n ");
    outFlag = OUTOK;

    for (i = 0; i < MAXROOMS;  i++) {
        if (legalMatch(i, target)) {
            mPrintf(" %s ", formRoom(i, TRUE, TRUE));
        }
    }
#endif
}

/************************************************************************/
/*      setUp()                                                         */
/************************************************************************/
void setUp(justIn)
char justIn;
{
    int g, i, j, ourSlot;
    extern int logTries;
    extern long DL_Total;

    echo                = BOTH;         /* just in case                 */

    if (!loggedIn)   {
        remoteSysop = FALSE;
        prevChar    = ' ';
        termWidth   = InitColumns;
        termLF      = TRUE;
/*      termNulls   = 5;      OBSOLETE! */
        expert      = FALSE;
        aide        = FALSE;
        sendTime    = TRUE;
        oldToo      = FALSE;
/*        HalfDup     = FALSE; */
	 	shuttleUser = FALSE;
        FloorMode   = FALSE;

        if (justIn)   {
            for (i = 0; i < UN_STACK; i++)
                UngotoStack[i] = -1;
            /* set up logBuf so everything is new...        */
            logTries = 0;
            heldMess = FALSE;
            for (i = 0; i < MAXVISIT;  i++)  logBuf.lbvisit[i] = cfg.oldest;

            /* no mail for un-logged anonymous folks: */
            roomTab[MAILROOM].rtlastMessage = cfg.newest;
            for (i = 0; i < MAILSLOTS;  i++)
                logBuf.lbMail[i].rbmsgNo = 0l;

            logBuf.lbname[0] = 0;

            for (i = 0; i < MAXROOMS;  i++) {
                if (roomTab[i].rtflags.PUBLIC) {
                    /* make public rooms known: */
                    g               = roomTab[i].rtgen;
                    logBuf.lbgen[i] = (g << GENSHIFT) + (MAXVISIT-1);
                } else {
                    /* make private rooms unknown: */
                    g               = (roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN;
                    logBuf.lbgen[i] = (g << GENSHIFT) + (MAXVISIT-1);
                }
                lPtrTab[i]  = logBuf.lbgen[i];
            }
        }
    } else {
        /* loggedIn: */
        if (justIn)   {
            DL_Total = 0l;
            remoteSysop = FALSE;
            for (i = 0; i < UN_STACK; i++)
                UngotoStack[i] = -1;
            heldMess = FALSE;
            /* set gen on all unknown rooms  --  INUSE or no: */
            for (i = 0;  i < MAXROOMS;  i++) {
                if (roomTab[i].rtflags.PUBLIC == 0) {
                    /* it is private -- is it unknown? */
                    if (((logBuf.lbgen[i] >> GENSHIFT) != roomTab[i].rtgen) ||
                         (!aide && i == AIDEROOM)
                       ) {
                        /* yes -- set   gen = (realgen-1) % MAXGEN */
                        j = (roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN;
                        logBuf.lbgen[ i ] =  (j << GENSHIFT) + (MAXVISIT-1);
                    }
                }
                else if ((logBuf.lbgen[i] >> GENSHIFT) != roomTab[i].rtgen)  {
                    /* newly created public room -- remember to visit it; */
                    j = roomTab[i].rtgen - (logBuf.lbgen[i] >> GENSHIFT);
                    if (j < 0)
                        g = -j;
                    else
                        g = j;
                    if (g != FORGET_OFFSET) {
                        logBuf.lbgen[i] = (roomTab[i].rtgen << GENSHIFT) +1;
                    }
                }
            }
            /* special kludge for Mail> room, to signal new mail:   */
            roomTab[MAILROOM].rtlastMessage =
                                 logBuf.lbMail[MAILSLOTS-1].rbmsgNo;

            /* slide lbvisit array down and change lbgen entries to match: */
            for (i = (MAXVISIT - 2);  i;  i--) {
                logBuf.lbvisit[i] = logBuf.lbvisit[i-1];
            }
            logBuf.lbvisit[(MAXVISIT - 1)]    = cfg.oldest;
            for (i = 0;  i < MAXROOMS;  i++) {
                if ((logBuf.lbgen[i] & CALLMASK)  <  (MAXVISIT-2)) {
                    logBuf.lbgen[i]++;
                }
                lPtrTab[i]  = logBuf.lbgen[i];
            }

            /* Slide entry to top of log table: */
            ourSlot = logTab[thisSlot].ltlogSlot;
            slideLTab(0, thisSlot);

            logTab[0].ltpwhash      = hash(logBuf.lbpw);
            logTab[0].ltnmhash      = hash(logBuf.lbname);
            logTab[0].ltlogSlot     = ourSlot;
            logTab[0].ltnewest      = cfg.newest;
        }
    }
    logBuf.lbvisit[0]   = cfg.newest;

    if (whichIO==CONSOLE) onConsole=TRUE;
	else onConsole=FALSE;

/*    onConsole   = (whichIO == CONSOLE); */

    if (thisRoom == MAILROOM)   fillMailRoom();
}

/************************************************************************/
/*      ShowVerbose() does display of a file for .Read Extended         */
/************************************************************************/
int ShowVerbose(fileName)
char *fileName;
{
#ifndef ONLYTHENET
    extern int DirAlign;
    extern char AlignChar;
    extern DIR_EXTRA GblOther;
    char *strchr();
    long size;

    outFlag = OUTOK;

    unopenSize(&size, fileName);
    BDSizeCount += size;

/*  mPrintf("%-14s%6ld |(%s) ", fileName, size, GblOther.Fdate); */
/*	mPrintf("%-12s |[%ld bytes, %s] ", fileName, size, GblOther.Fdate); */
	mPrintf("%-15s Upload date: %s  (%ld bytes)",
		fileName, GblOther.Fdate, size);

    if (FindFileComment(fileName)) {
		shrtColor(colTable.level1 /* A_RED */);
		mPrintf(termWidth > 40 ? "\n      " : "\n  ");
        msgBuf.mbtext[strLen(msgBuf.mbtext) - 1] = 0;
        DirAlign = (termWidth > 40 ? 5 : 1 ); /* was 21 */
        AlignChar = ' '; /* was '|' */
        mFormat(strchr(msgBuf.mbtext, ' '));
		doCR(); /* K2NE on 89Feb04 */
		shrtColor(colTable.level2 /* A_BLUE */);
   }
/*  mPrintf(" (%s)", GblOther.Fdate); */
    DirAlign = 0;
    doCR();
    mAbort();       /* chance to next(!)/pause/skip */
#endif
}

/************************************************************************/
/*      systat() prints out current system status                       */
/************************************************************************/
void systat()
{
    int   i;
    MSG_NUMBER average, work;
    int   roomCount, dummy;
    char dummyKludge[8];

    for (roomCount = i = 0; i < MAXROOMS; i++)
        if (roomTab[i].rtflags.INUSE) roomCount++;
    sprintf(dummyKludge, "%s", "why"); /* don't ask! */
    if (!shortWay) doInfobanner();
    if (loggedIn && !shortWay) {
        mPrintf("\n Online: %s%s ",
			aide && !shortWay ? "Aide, " : "",
			logBuf.lbname);
        }

    else if (loggedIn && shortWay) {

/* Show the SYSOP on CONSOLE if we are in CHAT with a modem user */

        if (whichIO==CONSOLE && gotCarrier()) {

			mPrintf("%-22s%-22sttyp0 %s", cfg.SysopName, "Citadel", hostDomainName[0] ? hostDomainName : "");

			doCR();
			}

/* Show the logged-in user, modem or otherwise */

		mPrintf("%-22s%-22sttyp%d %s [%ld min]",
				 logBuf.lbname, roomBuf.rbflags.PUBLIC
									? roomBuf.rbname : "Citadel",
				 gotCarrier() ? ourBBSport+1 : 0,
				 hostDomainName[0] ? hostDomainName : "\t",
				 chkTimeSince(3)/60);

		doCR();
        if (shuttleActive) {
		    dummy= gotCarrier() ? 3 : 2;

/* Show the TELNET DAEMON if the SHUTTLE PORT is active */

			mPrintf("%-22s%-22sttyp%d %s", "Telnet Server", "Available",
				SHUTTLE_POINTER+1, hostDomainName[0] ? hostDomainName : "");
			doCR();

/* Show the "other vacant ports" which someday won't always be vacant! */

            do {
				mPrintf("%-22s%-22sttyp%d", "Idle","", dummy++);
				doCR();
				} while (dummy!=9);
			doCR();
			}

/* Then get the heck outta here! */

		return;
		}


	mPrintf("[Chat: O%s]\n", (cfg.BoolFlags.noChat) ? "FF" : "N");

    if (logBuf.lbflags.NET_PRIVS)
            mPrintf(" Net privs: %d LD credits\n",
                    logBuf.credit);
/*	    } */

/* #ifdef K2NE_TEST */
    mPrintf(" \n %ld messages,",              cfg.newest-cfg.oldest +1);
    mPrintf(" last is %lu,\n",             cfg.newest              );
    mPrintf(" %dK message space,\n", cfg.maxMSector / (1024 / MSG_SECT_SIZE));
    mPrintf(" %d-entry log\n",             cfg.MAXLOGTAB           );
    mPrintf(" %d room slots, %d in use\n", MAXROOMS, roomCount);
    if (cfg.oldest > 1) work = cfg.maxMSector;
	    else 	work = cfg.catSector;
    work *= MSG_SECT_SIZE;
    if (cfg.oldest > 1) average = (work) / (cfg.newest - cfg.oldest + 1);
	    else 	average = (work) / (cfg.newest);
    mPrintf(" Avg. message length: %ld\n",  average);
/* #endif */

    if (cfg.lastCaller[0])
	    mPrintf(" Last user:  %s\n", cfg.lastCaller);
    if (shortSession && loggedIn) timeToGo();
}

/************************************************************************/
/*      wildCard() Do something with the directory                      */
/************************************************************************/

int wildCard(fn, filename, needToMove, phrase)
int  (*fn)(char *str);
char *filename;         /* may be ambiguous.  No drive or user numbers. */
char needToMove;
char *phrase;           /* search file comments for this phrase         */

{
#ifndef NOTONLYTHENET
    int   fileCount, realCount;
    struct dirList  list[DIR_SIZE];

    if (needToMove)
        if (!setSpace(&roomBuf)) {
            printf("Error!\n");
            return 0;
        }

    if ((fileCount = CitGetFileList(filename, list, DIR_SIZE)) == 0) {
        /* no such file */
        if (inNet == NON_NET) mPrintf("no %s\n ", filename);
        homeSpace();
        return 0;
    }
    realCount = wild2Card(list, fileCount, fn, phrase);
    freeFileList(list, fileCount);
    if (needToMove) homeSpace();
    return realCount;
#endif
}

int wild2Card(list, fileCount, fn, phrase)
struct dirList  list[];
int             fileCount;
int             (*fn)(char *n);
char            *phrase;
{
#ifndef NOTONLYTHENET
    struct dirList   *fp;
    extern DIR_EXTRA GblOther;
    int              MatchCount = 0;

    qsort(list, fileCount, sizeof list[0], sortDir);
    outFlag     = OUTOK;
    StFileComSearch();
    for (fp = list;  fileCount-- && outFlag != OUTSKIP;  fp++) {
        if (strLen(phrase) == 0 || FindFileComment(fp->unambig)) {
            if (strLen(phrase) == 0 ||
                matchString(msgBuf.mbtext, phrase, lbyte(msgBuf.mbtext)) !=
                                                        NULL) {
                MatchCount++;
                copy_struct(fp->otherStuff, GblOther);
                (*fn)(fp->unambig);
            }
        }
    }
    EndFileComment();
    return MatchCount;
#endif
}

int sortDir(s1, s2)
struct dirList *s1, *s2;
{
    return strCmp(s1->unambig, s2->unambig);
}

void UngotoMaintain(lRoom)
int lRoom;      /* Room we just left */
{
                        /* Move stack down 1 element */
    movmem(UngotoStack, UngotoStack + sizeof(int),
                                        (UN_STACK - 1) * sizeof(int));
                        /* Add new element */
    UngotoStack[0] = lRoom;
}

#ifdef QTEST
protocolInit() /* read file PROTOTBL.SYS from homeArea if present */
{
 FILE *ctdlProtoFile;
 char noFile;
 char *currentProto;

 currentProto[30];
 noFile=FALSE;
 SYSproto=0;
 ctdlProtoFile = fopen("prototbl.sys","rt");
 if (ctdlProtoFile==NULL) {
        fclose(ctdlProtoFile);
        noFile=TRUE;
		}
 if (!noFile) {
	SYSproto=1;
    while (ctdlProtoFile!=NULL) {
		fgets(currentProto, 30, ctdlProtoFile);
    	currentProto[strlen(currentProto)-1]='\0';
        strcpy(SYSproto==1 ? sysProt1 :
               SYSproto==2 ? sysProt2 :
		       SYSproto==3 ? sysProt3 :
							 sysProt4, currentProto);
		SYSproto++;
        }

	fclose(ctdlProtoFile);
	}
 return SYSproto;
}
#endif


initVariables()
{
    cfg.weAre=CITADEL;
	noAudibleRing=FALSE;
	noNoises=FALSE;
	autoNet=FALSE;
    justOut=FALSE;
	unixStyle=FALSE;
	locFIDO=FALSE;
    Net_Monitor=FALSE;
	mailTag=FALSE;
	notDoorUser=FALSE;
	queryDenied=FALSE;
	runningAsDoor=FALSE;
	siege=FALSE;
    privateStatus=FALSE;
	justDidNet=FALSE;
	frontEnd=FALSE;
	timeInDoors=0;
	userMessages=0;
    archiveLogFlag=TRUE;
    frontEndNetFlag=FALSE;
	specialPrompt=FALSE;
    onlyAsNet=FALSE;
	AideNetTrigger=FALSE;
	shortSession=FALSE;
	sentNotice=FALSE;
	ansi=FALSE;
    sysopColors=FALSE;
    chatFlag=FALSE;
	uploadingFlag=FALSE;
	uniqueFlag=FALSE;
	jiggleFlag=FALSE;
    anchorCounter=0;
	fastOut=FALSE;
	jumpOut=FALSE;
	shortStuff=FALSE;
	tryToLogIn=20;
    InitColumns=79; /* modernization! */
    autoFIDOlink=FALSE;
	stickingAround=TRUE;
	linkerNumber=0;
    ringsToAnswer=0;
    numberOfRings=0;
    answerGuard=TRUE;
	manualNet=FALSE;
	userBioSpecial=FALSE;
    shuttleActive=FALSE;
	killScreen=FALSE;
}
