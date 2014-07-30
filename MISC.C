/************************************************************************/
/*                              misc.c                                  */
/*                                                                      */
/*      Random functions.                                               */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 88Dec23 <br> gutted internal protocols and removed all WXMODEM       */
/* 86Aug19 HAW  Kill history because of space problems.                 */
/* 84Jun10 JLS  Function changedate() installed.                        */
/* 84May01 HAW  Starting 1.50a upgrade.                                 */
/* 83Mar12 CrT  from msg.c                                              */
/* 83Mar03 CrT & SB   Various bug fixes...                              */
/* 83Feb27 CrT  Save private mail for sender as well as recipient.      */
/* 83Feb23      Various.  transmitFile() won't drop first char on WC... */
/* 82Dec06 CrT  2.00 release.                                           */
/* 82Nov05 CrT  Stream retrieval.  Handles messages longer than MAXTEXT.*/
/* 82Nov04 CrT  Revised disk format implemented.                        */
/* 82Nov03 CrT  Individual history begun.  General cleanup.             */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      ARCread()               reads the TOC of an SEA ARC file.       */
/*      calcrc()                calculates CRC                          */
/*      changeDate()            allow changing of date                  */
/*      CheckDLimit()           exceeded download time limit?           */
/*      civTime()               MilTime to CivTime                      */
/*      configure()             sets terminal parameters via dialogue   */
/*      crashout()              crashes out of Citadel in case of bug   */
/*      doFormatted()           for wildCard                            */
/*      doCR()                  newline on modem and console            */
/*      download()              menu-level routine for WC-protocol sends*/
/*      formRoom()              room prompt formatting                  */
/*      getCdate()              gets date from system clock.            */
/*      HelpIfPresent()         print help file if present              */
/*      ingestFile()            puts file in held message buffer        */
/*      lbyte()                 finds 0 byte of a string                */
/*      patchDebug()            display/patch byte                      */
/*      printDate()             prints out date                         */
/*      putBufChar()            .EWM/.EXM/.EWN/.EXN internal            */
/*      putFLChar()             readFile() -> disk file interface       */
/*      reconfigure()           Reconfigures a user                     */
/*      TranFiles()             Handles file transfers to users         */
/*      transmitFile()          send a host file, no formatting         */
/*      tutorial()              first level for printing a help file    */
/*      upLoad()                menu-level read-via-WC-protocol fn      */
/*      visible()               convert control chars to letters        */
/*      writeTutorial()         prints a .hlp file                      */
/*      readRoomInfoFile()      reads a room description file if present*/
/************************************************************************/

/************************************************************************/
/*                 External variable declarations in MISC.C             */
/************************************************************************/
char  *monthTab[13] = {"", "Jan", "Feb", "Mar",
                           "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep",
                           "Oct", "Nov", "Dec" };
char  *HumanMonthTab[13] = {"", "January", "February", "March",
                                "April", "May", "June", "July",
                                "August", "September", "October",
                                "November", "December" };
char   human;
FILE   *upfd;
int    masterCount;
int    acount;
int    byteRate;      /* Bytes/sec that modem is set for.     */
int    DirAlign = 0;
char   AlignChar, Profile=FALSE, RoomDescribe=FALSE;

char   *NoFileStr = "\n No %s.\n";
char   *who_str = "who";
char   *strFile = "filename";
char   *ALL_LOCALS  = "&L";
char   *R_SH_MARK   = "&&";
char   *LOC_NET     = "++";
char   *NON_LOC_NET = "%%";
char   *WRITE_LOCALS = "All Local Systems";
char   *APrivateRoom = "A Private Room";
char   *LCHeld = "log%d.hld";
char   longMessage = FALSE;
char   FormatFlag  = FALSE;
char   FileDescribe = FALSE;

char   hostDomainName[80];

long   Dl_Limit = -1l, End_Dl_Limit = -1l, DL_Total;   /* Blech */

PROTO_TABLE Table[] = {
        { NULL, (IS_NUMEROUS),
        	"ASCII", NULL, NULL, NULL,
             outMod, 1, NULL, NULL },

        { XTime, (RIGAMAROLE | IS_DL),
        	"Xmodem", "Xmodem", "wcdown.blb", "wcupload.blb",
             sendWCChar, SECTSIZE, NULL, XYClear },

        { YTime, (IS_NUMEROUS | RIGAMAROLE | IS_DL | NEEDS_FIN | NEEDS_HDR),
            "Ymodem BATCH", "Ymodem SINGLE", "ymdown.blb", "ymodemup.blb",
             outMod, YM_BLOCK_SIZE, NULL, XYClear },

        { XTime, (RIGAMAROLE | IS_DL), "Windowed-XModem",
            "Windowed-XModem", "wxdown.blb", "wxup.blb",
             outMod, SECTSIZE,NULL, NULL },

	    { YTime, (IS_NUMEROUS | RIGAMAROLE | IS_DL | NEEDS_FIN | NEEDS_HDR),
			"Zmodem", "Zmodem", "zmdown.blb", "zmupload.blb",
			 outMod, YM_BLOCK_SIZE, NULL, NULL },

	    { YTime, (IS_NUMEROUS | RIGAMAROLE | IS_DL | NEEDS_FIN | NEEDS_HDR),
			"Jmodem", "Jmodem", "jmdown.blb", "jmupload.blb",
			 outMod, YM_BLOCK_SIZE, NULL, NULL }

#ifdef QTEST
  	   ,{ YTime, (IS_NUMEROUS | RIGAMAROLE | IS_DL | NEEDS_FIN | NEEDS_HDR),
			"", "", "s1down.blb", "s1upload.blb",
			 outMod, YM_BLOCK_SIZE, NULL, NULL },

 	    { YTime, (IS_NUMEROUS | RIGAMAROLE | IS_DL | NEEDS_FIN | NEEDS_HDR),
			"", "", "s2down.blb", "s2upload.blb",
			 outMod, YM_BLOCK_SIZE, NULL, NULL },

 	    { YTime, (IS_NUMEROUS | RIGAMAROLE | IS_DL | NEEDS_FIN | NEEDS_HDR),
			"", "", "s3down.blb", "s3upload.blb",
			 outMod, YM_BLOCK_SIZE, NULL, NULL },

	    { YTime, (IS_NUMEROUS | RIGAMAROLE | IS_DL | NEEDS_FIN | NEEDS_HDR),
			"", "", "s4down.blb", "s4upload.blb",
			 outMod, YM_BLOCK_SIZE, NULL, NULL }



#endif
} ;

int fixVers = 285;
int majorVers = 69;

#define AUDIT   9000
char            audit[AUDIT];

/************************************************************************/
/*                 External variable definitions for MISC.C             */
/************************************************************************/
extern CONFIG    cfg;            /* Lots an lots of variables    */
extern paintBrush colTable;      /* the ANSI rainbow             */
extern logBuffer logBuf;         /* Person buffer                */
extern aRoom     roomBuf;        /* Room buffer                  */
extern rTable    *roomTab;
extern MSG_BUF   msgBuf;
extern MSG_BUF   tempMess;
extern NetBuffer netBuf;
extern int       outPut;
extern char             onConsole;
extern AN_UNSIGNED      crtColumn;      /* where are we on screen now?  */
extern char             loggedIn;       /* Is we logged in?             */
extern char             outFlag;        /* Output flag                  */
extern char             haveCarrier;    /* Do we still got carrier?     */
extern char             heldMess;
extern int              TransProtocol;  /* transfer protocol in use     */
extern char             prevChar;       /* previous char output         */
extern char             textDownload;   /* flag                         */
extern int              thisRoom;
extern int              thisLog;
extern char             whichIO;        /* Where I/O is                 */
extern char             echo;           /* Should we echo? echo? echo?  */
extern FILE             *msgfl;
extern FILE             *roomfl;
extern FILE             *logfl;
extern int              exitValue;
extern int              msgInCount;
extern char             PrintBanner, remoteSysop;
extern char  			callLogPosting[800];
extern char             alterNet;  /* FIDO importing stuff */
extern char             pausePromptFlag, mailTag, ansi, justVisiting;
extern char				aideCalledThis /* , messageCalledThis */;

#ifdef NO_PK
/************************************************************************/
/*      ARCread() reads the TOC of an SEA ARC file, displays it         */
/************************************************************************/
int ARCDir(char *fn)
/* char *fn; */
{
    extern char *READ_ANY;
    FILE        *fd;
    char        c;
    ARCbuf      buf;
    int         count = 0;
    long        compressed = 0l, realsize = 0l;

    mPrintf("\n %s", fn);
    if (FindFileComment(fn))
        mPrintf(": %s", strchr(msgBuf.mbtext, ' '));
    mPrintf("\n ");
    if ((fd = fopen(fn, READ_ANY)) == NULL) {
        mPrintf("Error!\n ");
        return TRUE;
    }
    do {
        fread(&c, 1, 1, fd);
        if (c != 0x1a) {
            mPrintf("\n Not an ARC file.\n ");
            break;
        }
        fread(&c, 1, 1, fd);
        if (c != 0) {
            if (fread(&buf, sizeof buf, 1, fd) <= 0) {
                mPrintf("\n Read failure!\n ");
                break;
            }
            else {
                count++;
                mPrintf("\n %-15s %7ld%8ld ", buf.name, buf.size, buf.length);
                        /* Parse out DOS style date/time */
                mPrintf("%d%s%02d",
                        ((buf.date & 0xfe00) >> 9) + 80,
                        monthTab[(buf.date & 0x1e0) >> 5],
                        buf.date & 0x1f);
                fseek(fd, buf.size, SEEK_CUR);
                compressed += buf.size;
                realsize   += buf.length;
            }
        }
    } while (c != 0);
    mPrintf("\n %-15s ------- -------", "");
    mPrintf("\n %5d %-10s%7ld%8ld\n ", count, "files", compressed, realsize);

    fclose(fd);
    return TRUE;
}
#endif

/************************************************************************/
/*      calcrc() Calculates CRC for a given block                       */
/************************************************************************/
unsigned int calcrc(ptr,count)
unsigned char *ptr;
int count;
{
  register unsigned int checksum;
  register int i;

  checksum=0;

    while (count--)
    {
      i=(checksum >> 8) & 0xff;
      i ^= *ptr++;
      i ^= i >> 4;
      checksum <<= 8;
      checksum ^= i;
      i <<= 5;
      checksum ^= i;
      i <<= 7;
      checksum ^= i;
    }
  return(checksum);
}

/************************************************************************/
/*      changedate() gets the date from the aide and remembers it       */
/************************************************************************/
void changeDate()
{
#ifndef ONLYTHENET
    int year, day, hours, minutes, mon;
    char *month;

    mPrintf("Date: %s\n ", formDate());
    getCdate(&year, &month, &day, &hours, &minutes);
    mPrintf("Time: %d:%02d\n ", hours, minutes);
    if (!getYesNo("Change date/time"))
        return ;

    do {
        year    = (int) getNumber("Year",  87l, 99l) + 1900;
        mon     = (int) getNumber("Month", 1l,  12l)       ;
        day     = (int) getNumber("Day",   1l,  31l)       ;
        hours   = (int) getNumber("Hour",   0l, 23l)       ;
        minutes = (int) getNumber("Minute", 0l, 59l)       ;
    } while (!setRawDate(year, mon, day, hours, minutes));
    InitEvents(FALSE);
#endif
}

/************************************************************************/
/*      CheckDLimit() Checks to see if the next d/l will exceed the time*/
/*      limit.                                                          */
/************************************************************************/
char CheckDLimit(long estimated)
/* long estimated; */
{
#ifndef ONLYTHENET
 char *problem;

 if (!aide && End_Dl_Limit!=-1l && DL_Total+estimated>=(Dl_Limit*60) ) {
	mPrintf("Not enough time.\n ");
    return FALSE;
    }
 else  if ((problem = ChkPreempt(estimated)) != NULL) {
    mPrintf("Conflicts with %s.\n ", problem);
    return FALSE;
  	}
 else if (End_Dl_Limit!=-1l) {
     doCR();

/* CtdlK2NE V5.17 */
     if (Dl_Limit != -1l) /* only show this if DLtime is limited */
	 	mPrintf("When done, %ld min. remain for downloads.",
					(Dl_Limit*60 - (DL_Total+estimated))/60 );


     }
 return TRUE;
#endif
}

/************************************************************************/
/*      civTime() Military time to Civilian time                        */
/************************************************************************/
void civTime(int *hours, char **which)
/* int *hours;
char **which; */
{
    if (*hours >= 12)
        *which = "pm";
    else
        *which = "am";
/* <br>
    if (*hours >= 13)
        *hours -= 12;
*/
	*hours = *hours % 12;
    if (*hours == 0)
        *hours = 12;
}

/************************************************************************/
/*      configure() sets up terminal width etc via dialogue             */
/************************************************************************/
void configure(char showVals)
/* char showVals; */
{
#ifndef ONLYTHENET
    termWidth   = (int) getNumber("# of characters your terminal\n screen can display on one line", 10l, 255l);
/******** new users will get NO nulls
 *  termNulls   = (int) getNumber(" #Nulls (normally 0)", 0l, 255l);
 ********/
	termNulls   = 0;   /* default - no screen pause prompt - K2NE */
    termLF      = getYesNo(" Do you need Linefeeds"      ) ? TRUE : FALSE;
    expert      = getYesNo(" Are you an experienced Citadel user")
                                                    ? TRUE : FALSE;
    if (expert) {
        sendTime = getYesNo(" Print time messages created") ? TRUE : FALSE;
        oldToo   = getYesNo(" Print last Old message on <N>ew Message request")
                                      ? TRUE : FALSE;

#ifdef HALFDUP
        if (showVals) HalfDup  =
             !getYesNo(" Full-duplex") ? TRUE : FALSE;
        else HalfDup = FALSE;
#endif

        FloorMode = getYesNo(" Floor mode");
    }
    else {
        oldToo = FALSE;
        sendTime = TRUE;
/*        FloorMode = HalfDup = FALSE; */
		FloorMode = FALSE;
    }
#endif
}

/************************************************************************/
/*      crashout() Problems?  Out we go!!!                              */
/*  Sometimes these problems are trivial - usually they are not.        */
/************************************************************************/
void crashout(char *message)
/* char *message; */
{
    FILE *fd;           /* Record some crash data */
    int  i;

    exitValue = CRASH_EXIT;
    outFlag = IMPERVIOUS;
    mPrintf("\n ERROR - TERMINATING.\n ");
    doCR();
    printf("\nRECONFIGURE!\n");
/*	printf("Contact Jersey Devil Citadel if you need more help.\n"); */
#ifdef K2NE_DEBUG
    printf("\nmsgfl  %d\nlogfl  %d\nroomfl %d\n",
            ferror(msgfl), ferror(logfl), ferror(roomfl));
#endif
    runHangup();
    logMessage(L_OUT, "", 0);
    logMessage(CRASH_OUT, "", 0);
    fclose(msgfl);
    fclose(roomfl);
    fclose(logfl);
    fd = safeopen("crash", "w");
    fprintf(fd, message);
    fclose(fd);
    fd = safeopen("audit", "w");
    for (i = 0; i < AUDIT; i++) {
        fputc(audit[i], fd);
        if ((i+1) % 70 == 0) fprintf(fd, "\n");
    }
    fprintf(fd, "\n\ncounter = %d\n", acount);
    fclose(fd);
/*    writeSysTab();   K2NE  */
    modemClose();
    systemShutdown();
    exit(exitValue);
}

/************************************************************************/
/*      doFormatted() does a tutorial for a wildCard call               */
/************************************************************************/
int doFormatted(char *fn)
/* char *fn; */
{
    tutorial(fn, FALSE);
}

/************************************************************************/
/*      doCR() does a newline on modem and console                      */
/************************************************************************/
void doCR()
{
    int i;
	extern char callLogFlag;
	extern int currLine;

    crtColumn   = 1;
    if (outFlag != OUTOK &&     /* output is being s(kip)ped    */
        outFlag != IMPERVIOUS)
        return;

    if (outPut == DISK)
    	fprintf(upfd, "\n");
    else {
		mputChar(NEWLINE);
        if (haveCarrier) {
            outMod('\r');
            if (termLF) outMod('\n');
	        }
        if (DirAlign != 0 && termWidth > 22)
            mPrintf("%*c%c ", DirAlign, ' ', AlignChar);
		currLine++;
    }
    prevChar    = ' ';
}

/************************************************************************/
/*      download() is the menu-level send-message-via-WC-protocol fn    */
/************************************************************************/
void download(whichMess, revOrder, protocol, allOrLocal, date, user, phrase)
char whichMess, revOrder, protocol, allOrLocal, *phrase;
label date, user;
{
	extern char jrnlFile[];
	int 		do_arc = FALSE;
    outFlag     = OUTOK;

    if (!expert && Table[protocol].BlbName != NULL)
        tutorial(Table[protocol].BlbName, TRUE);


    if (protocol != ASCII) {
        mPrintf("\nExtracting. Wait.\n ");
		strcpy(jrnlFile,"JOURNAL.CAP");
		if (!redirect(TRUE)) {
			outPut			= NORMAL;
			protocol		= ASCII;
			TransProtocol	= protocol;
		}
    }

	TransProtocol = protocol;

    if (whichMess != GLOBALnEW)
        showMessages(whichMess, revOrder, allOrLocal, date, user, phrase);
    else
        doGlobal(revOrder, allOrLocal, date, user, phrase);

    echo 			= BOTH;
	TransProtocol	= ASCII;

	if (protocol > ASCII) {
		fclose(upfd);
		outPut = NORMAL;
        if (protocol != 5) {
			if (do_arc = getYesNo("Compress before sending")) {
				system("pkarc u journal journal.cap");
				ScrNewUser();
				}
			}
        if (!getYesNo("Ready to begin transfer"))  return;
		doTempShell(FALSE, protocol, do_arc ? "JOURNAL.ARC" : "JOURNAL.CAP",
		            FALSE);
		unlink("JOURNAL.CAP");
		unlink("JOURNAL.ARC");
	}
    echo = BOTH;
    TransProtocol = ASCII;

    setUp(FALSE);
}

/************************************************************************/
/*      doGlobal() Does .R{W,X}G                                        */
/************************************************************************/
void doGlobal(revOrder, allOrLocal, date, user, phrase)
char revOrder;
char allOrLocal, *phrase;
label date, user;
{
#ifndef ONLYTHENET
    while (outFlag != OUTSKIP && gotoRoom("", 'R') &&
                (gotCarrier() || onConsole)) {
        givePrompt();
        mPrintf("read new\n ");
        showMessages(NEWoNLY, revOrder, allOrLocal, date, user, phrase);
    }
#endif
}

/************************************************************************/
/*      formHeader() returns a string with the msg header formatted     */
/************************************************************************/
char *formHeader()
{
#ifndef ONLYTHENET
    static char header[100];
	static char Vheader[200];
    static char numberHeader[15];
	Vheader[0] = 0;     /* for verbose headers      */
    header[0] = 0;      /* Initialize the genie.... */

    if (msgBuf.mbdate[ 0]) {
		 sPrintf(numberHeader, "[%d] ", msgInCount+1);
		 sPrintf(lbyte(header),	" %s%s ",
            logBuf.lbflags.lflag8 == FALSE ? numberHeader : "",
	        msgBuf.mbdate);
/*		 sPrintf(lbyte(Vheader), " MSG#: %d\n DATE: %s ",
			msgInCount+1, msgBuf.mbdate); */
		 if (logBuf.lbflags.lflag8 == FALSE)
			sPrintf(lbyte(Vheader), " MSG#: %d\n", msgInCount+1);
         sPrintf(lbyte(Vheader), " DATE: %s ", msgBuf.mbdate);
		 }
    if (msgBuf.mbtime[ 0] && sendTime) {
         sPrintf(lbyte(header), "%s ", msgBuf.mbtime);
         sPrintf(lbyte(Vheader), " TIME: %s ", msgBuf.mbtime);
		 }
    if (msgBuf.mbauth[ 0]) {
        sPrintf(lbyte(header), "from %s",    msgBuf.mbauth );
        sPrintf(lbyte(Vheader), "\n FROM: %s",    msgBuf.mbauth );
		}
    if (msgBuf.mboname[0]) {
		sPrintf(lbyte(header), " @%s", msgBuf.mboname);
 		sPrintf(lbyte(Vheader), "\n   AT: %s", msgBuf.mboname);
		}
    if (strCmpU(msgBuf.mbroom, roomBuf.rbname) != SAMESTRING) {
        strCat(header, " in ");
		strCat(Vheader, "\n   IN: ");
        if (roomExists(msgBuf.mbroom) != ERROR) {
            sPrintf(lbyte(header), formRoom(roomExists(msgBuf.mbroom), FALSE,
                                                                FALSE));
            sPrintf(lbyte(Vheader), formRoom(roomExists(msgBuf.mbroom), FALSE,
                                                                FALSE));
			}
        else {
            sPrintf(lbyte(header), "%s>", msgBuf.mbroom);
			sPrintf(lbyte(Vheader), "%s>", msgBuf.mbroom);
			}
    }

    if (msgBuf.mbto[   0]) {
		 sPrintf(lbyte(header), " to %s", msgBuf.mbto);
		 sPrintf(lbyte(Vheader), "\n   TO: %s", msgBuf.mbto);
		 }
    if (msgBuf.mbaddr[ 0] &&
            strCmpU(msgBuf.mbaddr, R_SH_MARK  ) != SAMESTRING &&
            strCmpU(msgBuf.mbaddr, LOC_NET    ) != SAMESTRING &&
            strCmpU(msgBuf.mbaddr, NON_LOC_NET) != SAMESTRING) {
        sPrintf(lbyte(header), " (on %s)", strCmpU(msgBuf.mbaddr, ALL_LOCALS) ?
                                        msgBuf.mbaddr : "All Local Systems");
		sPrintf(lbyte(Vheader), "\n   ON: %s", strCmpU(msgBuf.mbaddr, ALL_LOCALS) ?
                                        msgBuf.mbaddr : "All Local Systems");
			}

	if (logBuf.lbflags.lflag5) {
/*		sPrintf(lbyte(Vheader), "\n "); */
		return Vheader;
		}
	else
	    return header;
#endif
}

/************************************************************************/
/*      lbyte() finds 0 byte of a string, returns pointer to it...      */
/************************************************************************/
char *lbyte(char *l)
/* char *l; */
{
    while (*l) l++;
    return l;
}

/************************************************************************/
/*      formRoom() returns a string with the room formatted             */
/************************************************************************/
char *formRoom(int roomNo, int showPriv, int noDiscrimination)
/* int roomNo, showPriv, noDiscrimination; */
{
#ifndef ONLYTHENET
    static char display[40];
    int         one, two;
    static char matrix[2][2] =
      {  { '>', ')' } ,
         { ']', ':' } } ;

    one = roomTab[roomNo].rtflags.ISDIR;
    two = roomTab[roomNo].rtflags.SHARED;
    if (roomTab[roomNo].rtflags.INUSE) {
        if (!noDiscrimination &&
            !roomTab[roomNo].rtflags.PUBLIC)
            strCpy(display, APrivateRoom);
        else {
            sPrintf(display, "%s%c%s%s",
                roomTab[roomNo].rtname,
                matrix[one][two],
                (!roomTab[roomNo].rtflags.PUBLIC && showPriv) ? "*" : "",
                (roomTab[roomNo].rtflags.ALTER_NET==TRUE) ? "!" : "");
        }
    }
    else display[0] = '\0';
    return display;
#endif
}

/************************************************************************/
/*      getCdate() retrieves system date and returns in the parameters  */
/************************************************************************/
void getCdate(int *year, char **month, int *day, int *hours, int *minutes)
/* int *year, *day, *hours, *minutes;
char **month; */
{
	int mon, seconds, milli;

    getRawDate(year, &mon, day, hours, minutes, &seconds, &milli);
    *year -= 1900;
    *month = monthTab[mon];
    if (human) *month = HumanMonthTab[mon];
}

/************************************************************************/
/*      HelpIfPresent() print help file if present                      */
/************************************************************************/
char HelpIfPresent(char *filename)
/* char *filename; */
{
    SYS_FILE fn;

    makeSysName(fn, filename, &cfg.homeArea);
    if (access(fn, 4) == 0) {
        tutorial(filename, TRUE);
        return TRUE;
    }
    else return FALSE;
}

/************************************************************************/
/*      ingestFile() Puts the given file in the held msg buffer         */
/************************************************************************/
void ingestFile(char *name)
/* char *name; */
{
#ifndef ONLYTHENET
    char  filename[100];        /* Paths, etc.... */
    char  fidoString[100];      /* a throwaway for PATH */
    char  checkFidoDir[100];    /* another one */
    char  foundPlace, fidoRoom[5];
	int   fidoRoomNr;

    FILE  *fd, *fidoDirList;
    int   c, index;
    extern char *READ_TEXT;

    foundPlace=FALSE;
    if (name == NULL)
        getNormStr(strFile, filename, 99, ECHO);
    else
        strCpy(filename, name);

    if ((fd = safeopen(filename, READ_TEXT)) == NULL) {
        mPrintf(NoFileStr, filename);
        return;
    }
    index = (heldMess) ? strLen(tempMess.mbtext) : 0;
	if (alterNet==TRUE) {
		fgets(fidoString, 100, fd);
        fidoDirList=fopen("linkdirs.sys", "r+t");
		while (!feof(fidoDirList)) {
			fgets(checkFidoDir, 100, fidoDirList);
			if (stricmp(checkFidoDir+5, fidoString)==0) {
				foundPlace=TRUE;
				strncpy(fidoRoom, checkFidoDir, 3);
                fidoRoom[3]='\0';
				fidoRoomNr=atoi(fidoRoom);
				if (thisRoom!=fidoRoomNr)
					gotoRoom(roomTab[fidoRoomNr].rtname, 'R');
                }
			if (foundPlace==TRUE) break;
			}
		fclose(fidoDirList);
        if (foundPlace==FALSE) {
			mPrintf("\n Can't find linked room\n");
			alterNet=FALSE;
			return;
			}
		rewind(fd);
		fseek(fd, strlen(fidoString), 0);

		}
    while ((c = getc(fd)) != EOF && index < MAXTEXT - 2) {
        if (c) tempMess.mbtext[index++] = c;
	    }
    tempMess.mbtext[index] = 0;
    fclose(fd);
    heldMess = TRUE;
#endif
}

/************************************************************************/
/*      formDate() forms the current date.                              */
/*      If 'human' is true, we return 'Month dd, 19yy'                  */
/*      otherwise the traditional Citadel 'yyMondd' is returned.        */
/************************************************************************/
char *formDate()
{
    static char dateLine[40];
    int  day, year, h, m;
    char *month;

    getCdate(&year, &month, &day, &h, &m);
    if (human) sPrintf(dateLine, "%s %d, 19%02d", month, day, year);
	else sPrintf(dateLine, "%d%s%02d", year, month, day);
	human=FALSE;
    return dateLine;
}

/************************************************************************/
/*      putBufChar() is used to upload messages via protocol            */
/*      returns: ERROR on problems else TRUE                            */
/************************************************************************/
int putBufChar(int c)
/* int  c; */
{
    char result;

    if (masterCount == MAXTEXT + 10) return TRUE;

    if (masterCount > MAXTEXT - 2) return ERROR;

        /* This is necessary for a ProComm bug */
    if (c == CPMEOF) {
        masterCount = MAXTEXT + 10;
        return TRUE;
    }
	if (c != 0x0D) {
		    c &= 0x7F;    /* strip high bit       */
		    result = cfg.filter[c];
			}
	if (c == 10 || c == 13) result = '\n';
    if (result == '\0') {
        return TRUE;
    }
    msgBuf.mbtext[masterCount++] = result;
    msgBuf.mbtext[masterCount]   = 0;   /* EOL just for luck    */
    return TRUE;
}

/************************************************************************/
/*      putFLChar() is used to upload files                             */
/*      returns: ERROR on problems else TRUE                            */
/************************************************************************/
int putFLChar(int c)
/* int c; */
{
    if (fputc(c, upfd) != EOF) return TRUE;
    /* else */                 printf("Write error: %d\n", ferror(upfd));
                               return ERROR;
}

/************************************************************************/
/*      reconfigure() Reconfigures a user                               */
/************************************************************************/
void reconfigure()
{
#ifndef ONLYTHENET
    char  *ON  = "ON";
    char  *OFF = "OFF";
    char userDataLine[255];
    label temp;
    int   netSpot;
    FILE *userFile;

    switch (toUpper(iChar())) {
    case 'A':           /* Forwarding address on the network */
        mPrintf("ddress");

        if (!cfg.BoolFlags.netParticipant) {
            mPrintf("\n Net disabled.\n ");
            break;
        }

        if (!loggedIn) {
            mPrintf("\n Log in!\n ");
            break;
        }

        if (!logBuf.lbflags.NET_PRIVS) {
            mPrintf("\n No net privs.\n ");
            break;
        }

        do {
            getString("system to forward Mail> to", temp, NAMESIZE, TRUE, TRUE);
            if (temp[0] == '?')
                writeNet(FALSE);
            else if (strLen(temp) == 0 && logBuf.lbfwd != -1) {
                if (getYesNo("Stop forwarding Mail>")) {
                    logBuf.lbfwd = -1;
                    break;
                }
            } else if (strLen(temp) != 0) {
                if ((netSpot = searchNameNet(temp)) == ERROR) {
                    mPrintf("'%s' not found.\n ", temp);
                }
                else {
                    getNet(netSpot);
                    logBuf.lbfwd = netSpot;
                    logBuf.lbNetGen = netBuf.nbGen;
/*                    if (!netBuf.nbflags.local && logBuf.credit == 0) */
                    if (!netBuf.nbflags.local && checkLDcredit() == 0)
                        mPrintf(
             "NOTE: '%s' is LD - you have no LD credits.\n ",
                                                        temp);
                    break;
                }
            }
            else break;
        } while (TRUE);
        break;
    case 'C':
        mPrintf("omplete Reconfigure");
        configure(FALSE);
        break;
    case 'E':
        mPrintf("xpert\n Now %s.\n ",
                        (expert = !expert) ? ON : OFF);
        break;
    case 'F':
        mPrintf("loor mode\n Now %s.\n ",
                        (FloorMode = !FloorMode) ? ON : OFF);
        break;

#ifdef HALFDUP
    case 'H':
        mPrintf("alf-duplex mode\n Now %s.\n ",
                        (HalfDup = !HalfDup) ? ON : OFF);
        break;
#endif

    case 'L':
        mPrintf("inefeeds\n Now %s.\n ",
                        (termLF = !termLF) ? ON : OFF);
        break;
/* #ifdef NEEDS_NULLS */
    case 'S':
        mPrintf("creen pause ('0' defeats)");
        termNulls   = (int) getNumber("# of lines before pausing", 0l, 50l);
        break;
/* #endif */
    case 'O':
        mPrintf("ld messsage on new\n Now %s.\n ",
                        (oldToo = !oldToo) ? ON : OFF);
        break;
    case 'R':
		mPrintf("egistration");

        sprintf(userDataLine, "ctdluser.%d", thisLog);
        if (access(userDataLine, 0)!=-1) {
#ifdef NEEDED
			showUserProfile(thisLog);
#else
            userFile=fopen(userDataLine, "rt");
			fgets(userDataLine, 255, userFile);
			doCR();
			mPrintf("Info is now P%s.\n ",strnicmp(userDataLine,"Y",1)==SAMESTRING ? "UBLIC" : "RIVATE");
			doCR();
            while (!feof(userFile)) {
				fgets(userDataLine, 255, userFile);
				mPrintf(userDataLine);
				}
			fclose(userFile);
			doCR();
#endif
			if (!getYesNo("Is this ok")) doRegister(thisLog);
			break;
			}
		doRegister(thisLog);
		break;
    case 'T':
        mPrintf("ime of messages\n Now %s.\n ",
                        (sendTime = !sendTime) ? ON : OFF);
        break;
    case 'U':
		mPrintf("nlisted flag now %s.\n ",
          (logBuf.lbflags.lflag9=!logBuf.lbflags.lflag9) ? ON : OFF);
        break;
	case 'G':
		mPrintf("\bGraphics mode\n Now %s.\n ",
			(logBuf.lbflags.lflag3 = !logBuf.lbflags.lflag3) ? "ANSI" : "text");
#ifdef ANSI /* AB_ADD Needed to correct graphics mode when toggled on/off */
			if (!logBuf.lbflags.lflag3) resetAnsiScreen();
#endif
		break;
	case 'P':
		mPrintf("\bPause between messages\n Now %s.\n ",
			(logBuf.lbflags.lflag4 = !logBuf.lbflags.lflag4) ? ON : OFF);
		break;
	case 'M':
		mPrintf("\bShow message #s\n Now %s.\n ",
			(logBuf.lbflags.lflag8 = !logBuf.lbflags.lflag8) ? OFF : ON);
        break;
	case 'V':
		mPrintf("\bVerbose Headers\n Now %s.\n ",
			(logBuf.lbflags.lflag5 = !logBuf.lbflags.lflag5) ? ON : OFF);
		break;
    case 'W':
        mPrintf("idth of screen\n");
/*      termWidth   = (int) getNumber(" Screen width", 10l, 255l); */
	    termWidth   = (int) getNumber("# of characters your terminal\n screen can display on one line", 10l, 255l);
        break;
    case '\r':
    case '\n':
    case '?':
        tutorial("confg.mnu", TRUE);
        mPrintf("\n Your current setup:\n ");
        shrtColor(colTable.level1 /* A_RED */);
        mPrintf("%s, ", (expert) ? "Expert" : "Non-expert");

#ifdef NEEDS_NULLS
        mPrintf("%sinefeeds, %d nulls,",
            termLF     ?  "L" : "No l",
            termNulls);
#else
        mPrintf("%sinefeeds,", termLF ? "L" : "No l");
#endif

        mPrintf(" screen width: %d.\n ", termWidth);
		if (termNulls>0) mPrintf("Pause after %d lines.\n ", termNulls);
        mPrintf("Msg. headers: %s.\n ",
					logBuf.lbflags.lflag5 ? "verbose" : "terse");
		mPrintf("Msg. #s: %s.\n ",
	                logBuf.lbflags.lflag8 ? "OFF" : "ON");
        mPrintf("%srint time messages created.\n ",
                        sendTime ? "P" : "Don't p");
        mPrintf("%srint last Old msg on <N>ew Message request.",
                        oldToo ? "P" : "Don't p");
        mPrintf("\n Graphics mode: %s.",
					logBuf.lbflags.lflag3 ? "ANSI" : "text");
		mPrintf("\n Display will%s pause between messages.",
        			logBuf.lbflags.lflag4 ? "" : " not");
        mPrintf("\n Your name is %sLISTED in the user-list.",
					logBuf.lbflags.lflag9 ? "NOT " : "");
/*        if (HalfDup) mPrintf("\n Using Half-Duplex mode."); */
        if (FloorMode) mPrintf("\n FLOOR mode.");
        if (logBuf.lbfwd != -1 && cfg.BoolFlags.netParticipant) {
            getNet(logBuf.lbfwd);
            if (logBuf.lbNetGen != netBuf.nbGen)
                logBuf.lbNetGen = -1;   /* Something changed, so cancel */
            else
                mPrintf("\n Forward Mail> to %s.", netBuf.netName);
        }
        shrtColor(colTable.level0 /* A_GREEN */);
/*      mPrintf("\n "); */
        return;
    default:
        mPrintf(" ? ('?' for menu)\n");
        return;
    }
#endif
}

/************************************************************************/
/*      SaveInterrupted() Save an interrupted message                   */
/************************************************************************/
void SaveInterrupted(SomeMsg)
MSG_BUF *SomeMsg;
{
#ifndef ONLYTHENET
    SYS_FILE temp;
    extern char *LCHeld, *WRITE_ANY;
    SYS_FILE save_mess;
    FILE *savefile;

    if (cfg.BoolFlags.HoldOnLost) {
        sPrintf(temp, LCHeld, thisLog);
        makeSysName(save_mess, temp, &cfg.holdArea);
        if (access(save_mess, 0) == -1) {
            if ((savefile = safeopen(save_mess, WRITE_ANY)) == NULL)
                printf("Save ERROR!\n");
            else {
#ifndef NO_CRYPT
                crypte(SomeMsg, sizeof *SomeMsg, thisLog);
#endif
                fwrite(SomeMsg, sizeof *SomeMsg, 1, savefile);
#ifndef NO_CRYPT
                crypte(SomeMsg, sizeof *SomeMsg, thisLog);
#endif
                fclose(savefile);
            }
        }
    }
#endif
}

/************************************************************************/
/*      TranFiles() handles transfer of files to users:                 */
/*      1. Gets number of files, number of bytes.                       */
/*      2. Performs time calculations.                                  */
/*      3. Starts up protocols.                                         */
/************************************************************************/
char downloadFile[100];

void TranFiles(int protocol, char *phrase)
/* int protocol;
char *phrase; */
{
    int  NumFiles;
    extern unsigned long netBytes;
    char FileSpec[100];

    getNormStr(protocol != 1 ? "filename(s)" : "filename",
				FileSpec, 99, ECHO);
    netBytes = 0l;
    NumFiles = wildCard(getSize, FileSpec, TRUE, phrase);

    if (NumFiles <= 0) {
        return;
		}

	if (!TranAdmin(protocol, NumFiles)) return;
  	if (protocol == ASCII) {
	    TranSend(ASCII, transmitFile, FileSpec, phrase, TRUE);
	} else {
	    startTimer(0);
		doTempShell(FALSE, protocol, FileSpec, TRUE);
		DL_Total += chkTimeSince(0); /* running total of user DL time */

		sPrintf(callLogPosting,			/* We don't really care about ".rt[f]" */
				"File \"%s\" downloaded from %s by %s [%s transfer]",
				FileSpec, formRoom(thisRoom, FALSE, FALSE),
				logBuf.lbname, Table[protocol].name);

	   	logMessage(19,"",FALSE);   /* K2NE -- See, easy! */
    }
}

/************************************************************************/
/*      TranAdmin() Transfer file administrator                         */
/************************************************************************/
char TranAdmin(int protocol, int NumFiles)
/* int protocol, NumFiles; */
{
    long seconds;
    extern unsigned long netBytes;

    if (Table[protocol].flags & RIGAMAROLE) {
        mPrintf("This %s transfer involves %ld bytes",
                                Table[protocol].name, netBytes);

        if (Table[protocol].flags & IS_NUMEROUS)
            mPrintf(" in %d file%s", NumFiles, NumFiles == 1 ? "" : "s");

#ifdef USE_IT_ANYWAY
        if (protocol < 4)
			mPrintf(" (%ld blocks).  ",

((netBytes+(Table[protocol].BlockSize-1))/Table[protocol].BlockSize));

        else
#endif

		mPrintf(". ");

        if (Table[protocol].TimeCalc != NULL && byteRate != 0) {
            (*Table[protocol].TimeCalc)(netBytes, &seconds);
            if (!CheckDLimit(seconds)) return FALSE;
            mPrintf("\n Duration: %ld min %2ld sec",
							seconds/60, seconds % 60);
	        }

        return getYesNo("Ready");
    }
    return TRUE;
}

/************************************************************************/
/*      TranSend() Does send work of TranFiles().                       */
/************************************************************************/
void TranSend(protocol, fn, FileSpec, phrase, NeedToMove)
char *FileSpec, *phrase, NeedToMove;
int protocol, (*fn)(char *fn);
{
    TransProtocol = protocol;
    startTimer(0);

    if (FormatFlag) {
        wildCard(doFormatted, FileSpec, NeedToMove, phrase);
        FormatFlag = FALSE;
    }
    else {
        wildCard(fn, FileSpec, NeedToMove, phrase);
    }


    if (Table[protocol].flags & IS_DL)
        DL_Total += chkTimeSince(0);

    TransProtocol = ASCII;
    if (Table[protocol].flags & RIGAMAROLE)
        oChar(BELL);
}

/************************************************************************/
/*      transmitFile() dumps a host file with no formatting             */
/************************************************************************/
int transmitFile(char *filename)
/* char *filename; */
{
    FILE *fbuf;
    int  c;
    long fileSize = 0l;
    extern char *READ_ANY;

    if (strLen(filename) != 0) {
        if ((fbuf = safeopen(filename, READ_ANY)) == NULL) {
            return(ERROR);
        }
        totalBytes(&fileSize, fbuf);
        if (Table[TransProtocol].flags & RIGAMAROLE)
            printf("%s: %s (%ld bytes, %ld blocks)\n",
                Table[TransProtocol].name, filename, fileSize,
((fileSize+(Table[TransProtocol].BlockSize-1))/Table[TransProtocol].BlockSize));
    }

    if (strLen(filename) != 0) {
        while ((c = getc(fbuf)) != ERROR && (c != CPMEOF || !textDownload))  {
			mputChar(c);
			if (gotCarrier())
				if (!outMod(c))
					break;
            if (mAbort() || (whichIO == MODEM && !gotCarrier()))
            	break;
        }

        fclose(fbuf);
    }
    return TRUE;
}

/************************************************************************/
/*      tutorial() prints file <filename> on the modem & console        */
/*      Returns:        TRUE on success else ERROR                      */
/************************************************************************/
char tutorial(char *filename, char addHelpArea)
/* char *filename;
char addHelpArea; */
{
    FILE     *fbuf;
    int      toReturn;
    SYS_FILE fn;
    extern char *READ_TEXT, ourHomeSpace[100];

    toReturn    = TRUE;
    if (addHelpArea == 3) {
        makeSysName(fn, filename, &cfg.netArea);
        if ((fbuf = safeopen(fn, READ_TEXT)) == NULL) {
            mPrintf(NoFileStr, filename);
			return   ERROR;    /* toReturn = ERROR; */
			}
		else
            {
			fclose (fbuf);
			transmitFile(fn);
			return	toReturn;
			}
        }

    else if (addHelpArea == 2)
        makeSysName(fn, filename, &cfg.call_log);
	else if (addHelpArea)
        makeSysName(fn, filename, &cfg.homeArea);
    else
        strCpy(fn, filename);

    if ((fbuf = safeopen(fn, READ_TEXT)) == NULL) {
        mPrintf(NoFileStr, filename);
        toReturn        = ERROR;
    } else {
        writeTutorial(fbuf, TRUE);
        fclose(fbuf);
    }

    return   toReturn;
}

/* int askForCRC = FALSE; */

/************************************************************************/
/*      upLoad() enters a file into current directory                   */
/************************************************************************/
char	uploadFile[NAMESIZE], uploadRoom[30];

void upLoad(char protocol)
/* char protocol; */
{
    char fileName[NAMESIZE];
    char *tmp, *q;
    char successful, oldExpert=FALSE;
    extern char *READ_TEXT, *WRITE_ANY;

    getNormStr(strFile, fileName, NAMESIZE, ECHO);
    if (strLen(fileName) == 0) return;

    if (strchr(fileName, ' ') != NULL || /* Bad file name -- bad, bad, bad! */
        strchr(fileName, ':') != NULL ||
        strchr(fileName, '\\') != NULL  ) {
        mPrintf("Bad file name.\n ");
        return ;
    }

    if (!setSpace(&roomBuf)) {          /* System error -- yucky. */
        return ;
    }

    if ((upfd = safeopen(fileName, READ_TEXT)) != NULL) {
                                    /* File already exists */
        mPrintf("\n A %s already exists.\n", fileName);
        fclose(upfd);
    } else {                    /* Go for it */

/*        getNormStr("a short description of the file",
                              msgBuf.mbtext, 500, ECHO); */

		mPrintf("\n Now describe the file:");
	    zero_struct(msgBuf.mbtext);
 		FileDescribe=TRUE;
 		oldExpert=expert;
 		expert=TRUE;
 		getText(ASCII);
 		expert=oldExpert;
 		FileDescribe=FALSE;

        if (!expert) {
            homeSpace();
            tutorial(Table[protocol].UpBlbName, TRUE);
            setSpace(&roomBuf);
        	}
        if ((upfd = safeopen(fileName, WRITE_ANY)) == NULL) {
            mPrintf("\n Can't create %s!\n", fileName);
        	}
		else {
		    fclose(upfd);
			unlink(fileName);
            if (getYesNo("Ready for transfer")) {
				if( protocol != ASCII) {
					doTempShell(TRUE, protocol, fileName, FALSE);
				}

        		if ((upfd = safeopen(fileName, READ_TEXT)) != NULL) {
					fclose(upfd);
	                tmp = GetDynamic(strLen(msgBuf.mbtext) + 1);
    	            strCpy(tmp, msgBuf.mbtext);
                    msgBuf.mbtext[strlen(msgBuf.mbtext)-1]='\0';
					sPrintf(callLogPosting, "%s [Uploaded by %s]",
						msgBuf.mbtext, logBuf.lbname);
        	        updFiletag(fileName, callLogPosting);
        	        homeSpace();
            	    zero_struct(msgBuf);
                	q = msgBuf.mbtext + sPrintf(msgBuf.mbtext,
                	        "File \"%s\" uploaded into %s by %s.",fileName,
                	         formRoom(thisRoom, FALSE, FALSE), logBuf.lbname);
    	            aideMessage(FALSE);
    	            sPrintf(q,"\n %s",tmp);
    	            putMessage();
					sPrintf(callLogPosting,
						"File \"%s\" uploaded into %s by %s [%s transfer]",
					fileName, formRoom(thisRoom, FALSE, FALSE), logBuf.lbname,
						Table[protocol].name);
	                logMessage(19,"",FALSE);
	                noteMessage(NULL, ERROR);
    	            noteRoom();
					strcpy(uploadFile,fileName);  /* K2NE */
            	    free(tmp);
            	}
			}
        }
    }
	homeSpace();
}
#ifdef NEED_VISIBLE
/************************************************************************/
/*      visible() converts given char to printable form if nonprinting  */
/************************************************************************/
char visible(c)
AN_UNSIGNED   c;
{
    if (c==0xFF)  c = '$'       ;   /* start-of-message in message.buf  */
    c               = c & 0x7F  ;   /* kill high bit otherwise          */
    if ( c < ' ') c = c + 'A' -1;   /* make all control chars letters   */
    if (c== 0x7F) c = '~'       ;   /* catch DELETE too                 */
    return(c);
}
#endif

/************************************************************************/
/*      writeTutorial() given a file, prints it                         */
/************************************************************************/
void writeTutorial(FILE *fd, char noviceWarning)
/* FILE *fd;
char noviceWarning; */
{
    char line[MAXWORD];
	extern char MenuFlag;

    rewind(fd);

    if (outFlag != IMPERVIOUS) outFlag     = OUTOK;
    if (!expert && noviceWarning) mPrintf("\n <J>ump <P>ause <S>top\n");
    if (MenuFlag==FALSE) mPrintf(" \n");
    while (fgets(line, MAXWORD, fd) && outFlag != OUTSKIP) {
		if (MenuFlag) {
			if (ansi) cprintf("%s", line);
			else printf("%s", line);
			}
        else {
			if (pausePromptFlag==TRUE) testPausePrompt();
			mPrintf("%s", line);
			}
		}
	MenuFlag = FALSE;
    if (noviceWarning && !PrintBanner) outFlag     = OUTOK;
}

void XTime(size, seconds)
long size, *seconds;
{
    *seconds = (long) 13 * size / (byteRate * 10);
}

void YTime(size, seconds)
long size, *seconds;
{
    *seconds = (long) 11 * size / (byteRate * 10);
}

int checkLDcredit()
{
 return logBuf.credit;
}

testPausePrompt()
{
 extern int currLine;

 if (currLine>termNulls-2 && termNulls != 0) {
	mPrintf("-More-");
	longMessage=TRUE;
	iChar();
	currLine=0;
    if (haveCarrier) outMod('\r');
	printf("\r");
	currLine=0;
	return TRUE;
	}
 return FALSE;
}

/*
 * The inspiration for the following code, which handles the generation
 * and/or modification of ROOM DESCRIPTION files was completely that of
 * Steve Williams ("Patriot").  The code may be mine, but the idea was
 * his, and it was a damned good one!  (Added for V6.04)
 */

/*  readRoomInfoFile()    reads room info file if present */

readRoomInfoFile() {

FILE *roomInfoFile;
char buildFileName[20];
int  roomNumber;

if (!justVisiting) 	mPrintf(" for Room\n ");
roomNumber=thisRoom;
sprintf(buildFileName, "roominfo.%d", roomNumber);

if (access(buildFileName, 00)== -1) {
	if (!justVisiting)
		mPrintf("No description found.\n ");
	return;
	}

tutorial(buildFileName, FALSE);

if (strLen(roomBuf.rbmoderator)) {
    doCR();
	doCR();
    mPrintf("Room Moderator is %s.", roomBuf.rbmoderator);
	}
doCR();
if (justVisiting) doCR();
return;
}


/* editRoomInfoFile()  edit room info or create if not there */

editRoomInfoFile() {

FILE *roomInfoFile;
char buildFileName[20];
int  roomNumber, wasExpert;

roomNumber=thisRoom;

sprintf(buildFileName, "roominfo.%d", roomNumber);
 if (access(buildFileName,0)!=-1) {
	mPrintf("Current Description:");
	doCR();
	tutorial(buildFileName,FALSE);
    if (!getYesNo("Enter new description"))  return;
	}

 mPrintf("\n Enter description now:\n ");
 zero_struct(msgBuf.mbtext);
 RoomDescribe=TRUE;
 wasExpert=expert;
 expert=TRUE;
 if (getText(ASCII)) {
     unlink(buildFileName);
	 roomInfoFile=fopen(buildFileName,"wt");
	 fprintf(roomInfoFile, "%s", msgBuf.mbtext);
	 fclose(roomInfoFile);
	 }
 expert=wasExpert;
 RoomDescribe=FALSE;
 return;
}

/*
 * readHelpFileList()
 *    Reads the list of *.HLP files from the HELPAREA directory,
 *    eliminating the need for a HELPOPT.HLP file, and making the
 *    help index completely auto-maintaining.  One less job for the
 *    lazy sysops among us!  As aren't we all?  (V6.05)
 */

/*#ifdef NEW*/

readHelpFileList()
{
 struct ffblk ffblk;
 int done;
 char currentHelp[12];
 SYS_FILE name;

 mPrintf("HELP topics available:");
 doCR();
 doCR();

 makeSysName(name, "*.hlp", &cfg.homeArea); /* set up filemask to scan */

 done=findfirst(name,&ffblk,0);
 while (!done) {
	sprintf(currentHelp, ffblk.ff_name);
    currentHelp[strlen(currentHelp)-4]=0;
	mPrintf("%-15s",currentHelp);
	done=findnext(&ffblk);
	}
 doCR();
 doCR();
 mPrintf("Type \".Help TOPIC\" for individual items.");
 doCR();
}

/*#endif*/

showUserProfile(int logPlace)
{
 char userDataLine[255];
 FILE *userFile;

 sprintf(userDataLine, "ctdluser.%d", logPlace);
 if (access(userDataLine, 0)!=-1) {
    userFile=fopen(userDataLine, "rt");
	fgets(userDataLine, 255, userFile);
	if (aideCalledThis) doCR();
	mPrintf("Registration: P%s.\n ",strnicmp(userDataLine,"Y",1)==SAMESTRING ? "UBLIC" : "RIVATE");
    if (strnicmp(userDataLine,"N",1)==SAMESTRING && !SomeSysop()) return;
	doCR();
    while (!feof(userFile)) {
		fgets(userDataLine, 255, userFile);
		mPrintf(userDataLine);
		}
	fclose(userFile);
	doCR();
	}
}


int aideProfile()
{
    logBuffer lBuf;
    char who[NAMESIZE];
    int logNo;

	if (aideCalledThis) mPrintf("rofile\n ");
	if (!getXString(who_str, who, NAMESIZE, NULL, NULL)) return (-1);

#ifdef NEWUSERBIO
	logNo=findPerson(/* messageCalledThis ? msgBuf.mbauth */ who, &lBuf);
#else
    logNo=findPartPerson(who,&lBuf);
#endif

	if (logNo == ERROR) return (-1);
	if (aideCalledThis) {
		showUserProfile(logNo);
		doCR();
		return (-1);
		}
	else {
         sprintf(who, "laston.%d", logNo);
		 if (access(who,0)!=-1) tutorial(who,FALSE);
		 else mPrintf(" No login record.\n ");

#ifdef NEWUSERBIO
		 showUserProfile(logNo);
		 sprintf(who, "user%d.cit", logNo);
		 if (access(who,0)!=-1) {
			doCR();
			mPrintf(" .Profile:");
			tutorial(who,FALSE);
			}
		 else {
			mPrintf("No .Profile");
			doCR();
			}
#endif
		 return (-1);
		 }
/*    else return logNo; */
}

#ifdef EVIL-STUFF
generalProfile()
{
    int logPosition;
    char userFile[12];

	aideCalledThis=FALSE;
	logPosition=aideProfile();
	if (logPosition==-1) return;


	sprintf(userFile, "laston.%d", logPosition);
 	if (access(userFile, 0) != -1) tutorial(userFile, FALSE);
    doCR();
    return;

	showUserProfile(logPosition);

    sprintf(userFile, "user%d.cit", logPosition);
    if (access(userFile, 0) != -1) tutorial(userFile, FALSE);
    doCR();

 }
#endif


checkDomain()
{
 FILE *hostFile;

    if (access("hostname.sys",0)==0) {
		hostFile=fopen("hostname.sys", "rt");
		fgets(hostDomainName, 80, hostFile);
		fclose(hostFile);
		}
    else setmem(hostDomainName,80,0);


}
