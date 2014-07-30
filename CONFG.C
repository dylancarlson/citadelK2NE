/************************************************************************/
/*                              confg.c                                 */
/*      configuration program for Citadel bulletin board system.        */
/************************************************************************/

#define CONFIGURE

#include "ctdl.h"

/************************************************************************/
/*                              History                                 */
/*                                                                      */
/* 85Dec26 HAW  Add CALL-LOG define.                                    */
/* 85Nov15 HAW  MS-DOS library update.                                  */
/* 85Oct27 HAW  Kill CERMETEK.                                          */
/* 85Oct17 HAW  Add paramVers, change bauds to array, searchBaud chg.   */
/* 85Oct16 HAW  Kill CLOCK, add officeStuff.                            */
/* 85Aug24 HAW  Add duomessage file, NETDISK specification.             */
/* 85Jul07 HAW  Update so won't go through total recon. on init.        */
/* 85May27 HAW  Start stuffing in auto-networking stuff.                */
/* 85May22 HAW  Start conversion to make log file size sysop selectable.*/
/* 85May11 HAW  Make "Lobby" sysop definable                            */
/* 85May06 HAW  Add bail out code.                                      */
/* 85May05 HAW  Add helpDisk parameter for 3 disk system.               */
/* 85Apr10 HAW  Fix logSort, alphabetize file.                          */
/* 85Mar11 HAW  Put all user functions in this file.                    */
/* 85Feb18 HAW  Add baud search stuff.                                  */
/* 85Jan20 HAW  Use MSDOS #define for date stuff.                       */
/* 84Sep05 HAW  Isolate strangeness in compiler's library.  See note.   */
/* 84Aug30 HAW  Onwards to MS-DOS!                                      */
/* 84Apr08 HAW  Update to BDS C 1.50a begun.                            */
/* 82Nov20 CrT  Created.                                                */
/************************************************************************/

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      dGetWord()              reads a word off disk                   */
/*      init()                  system startup initialization           */
/*      main()                  main controller                         */
/*      illegal()               abort bottleneck                        */
/*      msgInit()               sets up cfg.catChar, catSect etc.       */
/*      zapMsgFile()            initialize ctdlmsg.sys                  */
/*      realZap()               does work of zapMsgFile()               */
/*      indexRooms()            build RAM index to ctdlroom.sys         */
/*      noteRoom()              enter room into RAM index               */
/*      zapRoomFile()           erase & re-initialize ctdlroom.sys      */
/*      setSpace()              set default disk and user#              */
/*      hash()                  hashes a string to an integer           */
/*      logInit()               builds the RAM index to CTDLLOG.SYS     */
/*      noteLog()               enters a userlog record into RAM index  */
/*      sortLog()               sort CTDLLOG by time since last call    */
/*      wrapup()                finishes and writes ctdlTabl.sys        */
/*      zapLogFile()            erases & re-initializes CTDLLOG.SYS     */
/************************************************************************/

/************************************************************************/
/*                              Strangenesses   (Hue, Jr., 12Sep84)     */
/*      Have discovered that the line:                                  */
/*      sscanf(line, "\"%s\"", str);                                    */
/*      is not parsed the same way by this compiler as it is by BDS;    */
/*      this is highly unfortunate and excrable.  So, all porters       */
/*      should note that scanf() is not, in any way, "portable."  If    */
/*      BDS is "non-standard", then the standard sucks.                 */
/************************************************************************/

struct GenList {
    char *GenName;
    int  GenVal;
} ;

FILE         *msgfl;    /* file descriptor for the msg file     */
char         *baseRoom;
int          mailCount=0;
char         msgZap =  FALSE,
             logZap =  FALSE,
             roomZap = FALSE;


static DATA_BLOCK sectBuf;

long FloorSize;

/************************************************************************/
/*                External variable declarations in CONFG.C             */
/************************************************************************/
extern CONFIG    cfg;       /* The configuration variable   */
extern MSG_BUF      msgBuf;    /* The -sole- message buffer    */
extern NetBuffer netBuf;
extern rTable    *roomTab;  /* RAM index of rooms           */
extern aRoom     roomBuf;   /* room buffer                  */
extern EVENT     *EventTab;
extern int              thisRoom;  /* room currently in roomBuf    */
extern int              thisNet;
extern LogTable    *logTab;   /* RAM index of pippuls         */
extern logBuffer logBuf;    /* Log buffer of a person       */
extern FILE             *logfl;    /* log file descriptor          */
extern FILE             *roomfl;   /* file descriptor for rooms    */
extern FILE             *netfl;
extern LogTable    *logTab;   /* RAM index of pippuls         */
extern int              thisLog;   /* entry currently in logBuf    */
extern NetTable  *netTab;

/************************************************************************/
/*      init() -- master system initialization                          */
/************************************************************************/
void init(attended)
int attended;
{
    extern char   *W_R_ANY;
    extern char   *R_W_ANY;
    extern char   *READ_ANY;
    extern char   *WRITE_ANY;
    unsigned char c;
    SYS_FILE      tempName;

    cfg.sizeLTentry = sizeof(*logTab);

    cfg.BoolFlags.debug       = FALSE;
    cfg.BoolFlags.noChat      = TRUE;

    /* shave-and-a-haircut/two bits pause pattern for ringing sysop: */
    cfg.shave[0]    = 40;
    cfg.shave[1]    = 20;
    cfg.shave[2]    = 20;
    cfg.shave[3]    = 40;
    cfg.shave[4]    = 80;
    cfg.shave[5]    = 40;
    cfg.shave[6]    =250;

    /* initialize input character-translation table:    */
    for (c = 0;  c < '\40';  c++) {
        cfg.filter[c] = '\0';           /* control chars -> nulls       */
    }
    for (c='\40'; c < 128;   c++) {
        cfg.filter[c] = c;              /* pass printing chars          */
    }
    cfg.filter[SPECIAL]     = SPECIAL;
    cfg.filter[CNTRLl]      = CNTRLl;
    cfg.filter[DEL      ]   = BACKSPACE;
    cfg.filter[BACKSPACE]   = BACKSPACE;
    cfg.filter[XOFF     ]   = 'P'      ;
    cfg.filter['\r'     ]   = NEWLINE  ;
    cfg.filter[CNTRLO   ]   = 'N'      ;

	cfg.whatRate[0] 	= '\0';
	cfg.lastCaller[0]	= '\0';
	cfg.SeeMail 		= FALSE;

    mvToHomeDisk(&cfg.homeArea);

    makeSysName(tempName, "ctdlmsg.sys",  &cfg.msgArea);
    if ((msgfl = fopen(tempName, R_W_ANY)) == NULL) {
        if (!attended)
            illegal("!System must be attended for creation!");
        printf(" %s not found, creating new file. \n", tempName);
        if ((msgfl = fopen(tempName, W_R_ANY)) == NULL)
            illegal("?Can't create the message file!");
        printf(" (Be sure to initialize it!)\n");
    }

    makeSysName(tempName, "ctdllog.sys", &cfg.logArea);
    /* open userlog file */
    if ((logfl = fopen(tempName, R_W_ANY)) == NULL) {
        if (!attended)
            illegal("!System must be attended for creation!");
        printf(" %s not found, creating new file. \n", tempName);
        if ((logfl = fopen(tempName, W_R_ANY)) == NULL)
            illegal("?Can't create log file!");
        printf(" (Be sure to initialize it!)\n");
    }

    makeSysName(tempName, "ctdlroom.sys", &cfg.roomArea);
    /* open room file */
    if ((roomfl = fopen(tempName, R_W_ANY)) == NULL) {
        if (!attended)
            illegal("!System must be attended for creation!");
        printf(" %s not found, creating new file. \n", tempName);
        if ((roomfl = fopen(tempName, W_R_ANY)) == NULL)
           illegal("?Can't create room file!");
        printf(" (Be sure to initialize it!)\n");
    }

    if (cfg.BoolFlags.netParticipant) {
        makeSysName(tempName, "ctdlnet.sys", &cfg.netArea);
        if ((netfl = fopen(tempName, READ_ANY)) == NULL) {
            printf(" %s not found, creating new file.\n", tempName);
            if ((netfl = fopen(tempName, WRITE_ANY)) == NULL)
                illegal("?Can't create the net file!");
        }
    }

    CheckFloors();

    printf("\n Erase and initialize log, message and/or room files?");
    if (attended)
        if (toUpper(simpleGetch()) == 'Y') {
            /* each of these has an additional go/no-go interrogation: */
            msgZap  = zapMsgFile();
            roomZap = zapRoomFile();
            logZap  = zapLogFile();
        }
}

/************************************************************************/
/*      main() for confg.c                                              */
/************************************************************************/
main(argc, argv)
int  argc;
char **argv;
{
    FILE *fBuf, *pwdfl;
    char line[90], status, *strchr(), *g;
    char onlyParams;
    char var[90];
    int  arg;
    int  i, offset = 1;
    extern char *READ_TEXT;
    int  SetVal;

    if (access(LOCKFILE, 0) != -1) {
        printf(
"You are apparently reconfiguring from within Citadel, which is a No No!\n"
"Do you wish to continue? ");
        if (toUpper(simpleGetch()) != 'Y')
            exit(7);
        unlink(LOCKFILE);
    }

    zero_struct(cfg);

    for (i = 1, onlyParams = FALSE;  i < argc; i++) {
        if (strCmpU(argv[i], "onlyParams") == SAMESTRING)
            onlyParams = TRUE;
    }

    if (onlyParams) argc--;
#ifdef NEW_PARMS
	cfg.paramVers = 12;
#else
    cfg.paramVers = 11;
#endif
    printf("Citadel:K2NE Configurator (V%d.1)\n\n", cfg.paramVers);

#ifdef SERIES6
    if (!checkForFossil() ) {
printf("The Citadel:K2NE Configurator was unable to detect the presence of a\n"
       "valid FOSSIL driver.  Citadel:K2NE requires a FOSSIL driver for modem\n"
       "I/O beginning with Version 6.01.\n\n");
printf("Please obtain a FOSSIL driver and install it as a DEVICE DRIVER in your\n"
	   "boot-time CONFIG.SYS file.  For information on FOSSIL driver installation\n"
	   "you are referred to:\n\n"
	   "             Fundamentals of FOSSIL Implementation and Use\n"
       "                    Version 5,  February 11,  1988\n"
       "                   Rick Moore, Solar Wind Computing\n\n"
	   "This document is included with many of the popular FOSSIL drivers available\n"
	   "on most MS-DOS bulletin boards.  If you need further assistance, please\n"
	   "contact:\n\n"
	   "             Vince Quaresima, K2NE         FIDOnet @ 1:266/33\n"
	   "             30 Marygold Avenue                      OR\n"
	   "             Browns Mills, NJ 08015        Packet @ N2LQH.NJ.USA.NA\n\n");

		exit(0);
		}
#endif

    cfg.weAre     = CONFIGUR;
    if (!(SetVal = readSysTab(FALSE, FALSE))) {
        cfg.sysPassword[0] = 0;
        cfg.MAXLOGTAB = 0;      /* Initialize, just in case             */
        if (onlyParams) {
            printf("'onlyParams' parameter ignored\n");
            onlyParams = FALSE;
        }
    }
    else {
        if (EventTab != NULL) free(EventTab);
        zero_struct(cfg.BoolFlags);
    }

    initSysSpec();              /* Call implementation specific code      */

    cfg.SysopName[0] = 0;
                                /* Citadel:K2NE sets some defaults           */
    cfg.LogTry = 20;            /* Allow 20 bad log tries before CLICK!      */
	cfg.Ansi = 0;               /* SysConsole default to NON-ANSI screen     */
	cfg.modemSpeed = 0;         /* Modem init defaults to SLOW mode.         */
	cfg.netLog = 0;             /* Assume no net-log is wanted.              */
    cfg.doorsOk = 0;            /* Assume new users do NOT get door privs    */
	cfg.limitSession = 0;       /* Default to NO session time-limit.         */
	cfg.FOSSIL_PORT = 0;        /* Default to COM1 then grab from #COM later */
    cfg.slowModemDelay=0;       /* Default to 0 otherwise set by sysop       */
    cfg.lockFossil=0;           /* If 1 then FOSSIL locks physical link      */
#ifdef NEW_PARMS
    cfg.DeadTime=1800l;         /* Default length of inactivity for Anytime  */
    cfg.AnyNetLength=180l;         /* Default length of an Anytime Net          */
#endif
    EventTab = NULL;
    if ((fBuf = fopen("ctdlcnfg.sys", READ_TEXT)) == NULL) {/* ASCII mode   */
        printf("?Can't find ctdlCnfg.sys!\n");
        exit(1);
    }

    while (fgets(line, 90, fBuf) != NULL) {
        if (line[0] != '#') continue;
        if (sscanf(line, "%s %d ", var, &arg)) {
            printf(line);
                   if (strCmp(var, "#CRYPTSEED" )    == SAMESTRING) {
                cfg.cryptSeed   = arg;
            } else if (strCmp(var, "#MESSAGEK"  )    == SAMESTRING) {
                cfg.maxMSector  = arg*(1024/MSG_SECT_SIZE);
            } else if (strCmp(var, "#LOGINOK"   )    == SAMESTRING) {
                cfg.BoolFlags.unlogLoginOk= arg;
            } else if (strCmp(var, "#NewNetPrivs")    == SAMESTRING) {
                cfg.BoolFlags.NetDft      = arg;
            } else if (strCmp(var, "#ENTEROK"   )    == SAMESTRING) {
                cfg.BoolFlags.unlogEnterOk= arg;
            } else if (strCmp(var, "#READOK"    )    == SAMESTRING) {
                cfg.BoolFlags.unlogReadOk = arg;
            } else if (strCmp(var, "#ROOMOK"    )    == SAMESTRING) {
                cfg.BoolFlags.nonAideRoomOk=arg;
            } else if (strCmp(var, "#ALLMAIL"   )    == SAMESTRING) {
                cfg.BoolFlags.noMail      = !arg;
            } else if (strCmp(var, "#MIRRORMSG" )    == SAMESTRING) {
                cfg.BoolFlags.mirror = arg;
            } else if (strCmp(var, "#LOGSIZE"   )    == SAMESTRING) {
                if (SetVal) {
                    if (cfg.MAXLOGTAB != arg)
                       illegal(
                         "LOGSIZE parameter does not equal old value!");
                }
                else {
                    cfg.MAXLOGTAB   = arg;
                    logTab = (LogTable *)
                           GetDynamic(sizeof(*logTab) * arg);
                }
            } else if (strCmp(var, "#MAXROOMS"  )    == SAMESTRING) {
                if (SetVal) {
                    if (MAXROOMS != arg)
                        illegal(
                          "MAXROOMS parameter does not equal old value!");
                }
                else {
                    MAXROOMS = arg;
                    roomTab = (rTable *)
                                   GetDynamic(MAXROOMS * sizeof *roomTab);
                }
            } else if (strCmp(var, "#MSG-SLOTS" )    == SAMESTRING) {
                if (SetVal) {
                    if (MSGSPERRM != arg)
                        illegal(
                          "MSGSPERRM parameter does not equal old value!");
                }
                else {
                    MSGSPERRM = arg;
                }
            } else if (strCmp(var, "#MAIL-SLOTS")    == SAMESTRING) {
                if (SetVal) {
                    if (MAILSLOTS != arg)
                        illegal(
                          "MAILSLOTS parameter does not equal old value!");
                }
                else {
                    MAILSLOTS = arg;
                }
            } else if (strCmp(var, "#SHARED-ROOMS")  == SAMESTRING) {
                if (SetVal) {
                    if (SHARED_ROOMS != arg)
                        illegal(
                      "SHARED-ROOMS parameter does not equal old value!");
                }
                else {
                    SHARED_ROOMS = arg;
                }
            } else if (strCmp(var, "#NET-ARCH-ROOMS")  == SAMESTRING) {
                if (SetVal) {
                    if (NET_ARCH_ROOMS != arg)
                        illegal(
                      "NET-ARCH-ROOMS parameter does not equal old value!");
                }
                else {
                    NET_ARCH_ROOMS = arg;
                }
            } else if (strCmp(var, "#NETWORK"   )    == SAMESTRING) {
                cfg.BoolFlags.netParticipant = arg;
            } else if (strCmp(var, "#LONG-HAUL" )    == SAMESTRING) {
                cfg.BoolFlags.longHaul = arg;
            } else if (strCmp(var, "#NET_AREA_SIZE") == SAMESTRING) {
                cfg.sizeArea = arg;
            } else if (strCmp(var, "#MAX_NET_FILE")    == SAMESTRING) {
                cfg.maxFileSize = arg;
            } else if (strCmp(var, "#AIDESEEALL")    == SAMESTRING) {
                cfg.BoolFlags.aideSeeAll = arg;
/* K2NE NEW on 91May13 */

			} else if (strCmp(var, "#LOGTRY") == SAMESTRING) {
				cfg.LogTry = arg;
			} else if (strCmp(var, "#ANSICONSOLE") == SAMESTRING) {
				cfg.Ansi = arg;
            } else if (strCmp(var, "#MODEMSPEED") == SAMESTRING) {
				cfg.modemSpeed = arg;
            } else if (strCmp(var, "#NETLOG") == SAMESTRING) {
				cfg.netLog = arg;
			} else if (strCmp(var, "#NEVERCHAT") == SAMESTRING) {
				cfg.neverChat = arg;
			} else if (strCmp(var, "#NEWDOORPRIVS") == SAMESTRING) {
				cfg.doorsOk = arg;
			} else if (strCmp(var, "#SESSIONTIMELIMIT") == SAMESTRING) {
				cfg.limitSession = arg;
			} else if (strCmp(var, "#DTR-DELAY") == SAMESTRING) {
				cfg.slowModemDelay = arg;
			} else if (strCmp(var, "#LOCKFOSSIL") == SAMESTRING) {
				cfg.lockFossil = arg;
#ifdef NEW_PARMS
			} else if (strCmp(var, "#DEADTIME") == SAMESTRING) {
				cfg.DeadTime = arg;
			} else if (strCmp(var, "#ANYNETLENGTH") == SAMESTRING) {
				cfg.AnyNetLength = arg;
#endif
/* end of K2NE NEW code */
            } else if (strCmp(var, "#SYSBAUD"   )    == SAMESTRING) {
                cfg.sysBaud   = arg;
                if (arg > 5 || arg < 0) {
                    illegal(
"Valid SYSBAUD values: 0=300, 1=3/12, 2=3/12/24, 3=3/12/24/48,\n4=3/12/24/48/96, 5=3/12/24/48/96/Your option");
                }
            } else if (strCmp(var, "#event"    ) == SAMESTRING) {
                offset = EatEvent(line, offset);
            } else if (strCmp(var, "#nodeTitle") == SAMESTRING) {
                readString(line, &cfg.codeBuf[offset], TRUE);
                cfg.nodeTitle = offset;
                while (cfg.codeBuf[offset])
                    offset++;
                offset++;

            } else if (strCmp(var, "#sysPassword") == SAMESTRING) {
                    readString(line, cfg.sysPassword, FALSE);
                    if ((pwdfl = fopen(cfg.sysPassword, READ_TEXT)) == NULL) {
                        printf("\nNo system password file found\n");
                        cfg.sysPassword[0] = 0;
                    }
                    else {
                        fgets(cfg.sysPassword, 199, pwdfl);
                      /*  cfg.sysPassword[strLen(cfg.sysPassword) - 1] = 0;*/
                        while ((g = strchr(cfg.sysPassword, '\n')) != NULL)
                            *g = 0;
                        if (strLen(cfg.sysPassword) < 15) {
                          printf("\nSystem password is too short -- ignored\n");
                            cfg.sysPassword[0] = 0;
                        }
                        fclose(pwdfl);
                    }

            } else if (strCmp(var, "#callOutSuffix") == SAMESTRING) {
                    readString(line, &cfg.codeBuf[offset], TRUE);
                    cfg.netSuffix = offset;
                    while (cfg.codeBuf[offset])
                        offset++;
                    offset++;
            } else if (strCmp(var, "#callOutPrefix") == SAMESTRING) {
                    readString(line, &cfg.codeBuf[offset], TRUE);
                    cfg.netPrefix = offset;
                    while (cfg.codeBuf[offset])
                        offset++;
                    offset++;

            } else if (strCmp(var, "#sysopName") == SAMESTRING) {
                    readString(line, msgBuf.mbtext, FALSE);
                    if (strLen(msgBuf.mbtext) > 19)
                        illegal("SysopName too long; must be less than 20");
                    strCpy(cfg.SysopName, msgBuf.mbtext);
            } else if (strCmp(var, "#nodeName" ) == SAMESTRING) {
                    readString(line, &cfg.codeBuf[offset], FALSE);
                    if (strLen(&cfg.codeBuf[offset]) > 19)
                        illegal("nodeName too long; must be less than 20");
                    cfg.nodeName    = offset;
                    while (cfg.codeBuf[offset]) /* step over string     */
                        offset++;
                    offset++;
            } else if (strCmp(var, "#nodeId"   ) == SAMESTRING) {
                    readString(line, &cfg.codeBuf[offset], FALSE);
                    if (strLen(&cfg.codeBuf[offset]) > 19)
                        illegal("nodeId too long; must be less than 20");
                    cfg.nodeId      = offset;
                    while (cfg.codeBuf[offset]) /* step over string     */
                        offset++;
                    offset++;
            } else if (strCmp(var, "#baseRoom") == SAMESTRING) {
                    readString(line, &cfg.codeBuf[offset], TRUE);
                    if (strLen(&cfg.codeBuf[offset]) > 19)
                        illegal("baseRoom too long; must be less than 20");
                    cfg.bRoom = offset;
                    baseRoom = &cfg.codeBuf[offset];
                    while (cfg.codeBuf[offset])
                         offset++;
                    offset++;
            } else if (strCmp(var, "#MainFloor") == SAMESTRING) {
                    readString(line, &cfg.codeBuf[offset], TRUE);
                    if (strLen(&cfg.codeBuf[offset]) > 19)
                        illegal("#MainFloor too long; must be less than 20");
                    cfg.MainFloor = offset;
                    while (cfg.codeBuf[offset])
                         offset++;
                    offset++;
            } else if (strCmp(var, "#alldone") == SAMESTRING) {
                break;
            } else {
                offset = sysSpecs(line, offset, &status);
                if (!status)
                    printf("? -- no variable '%s' known! -- ignored.\n", var);
            }
        }
    }

    initLogBuf(&logBuf);
    initRoomBuf(&roomBuf);
    initNetBuf(&netBuf);

    if (!SysDepIntegrity(&offset))
        exit(2);

    printf("offset=%d\n", offset);

    if (offset < MAXCODE) {
       if (!onlyParams) init(!(argc - 1));
       else CheckFloors();
       wrapup(onlyParams);
    } else {
        illegal(
"\7codeBuf[] overflow! Recompile with larger MAXCODE or reduce ctdlCnfg.sys\7"
        );
    }
}

/***********************************************************************/
/*    readString() reads a '#<id> "<value>"  since scanf can't         */
/***********************************************************************/
void readString(source, destination, doProc)
char *source, *destination;
char doProc;
{
    char string[300], last = 0;
    int  i, j;

    for (i = 0; source[i] != '"' && source[i]; i++)
        ;

    if (!source[i]) {
        sPrintf(string, "Couldn't find beginning \" in -%s-", source);
        illegal(string);
    }

    for (j = 0, i++; source[i] && 
                    (source[i] != '"' || (doProc && last == '\\'));
                                             i++, j++) {
        string[j] = source[i];
        last = source[i];
    }
    if (!source[i]) {
        sPrintf(string, "Couldn't find ending \" in -%s-", source);
        illegal(string);
    }
    string[j] = '\0';
    strCpy(destination, string);
    if (doProc) xlatfmt(destination);
}

/************************************************************************/
/*    isoctal() xlatfmt() -- contributed by Dale Schumacher, allow      */
/*    embedding of formatting info a la' "C" style: \n, \t, etc....     */
/************************************************************************/
char isoctal( c )
int c;
{
        return(( c >= '0' ) && ( c <= '7' ));
}

void xlatfmt( s )
char *s;
{
        register char *p, *q;
        register int i;

        for( p=q=s; *q; ++q ) {
                if ( *q == '\\' )
                        switch( *++q ) {
                                case 'n' :
                                        *p++ = '\n';
                                        break;
                                case 't' :
                                        *p++ = '\t';
                                        break;
                                case 'r' :
                                        *p++ = '\r';
                                        break;
                                case 'f' :
                                        *p++ = '\f';
                                        break;
                                default :
                                        if ( isoctal( *q )) {
                                                i = (( *q++ ) - '0' );
                                                if ( isoctal( *q )) {
                                                        i <<= 3;
                                                        i += (( *q++ ) - '0' );
                                                        if ( isoctal( *q )) {
                                                                i <<= 3;
                                                                i += (*q -'0');
                                                        }
                                                        else
                                                             --q;
                                                }
                                                else
                                                        --q;
                                                *p++ = 0xFF & ((char) i);
                                        }
                                        else
                                                *p++ = *q;
                                        break;
                        }
                else
                        *p++ = *q;
        }
        *p = '\0';

}

/***********************************************************************/
/*    illegal() Prints out configur error message and aborts           */
/***********************************************************************/
void illegal(errorstring)
char *errorstring;
{
    printf("\007\nERROR IN CONFIGURATION:\n%s\nABORTING", errorstring);
    exit(7);
}

/************************************************************************/
/*      dGetWord() fetches one word from current message, off disk      */
/*      returns TRUE if more words follow, else FALSE                   */
/************************************************************************/
char dGetWord(dest, lim)
char *dest;
int  lim;
{
    char c;

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

/************************************************************************/
/*      msgInit() sets up lowId, highId, cfg.catSector and cfg.catChar, */
/*      by scanning over message.buf                                    */
/************************************************************************/
void msgInit()
{
    MSG_NUMBER first, here;
    extern struct mBuf mFile1;

    startAt(msgfl, &mFile1, 0, 0);
    getMessage();

    /* get the ID# */
    first = atol(msgBuf.mbId);
    printf("message# %lu\n", first);

    cfg.newest = cfg.oldest = first;

    cfg.catSector   = mFile1.thisSector;
    cfg.catChar     = mFile1.thisChar;

    for (getMessage();
             here = atol(msgBuf.mbId), here != first;
                      getMessage()) {
/*K2NE*/
        cprintf("message# %lu\r", here);

        if (strCmpU("Mail", msgBuf.mbroom) == 0) mailCount++;

        /* find highest and lowest message IDs: */
        if (here < cfg.oldest && here != 0l) {
            cfg.oldest = here;
        }
        if (here > cfg.newest) {
            cfg.newest = here;

            /* read rest of message in and remember where it ends,      */
            /* in case it turns out to be the last message              */
            /* in which case, that's where to start writing next message*/
            while (dGetWord(msgBuf.mbtext, MAXTEXT));
            cfg.catSector   = mFile1.thisSector;
            cfg.catChar     = mFile1.thisChar;
        }
    }
    printf("\n\noldest=%lu\n", cfg.oldest);
    printf("newest=%lu\n", cfg.newest);
}

/************************************************************************/
/*      zapMsgFile() initializes message.buf                            */
/************************************************************************/
char zapMsgFile()
{
    extern char *W_R_ANY;
    char fn[80];

    printf("\nDestroy all current messages? ");
    if (toUpper(simpleGetch()) != 'Y')   return FALSE;

    if (cfg.BoolFlags.mirror) printf("Creating primary message file.\n");
    realZap();
    if (cfg.BoolFlags.mirror) {
        fclose(msgfl);
        makeSysName(fn, "ctdlmsg.sys", &cfg.msg2Area);
        if ((msgfl = fopen(fn, W_R_ANY)) == NULL)
            illegal("?Can't create the secondary message file!");
        printf("Creating secondary message file.\n");
        realZap();
    }
    return TRUE;
}

/************************************************************************/
/*      realZap() does work of zapMsgFile                               */
/************************************************************************/
char realZap()
{
    int   i;
    unsigned sect;

    /* put null message in first sector... */
    sectBuf[0]  = 0xFF; /*   \                                */
    sectBuf[1]  =  '1'; /*    >  Message ID "1" MS-DOS style  */
    sectBuf[2]  = '\0'; /*   /                                */
    sectBuf[3]  =  'M'; /*   \    Null messsage               */
    sectBuf[4]  = '\0'; /*   /                                */

    cfg.newest = cfg.oldest = 1l;

    cfg.catSector   = 0;
    cfg.catChar     = 5;

    for (i=5;  i<MSG_SECT_SIZE;  i++) sectBuf[i] = 0;

#ifndef NO_CRYPT
    crypte(sectBuf, MSG_SECT_SIZE, 0);       /* encrypt      */
#endif
    if (fwrite(sectBuf, MSG_SECT_SIZE, 1, msgfl) != 1) {
        printf("zapMsgFil: write failed\n");
    }

#ifndef NO_CRYPT
    crypte(sectBuf, MSG_SECT_SIZE, 0);       /* decrypt      */
#endif
    sectBuf[0] = 0;
#ifndef NO_CRYPT
    crypte(sectBuf, MSG_SECT_SIZE, 0);       /* encrypt      */
#endif
    printf("\n%d sectors to be cleared\n", cfg.maxMSector);
    for (sect = 1l;  sect < cfg.maxMSector;  sect++) {
        printf("%u\r", sect);
        if (fwrite(sectBuf, MSG_SECT_SIZE, 1, msgfl) != 1) {
            printf("zapMsgFil: write failed\n");
        }
    }
#ifndef NO_CRYPT
    crypte(sectBuf, MSG_SECT_SIZE, 0);       /* decrypt      */
#endif
    return TRUE;
}

/************************************************************************/
/*      indexRooms() -- build RAM index to CTDLROOM.SYS, displays stats.*/
/************************************************************************/
void indexRooms()
{
    int  goodRoom, m, roomCount, slot;

    zero_struct(roomBuf.rbflags);
    zero_struct(roomBuf.rbArea);
    roomBuf.rbgen = 0;
    roomBuf.rbname[0] = 0;
    roomBuf.rbmoderator[0] = 0;
    roomBuf.rbShareType = 0;
    for (m = 0; m < MSGSPERRM; m++) {
        roomBuf.msg[m].rbmsgNo = 0l;
        roomBuf.msg[m].rbmsgLoc = 0;
    }

    strCpy(roomBuf.rbname, "Mail");
    roomBuf.rbflags.PUBLIC =
    roomBuf.rbflags.PERMROOM =
    roomBuf.rbflags.INUSE = TRUE;
    putRoom(MAILROOM);

    roomCount = 0;
    for (slot = 0;  slot < MAXROOMS;  slot++) {
        getRoom(slot);
        printf("Checking room #%d: ", slot);
        if (roomBuf.rbflags.INUSE == 1) {
            roomBuf.rbflags.INUSE = 0;          /* clear "inUse" flag */
            if (roomBuf.rbFlIndex >= (int) FloorSize)
                roomBuf.rbFlIndex = 0;

            for (m = 0, goodRoom = FALSE; m < MSGSPERRM && !goodRoom; m++) {
                if (roomBuf.msg[m].rbmsgNo > cfg.oldest) {
                    goodRoom    = TRUE;
                }
            }
            if (goodRoom   || roomBuf.rbflags.PERMROOM == 1)   {
                roomBuf.rbflags.INUSE = 1;
            }

            if (roomBuf.rbflags.INUSE == 1) {
                if (slot == 0)                     /* Ugly kludge */
                    strCpy(roomBuf.rbname, baseRoom);
                roomCount++;
            }
            else {
                zero_struct(roomBuf.rbflags);
            }
        }
        printf("%s\n",
               (roomBuf.rbflags.INUSE == 1) ? roomBuf.rbname : "<not in use>");
        if (roomBuf.rbflags.INUSE && roomBuf.rbflags.SHARED)
            roomTab[slot].rtlastNet = findHighestNative();
        else
            roomTab[slot].rtlastNet = 0l;
        noteRoom();
        putRoom(slot);
    }
    printf(" %d of %d rooms in use\n", roomCount, MAXROOMS);
}

/************************************************************************/
/*      findHighestNative() Finds highest native message in a room      */
/************************************************************************/
MSG_NUMBER findHighestNative()
{
    int rover;
    MSG_NUMBER ourHighest;
    theMessages *temp;

    temp = (theMessages *) GetDynamic(MSG_BULK);
    copy_ptr(roomBuf.msg, temp, MSGSPERRM);
    qsort(temp, MSGSPERRM, sizeof temp[0], msgSort);

    ourHighest = 0l;
    for (rover = 0; rover < MSGSPERRM; rover++) {
        if (temp[rover].rbmsgNo != 0l &&
                     findMessage(temp[rover].rbmsgLoc, temp[rover].rbmsgNo)) {
            if (strCmpU(msgBuf.mbaddr, R_SH_MARK) == SAMESTRING ||
                strCmpU(msgBuf.mbaddr, NON_LOC_NET) == SAMESTRING) {
                ourHighest = temp[rover].rbmsgNo;
                break;
            }
        }
    }
    free(temp);
    return ourHighest;
}

int msgSort(s1, s2)
theMessages *s1, *s2;
{
        if (s1->rbmsgNo < s2->rbmsgNo) return 1;
        if (s1->rbmsgNo > s2->rbmsgNo) return -1;
        return 0;
}

/************************************************************************/
/*      noteRoom() -- enter room into RAM index array.                  */
/************************************************************************/
void noteRoom()
{
    int   i;
    MSG_NUMBER last;

    last = 0l;
    for (i = 0;  i < MSGSPERRM;  i++)  {
        if (roomBuf.msg[i].rbmsgNo > cfg.newest) {
            roomBuf.msg[i].rbmsgNo = 0l;
        }
        if (roomBuf.msg[i].rbmsgNo > last) {
            last = roomBuf.msg[i].rbmsgNo;
        }
    }
    roomTab[thisRoom].rtlastMessage = last           ;
    roomTab[thisRoom].rtShareType   = roomBuf.rbShareType;
    strCpy(roomTab[thisRoom].rtname, roomBuf.rbname) ;
    roomTab[thisRoom].rtgen            = roomBuf.rbgen  ;
    roomTab[thisRoom].rtFlIndex        = roomBuf.rbFlIndex;
    copy_struct(roomBuf.rbflags, roomTab[thisRoom].rtflags);
}

/************************************************************************/
/*      zapRoomFile() erases and re-initializes CTDLROOM.SYS            */
/************************************************************************/
char zapRoomFile()
{
    int i;

    printf("\nWipe room file? ");
    if (toUpper(simpleGetch()) != 'Y') return FALSE;
    printf("\n");

    zero_struct(roomBuf.rbflags);

    roomBuf.rbgen            = 0;
    roomBuf.rbname[0]        = 0;   /* unnecessary -- but I like it...  */
    for (i = 0;  i < MSGSPERRM;  i++) {
        roomBuf.msg[i].rbmsgNo =  0l;
        roomBuf.msg[i].rbmsgLoc = 0 ;
    }

    printf("maxrooms=%d\n", MAXROOMS);

    for (thisRoom = 0;  thisRoom < MAXROOMS;  thisRoom++) {
        printf("clearing room %d\r", thisRoom);
        RoomSys(thisRoom);
        putRoom(thisRoom);
        noteRoom();
    }
    printf("\n");

    /* Lobby> always exists -- guarantees us a place to stand! */
    thisRoom            = 0             ;
    strCpy(roomBuf.rbname, baseRoom)    ;
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;
    RoomSys(0);

    putRoom(LOBBY);
    noteRoom();

    /* Mail> is also permanent...       */
    thisRoom            = MAILROOM      ;
    strCpy(roomBuf.rbname, "Mail")      ;
    RoomSys(1);
        /* Don't bother to copy flags, they remain the same (right?)    */
    putRoom(MAILROOM);
    noteRoom();

    /* Aide> also...                    */
    thisRoom            = AIDEROOM      ;
    strCpy(roomBuf.rbname, "Aide")      ;
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = FALSE;
    roomBuf.rbflags.INUSE    = TRUE;
    RoomSys(2);
    putRoom(AIDEROOM);
    noteRoom();

    return TRUE;
}

/************************************************************************/
/*      logInit() indexes ctdllog.sys                                   */
/************************************************************************/
void logInit()
{
    int i;
    int count = 0;

#ifdef RIGHT
    if (rewind(logfl) != 0) illegal("Rewinding logfl failed!");
#else
    rewind(logfl);
#endif
    /* clear logTab */
    for (i = 0; i < cfg.MAXLOGTAB; i++) logTab[i].ltnewest = 0l;

    /* load logTab: */
    for (thisLog = 0;  thisLog < cfg.MAXLOGTAB;  thisLog++) {
        cprintf("log#%3d", thisLog);
        getLog(&logBuf, thisLog);

        /* count valid entries:             */
        if (logBuf.lbflags.L_INUSE == 1) {
            count++;
            cprintf("   %s", logBuf.lbname);
        }
        else cprintf("   <not in use>");
        cprintf("\n");

        /* copy relevant info into index:   */
        logTab[thisLog].ltnewest = logBuf.lbvisit[0];
        logTab[thisLog].ltlogSlot= thisLog;
        if (logBuf.lbflags.L_INUSE == 1) {
            logTab[thisLog].ltnmhash = hash(logBuf.lbname);
            logTab[thisLog].ltpwhash = hash(logBuf.lbpw  );
        }
        else {
            logTab[thisLog].ltnmhash = 0;
            logTab[thisLog].ltpwhash = 0;
        }
    }
    printf(" logInit--%d valid log entries\n", count);
    printf("sortLog...\n");
    qsort(logTab, cfg.MAXLOGTAB, cfg.sizeLTentry, logSort);
}

/************************************************************************/
/*      logSort() Sorts 2 entries in logTab                             */
/************************************************************************/
int logSort(s1, s2)
LogTable *s1, *s2;
{
    if (s1->ltnmhash == 0 && s2->ltnmhash == 0)
        return 0;
    if (s1->ltnmhash == 0 && s2->ltnmhash != 0)
        return 1;
    if (s1->ltnmhash != 0 && s2->ltnmhash == 0)
        return -1;
    if (s1->ltnewest < s2->ltnewest)
        return 1;
    if (s1->ltnewest > s2->ltnewest)
        return -1;
    return 0;
}

/************************************************************************/
/*      noteLog() notes logTab entry in RAM buffer in master index      */
/************************************************************************/
void noteLog()
{
    int i, slot;

    /* figure out who it belongs between:       */
    for (i = 0;  logTab[i].ltnewest > logBuf.lbvisit[0];  i++);

    /* note location and open it up:            */
    slot = i;
    slideLTab(slot, cfg.MAXLOGTAB-1);

    /* insert new record */
    logTab[slot].ltnewest       = logBuf.lbvisit[0]  ;
    logTab[slot].ltlogSlot      = thisLog            ;
    logTab[slot].ltpwhash       = hash(logBuf.lbpw)  ;
    logTab[slot].ltnmhash       = hash(logBuf.lbname);
}

/************************************************************************/
/*      slideLTab() slides bottom N slots in logTab down.  For sorting. */
/************************************************************************/
void slideLTab(slot, last)
int slot;
int last;
{
    int i;

    /* open slot up: (movmem isn't guaranteed on overlaps) */
    for (i = last - 1;  i >= slot;  i--)  {
        movmem(&logTab[i], &logTab[i + 1], cfg.sizeLTentry);
    }
}

/************************************************************************/
/*      wrapup() finishes up and writes ctdlTabl.sys out, finally       */
/************************************************************************/
void wrapup(onlyParams)
char onlyParams;
{
    printf("\ncreating ctdlTabl.sys table\n");
    if (!onlyParams) {
        if (!msgZap)  msgInit();
        if (!roomZap) indexRooms();
        cfg.weAre = CITADEL;
        if (!logZap)  logInit();
        netInit();
        if (cfg.BoolFlags.netParticipant)
            fclose(netfl);
        fclose(roomfl);
        fclose(msgfl);
        fclose(logfl);
        printf("%d of the messages were Mail\n", mailCount);
    }
    printf("writeSysTab = %d\n", writeSysTab());
}

/************************************************************************/
/*      zapLogFile() erases & re-initializes userlog.buf                */
/************************************************************************/
char zapLogFile()
{
    int  i;

    printf("\nWipe out log file? ");
    if (toUpper(simpleGetch()) != 'Y')   return FALSE;
    printf("\n");

    /* clear RAM buffer out:                    */
    logBuf.lbflags.L_INUSE = FALSE;
    for (i = 0;  i < MAILSLOTS;  i++) {
        logBuf.lbMail[i].rbmsgLoc = 0l;
        logBuf.lbMail[i].rbmsgNo  = 0l;
    }
    for (i = 0;  i < NAMESIZE;  i++) {
        logBuf.lbname[i] = 0;
        logBuf.lbpw[i]   = 0;
    }

    /* write empty buffer all over file;        */
    for (i = 0; i < cfg.MAXLOGTAB;  i++) {
        printf("Clearing log #%d\n", i);
        putLog(&logBuf, i);
        logTab[i].ltnewest = logBuf.lbvisit[0];
        logTab[i].ltlogSlot= i;
        logTab[i].ltnmhash = hash(logBuf.lbname);
        logTab[i].ltpwhash = hash(logBuf.lbpw  );
    }
    return TRUE;
}

/************************************************************************/
/*      netInit() Initialize RAM index for net                          */
/************************************************************************/
void netInit()
{
    label temp;
    int i = 0;
    long length;

    if (!cfg.BoolFlags.netParticipant) return;
    totalBytes(&length, netfl);
    cfg.netSize = (int) (length / NB_TOTAL_SIZE);
    if (cfg.netSize)
        netTab = (NetTable *) GetDynamic(sizeof (*netTab) * cfg.netSize);
    else
        netTab = NULL;

    while (i < cfg.netSize) {
        getNet(i);
        netTab[i].netTRooms = (struct shared_room *) 
            GetDynamic(sizeof (*netBuf.netRooms) * SHARED_ROOMS);
        normId(netBuf.netId, temp);
        netTab[i].ntnmhash = hash(netBuf.netName);
        netTab[i].ntidhash = hash(temp);
        copy_struct(netBuf.nbflags, netTab[i].ntflags);
        movmem(netBuf.netRooms, netTab[i].netTRooms, 
                                sizeof *netBuf.netRooms * SHARED_ROOMS);
        netTab[i].ntMemberNets = netBuf.MemberNets;
        printf("System %3d. %s\n", i,
           (netBuf.nbflags.in_use) ? netBuf.netName : "<not in use>");
        i++;
    }
}

/************************************************************************/
/*      strCmpU() is strcmp(), but ignoring case distinctions           */
/************************************************************************/
int strCmpU(s, t)
char s[], t[];
{
    int  i;

    i = 0;

    while (toUpper(s[i]) == toUpper(t[i])) {
        if (s[i++] == '\0')  return 0;
    }
    return  toUpper(s[i]) - toUpper(t[i]);
}

/************************************************************************/
/*      crashout() fatal error, for library functions                   */
/************************************************************************/
void crashout(str)
char *str;
{
    illegal(str);
}

/************************************************************************/
/*      findMessage() gets all set up to do something with a message    */
/************************************************************************/
char findMessage(loc, id)
SECTOR_ID  loc;         /* sector in message.buf        */
MSG_NUMBER id;          /* unique-for-some-time ID#     */
{
    long atol();
    MSG_NUMBER here;
    extern struct mBuf mFile1;

    startAt(msgfl, &mFile1, loc, 0);

    do {
        getMessage();
        here = atol(msgBuf.mbId);
    } while (here != id &&  mFile1.thisSector == loc);

    return ((here == id));
}

void CheckFloors()
{
    SYS_FILE     tempName;
    FILE         *flrfl;
    struct floor FloorBuf;
    extern char  *R_W_ANY, *WRITE_ANY;

    makeSysName(tempName, "ctdlflr.sys", &cfg.floorArea);
    if ((flrfl = fopen(tempName, R_W_ANY)) == NULL) {
        printf(" %s not found, creating new file.\n", tempName);
        if ((flrfl = fopen(tempName, WRITE_ANY)) == NULL)
            illegal("?Can't create the floor file!");
    }
    strCpy(FloorBuf.FlName, cfg.codeBuf + cfg.MainFloor);
    FloorBuf.FlInuse = TRUE;
    fwrite(&FloorBuf, sizeof FloorBuf, 1, flrfl);
    totalBytes(&FloorSize, flrfl);
    FloorSize /= sizeof FloorBuf;
    fclose(flrfl);
}

#ifdef SERIES6
int checkForFossil()
{
 unsigned int fossilResult;
 int toReturn=TRUE;
 fossilResult=f_init(0);
 f_dtr(0,0);
 if (fossilResult!=6484) {
	toReturn=FALSE;
	fossilResult=f_init(1);
	f_dtr(1,0);
 	if (fossilResult!=6484) toReturn=FALSE;
    else toReturn=TRUE;
	}
 if (toReturn==TRUE) clrscr();
 return toReturn;
}
#endif