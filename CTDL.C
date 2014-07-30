
/*
 * citadel.c    Command-interpreter, Citadel:K2NE
 */

#include "ctdl.h"

/*
 * history
 *
 * 89Mar12 VAQ  K2NE history moved to K2NE INCREM.xxx &CTDL_C.LOG files.
 * 86Aug16 HAW  Kill history for file because of space problems.
 * 83Mar08 CrT  Aide-special functions installed & tested...
 * 83Feb24 CrT/SB Menus rearranged.
 * 82Dec06 CrT  2.00 release.
 * 82Nov05 CrT  removed main() from room2.c and split into sub-fn()s
 */

/*
 * Contents
 *
 * doAide()
 * doChat()
 * doEnter()
 * doForget()
 * doGoto()
 * doHelp()
 * doKnown()
 * doLogin()
 * doLogout()
 * doRead()
 * doRegular()
 * doSkip()
 * doSysop()
 * doBackupToRoom()
 * getCommand()
 * greeting()
 * main()
 */

/*
 *     External decls in CITADEL.C
 */

char ExitToMsdos=FALSE;
int  exitValue=CRASH_EXIT;
static char NoChatAtAll=FALSE;
char ARCready=FALSE, ZIPready=FALSE, LHARCready=FALSE, UNZOOready=FALSE;
char archiveLogFlag=TRUE;
char  DoTimes=FALSE;
char  *confirm="Confirm";
char  *NoDownloads="\n No downloading!\n";
char  *Qxtr=" ?(Type '?' for menu)\n ";
char  *logTrailer="ystem log\n";
char  *noSuchPerson="Not found\n ";
char  noInit=FALSE, justOut;
char  doorLogFlag=FALSE;
char  *badChoice="Not supported.\n ";
int	  fieldSuppress=TRUE; /* Net_Switch stuff */
int   CITCOLOR=0, CITSTATUS=0, jumper=0;
char  baudLOCK=FALSE; /* speeds up anytimeNet receive init */
char  profile=FALSE, sentNotice=FALSE, shortSession=FALSE, fastModem=FALSE;
int	  userMessages=0, nrCalls=0, nrPosts=0, netLogCount=0;
int   linkLayerCount=0; /* keep tabs! */
int	  notDoorUser=FALSE;
int   oldDayVal=0, queryDenied=FALSE, timeOnLine=0;
int   uniqueMsgNr=0, anchorCounter=0;
int   tryToLogIn=20, linkerNumber=0, evMode=0, howMany, timeForBlank;
char  shuttleActive, globalAllFinished, lastHowMany, blockAll, blankVariable;
char  runningAsDoor=FALSE;
char  siege=FALSE;
char  siegeResult=FALSE, privateStatus=FALSE, justDidNet=FALSE;
char  frontEnd=FALSE; /* Binkley thing */
char  chatFlag=FALSE, uniqueFlag=FALSE, jiggleFlag=FALSE;
char  frontEndNetFlag=FALSE, specialPrompt=FALSE;
char  onlyAsNet=FALSE, AideNetTrigger=FALSE;
char  unVal=FALSE, ansi=FALSE, ANSI_GRAPHICS=FALSE;
char  sysopColors=FALSE, lastFiveFlag=FALSE;
char  numberCalls[10], numberPosts[10], uploadingFlag=FALSE;
char  *messageTick="", headerFlag=FALSE, shortStuff=FALSE;
char  fastOut=FALSE, jumpOut=FALSE;
char  unixStyle=FALSE, *userEnterName="";
char  alterName[NAMESIZE], alterLinkName[NAMESIZE];
char  alterNet=FALSE, locFIDO=FALSE, autoFIDOlink=FALSE, stickingAround=FALSE;
char  shuttleStyle[10];
char  modemStartupString[100], modemResetString[100], modemIdleString[50];
char  modemAnswerString[50];
char  answerGuard, autoNet, noAudibleRing, noNoises, aideCalledThis;
char  fastModemNetPrefix[50];
struct date today;

/*
 *    Extern vars. for CITADEL.C
 */

extern CONFIG cfg;  			     /* The main variable to be saved	*/
extern aRoom     roomBuf;       	 /* Room buffer         */
extern paintBrush colTable;          /* the ANSI rainbow    */
extern MSG_BUF      msgBuf;          /* Message buffer      */
extern MSG_BUF      tempMess;        /* Message buffer      */
extern logBuffer logBuf;         	 /* Person's log buffer */
extern rTable    *roomTab;      	 /* Room index for RAM  */
extern LogTable    *logTab;     	 /* Log  index for RAM  */
extern int         thisRoom;    /* Current room             */
extern long int    pulledMLoc;  /* Loc of msg to be pulled  */
extern MSG_NUMBER  pulledMId;   /* Id of msg to be pulled   */
extern char        remoteSysop;
extern char        onConsole;   /* Where IO is ...          */
extern char        whichIO;     /* Where IO is ...          */
extern char        outFlag, human;
extern char        loggedIn;    /* Are we logged in?        */
extern char        echo;
extern char        newCarrier;      /* Just got carrier, hurrah! */
extern char        justLostCarrier;
extern char        textDownload;    /* flag */
extern char        haveCarrier;
extern char        *baseRoom;
extern char        heldMess;
extern char        anyEcho;
extern char        PrintBanner, shuttleUser;
extern int         CONTRAST;
extern int      acount;
extern int		ourBBSport, k2neLocalOnlyFlag;
#define AUDIT   9000
extern char     audit[AUDIT];
extern int		dumpDeadWood, netMessageTotal, currLine, ringsToAnswer;;
extern int      numberOfRings;
extern char     *who_str;
extern char     Net_Monitor, manualNet, specHook, shortWay, subSystemUser;
extern char     mailTag; /*, tocWide; */
extern char     *thisOne, NewVideo, pausePromptFlag;
extern char     *whatBaudRate;
extern char     leaveAfterTransfers; /*, didRing;*/

extern void    (*StopVideo)(void);


#ifndef ZIPsupport
int zipFileDir=FALSE;
#endif

/*
 *  doAide() - aide-only menu
 */

char doAide(char moreYet, char first)
{
#ifndef ONLYTHENET
    label oldName;
    int  rm;
    char chatStack=FALSE;
    int   year, day, hours, minutes;
    char  *month;
    extern char *APrivateRoom, callLogPosting[800];

    unVal=FALSE;
	if (!aide || !loggedIn) {
        if (loggedIn &&
            strCmpU(logBuf.lbname, roomBuf.rbmoderator) == SAMESTRING) {
            mPrintf("dit room\n  \n");
            renameRoom();
            return TRUE;
        }
        return FALSE;
    }
    shrtColor(colTable.level1);
    if (moreYet)   first='\0';

    mPrintf("\bAide fn: ");

    if (first)     oChar(first);

    switch (toUpper(   first ? first : iChar()    )) {
    case 'B':
	  if (onConsole || remoteSysop) {
		  mPrintf("ackup logs\n ");
		  if (!getYesNo(confirm)) break;
          backSysLog();
		  mPrintf("\n");
		  }
	  else doPassWarn();
	  break;
    case 'C':
        if (NoChatAtAll && !SomeSysop())
            tutorial("nochat.blb", TRUE);
        else {
            chatStack=cfg.BoolFlags.noChat;
            cfg.BoolFlags.noChat=FALSE;
            mPrintf("hat\n ");
            if (whichIO == MODEM)       ringSysop();
            else                        interact(TRUE) ;
            cfg.BoolFlags.noChat=chatStack;
        }
        break;
    case 'D':
#ifdef OLD
		mPrintf("elete empty rooms\n ");
		sPrintf(msgBuf.mbtext, "Empty rooms deleted [%s]: ", logBuf.lbname);
		if (!getYesNo(confirm)) break;
		strCpy(oldName, roomBuf.rbname);
		indexRooms();

		if ((rm=roomExists(oldName)) != ERROR) getRoom(rm);
		else getRoom(LOBBY);

		aideMessage(FALSE);
#endif
		mPrintf("escribe room\n ");
        editRoomInfoFile();
		break;
    case 'E':
        mPrintf("dit room\n \n");
        renameRoom();
        break;
    case 'K':
        if (thisRoom < 3) {
			mPrintf("\b?");
			break;
			}

#ifdef OLD
        mPrintf("ill room\n ");
		if (thisRoom < 3) { /* Lobby 0, Mail 1, Aide 2 */
			mPrintf(" Not here!");
            break;
        }
#endif

		mPrintf("ill room\n ");
        if (!getYesNo(confirm))
        	break;

        sPrintf(msgBuf.mbtext, "%s> killed by %s",
						roomBuf.rbname, logBuf.lbname);
        strCpy(callLogPosting, msgBuf.mbtext);

		logMessage(19,"",FALSE);
        aideMessage(FALSE);
        roomBuf.rbflags.INUSE=FALSE;
        putRoom(thisRoom);
        noteRoom();
        getRoom(LOBBY);
		ScrNewUser();
        break;
    case 'S':
        mPrintf("et Date\n ");
        changeDate();
        break;
    case 'N':
		if (onConsole || remoteSysop) {
			mPrintf("et trigger\n ");
			AideNetTrigger=TRUE;
        	sPrintf(callLogPosting,
			   "Net ordered by %s", logBuf.lbname);
			logMessage(19,"",FALSE);
			}
		else doPassWarn();
		break;
	case 'Q':
		mPrintf("uery ");
        first=toUpper(iChar());
        switch (first) {
		case 'A':
        case 'D':
		case 'S':
			mPrintf(first == 'A' ? "rchive log\n " :
					first == 'D' ? "oor log\n " : logTrailer);
			logReader(first == 'A' ? "archive.log" :
					  first == 'D' ? "doorlog.sys" : "calllog.sys");
			break;
		case 'B':
			mPrintf("ackup ");
			first=toUpper(iChar());
			switch (first) {
            case 'D':
			case 'S':
				mPrintf(first == 'D' ? "oor log\n " : logTrailer);
				logReader(first == 'D' ? "doorlog.bak" : "calllog.bak");
				break;
			case 'L':
			case 'N':
			case 'R':
				mPrintf("%s log\n ",
				  first == 'N' ? "et" :
				  first == 'L' ? "ocal FIDO" : "emote FIDO");
				speedReader(first == 'N' ? "netlog.bak" :
				  first == 'L' ? "fidocons.bak" : "fidocom1.bak");
				break;
			default:
				if (!expert)	mPrintf(Qxtr);
				else 			mPrintf(" ?\n ");
            	break;
        	}
			break;
		case 'U':
			mPrintf("ser list\n\n");
			queryUserlist(TRUE);
			break;
		case 'V':
			unVal=TRUE;
			mPrintf("alidate-list\n ");
			queryUserlist(FALSE);
			break;

#ifndef NO_FIDO_FLUFF
		case 'L':
		case 'R':
			mPrintf("%s FIDO log\n ", (first == 'L') ? "ocal" : "emote");
			speedReader((first == 'L') ? "fidocons.log" : "fidocom1.log");
			break;
#endif

        case 'N':
			mPrintf("et log\n\n");
			netReader();
			break;
		case 'T':
			human=TRUE;
			mPrintf("\otals for %s:", formDate());
			mPrintf("\n Callers: %d  Messages: %d", nrCalls, nrPosts+userMessages);
			mPrintf("\n Net Sessions: %d  Nodes Reached: %d  Msgs Forwarded: %d\n ",
                        netLogCount, linkLayerCount, netMessageTotal);
			if (shortSession) mPrintf("Session limit: %d minutes\n ", timeOnLine/60);
			human=FALSE;
			break;
		default:
			if (!expert) 	mPrintf(Qxtr);
			else 			mPrintf(" ?\n ");
            break;

		} /* switch */
		break;
    case '?':
        tutorial("aide.mnu", TRUE);
        break;
    default:
        if (!expert)    mPrintf(Qxtr);
        else            mPrintf(" ?\n ");
        break;
    }
    shrtColor(colTable.level0);
    return TRUE;
#endif
#ifdef ONLYTHENET
    return FALSE;
#endif
}

/*
 *      doChat()
 */
void doChat(char moreYet, char first)
  /* TRUE to accept following parameters  */
  /* first parameter if TRUE              */
{
#ifndef ONLYTHENET
    if (moreYet)   first='\0';

    if (first)     oChar(first);

    mPrintf("\bChat ");

    if (whichIO != MODEM) {
        interact(TRUE) ;
        return ;
    }
	if (!loggedIn) {
		mPrintf("\b? Log in!\n ");
		return;
		}
	logMessage(CHAT_FLAG, "", FALSE);
	if (CONTRAST < 128) {
		CONTRAST=CONTRAST+128;
		if (ansi) chatFlag=TRUE;
		ScrNewUser();
		}
    if (cfg.BoolFlags.noChat)   {
        tutorial("nochat.blb", TRUE);
        return;
	    }

    ringSysop();
#endif
}

/*
 * doSysop() -- remote sysop cmds
 */

char doSysop(char first)
            /* first parameter if TRUE */

{
    MSG_NUMBER  temp;
    char        systemPW[200], tmp[10];
    extern char *LCHeld, callLogPosting[800];
    label       who;
    logBuffer   lBuf;
    int         logNo=0, ltabSlot=0, i;
    SYS_FILE    killHeld;
    char confirmResult=FALSE, tempstr[480];

#ifndef ONLYTHENET
	profile=FALSE;
    if (!onConsole && !remoteSysop) {
        if (!aide || strLen(cfg.sysPassword) == 0)
            return TRUE;
        echo       =CALLER;
        getNormStr("password", systemPW, 199, NO_ECHO);
        echo       =BOTH;
        if (strCmp(systemPW, cfg.sysPassword) != 0)
            return TRUE;
        remoteSysop=TRUE;
        sPrintf(callLogPosting, "Sysop cmds accessed by %s", logBuf.lbname);
		logMessage(19,"",FALSE);
    }

    initLogBuf(&lBuf);

    while (onLine()) {

        outFlag=OUTOK;
		doCR();
        mPrintf("%s% ", cfg.codeBuf+cfg.nodeName);

        switch (toUpper(first ? first : iChar())) {
        case 'W':
			evMode=1;
            EventShow();        /* Debug stuff */
            break;
#ifdef MSG_FINDER
        case 'Y':
            mPrintf("\n ");
            for (i=0; i < MSGSPERRM; i++)
                mPrintf("%ld: %d\n ", roomBuf.msg[i].rbmsgNo,
                                      roomBuf.msg[i].rbmsgLoc);
            break;
#endif
#ifdef DEBUG_MSGS
        case 'U':
            mPrintf("msg#: ");
            gets(msgBuf.mbtext);
            temp=atol(msgBuf.mbtext);
            mPrintf("loc: ");
            gets(msgBuf.mbtext);
            i=atoi(msgBuf.mbtext);
            note2Message(temp, i);
            putRoom(thisRoom);
            noteRoom();
            break;
        case 'Z':
            mPeek(); break;
#endif
        case 'F':
            mPrintf("\bFile grab\n ");
            ingestFile(NULL);
            break;
        case 'A':
            mPrintf("\bAbort\n ");
            killLogBuf(&lBuf);
			if (!gotCarrier() && !loggedIn) doTotals();
            return FALSE;
#ifndef BRIAN
        case 'C':
            mPrintf("\bChat mode %sabled\n ",
                (cfg.BoolFlags.noChat=!cfg.BoolFlags.noChat)
                ? "dis" : "en");
            ScrNewUser();
            break;
#ifndef DEVELOP_VERSION
		case 'U':
            profile=TRUE;
		case 'V':
			mPrintf(profile ? "ser profile\n " : "\bValidation\n ");
			if (!getXString(who_str, who, NAMESIZE, NULL, NULL)) break;
			logNo=findPartPerson(who, &lBuf);
			if (logNo == ERROR) {
            	mPrintf(noSuchPerson);
                break;
				}
	if (profile==FALSE) {
			if (loggedIn  &&  strCmpU(logBuf.lbname, thisOne /* who */)==SAMESTRING) {
				mPrintf("\n Illegal!\n ");
				break;
				}
            mPrintf("%s gets validated\n", thisOne /* who */);
			confirmResult=getYesNo(confirm);
			confirmResult=!confirmResult;
			lBuf.lbflags.lflag7=confirmResult;
			if (lBuf.lbflags.lflag7==1) { /* 1 if UNvalidated user! */
    			putLog(&lBuf, logNo);
				break;
				}

			mPrintf("%s gets D/load privs.\n",
							thisOne); /* TRUE if NO d/l privs */
			confirmResult=getYesNo(confirm);
			confirmResult=!confirmResult;
			lBuf.lbflags.lflag6=confirmResult;

			mPrintf("%s gets Door privs.\n",
							thisOne);     /* TRUE if NO door privs */
			confirmResult=getYesNo(confirm);
			confirmResult=!confirmResult;
			lBuf.lbflags.lflag2=confirmResult;

			mPrintf("%s gets Net privs.\n",
							thisOne);     /* TRUE if HAS net privs */
			confirmResult=getYesNo(confirm);
			lBuf.lbflags.NET_PRIVS=confirmResult;

            mPrintf("%s gets Linked Net privs.\n", thisOne /* who */);
			confirmResult=getYesNo(confirm);
			confirmResult=!confirmResult; /* TRUE if NO altnet privs */
			lBuf.lbflags.RUGGIE=confirmResult;

			mPrintf("%s gets '*' access.\n", thisOne);
			confirmResult=getYesNo(confirm);
            lBuf.lbflags.SUBSYSTEM_OK=confirmResult;

		}

            if (profile) {
				aideCalledThis=TRUE;
				showUserProfile(logNo);
				aideCalledThis=FALSE;
				}
			mPrintf(" User: %s\n   %sD/loads, %sDoors, %sNet, %sLinked-net, %s'*' privs.%s",
					 thisOne,
					 (lBuf.lbflags.lflag6==TRUE) ? "NO " : "",
					 (lBuf.lbflags.lflag2==TRUE) ? "NO " : "",
                     (lBuf.lbflags.NET_PRIVS==TRUE) ? "" : "NO ",
					 (lBuf.lbflags.RUGGIE==TRUE) ? "NO " : "",
					 (lBuf.lbflags.SUBSYSTEM_OK==TRUE) ? "" : "NO ",
					 profile ? "\n" : "");


			if (profile==FALSE) {
				if (getYesNo(confirm))
	           		putLog(&lBuf, logNo);
				}
			profile=FALSE;
			break;

#endif
#endif
#ifdef DEVELOP_VERSION
        case 'D':
            cfg.BoolFlags.debug=!cfg.BoolFlags.debug;
            mPrintf("\bDebug switch=%d\n \n", cfg.BoolFlags.debug);
            break;
#endif


        case 'K':
            mPrintf("\bKill user\n ");
            if (!getXString(who_str, who, NAMESIZE, NULL, NULL)) break;
            logNo  =findPerson(who, &lBuf);
            if (logNo == ERROR)   {
                mPrintf(noSuchPerson);
                break;
	            }
            if (lBuf.credit != 0)
                mPrintf("%s has %d credit for LD!", who);
            if (!getYesNo(confirm))   break;
            mPrintf("%s deleted\n ", who);
			if (onLine() && loggedIn) {
		        sPrintf(callLogPosting, "User [%s] killed by %s",
						who, logBuf.lbname);
      			logMessage(19,"",FALSE);
				}
            ltabSlot=PWSlot(lBuf.lbpw, FALSE);
            lBuf.lbname[0]='\0';
            lBuf.lbpw[0  ]='\0';
            lBuf.lbflags.L_INUSE=FALSE;

            putLog(&lBuf, logNo);

            logTab[ltabSlot].ltpwhash=0;
            logTab[ltabSlot].ltnmhash=0;

            homeSpace();
            sprintf(tempstr, "user%d.cit", logNo);
			unlink(tempstr);

            if (cfg.BoolFlags.HoldOnLost) {
                sPrintf(tmp, LCHeld, logNo);
                makeSysName(killHeld, tmp, &cfg.holdArea);
                unlink(killHeld);
    	        }

            break;
        case 'M':
            mPrintf("\bSystem now on MODEM\n ");
            if (whichIO != MODEM) {
                whichIO=MODEM;
                setUp(FALSE);
   		        }
            mPrintf("Chat %sabled\n ",
                 cfg.BoolFlags.noChat ? "dis" : "en");
            if (!gotCarrier()) {
				doTotals();
				EnableModem();
				}
#ifdef NEED_VISIBLE
            if (visibleMode) mPrintf("Visible mode on\n ");
#endif
            killLogBuf(&lBuf);
            ScrNewUser();
            startTimer(1); /* start anytimeNet timer */
            return FALSE;
        case 'O':
            mPrintf("\bOther Cmds.\n ");
            systemCommands();
            break;
        case 'P':
            mPrintf("\bAide status toggle\n ");
            getNormStr(who_str, who, NAMESIZE, ECHO);
            logNo  =findPerson(who, &lBuf);
            if (logNo == ERROR)   {
                mPrintf(noSuchPerson);
                break;
	            }

            if (lBuf.lbflags.AIDE == 1) {
                lBuf.lbflags.AIDE=0;
                lBuf.lbgen[AIDEROOM]=((roomTab[AIDEROOM].rtgen-1) % MAXGEN)
                                                                << GENSHIFT;
    	        }
            else {
                lBuf.lbflags.AIDE=1;
                lBuf.lbgen[AIDEROOM]=roomTab[AIDEROOM].rtgen << GENSHIFT;
                lBuf.lbgen[AIDEROOM] += MAXVISIT-1;
        	    }
            mPrintf("%s %ss aide status\n ",
                who, (lBuf.lbflags.AIDE == 1) ? "get" : "lose");
            if (!getYesNo(confirm)) break;

            putLog(&lBuf, logNo);

            if (loggedIn  &&  strCmpU(logBuf.lbname, who)==SAMESTRING) {
                aide=(lBuf.lbflags.AIDE == 1) ? TRUE : FALSE;
                logBuf.lbgen[AIDEROOM]=lBuf.lbgen[AIDEROOM];
	            }
            break;

	    case 'S':
    	    mPrintf("\bSet Date\n ");
        	changeDate();
	        break;

        case 'X':
            mPrintf("\bExit to DOS\n \n");
            if (!getYesNo(confirm))   break;
            ExitToMsdos=TRUE;
            exitValue  =(remoteSysop) ? REMOTE_SYSOP_EXIT : SYSOP_EXIT;
            return FALSE;
        case 'N':
            netStuff();
            break;
        case '?':
			if (!gotCarrier()) {
				doConsoleHelp("operator.blb");
				break;
				}
            tutorial("ctdlopt.mnu", TRUE);
            break;
        default:
            if (!expert)    mPrintf(Qxtr);
            break;
        }
    }
    killLogBuf(&lBuf);
	alterNet=FALSE;
    return ERROR;
#endif
#ifdef ONLYTHENET
	return TRUE;
#endif
}


/*
 *  doEnter()  E(nter) command
 */
void doEnter(char moreYet, char first)
	/* TRUE to accept following parameters  */
	/* first parameter if TRUE              */
{

#define CONFIGURATION   0
#define MESSAGE         1
#define PASSWORD        2
#define ROOM            3
#define ENTERFILE       4
#define CONTINUED       5
#define NETWORK         6

#define NOPE            0
#define GIVEMENU        1
#define NOMENU          2
    char what;
    char abort, done, WC;
    char letter;

	echo=BOTH;
    if (moreYet)   first='\0';

    abort      =NOPE   ;
    done       =FALSE  ;
    WC         =ASCII  ;
    what       =MESSAGE;
	k2neLocalOnlyFlag=FALSE;

    mPrintf("\bEnter ");

    if (thisRoom != MAILROOM && !loggedIn &&
                                    !cfg.BoolFlags.unlogEnterOk) {
        mPrintf("Only SYSOP MAIL until logged in\n ");
        return;
    }

    if (roomBuf.rbflags.READ_ONLY) {
	    if (!onConsole && !remoteSysop) {
			mPrintf("\n READ-ONLY!");
			return;
			}
        }

    if (first)     oChar(first);

    do  {
        outFlag=OUTOK;

        letter=(toUpper(first ? first : iChar()));
        switch (letter) {
        case '\r':
        case '\n':
            moreYet    =FALSE;
            break;

#ifdef DO_KERMIT
		case 'K':
			mPrintf("ermit ");
			WC=6;
			break;
#endif
		case 'W':
        case 'X':
        case 'Y':
		case 'Z':
		case 'J':
            mPrintf("%smodem protocol ", letter == 'W' ? "indowed-X" : "");
			if      (letter == 'Y') WC=YMDM;
			else if (letter == 'W') WC=3;
			else if (letter == 'X') WC=XMDM;
			else if (letter == 'Z') WC=4;
			else if (letter == 'J') WC=5;
            break;
        case 'F':
            if (roomBuf.rbflags.ISDIR == 1) {
                if (WC == ASCII) {
                    mPrintf("\bXmodem F");
                    WC=XMDM;
                }
                mPrintf("ile upload ");
                if ((abort=((!loggedIn) ? NOMENU : NOPE)) == NOMENU)
                    mPrintf("\n Log in!\n ");
                else if ((abort=((!roomBuf.rbflags.UPLOAD)
                                                 ? NOMENU : NOPE)) == NOMENU)
                 mPrintf(" ?\n ");
                what   =ENTERFILE;
                done   =TRUE;
                break;
            }
        default:
            abort=GIVEMENU;
            break;
        case 'C':
            if (WC != ASCII)
                abort=GIVEMENU;
            else {
                mPrintf("\bConfiguration ");
                reconfigure();
                what=CONFIGURATION;
                done=TRUE;
            }
            break;
		case 'L':
			mPrintf("\bLocal-Only Message ");
			k2neLocalOnlyFlag=TRUE;
			what=MESSAGE;
			done=TRUE;
			break;
        case 'M':
            mPrintf("\bMessage "      );
            what=MESSAGE;
            done=TRUE;
            break;
        case 'P':
            if (WC != ASCII)
                abort=GIVEMENU;
            else {
                mPrintf("\bPassword "     );
                what=PASSWORD;
                done=TRUE;
            }
            break;
        case 'R':
            if (WC != ASCII)
                abort=GIVEMENU;
            else {
                mPrintf("\bRoom ");
                if (!cfg.BoolFlags.nonAideRoomOk && !aide)   {
                    mPrintf("\n You can't do that!\n ");
                    abort=NOMENU;
                    break;
                }
                if (!loggedIn) {
                    mPrintf(" Log in!\n");
                    abort=NOMENU;
                    break;
                }
                what=ROOM;
                done=TRUE;
            }
            break;
        case 'H':
            if (WC != ASCII)
                abort=GIVEMENU;
            else {
                mPrintf("\bHeld Msg.");
                what=CONTINUED;
                done=TRUE;
            }
            break;
        case 'N':
            mPrintf("\bNet-Message ");
			k2neLocalOnlyFlag=FALSE;
            what=NETWORK;
            done=TRUE;
            break;
        }
        first='\0';
        if (abort != NOPE) {
            mPrintf("? ");
            if (abort == GIVEMENU && (!expert || letter == '?'))
                tutorial("entopt.mnu", TRUE);
        }
    } while (!done && moreYet && abort == NOPE);

    doCR();

    if (abort == NOPE) {
        if (whichIO != CONSOLE &&
            (thisRoom == MAILROOM || roomTab[thisRoom].rtflags.ANON) &&
            (cfg.SeeMail != TRUE))
            echo=CALLER;
        if (loggedIn && roomBuf.rbflags.SHARED &&
                roomBuf.rbflags.AUTO_NET &&
                (roomBuf.rbflags.ALL_NET || logBuf.lbflags.NET_PRIVS)
                && what == MESSAGE && !k2neLocalOnlyFlag)
            what=NETWORK;
		if (thisRoom==MAILROOM && logBuf.lbflags.NET_PRIVS==FALSE)
			what=MESSAGE;

        switch (what) {
        case MESSAGE  : makeMessage(WC);  break;
        case PASSWORD : newPW()        ;  break;
        case ROOM     : makeRoom(); ScrNewUser() ;  break;
        case ENTERFILE: upLoad(WC)     ;  break;
        case CONTINUED: hldMessage()   ;  break;
        case NETWORK  : netMessage(WC) ;  break;
        }
        echo=BOTH;
    }
}

/*
 *  doForget()  -  (Forget room) command
 */
void doForget(char expand)
{
#ifndef ONLYTHENET
    int i=0;

    if (!expand) {
        mPrintf("ap (forget) %s \n ", roomBuf.rbname);
		if (thisRoom < 3) { /* not lobby, mail or aide */
            mPrintf("Can't zap \"%s\"\n ", roomBuf.rbname);
            return;
        }
        if (!getYesNo(confirm))   return;
        i=(roomBuf.rbgen + FORGET_OFFSET) % MAXGEN;   /* Set up offset*/
        logBuf.lbgen[thisRoom]=i << GENSHIFT;         /* Save it      */
        gotoRoom(cfg.codeBuf+cfg.bRoom, 'S');
    }
    else {
        mPrintf("\b\b ");
        listRooms(FORGOTTEN);
    }
#endif
}

/*
 *  doGoto()  -  G(oto) command
 */
void doGoto(char expand)
	/* TRUE to accept following parameters  */
{
#ifndef ONLYTHENET
    label roomName;

    outFlag=IMPERVIOUS;
    mPrintf("\bGoto ");

    if (!expand) {
        gotoRoom("", 'R');
        return;
    }

    mPrintf("room: ");
    getNormStr("", roomName, NAMESIZE, ECHO);

    if (roomName[0] == '?') listRooms(NOT_INTRO);
    else gotoRoom(roomName, 'R');
#endif
}

/*
 *  doHelp()  H(elp) command
 */
void doHelp(char expand)
	/* TRUE to accept following parameters  */
{
#ifndef ONLYTHENET
    label fileName;

	shrtColor(colTable.level1);
    mPrintf("\bHelp ");
    if (!expand) {
        mPrintf("\n\n");
        tutorial("dohelp.hlp", TRUE);
        return;
    }

    getNormStr("", fileName, 9, ECHO);

    if (strLen(fileName) == 0)
        strCpy(fileName, "dohelp");

    if (fileName[0] == '?') {
		shrtColor(colTable.level2);
        readHelpFileList();
		}

    else {
        strCat(fileName, ".hlp"); /* only allow *.HLP files */
        tutorial(fileName, TRUE);
    }
#endif
}

/*
 * doKnown()  K(nown rooms) command.
 */
void doKnown(char expand)
	/* TRUE to accept following parameters  */
{
#ifndef ONLYTHENET
  char letter='\0';

    mPrintf("\bKnown ");
    if (!expand) {
        mPrintf("rooms \n ");
        listRooms(NOT_INTRO);
    }
    else
	{
		letter=(toUpper(iChar()));
        switch (letter) {
		   case 'A':
		   case 'C':
		   case 'D':
           case 'E':
		   case 'G':
		   case 'N':
		   case 'P':
		   case 'F':
           case 'Z':
		   case 'H': shrtColor((letter=='D' || letter=='N') ? colTable.level2
					  : (letter=='Z' || letter=='F') ? colTable.level3
					  : colTable.level1);
					 mPrintf("\b%s rooms ", letter == 'D' ? "Directory"
					 : letter == 'C' ? "Cross-linked network"
					 : letter == 'E' ? "External-net"
					 : letter == 'F' ? "Forgotten"
					 : letter == 'G' ? "Gateway"
					 : letter == 'Z' ? "Zapped"
                     : letter == 'N' ? "Networked"
                     : letter == 'A' ? "Anonymous"
					 : letter == 'P' ? "Private"
					 : "Hidden");
					 listRooms(letter=='N' ? 8 :
							letter=='G' ? 11 :
							letter=='C' ? 10 :
							letter=='E' ? 9 :
							letter=='Z' || letter=='F' ? FORGOTTEN :
							letter=='D' ? 5 :
							letter=='A' ? 7 : 6);
					 break;
		   case 'R': mPrintf("\bRooms\n Search string: ");
					 searchRooms();
					 break;
           case '?': mPrintf("\n ");
			         tutorial("knownopt.mnu", TRUE);
					 break;
         }
    }
 shrtColor(colTable.level0);
#endif
}

/*
 * doLogin()  L(ogin) command
 */
void doLogin(char moreYet)
	/* TRUE to accept following parameters  */
{
#ifndef ONLYTHENET
    label passWord;

    if (loggedIn) {
		if (thisRoom!=MAILROOM && !moreYet) lastFiveFlag=TRUE;
        else mPrintf(" ?\n ");
        return;
	    }

    mPrintf("\bLogin ");
    if (!moreYet)   mPrintf("\n");

    echo       =CALLER;
    getNormStr(moreYet ? "" : " password (just carriage return if new)",
                              passWord, NAMESIZE, (moreYet) ? ECHO : NO_ECHO);
    echo       =BOTH;
    login(passWord);
#endif
}

/*
 * doLogout() T(erminate) command
 */
extern char roomLevelFlag, versionTag[20], *VERTAG;

void doLogout(char expand, char first)
	/* TRUE to accept following parameters  */
	/* TRUE if first parameter */
{
#ifndef ONLYTHENET
char quitFlag=FALSE;

    if (expand)   first='\0';

    mPrintf("\bTerminate ");

    if (heldMess && !cfg.BoolFlags.HoldOnLost)
     mPrintf("\n * Check your Hold Buffer!\n ");

    if (first)   oChar(first);

    switch (toUpper(    first ? first : iChar()    )) {
    case '?':
        tutorial("logout.mnu", TRUE);
        break;
    case 'Q':
		mPrintf("\bQuit-also\n ");
		quitFlag=TRUE;
    case 'Y':
        if (!quitFlag)  mPrintf("\bYes\n ");
		quitFlag=FALSE;
        if (!expand) {
            if (!getYesNo(confirm))   break;
	        }
	   	logMessage(2,"",FALSE);   /* normal logout */
        if (!onLine()) {
			/* fakeBanner(); */
			break;
			}
		fakeBanner();
		if (runningAsDoor || frontEnd) {
			unlink("ctdltabl.sys");
			specialExit();
			}
        terminate(TRUE, TRUE);
        break;
	case 'S':
        mPrintf("\bStay\n ");
		logMessage(2,"",FALSE);
        logMessage(18,"",FALSE);  /* continued */
        terminate(FALSE, TRUE);
        stickingAround=TRUE;
        break;
    case 'A':
        mPrintf("\bAbort\n ");
		fakeBanner();
	   	logMessage(2,"",FALSE);   /* logout */
		if (runningAsDoor) {
			unlink("ctdltabl.sys");
			specialExit();
			}
        terminate(TRUE, FALSE);
    }
 if (specHook==TRUE) {
	specHook=FALSE;
	doFixStatus();
	}
#endif
}

/*
 * doRead()  R(ead) command
 */

char notThisUser=FALSE; /* for ".read [user]" */

void doRead(char moreYet, char first)
	/* TRUE to accept following parameters  */
	/* first parameter if TRUE              */
{
#ifndef ONLYTHENET
#define PHRASE_SIZE     50
    char abort=FALSE,
         extDir=FALSE,
         doDir=FALSE,
         done =FALSE,
         hostFile=FALSE,
         whichMess=NEWoNLY,
         revOrder=FALSE,
         status=FALSE,
         allOrLocal=ALL_MESSAGES,
         SrchUser=FALSE,
         phrase[PHRASE_SIZE], srchPhrase=FALSE,
         protocol=ASCII,
		 ReadZipfile=FALSE,
         ReadArchive=FALSE;
    char letter='\0', zeta='\0';
    char fileName[100], skipNoteLine[80];
    label UserName;
    extern int byteRate;
    extern FILE* upfd;
    extern char journalMessage, FormatFlag;
#ifdef BRIAN
    extern int outPut;
#else
    extern int outPut, tradeWars;
#endif
    if (moreYet)   first='\0';

    if (!blockAll) mPrintf(headerFlag==TRUE ? "\bQuikHeaders " : "\bRead ");

    if (thisRoom == MAILROOM && !loggedIn  &&
                                  !cfg.BoolFlags.unlogReadOk)   {
        showMessages(whichMess, revOrder, ALL_MESSAGES, "", "", "");
        return;
    }

    if (!loggedIn  &&  !cfg.BoolFlags.unlogReadOk && thisRoom != LOBBY)   {
        mPrintf("Log in!\n ");
        return; /* users can ALWAYS read in LOBBY */
    }

    if (first)     oChar(first);

    do {
        outFlag=OUTOK;
        done=TRUE;

        letter=toUpper(first ? first : (zeta=iChar())=='Z' ? '!' : zeta);

        switch (letter) {
        case '\n':
        case '\r':
            moreYet    =FALSE;
            break;
        case 'A':
            if (hostFile) break;
            if (roomBuf.rbflags.ISDIR && loggedIn)   {
                mPrintf("\bArchive-");
                if (roomBuf.rbflags.DOWNLOAD || SomeSysop()) {
                    letter=toUpper(iChar());
                    switch (letter) {
                    case 'T':
                    case 'B':
                    case 'F':
                        mPrintf("\bFile(s)\n ");
                        if (!ARCready) {
                            mPrintf(badChoice);
                            abort=TRUE;
	                        }
                        ReadArchive=2;
                        break;
                    default:
                        mPrintf(" ? \n ");
                        abort=TRUE;
                    }
                    done=TRUE;
                    break;
                }
                else {
                    mPrintf(NoDownloads);
                    abort=TRUE;
                    break;
                }
            }
            goto common_error;
		case '+':
            if (!hostFile) {
				mPrintf("\blast 5 msgs\n ");
				revOrder=FALSE;
				whichMess= OLDaNDnEW;
				}
			break;
		case '#':
			if (!hostFile) {
				mPrintf("\b");
				revOrder=FALSE;
				whichMess=OLDaNDnEW;
				}
			break;
        case 'F':
            if (!hostFile) {
                mPrintf("\bForward ");
                revOrder =FALSE;
                whichMess=OLDaNDnEW;
            }
            else {
                mPrintf("\bFormatted");
                FormatFlag=TRUE;
            }
            break;
        case 'G':
            if (hostFile) break;
            mPrintf("\bGlobal new-messages ");
            whichMess=GLOBALnEW;
            break;

        case 'L':
            if (cfg.BoolFlags.netParticipant) {
                mPrintf("\bLocal-only ");
                done       =FALSE;
                allOrLocal =LOCAL_ONLY;
            }
            else {
                mPrintf("? ");
                abort=TRUE;
            }
            break;
        case 'N':
            if (hostFile) break;
            mPrintf("\bNew ");
            whichMess=NEWoNLY;
            break;
        case 'O':
            if (hostFile) break;
            mPrintf("\bOld Reverse ");
            revOrder =TRUE;
            whichMess=OLDoNLY;
            break;
        case 'R':
            if (hostFile) break;
            mPrintf("\bReverse ");
            revOrder =TRUE;
            whichMess=OLDaNDnEW;
            break;
        case 'S':
            if (hostFile) break;
            mPrintf("\bStatus\n ");
            status=TRUE;
            break;
		case 'W':
		case 'J':
        case 'X':
        case 'Y':
		case 'Z':
            if (hostFile) break;
            mPrintf("%sModem ", letter == 'W' ? "indowed-X" : "");
			if      (letter == 'X') protocol=XMDM;
			else if (letter == 'W') protocol=3; /* windowed xmodem var */
			else if (letter == 'Y') protocol=YMDM;
			else if (letter == 'Z') protocol=4; /* temp var for Zmodem */
			else if (letter == 'J') protocol=5; /* temp var for Jmodem */
            done=FALSE;
            break;
        case 'B':
            if (hostFile) break;
            if (!loggedIn || logBuf.lbflags.lflag6==TRUE) {
				mPrintf("\n Not allowed!\n ");
				abort=TRUE;
				break;
				}
            if (roomBuf.rbflags.ISDIR == 1 && loggedIn)   {
                mPrintf("\bBinary file%s ", protocol > 1 ? "s" : "");
                if (roomBuf.rbflags.DOWNLOAD == 1 || TheSysop() ||
                                                          remoteSysop) {
                    hostFile    =TRUE ;
                    textDownload=FALSE;
                    break;
                }
                else {
                    mPrintf(NoDownloads);
                    abort=TRUE;
                    break;
                }
            }
        case 'E':
        case 'D':
            if (hostFile) break;
            if (roomBuf.rbflags.ISDIR == 1)   {
                mPrintf((letter == 'D') ? "irectory " : "xtended-directory ");
                if (roomBuf.rbflags.DOWNLOAD || TheSysop() || remoteSysop) {
                    if (letter == 'D') doDir  =TRUE;
                    else               extDir =TRUE;
                    break;
                }
                else {
                    mPrintf(NoDownloads);
                    abort=TRUE;
                    break;
                }
            }
            goto common_error; /* Yuck */
        case 'T':
            if (hostFile) break;
            if (roomBuf.rbflags.ISDIR == 1 && loggedIn)   {
                mPrintf("\bTextfile(s) ");
                if (roomBuf.rbflags.DOWNLOAD || TheSysop() || remoteSysop) {
                    hostFile    =TRUE;
                    pausePromptFlag=TRUE;
                    textDownload=TRUE;
					currLine=0;
                    if (protocol == ASCII) done  =FALSE;
                    break;
                }
                else {
                    mPrintf(NoDownloads);
                    abort=TRUE;
                    break;
                }
            }
            goto common_error; /* Yuck */
        case 'I':
            if (SomeSysop()) {
                mPrintf("\bInvited-users\n ");
                doInviteDisplay();
                return ;
            }
            break;
        case 'P':
            mPrintf("\bPhrase ");
            srchPhrase=TRUE;
            done=FALSE;
            break;
        case 'U':
            mPrintf("\bUser ");
            SrchUser=TRUE;
            done=FALSE;
            break;
		case 'H':
			if (hostFile) break;
			if (roomBuf.rbflags.ISDIR && loggedIn) {
				mPrintf("\bLHARC file ");
				if (roomBuf.rbflags.DOWNLOAD || SomeSysop()) {
                    letter=toUpper(iChar());
                    switch (letter) {
/*
 * Consult INCREM.007 and/or INCREM.008 for proper placement
 * and usage for TOC_READ.COM which MUST be distributed with this
 * code!  (VAQ)
 */
	                    case 'T':
    	                case 'B':
        	            case 'F':
            	            mPrintf("\bFile(s)\n ");
	                        if (!LHARCready) {
                            mPrintf(badChoice);
                            abort=TRUE;
                        }
                        ReadZipfile=3;
                        break;
                    default:
                        mPrintf(" ? \n ");
                        abort=TRUE;
                    }
                    done=TRUE;
                    break;
                }
                else {
                    mPrintf(NoDownloads);
                    abort=TRUE;
                    break;
                }
            }
			goto common_error;
		case 'C':
			if (hostFile) break;
			if (roomBuf.rbflags.ISDIR && loggedIn) {
				mPrintf("\bCompressed/ZOO file ");
				if (roomBuf.rbflags.DOWNLOAD || SomeSysop()) {
                    letter=toUpper(iChar());
                    switch (letter) {

	                    case 'T':
    	                case 'B':
        	            case 'F':
            	            mPrintf("\bFile(s)\n ");
	                        if (!LHARCready) {
                            mPrintf(badChoice);
                            abort=TRUE;
                        }
                        ReadZipfile=4;
                        break;
                    default:
                        mPrintf(" ? \n ");
                        abort=TRUE;
                    }
                    done=TRUE;
                    break;
                }
                else {
                    mPrintf(NoDownloads);
                    abort=TRUE;
                    break;
                }
            }
			goto common_error;
		case '!': /* accessed by Uppercase "Z" */
			if (hostFile) break;
            if (roomBuf.rbflags.ISDIR && loggedIn)   {
                mPrintf("\bZIPfile-");
                if (roomBuf.rbflags.DOWNLOAD || SomeSysop()) {
                    letter=toUpper(iChar());
                    switch (letter) {
                    case 'T':
                    case 'B':
                    case 'F':
                        mPrintf("\bFile(s)\n ");
                        if (!ZIPready) {
                            mPrintf(badChoice);
                            abort=TRUE;
                        }
                        ReadZipfile=2;
                        break;
                    default:
                        mPrintf(" ? \n ");
                        abort=TRUE;
                    }
                    done=TRUE;
                    break;
                }
                else {
                    mPrintf(NoDownloads);
                    abort=TRUE;
                    break;
                }
            }
			goto common_error;
        default:
common_error:
            mPrintf("? ");
            abort      =TRUE;
            setUp(FALSE);
            if (expert)   break;
        case '?':
            tutorial("readopt.mnu", TRUE);
            abort      =TRUE;
            break;
        }
        first='\0';
    } while (!done && moreYet && !abort);

    if (abort) return;

    if (status) {
        systat();
        return;
    }


    if (ReadArchive) {

        if (ReadArchive == 1) {
	        getNormStr("ARCfile filename", fileName, 99, ECHO);
    	    if (strLen(fileName) == 0) {
				mPrintf("Bad response!\n ");
				return;
				}
       		if (!setSpace(&roomBuf)) { /* always move to roomDir */
	            printf("ERROR!\n");
    	        return;
		        }
			doZipDir(fileName,1); /* 1 for ARC, 0 for ZIP */
			homeSpace();
        	return;
			}

        else if (ReadArchive == 2)
            SendCompressedFiles(protocol, 1); /* '1' indicates ARC file */
        return;
    }

	if (ReadZipfile) {
        if (ReadZipfile == 1) {
#ifndef IFL
	        getNormStr("ZIPfile filename", fileName, 99, ECHO);
    	    if (strLen(fileName) == 0) {
				mPrintf("Bad response!\n ");
				return;
				}
       		if (!setSpace(&roomBuf)) { /* always move to roomDir */
	            printf("ERROR!\n");
    	        return;
		        }
			doZipDir(fileName,0); /* 1 for ARC, 0 for ZIP */
			zipFileDir=FALSE;
			homeSpace();
        	return;
#endif
			}
		else if (ReadZipfile>1 && ReadZipfile<5)
			SendCompressedFiles(protocol, ReadZipfile==2 ? 0 : ReadZipfile-1);


        return;
    	}

    if (doDir || extDir) {
        getNormStr("", fileName, 99, ECHO);

        if (strLen(fileName) == 0) strCpy(fileName, "*.*");
        if (srchPhrase)
            getString("search phrase", phrase, PHRASE_SIZE, TRUE, TRUE);
        else phrase[0]=0;

        doDirectory(doDir, fileName, phrase);
        if (journalMessage) {
            if (redirect(FALSE)) {
                doDirectory(doDir, fileName, phrase);
                fclose(upfd);
                outPut=NORMAL;
            }
            journalMessage=FALSE;
        }
        return;
    }


    if (hostFile) {
        if (srchPhrase)
            getString("search phrase", phrase, PHRASE_SIZE, TRUE, TRUE);
        else phrase[0]=0;
        TranFiles(protocol, phrase);
        return;
    }

    if (moreYet) {
        getString("", fileName, NAMESIZE, TRUE, TRUE);
        if (fileName[0] == '?') {
            tutorial("readdate.blb", TRUE);
            return ;
        }
    }
    else {
        doCR();
        fileName[0]=0;
    }

    if (SrchUser) {
        getNormStr("user", UserName, NAMESIZE, ECHO);

        if (strlen(UserName) > 0) {
    		sprintf(skipNoteLine, "Skip messages by \"%s\"", UserName);
			if (getYesNo(skipNoteLine))
				notThisUser=TRUE;
			else notThisUser=FALSE;
			}
    }
    else UserName[0]=0;

    if (srchPhrase)
        getString("search phrase", phrase, PHRASE_SIZE, TRUE, TRUE);
    else phrase[0]=0;

    download(whichMess, revOrder, protocol, allOrLocal, fileName, UserName,
                                phrase);
#endif
}

/*
 *   doDirectory()  -  the read directory commands
 */
void doDirectory(char doDir, char *fileName, char *phrase)
/* char doDir;
char *fileName;
char *phrase; */
{
#ifndef ONLYTHENET
    extern long BDSizeCount;
    BDSizeCount    =0; /* global fDir() totals bytes in   */
    shrtColor(colTable.level2);
    wildCard(doDir ? fDir : ShowVerbose, fileName, TRUE, phrase);
    mPrintf("\n %ld bytes total.\n ", BDSizeCount);
    shrtColor(colTable.level1);
    giveSpaceLeft(&roomBuf);
	shrtColor(colTable.level0);
#endif
}

#define MAX_USER_ERRORS 25
/*
 *   doRegular()
 */
char doRegular(char x, char c)
/* char x, c; */
{
    static int errorCount=0;
    char       toReturn;

	extern char hostFile, letter, textDownload;

	pausePromptFlag=FALSE;
	headerFlag=FALSE;
    toReturn=FALSE;
	shrtColor(colTable.level0);

    if (strchr("BCDEFGHKLMNORSTUVW", toUpper(c)) != NULL) errorCount=0;
    else errorCount++;
    if (logBuf.lbflags.lflag7==TRUE) {
		outFlag=IMPERVIOUS;
		if (!HelpIfPresent("notyet.blb"))
			mPrintf("\n VALIDATION still pending.\n ");
		outFlag=OUTOK;
		runHangup();
		}

    switch (c) {
#ifndef ONLYTHENET
    case '*': if (!logBuf.lbflags.SUBSYSTEM_OK
						|| shuttleActive==FALSE || !loggedIn) break;
			  else {
			  	mPrintf("\bSubSystem");
                shuttleUser=TRUE;
	            netStuff();
				shuttleUser=FALSE;
				}
			  break;
	case '@': shortWay=TRUE; mPrintf("\b "); doCR();

			  mPrintf("%-22s%-22s%s", "NAME", "READING/DOING", "TTY   HOST"); doCR();

			  systat(); shortWay=FALSE; break;
    case 'B': mPrintf("\bBack-up to ");
			  doBackupToRoom(x); break;
    case 'C': doChat(  x, '\0');  break;
	case 'D':
		if (x) doDoors();
		else {
			jumpToXmodem();
			if (leaveAfterTransfers==TRUE) {
				leaveAfterTransfers=FALSE;
				doCR();
			    mPrintf(" ");
				logMessage(2,"",FALSE);
				fakeBanner();
		        terminate(TRUE, TRUE);
				break;
				}
			}
		break;
    case 'E': doEnter( x, 'm' ); break;
    case 'F':
/*		if (x) {
			mPrintf("\bFind systems");
			doCR();
			doFindSystem();
			}
		else*/
		 doRead(x, 'f');
		break;
    case 'G':
		doGoto(  x);
		if ( x==FALSE && globalAllFinished) {
			if (!HelpIfPresent(expert ? "alldonex.blb" : "alldone.blb"))
				mPrintf("\n  All rooms checked; no new messages.");
			if (getYesNo("  Do you want to log off")) {
                mPrintf(" ");
			   	logMessage(2,"",FALSE);   /* normal logout */
				fakeBanner();
		        terminate(TRUE, TRUE);
				break;
				}
			}
		break;
    case 'H': doHelp(  x); break;
    case 'I': mPrintf("nformation"); readRoomInfoFile(); break;
    case 'K': doKnown( x); break;
	case 'Q':
		headerFlag=TRUE;
		if (x) doRead(x, 'r');
		else doRead(x, 'f');
		headerFlag=FALSE;
		break;
    case '#': if (thisRoom==MAILROOM) break;
			  if (anchorCounter==0) break;
			  mPrintf("\bRead last X"); doCR();
			  specialPrompt=TRUE;
			  howMany=getNumber("Enter how many messages you want to read", 1l, anchorCounter);
			  specialPrompt=FALSE;
			  lastHowMany=TRUE;
              blockAll=TRUE;
			  doRead(x, '#');
			  lastHowMany=FALSE;
			  blockAll=FALSE;
			  break;
    case 'L':
		doLogin( x);
	    if (lastFiveFlag==TRUE) doRead(x,'+');
		break;
	case 'M': /* go directly to Mail room */
        if (x) {
            if (!cfg.BoolFlags.netParticipant)
				mPrintf("\bNone listed.\n ");
			else {
				mPrintf("ailable Nodes:\n ");
				shrtColor(colTable.level2);
				showNetMailNodes();
				shrtColor(colTable.level0);
				}
			}
		else {
			 mPrintf("ail\n ");
	         gotoRoom("Mail", 'R');
			 }				     break;
    case 'N': doRead(  x, 'n' ); break;
	case 'O':
		if (x) doDoors();
		else doRead(x, 'o');	 break;
#ifdef USERINFO
	case 'P':
			if (loggedIn) {
				if (x) {
					mPrintf("rofile create/change");
					doUserInfoMake();
					}
                else {
		            mPrintf("rofile a user");
#ifndef NEWUSERBIO
    	 	        doUserInfo();
#else
					aideProfile();
#endif
					}
				}
            break;
#endif
    case 'R': doRead(  x, 'r' ); break;
    case 'S': doSkip(  x);		 break;
    case 'T': doLogout(x, 'q' ); break;
    case 'U':
			if (!loggedIn) {
				mPrintf(" ?");
				break;
				}
	        else 	doUploading();	 break;
	case 'V':
		doTOCview();
		break;
	case 'W':
		if (x) {
			mPrintf(" ?");
			break;
			}
        if (loggedIn) {
			if (thisRoom!=MAILROOM) {
				dayShow();
				break;
                }
			mPrintf("\bWho ELSE is here?\n ");
			unVal=FALSE;
			queryUserlist(FALSE);
			break;
        } else {
			mPrintf("\bLogin!");
		}
		break;
    case '\'':
    case ';': toReturn=!DoFloors();   break;
    /* room and floor traversal commands (K2NE) */
    case '+': doNext();      break; /* room */
   	case '-': doLast();      break; /* room */
    case '>': doNextFloor(); break;
 	case '<': doLastFloor(); break;
#endif
    case 0:
        if (newCarrier)   {
	        baudLOCK=TRUE; /* AnytimeNet-receive speedup */
            if (!noInit) greeting();
            newCarrier =FALSE;
			stickingAround=FALSE;
	        baudLOCK=FALSE;
			if (siege && !justDidNet && !onlyAsNet) doSecureLogin();
			break;
	        }
        if (justLostCarrier) {
            justLostCarrier=FALSE;
			doTotals();
	        }
        break;  /* irrelevant value */
#ifndef ONLYTHENET
    case '?':
        tutorial(loggedIn  ? "mainopt.mnu" : "unlogopt.mnu", TRUE);
        if (whichIO == CONSOLE)   mPrintf("\n^l: Sysop menu\n ");
        break;

    case 'A': if (!doAide(x, 'E'))  toReturn=TRUE; break;
    case 'Z': doForget(x); break;
#endif

    default:
        if (errorCount > MAX_USER_ERRORS) {
            logMessage(EVIL_SIGNAL, "", 'E');
            runHangup();
        }
        toReturn=TRUE;
        break;
    }
    return  toReturn;
}


/*
 *   doSkip()  -  the <S>kip a room command
 */
void doSkip(char expand)
/* char expand; */
{
#ifndef ONLYTHENET
    label roomName;  /* In case of ".Skip" */

    outFlag=IMPERVIOUS;
    mPrintf("\bSkip %s> goto ", roomTab[thisRoom].rtname);
    if (expand)
        getNormStr("", roomName, NAMESIZE, ECHO);
    else
        roomName[0]='\0';
    if (roomName[0] == '?')
        tutorial("skip.hlp", TRUE);
    else {
        roomTab[thisRoom].rtflags.SKIP=1; /* Set bit */
        gotoRoom(roomName, 'S');
    }
#endif
}


/*
 *   doBackupToRoom()  "Ungoto"
 */
void doBackupToRoom(char moreYet)
/* char moreYet; */
{
#ifndef ONLYTHENET
    label target;

    if (!moreYet) {
        strCpy(target, "");
    }
    else {
        mPrintf("room: ");
        getNormStr("", target, NAMESIZE, ECHO);
    }
    retRoom(target);
	ScrNewUser();
#endif
}

/*
 *   getCommand() prints menu prompt and gets command char
 *    returns char via parameter and expand flag as value  --
 *    i.e., TRUE if parameters follow else FALSE.
 */
char getCommand(char *c)
/* char *c; */
{
    char expand;

    outFlag=OUTOK;

    givePrompt();

    *c=toUpper(iChar());

    expand =(
        *c == ' '
        ||
        *c == '.'
        ||
        *c == ','
        ||
        *c == '/'
    );
    if (expand) *c=toUpper(iChar());

    if (justLostCarrier) {
        justLostCarrier=FALSE;
        terminate(FALSE, TRUE);
    }
    return expand;
}

/*
 *   greeting() system-entry blurb etc
 */
void greeting()
{
    extern char *VERSION;
    extern int byteRate;

    if (loggedIn) terminate(FALSE, TRUE);

    setUp(TRUE);
	pause(byteRate>250 ? 80 : 15); /*2nd was 15*/

    setmem(audit, AUDIT, ' ');
    acount=0;

	if (!noInit || frontEnd) {
		PrintBanner=TRUE; /* signal for anytime net */
		purgeFossilBuffs(); /* flushes modemINbuffer for those */
		}                   /* that insist on sending strings! */
    doCR();
    expert=TRUE;
/*	didRing=FALSE; */

    if (gotCarrier() ) {
		if (byteRate > 250) doPP(); /* does a longer pause if baud>2400 */
		else pause(150);
		}

    if (!HelpIfPresent("banner.blb")) {
		pause(50);
        mPrintf("Welcome to %s\n", cfg.codeBuf + cfg.nodeTitle);
		}
    expert=FALSE;
    mPrintf("\n Running: Citadel:K2NE\n Version %s\n ", VERSION);

    mPrintf("\n At most prompts, \"?\" brings Help.\n \n  ");
    if (!onLine()) mPrintf(formDate());
    mPrintf("\n ");

    printf("Chat %sabled\n", cfg.BoolFlags.noChat ? "dis" : "en");
    printf("\n 'MODEM' mode.\n ");

    printf("Press ESC for CONSOLE.\n ");
	purgeFossilBuffs();

    gotoRoom(cfg.codeBuf+cfg.bRoom, 'R');
    setUp(TRUE);
    if (onlyAsNet && outFlag != NET_CALL) {
        HelpIfPresent("onlynet.blb");
		runHangup();
		return;
		}
    PrintBanner=FALSE;
	justDidNet=FALSE;
    if (outFlag == NET_CALL) {
        netController(0, 0, NO_NETS, ANY_CALL);   /* so we don't call out */
		justDidNet=TRUE;
        outFlag=OUTOK;
		return;
    	}
	outFlag=OUTOK;
}

/*
 *   main() - central menu code
 */

void main(int argc, char **argv)
{
    extern char logNetResults, netDebug, InvVideo, ShowClock;
    extern char callLogPosting[800];

#ifndef MAJOR_RELEASE
    extern int InitColumns, tradeWars, timeInDoors;
#endif
    char c='\0', x='\0';
    char errMsg;
    FILE *modemThing;
DoTimes=FALSE;

CITCOLOR=0;  /* Default screen and statbar colors */
CITSTATUS=7; /* a good choice */
ANSI_GRAPHICS=TRUE;

#ifndef HUE_STYLE
NewVideo=TRUE;
InvVideo=TRUE;
ShowClock=TRUE;
#endif


initVariables();
aideCalledThis=FALSE;

    errMsg=FALSE;
    while (argc >= 2) {
        argc--;
		if (strCmpU(argv[argc], "asdoor") == SAMESTRING)
			runningAsDoor=TRUE;
		if (strCmpU(argv[argc], "bink") == SAMESTRING) {
			frontEnd=TRUE;
			noInit  =TRUE;
			}
		if (strCmpU(argv[argc], "lock") == SAMESTRING)
			siege=TRUE;
		if (strCmpU(argv[argc], "private") == SAMESTRING) {
			siege=TRUE;
			privateStatus=TRUE;
			}
        if (strCmpU(argv[argc], "netonly") == SAMESTRING)
			onlyAsNet=TRUE;
#ifdef HUE_STYLE
        else if (strCmpU(argv[argc], "+netdebug") == SAMESTRING) {
            printf("netdebug is on\n");
            netDebug=TRUE;
			}
        else if (strCmpU(argv[argc], "+newvideo") == SAMESTRING)
            NewVideo=TRUE;
        else if (strCmpU(argv[argc], "+inv") == SAMESTRING)
            InvVideo=TRUE;
        else if (strCmpU(argv[argc], "-clock") == SAMESTRING)
            ShowClock=FALSE;
#endif
        else if (strncmp(argv[argc], "noluck=", 7) == SAMESTRING)
            parseBadRes(argv[argc]);

#ifndef MAJOR_RELEASE
		else if (strncmp(argv[argc], "doormin=", 8) == SAMESTRING)
            timeInDoors=max(atoi(argv[argc] + 8), 10);
        else if (strncmp(argv[argc], "rings=", 6) == SAMESTRING)
			ringsToAnswer=atoi(argv[argc]+6);
        else if (strncmp(argv[argc], "blanker=", 8) == SAMESTRING) {
			blankVariable=TRUE;
			timeForBlank=60*(atoi(argv[argc]+8));
			startTimer(0);
			}

#ifndef NEW_PARMS
        else if (strncmp(argv[argc], "dead=", 5) == SAMESTRING)
            AnyKludge(argv[argc] + 5);
#endif
		else if (strncmp(argv[argc], "color=", 6) == SAMESTRING)
            parseColors(argv[argc] + 6);
        else if (strncmp(argv[argc], "coltb=", 6) == SAMESTRING) {
			parseAnsiColors(argv[argc] + 6);
			sysopColors=TRUE;
			}
		else if (strncmp(argv[argc], "altlink=", 8) == SAMESTRING) {
			autoFIDOlink= TRUE; /* special session  */
			noInit      =TRUE; /* leave modem dead */
			sprintf(alterLinkName, "%s", argv[argc] + 8);
			}
		else if (strncmp(argv[argc], "shuttle=", 8) == SAMESTRING) {
			sprintf(shuttleStyle, "%s", argv[argc] + 8);
			}

#endif
		else if (strCmpU(argv[argc], "noringer") == SAMESTRING) {
			noAudibleRing=TRUE;
			}
        else if (strCmpU(argv[argc], "nonoise") == SAMESTRING) {
			noNoises=TRUE;
			}
		else if (strCmpU(argv[argc], "+noinit") == SAMESTRING) {
			noInit     = TRUE; /* leave modem alone!   */
			doorLogFlag= TRUE; /* just did door          */
			tradeWars  =FALSE; /* no kludge-DTR needed     */
			}
		else if (strCmpU(argv[argc], "+doors") == SAMESTRING) {
			noInit     =TRUE;
			doorLogFlag=TRUE;
			tradeWars  =TRUE;  /* do reBOOT on terminate */
			}
        else if (strCmpU(argv[argc], "+recover") == SAMESTRING) {
            errMsg=TRUE;
	        }
    }
    if (access("initline.sys", 0)!=0) { /* INITLINE.SYS missing should  */
		cprintf("\nCOM problem!\n");     /* imply a CONFG failure and    */
        sleep(2);                       /* stop us from coming online!  */
		exit(CRASH_EXIT);               /* CRASH_EXIT is for Batch use. */
		}

    modemThing=fopen("initline.sys","rt");       /* modem init string file  */
	fgets(modemStartupString, 100, modemThing);
	fclose(modemThing);
    strcpy(modemResetString, "AT");
	if (access("resetmdm.sys",0)==0) {           /* modem reset string file */
	    modemThing=fopen("resetmdm.sys", "rt");
		fgets(modemResetString, 100, modemThing);
		fclose(modemThing);
		}
    else strcpy(modemResetString, "AT\r");
	if (access("idleline.sys",0)==0) {           /* modem idle string file  */
	    modemThing=fopen("idleline.sys", "rt");
		fgets(modemIdleString, 50, modemThing);
		fclose(modemThing);
		}
    else strcpy(modemIdleString, "AT S0=0\r"); /* modem idle string default */
	if (access("answrmdm.sys",0)==0) {           /* modem idle string file  */
	    modemThing=fopen("answrmdm.sys", "rt");
		fgets(modemAnswerString, 50, modemThing);
		fclose(modemThing);
		}
    else strcpy(modemAnswerString, "ATA"); /* modem answer string default */
    if (access("fastnet.sys",0)==0) {
		modemThing=fopen("fastnet.sys", "rt");
		fgets(fastModemNetPrefix, 50, modemThing);
		fclose(modemThing);
		}
	else strcpy(fastModemNetPrefix, "AT V0 W0 DT");

    checkDomain(); /* grab the host.domain if present */

    initCitadel();
#ifdef NEW_PARMS
    DeadTime=cfg.DeadTime * 60l;
	AnyNetLen=cfg.AnyNetLength;
#endif
	tryToLogIn   =cfg.LogTry;
    logNetResults=cfg.netLog;
    NoChatAtAll  =cfg.neverChat;
    notDoorUser  =!cfg.doorsOk;
    ourBBSport   =cfg.FOSSIL_PORT;
    if (cfg.limitSession != 0) {
		shortSession=TRUE;
        timeOnLine=60 * cfg.limitSession;
        }
	if (noInit) getSessionData();
	if (!sysopColors) standbyAnsi();
	multi_init();  /*  check for DDOS/DV and setup timesharing */
    startTimer(1); /* start anytimeNet timer */

    if (errMsg) {
        sPrintf(callLogPosting, "%s", "File-system reconstructed by Citadel.");
		logMessage(19,"",FALSE); /* no longer sent to Aide> */
        }
	if (ansi) CITSTATUS=7;
	gate_keeper();
    ScrNewUser();
	if (!noInit) {
	    if (autoFIDOlink==FALSE) logMessage(FIRST_IN, "", FALSE);
		doTotals();
		}

    if (autoFIDOlink==TRUE) {
			echo=NEITHER;
			expert=TRUE;
			doLocalFIDO();
			}
    answerGuard=FALSE;
    while (!ExitToMsdos)  {

        x      =getCommand(&c);

        outFlag=OUTOK;

        if ((c==CNTRLl)  ?  doSysop('\0')  :  doRegular(x, c))  {
            if (!expert)    mPrintf("%s \n", Qxtr);
            else            mPrintf(" ?\n \n");
        }
    }

    answerGuard=TRUE;
    if (loggedIn) terminate(TRUE, TRUE);
    if (autoFIDOlink==FALSE) logMessage(LAST_OUT, "", FALSE);
	gate_keeper();
    writeSysTab();
    if (autoFIDOlink==FALSE) modemClose();
    makeLinkedRoomList();
    systemShutdown();
    exit(exitValue);
}

/*
 *    Function doSecureLogin() for private systems
 *    Brings new user directly to 'enter mail to sysop' and
 *    immediately terminates call upon saving or aborting msg.
 *    Drops carrier on bad password, assumes it's a hack attempt.
 */
doSecureLogin()
{
     label passWord;
	 char shortWord=TRUE;
	 int bump=1;

     userEnterName=GetDynamic(NAMESIZE);
	 siegeResult=TRUE;
	 unixStyle=TRUE;
special_place:
     specialPrompt=TRUE;
     getNormStr("Username", userEnterName, NAMESIZE, ECHO);
     specialPrompt=FALSE;
	 printf("Password: ");
     echo=CALLER;
     shortStuff=TRUE;
	 specialPrompt=TRUE;
     getNormStr("Password", passWord, NAMESIZE, NO_ECHO);
	 specialPrompt=FALSE;
	 shortStuff=FALSE;
     echo=BOTH;
	 if (!gotCarrier()) {
		justLostCarrier=TRUE;
		return;
		}
	 if (strlen(passWord) > 2 ) {
		shortWord=FALSE;
		login(passWord);
		}
	 if (!siegeResult || shortWord) {

			if (shortWord) {
				siegeResult=FALSE;
				if (privateStatus) expert=TRUE;
		        HelpIfPresent(privateStatus ? "private.blb" : "secure.blb");
    			if (privateStatus || !getYesNo("Do you want to apply for an account")) {
					if (bump<2 && !privateStatus) {
						bump++;
						doCR();
						goto special_place;
						}
					if (gotCarrier())  terminate(TRUE,TRUE);
						else justLostCarrier=TRUE;
					siegeResult=TRUE;
					expert=FALSE;
					return;
					}
        		HelpIfPresent("appnote.blb");
			    gotoRoom("Mail", 'R');
				siegeResult=TRUE;
  				dumpDeadWood=FALSE;
				doEnter(FALSE, 'M');
                }
		if (gotCarrier()) terminate(TRUE,TRUE);
		else {
			 setUp(TRUE);
    	     getRoom(LOBBY); /* User starts in lobby */
			 ScrNewUser();
             }
		}
}

doLocalFIDO(void)
{
#ifndef ONLYTHENET
 static char msgImportName[20], loopyFlag;
 static int messagePrefix=0;
 FILE *theFidoFile;

 alterNet=TRUE;
 strcpy(alterName, alterLinkName);
 if (access("1.alt", 0)!=0) {
	mPrintf("None found\n ");
    ExitToMsdos=TRUE;
	return;
	}
 messagePrefix=1;
 loopyFlag=TRUE;
 do {
	sprintf(msgImportName, "%d", messagePrefix);
	strcat(msgImportName, ".alt");
	if (access(msgImportName, 0)==0) {
		linkerNumber++;
		ingestFile(msgImportName);
		if (alterNet==FALSE) loopyFlag=FALSE;
       	hldMessage();
		unlink(msgImportName);
		messagePrefix++;
		}
	else loopyFlag=FALSE;
	} while (loopyFlag==TRUE);
 alterNet=FALSE;
 ExitToMsdos=TRUE;
 return;
#endif
}