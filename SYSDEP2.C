/*********************************************************************
 *                           sysdep2.c                               *
 *                                                                   *
 *   This is the repository of most of the system dependent code     *
 *   in Citadel.  We hope, pray, and proselytize, at least. HAW      *
 *                                                                   *
 *-------------------------------------------------------------------*
 *                                                                   *
 *   The multi-user routines have been added to instill more gentle- *
 *   manly behaviour by Cita-piggy while being used in a Multi-      *
 *   tasking environment. Currently is DDOS and DesqView cognizant   *
 *   I have tried to dome something for Windows, but Cit needs mucho *
 *   mutilation to run under Windows with any couth.   <br>          *
 *                                                                   *
 *-------------------------------------------------------------------*
 *                                                                   *
 *   The FOSSIL routines have been added to (finally!) allow the     *
 *   removal of the "mini-interpeter" from Citadel.  Let's face it,  *
 *   folks.  It's 1991.  That stuff had its place under CP/M when    *
 *   this beastie was first written, but its time has passed.  (VAQ) *
 *                                                                   *
 *********************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 91Oct17 VAQ  removed old interpreter-driven modem code; added FOSSIL */
/*              code and supporting functions (see LIBFOSSL.C)          */
/* 89Mar18 <br> added multi-user routines								*/
/* 86Dec14 HAW  Reorganized into areas.                                 */
/* 86Nov25 HAW  Created.                                                */
/************************************************************************/

#define SYSTEM_DEPENDENT
#define TIMER_FUNCTIONS_NEEDED

#include "ctdl.h"
#include "sys\stat.h"
#include "ctype.h"

#define NONE		0
#define DOUBLEDOS	1
#define DESQVIEW	2

#define PORT_POINTER cfg.FOSSIL_PORT /* a useful space-saver */

int	MultiTasker = NONE;
int grabOneChar();
int peekAhead(void);
int ringsToAnswer;
int numberOfRings;
int ourBBSport;
int SHUTTLE_POINTER;  /* = (-1*(PORT_POINTER-1)); */

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*              MODEM HANDLING:                                         */
/*      inp()                   modem input with FOSSIL compliance      */
/*      MIReady()               check FOSSIL for data ready             */
/*      modemClose()            closes down modem I/O via FOSSIL        */
/*      outMod()                bottom-level modem output via FOSSIL    */
/*              SYSTEM FORMATTING:                                      */
/*      dPrintf()               printf() that writes to disk            */
/* #    mspr()                  utility for formatted printing          */
/*      mPrintf()               writes a line to modem & console        */
/*      mTrPrintf()             special mPrintf for WC transfers        */
/*      mYMprintf()             special mPrintf for YMODEM transfers    */
/* #    splitF()                debug formatter                         */
/*              TIMERS:                                                 */
/*      chkTimeSince()          check how long since timer initialized  */
/* #    milliTimeSince()        How long in milliseconds have passed    */
/*      pause()                 pauses for N/100 seconds                */
/* #    setTimer()              start a specific timer.                 */
/*      startTimer()            Initialize a general timer              */
/*      timeSince()             how long since general timer init       */
/*      ReadDate()              interprets time and returns it in secs. */
/*              CONSOLE STUFF, continued                                */
/*      ScreenUser()            update fn for givePrompt()              */
/*      ScrNewUser()            status line updates                     */
/*      SpecialMessage()        special status line messages            */
/*              MISCELLANEOUS:                                          */
/* #    diskSpaceLeft()         amount of space left on specified disk  */
/*      getRawDate()            gets date from system                   */
/*      giveSpaceLeft()         give amount of space left on disk sys   */
/* #    initBadList()           read in list of bad filenames           */
/* #    interpret()             interprets a configuration routine      */
/* #    nodie()                 for ^C handling                         */
/*      receive()               read modem char or time out             */
/*      runPCPdial()            does a PCPursuit dial                   */
/*      safeopen()              opens a file                            */
/*      setRawDate()            set date (system dependent code)        */
/*      systemCommands()        run outside commands in the O.S.        */
/*      systemInit()            system dependent init                   */
/*      systemShutdown()        system dependent shutdown               */
/*      WhatDay()               returns what day it is                  */
/*                                                                      */
/*              # == local for this implementation only                 */
/************************************************************************/
#ifdef OLD_STYLE
#define COMBUF_SIZE     5000
#else
#define COMBUF_SIZE     1200    /* This is closely tied to CITZEN.ASM!! */
#endif
#define INTA00          0x20
#define INTA01          0x21                    /* Interrupt controller */
#define EOI             0x20                    /* End of Interrupt     */

/************************************************************************/
/*      Globals -- there shouldn't be anything here but statics and     */
/*      externs.                                                        */
/************************************************************************/
char NewVideo = FALSE;
char InvVideo = FALSE;
/*char didRing;*/

void     (*StartVideo)(char *info),
         (*ChangeStatus)(char *info),
         (*StopVideo)(void);
static char Refresh = 0;

extern char *R_W_ANY;
extern char *READ_ANY;
extern char *READ_TEXT;
extern char *APPEND_TEXT;
extern char *APPEND_ANY;
extern char *A_C_TEXT;
extern char *WRITE_TEXT;
extern char *W_R_ANY;
extern char *WRITE_ANY;
extern char fastModem, isShuttle;
extern char runningAsDoor;
extern char frontEnd, chatFlag, killScreen;
extern char binkleyBaudRate[10];
extern char modemStartupString[100], modemResetString[100], versionTag[20];
static char FirstInit = TRUE;

/* Here's the rest of the goo */
struct timePacket {
        long tPday, tPhour, tPminute, tPsecond, tPmilli;
} ;

struct any_list DirBase;
char *DirFileName = "ctdldir.sys";

extern logBuffer logBuf;         /* Log buffer of a person       */
extern aRoom     roomBuf;
extern MSG_BUF      msgBuf;
extern CONFIG    cfg;            /* Lots an lots of variables    */
extern NetBuffer netBuf;
extern char onConsole;                  /* Who's in control?!?          */
extern char whichIO;                    /* CONSOLE or MODEM             */
extern char anyEcho;
extern char echo;
extern char modStat;
extern char echoChar;
extern char haveCarrier;
extern char outFlag;
extern char *strFile;
extern char *indexTable;
extern char loggedIn;
extern char netMasterFlag;
extern char answerGuard, ansi, barBlock;
extern char modemAnswerString[50];
char straight = FALSE;

/************************************************************************/
/* Section 3.1. MODEM HANDLING:                                         */
/*    These functions are responsible for handling modem I/O.           */
/************************************************************************/

char intSet = FALSE;

static unsigned char old_inta01;


#ifdef OLD_STYLE

static unsigned char combuf[COMBUF_SIZE];

#else

#define combuf buffin

#ifdef ZENITH || BOTHTYPES
extern unsigned char buffin[COMBUF_SIZE];
#else
unsigned char buffin[COMBUF_SIZE];
#endif

#endif
static int cptr, head;

unsigned int portStatus();
unsigned int shuttleStatus();

/************************************************************************/
/*      inp() reads data from port.  Should not be called if there is   */
/*      no data present (for good reason).                              */
/************************************************************************/
unsigned char inp()
{
 return grabOneChar();
}


/*************************************************************************/
/*inpShuttle() reads data from port.  Should not be called if there is   */
/*       no data present (for good reason).                              */
/*************************************************************************/
unsigned char inpShuttle()
{
 return grabOneShuttleChar();
}



/************************************************************************/
/*      MIReady() Ostensibly checks to see if input from modem ready    */
/************************************************************************/
int MIReady()
{
 int toReturn;

 return (f_status(PORT_POINTER) & F_RDA);
}

/************************************************************************/
/* ShuttleReady() Ostensibly checks to see if input from modem ready    */
/************************************************************************/
int ShuttleReady()
{
 int toReturn;

 return (f_status(SHUTTLE_POINTER) & F_RDA);
}


/************************************************************************/
/*      modemClose() Responsible for shutting down I/O via the FOSSIL   */
/************************************************************************/
void modemClose()
{
 delay(20);
 fossilDTR(FALSE);
 delay(50);
}

/************************************************************************/
/*    shuttleClose() Responsible for shutting down I/O via the FOSSIL   */
/************************************************************************/
void shuttleClose()
{
 delay(20);
 shuttleDTR(FALSE);
 delay(50);
}


/***************************************************************
 *      outMod stuffs a char out the modem port via the FOSSIL *
 ***************************************************************/
char outMod(c)
int c;
{

 normalFossilTransmit(c);

 while ( !(portStatus() & F_TSRE) );

 return TRUE;
}

/****************************************************************
 *outShuttleMod stuffs a char out the modem port via the FOSSIL *
 ****************************************************************/
char outShuttleMod(c)
int c;
{

 normalShuttleTransmit(c);

 while ( !(shuttleStatus() & F_TSRE) );

 return TRUE;
}



/************************************************************************/
/* Section 3.7. SYSTEM FORMATTING:                                      */
/*    These functions take care of formatting to strange places not     */
/* handled by normal C library functions.                               */
/*   dPrintf() print to disk, using putMsgChar().                       */
/*   mPrintf() print out the modem port via a mFormat() call.           */
/*   mWCprintf() print out modem port via WC, append a 0 byte.          */
/*   splitF() debug function, prints to both screen and disk.           */
/************************************************************************/


/*********************************************************************
 *   dPrintf() write from format+args to disk, appends a null byte   *
 *********************************************************************/
void dPrintf(char *format, ...)
{
    va_list argptr;
    char garp[MAXWORD];
    int i;

    va_start(argptr, format);
    vsprintf(garp, format, argptr);
    va_end(argptr);
    tsoff();	   /* turn off timesharing for disk write 	*/
    doRTS(FALSE);  /* stop modem */
    for (i = 0; garp[i]; i++) putMsgChar(garp[i]);
	    putMsgChar(0);
    doRTS(TRUE);   /* start modem */
	tson();		   /* turn timesharing back on again */
}

/************************************************************************/
/*      mPrintf() formats format+args to modem and console              */
/************************************************************************/
int mPrintf(char *format, ...)
{
    va_list argptr;
    char garp[2000];

    va_start(argptr, format);
    vsprintf(garp, format, argptr);
    va_end(argptr);
    mFormat(garp);
}

/************************************************************************/
/*      printf() formats format+args to modem and console               */
/************************************************************************/
int printf(const char *format, ...)
{
    va_list argptr;
    char    garp[2000];
    int     i;

    va_start(argptr, format);
    vsprintf(garp, format, argptr);
    va_end(argptr);
    if (straight) for (i = 0; garp[i]; i++) DoBdos(6, garp[i]);
    else          for (i = 0; garp[i]; i++) mputChar(garp[i]);
}

/************************************************************************/
/*      splitF() formats format+args to file and console                */
/************************************************************************/
void splitF(FILE *diskFile, char *format, ...)
{
    va_list argptr;
    char garp[MAXWORD];

    va_start(argptr, format);
    vsprintf(garp, format, argptr);
    va_end(argptr);
    if (ansi==TRUE) cprintf("%s", garp);
	else printf("%s", garp);
if (strLen(garp) > MAXWORD) {
    killConnection();
    exit(3);
}
    if (diskFile != NULL) {
		purgeFossilBuffs();
		doRTS(FALSE);
        fprintf(diskFile, garp);
        fflush(diskFile);
        doRTS(TRUE);
		purgeFossilBuffs();
    }
}

/************************************************************************/
/*      mTrPrintf() formats format+args to a transmission function      */
/************************************************************************/
void mTrPrintf(char *format, ...)
{
    va_list argptr;
    char garp[200];
    int i;


    va_start(argptr, format);
    vsprintf(garp, format, argptr);
    va_end(argptr);
    for (i = 0; garp[i]; i++)
			sendITLchar(garp[i]);

    sendITLchar(0);      /* Send NULL since it did before */
}

/************************************************************************/
/* Section 3.8. TIMERS:                                                 */
/*    Basically, the idea here is that two functions are available to   */
/* the rest of Citadel.  One starts a timer.  The other allows checking */
/* that timer, to see how much time has passed since that timer was     */
/* started.  The remainder of the functions in this section are internal*/
/* to this implementation, mostly for use by receive().                 */
/* 88Jun28: Now multiple timers accessible to Citadel are supported.    */
/************************************************************************/

static struct timePacket localTimers[5];

/************************************************************************/
/*      chkTimeSince() buffer for timing stuff.  A call to startTimer() */
/*      must have preceded calls to this function.                      */
/*      RETURNS: Time in seconds since last call to startTimer().       */
/************************************************************************/
long chkTimeSince(TimerId)
int TimerId;
{
    return timeSince(localTimers + TimerId);
}

/************************************************************************/
/*      milliTimeSince() Calculate how many milliseconds have passed    */
/************************************************************************/
long milliTimeSince(Slast)
struct timePacket *Slast;
{
    long retVal;
    struct time timeblk;

    gettime(&timeblk);
    retVal = (timeblk.ti_sec != Slast->tPsecond) ? 100 : 0;
    retVal += timeblk.ti_hund - Slast->tPmilli;

    return retVal;
}

/************************************************************************/
/*      pause() busy-waits N/100 seconds                                */
/************************************************************************/
void pause(i)
int i;
{
    struct timePacket x;
    long              (*fn)(struct timePacket *r), limit;

    fn = (i <= 99) ? milliTimeSince : timeSince;
    limit = (i <= 99) ? (long) i : (long) (i / 100);    /* Kludge */
    setTimer(&x);
    while ((*fn)(&x) <= limit)
        ;
}

/************************************************************************/
/*      setTimer() get ready for timing something                       */
/************************************************************************/
void setTimer(Slast)
struct timePacket *Slast;
{
    struct date dateblk;
    struct time timeblk;

    getdate(&dateblk);
    gettime(&timeblk);

    Slast->tPday     = (long) dateblk.da_day;
    Slast->tPhour    = (long) timeblk.ti_hour;
    Slast->tPminute  = (long) timeblk.ti_min;
    Slast->tPsecond  = (long) timeblk.ti_sec;
    Slast->tPmilli   = (long) timeblk.ti_hund;
}

/************************************************************************/
/*      startTimer() Initialize a general timer                         */
/************************************************************************/
void startTimer(TimerId)
int TimerId;
{
    setTimer(localTimers + TimerId);
}

/************************************************************************/
/*      timeSince() Calculate how many seconds have passed since "x"    */
/************************************************************************/
static long timeSince(Slast)
struct timePacket *Slast;
{
    long retVal;
    struct date dateblk;
    struct time timeblk;

    getdate(&dateblk);
    gettime(&timeblk);
	giveaway(1);	/*  we are generally marking time here anyway, so giveaway
						remainder of the 55 ms slice						*/

    retVal = (Slast->tPday == dateblk.da_day ? 0l : 86400l);
    retVal += ((timeblk.ti_hour - Slast->tPhour) * 3600);
    retVal += ((timeblk.ti_min - Slast->tPminute) * 60);
    retVal += (timeblk.ti_sec - Slast->tPsecond);
    return retVal;
}

/************************************************************************/
/*      ReadDate() interprets the string and returns it in seconds      */
/************************************************************************/
int ReadDate(date, RetTime)
char *date;
long *RetTime;
{
    static char *MonthTab[] = {
          "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
          "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };
    int rover, found;
    int   month, day, hours, minutes, seconds, milli;
    label mon;
    struct date dptr;
    struct time tptr;

    if (strLen(date) == 0) return FALSE;

    if (isdigit(date[0])) {
        dptr.da_year = atoi(date) + 1900;
        while (isdigit(*date)) date++;
    }
    else {
        getRawDate(&dptr.da_year, &month, &day, &hours, &minutes,
                                        &seconds, &milli);
    }
    for (rover = 0; isalpha(*date); date++, rover++)
        mon[rover] = toUpper(*date);

    mon[rover] = 0;
    if (rover == 0) return ERROR;

    for (found = rover = 0; rover < NumElems(MonthTab); rover++)
        if (strncmp(mon, MonthTab[rover], strLen(mon)) == SAMESTRING) {
            found++;
            dptr.da_mon = rover + 1;
        }

    if (found != 1) return ERROR;

    if ((dptr.da_day = atoi(date)) == 0) return ERROR;
    zero_struct(tptr);
    *RetTime = dostounix(&dptr, &tptr);
    return TRUE;
}

long CurAbsolute()
{
    struct date dateblk;
    struct time timeblk;

    getdate(&dateblk);
    gettime(&timeblk);

    return dostounix(&dateblk, &timeblk);
}

/************************************************************************/
/*      Section 3.3 continued: Console stuff.                           */
/************************************************************************/

char CurTime[10] = "";
/************************************************************************/
/*      ScreenUser() called from givePrompt                             */
/************************************************************************/
void ScreenUser() /* Beginning with V4.02:K2NE we force NewVideo = TRUE */
{
    if (Refresh) {
        Refresh = 0;
        (*StopVideo)();
        VideoInit();
    }
}

void ScrTimeUpdate(hr, mn)
int hr, mn;
{
    char *civ;

    civTime(&hr, &civ);
    sPrintf(CurTime, "%d:%02d %s", hr, mn, civ);
    ScrNewUser();
}

/************************************************************************/
/*      ScrNewUser() called when changes occur that might impact the    */
/*                   status line.                                       */
/************************************************************************/
void ScrNewUser()
{
    int yr, dy, hr, mn;
    char *mon, work[80], *civ, sLimit[80];
	static char showLimit[35] = "";
	static char OnTime[20] = "";
    extern char CallSysop;
    extern char *curBaud;
	extern long Dl_Limit;

    if (isShuttle) return;

	if (killScreen==TRUE) return;

    if (NewVideo) {

        ltoa(Dl_Limit, showLimit, 10);
		strcpy(sLimit, "D:");
		strcat(sLimit, showLimit);
        if (strlen(cfg.lastCaller) < 17) strcat(sLimit, " min");

        if (onLine() && strLen(OnTime) == 0) {
            getCdate(&yr, &mon, &dy, &hr, &mn);
            civTime(&hr, &civ);
            sPrintf(OnTime, " %d:%02d %s", hr, mn, civ);
        }
        else if (!onLine()) {
/*			sPrintf(work, "%-10s%-3s %c%-2s%11s%8s [%s]", */
			sPrintf(work, "%s%s%9s%8s [%s]%s%s",
		          (strlen(cfg.lastCaller) != 0) ?
					"Last On: " : "",
		          (strlen(cfg.lastCaller) != 0) ? cfg.lastCaller : "",
				  CurTime,
				  formDate(),
				  cfg.SeeMail ? "VIS" : "PRI",
    	          cfg.BoolFlags.noChat ? "   " : " C ",
				  /* , tempDisabled ? "{D}" : ""); */
                  Dl_Limit > 0 ? sLimit : "");
            (*ChangeStatus)(work);
			OnTime[0]=0;
			return;
        }
			if (barBlock==TRUE) {
				barBlock=FALSE;
				return;
				}
			sPrintf(work, "%s%-20s [%s] On:%-9s|Time: %s %s%s[%s]",
				aide ? "*" :
				strCmpU(logBuf.lbname,cfg.lastCaller)==SAMESTRING ? "!" : " ",
				logBuf.lbname,
				(chatFlag) ? "CHAT" :
				   (frontEnd) ? binkleyBaudRate :
				   (runningAsDoor) ? "DOOR" :
				   (curBaud==NULL) ? "S" : curBaud,
				OnTime,
				CurTime,
                cfg.BoolFlags.noChat ? "" : "C ",
                CallSysop ? "^T" : "",
				( strlen(roomBuf.rbname) > 0 ) ? roomBuf.rbname : "");
        (*ChangeStatus)(work);
#ifdef READY
        topStatBar();
#endif
    }
}


/************************************************************************/
/*      SpecialMessage() Print a special* message on status line        */
/************************************************************************/
void SpecialMessage(message)
char *message;
{
 extern char roomLevelFlag;

    (*ChangeStatus)(message);
	roomLevelFlag = FALSE;
}

/************************************************************************/
/*      Section 3.9. MISCELLANEOUS                                      */
/************************************************************************/

/************************************************************************/
/*      diskSpaceLeft() Amount of space left on specified disk          */
/************************************************************************/
void diskSpaceLeft(drive, sectors, bytes)
char drive;
long *bytes;
long *sectors;
{
    struct dfree dfreeblk;

    getdfree(toUpper(drive) - 'A' + 1, &dfreeblk);
    *bytes = (long) dfreeblk.df_avail * (long) dfreeblk.df_bsec *
                                     (long) dfreeblk.df_sclus;
    *sectors = ((*bytes) + 127) / SECTSIZE;
}

/************************************************************************/
/*      getRawDate() gets raw date from MSDOS                           */
/************************************************************************/
void getRawDate(year, month, day, hours, minutes, seconds, milli)
int *year, *month, *day, *hours, *minutes, *seconds, *milli;
{
    struct date dateblk;
    struct time timeblk;

    getdate(&dateblk);
    gettime(&timeblk);

    *year  = dateblk.da_year;
    *month = dateblk.da_mon;
    *day  = dateblk.da_day ;
    *hours = timeblk.ti_hour;
    *minutes = timeblk.ti_min ;
    *seconds = timeblk.ti_sec ;
    *milli = timeblk.ti_hund;
}

/************************************************************************/
/*      giveSpaceLeft() give amount of space left on system.            */
/************************************************************************/
void giveSpaceLeft(roomData)
aRoom *roomData;
{
    long sectors, bytes;

    char  dir[150], drive;
    extern struct any_list DirBase;

    if (findListName(&DirBase, roomData->rbArea) != NULL) {
        strCpy(dir, findListName(&DirBase, roomData->rbArea));
        MSDOSparse(dir, &drive);
        diskSpaceLeft(drive, &sectors, &bytes);
  		printf("\nDrive %c: %ld bytes free\n", drive, bytes);
    }
}

/************************************************************************/
/*      initDirList() initialize dir list                               */
/************************************************************************/
void initDirList()
{
    SYS_FILE filename;

    makeSysName(filename, DirFileName, &cfg.roomArea);
    initList(filename, &DirBase);
    SeparateValues(&DirBase);
}


/************************************************************************/
/*      nodie()                                                         */
/************************************************************************/
int nodie()
{
    return 1;
}

/************************************************************************/
/*      Control_C() DOS handler for control C                           */
/************************************************************************/
int Control_C()
{
    Refresh++;
    return 1;
}

/************************************************************************/
/*      receive() gets a modem character, or times out ...              */
/*      Returns:        char on success else ERROR                      */
/************************************************************************/
int receive(seconds)
int  seconds;
{
#ifdef NEW_WAY
	static char readyFlag=FALSE;
#endif
    struct timePacket x;
    long   (*fn)(struct timePacket *r), limit;

    if (!gotCarrier()) return ERROR;
    fn = (seconds == 1) ? milliTimeSince : timeSince;
    limit = (seconds == 1) ? 99l : (long) seconds;      /* Kludge */
    setTimer(&x);
#ifdef NEW_WAY
    do {
        readyFlag=MIReady();
        if (readyFlag==TRUE) break;
	    } while ((*fn)(&x) <= limit);
    if (readyFlag==TRUE) return inp();
    else return ERROR;

    do {
		if ( MIReady() ) return inp();
		} while ((*fn)(&x) <= limit);
	return ERROR;
#endif
	do {
		if (f_status(PORT_POINTER) & F_RDA) return inp();
		} while ((*fn)(&x) <= limit);
	return ERROR;
}

/************************************************************************/
/*      setRawDate() set system date                                    */
/************************************************************************/
char setRawDate(year, month, day, hour, mins)
int year, month, day, hour, mins;
{
    struct time timeblk;
    struct date dateblk;

    timeblk.ti_min  = mins;
    timeblk.ti_hour = hour;
    timeblk.ti_sec  = 0   ;
    timeblk.ti_hund = 0   ;
    dateblk.da_year = year;
    dateblk.da_day  = day ;
    dateblk.da_mon  = month;
    setdate(&dateblk);
    settime(&timeblk);
    return TRUE;
}

/************************************************************************/
/*      runPCPdial()  does a PCPursuit dial                             */
/************************************************************************/
void runPCPdial()
{
#ifdef READY
    char call[60];
    long ptrtoabs();
    label normResult;

    normId(netBuf.netId, normResult);
    sPrintf(call, "pcpdial %s %lx", normResult + 2, ptrtoabs(&cfg));
    SysWork(printf, call);
#else
    printf("NOPE!\n");
#endif
}

/************************************************************************/
/*      SysWork() does system call and error processing                 */
/************************************************************************/
void SysWork(form, cmdLine)
char *cmdLine;
void (*form)();
{
    system(cmdLine);
}

/************************************************************************/
/*      safeopen() Opens a file                                         */
/************************************************************************/
FILE *safeopen(fn, mode)
char *fn;
char *mode;
{
    FILE *fd;
    struct stat buff;

    if ((fd = fopen(fn, mode)) != NULL) {
        if (fstat(fileno(fd), &buff) == 0) {
            if (buff.st_mode & S_IFCHR) {
                fclose(fd);
                fd = NULL;
            }
        }
    }
    return fd;
}

/************************************************************************/
/*      specCmpU() special compare for this version's file tags         */
/************************************************************************/
int specCmpU(f1, f2)
char *f1, *f2;
{
    while (toUpper(*f1) == toUpper(*f2)) f1++, f2++;
    if (*f1 == 0 && *f2 == ' ') return SAMESTRING;
    if (*f2 == 0) return -1;
    if (toUpper(*f1) < toUpper(*f2)) return -1;
    return 1;
}

/************************************************************************/
/*      systemCommands() Some MS-DOS commands                           */
/************************************************************************/
void systemCommands()
{
    char filename[200];
    FILE *fd;
	char *tempPrompt, tempWork[80];

#ifdef BRIAN
	        mPrintf("\n System cmds: ");
#else
    while (onLine()) {
        outFlag = OUTOK;

        mPrintf("\n System cmds: ");

        switch (toUpper(iChar())) {
        case 'X':
            mPrintf("\bExit\n ");
            return ;
        case 'D':
            mPrintf("elete file\n ");
            getNormStr(strFile, filename, 199, ECHO);
            doCR();
            mPrintf("File %s.\n ", (unlink(filename) == 0) ?
                                "deleted" : "not found");
            break;
        case 'O':
            mPrintf("utside cmds\n Wait.");
#endif
            writeSysTab();
            if ((fd = safeopen(LOCKFILE, "w")) != NULL) {
                fclose(fd);
            }
            getString("command line", filename, 198, FALSE, ECHO);
            (*StopVideo)();
			if (haveCarrier && strlen(filename)<2) {
	            homeSpace();
    	        unlink(indexTable);
        	    unlink(LOCKFILE);
            	break;
                }

            if ( (tempPrompt=getenv("PROMPT"))==0) sprintf(tempWork, "PROMPT=(Citadel resident)$_$p$g");
			else sprintf(tempWork, "PROMPT=(Citadel resident)$_%s", tempPrompt);
			putenv(tempWork);

            SysWork(mPrintf, filename);

            setmem(tempWork, strlen(tempWork), '\0');
			sprintf(tempWork, "PROMPT=%s", tempPrompt);
			putenv(tempWork);

            homeSpace();
            unlink(indexTable);
            unlink(LOCKFILE);
            if (NewVideo && onConsole) {
                mPrintf(" \nAny key.");
                getch();
            }
            VideoInit();
#ifndef BRIAN
            break;
        case '?':
            tutorial("sysopt.mnu", TRUE);
            break;
        default:
            mPrintf(" ?\n ");
            break;
        }
    }
#endif
}

/************************************************************************/
/*      systemInit() system dependent initialization                    */
/************************************************************************/
void systemInit()
{
    extern char locDisk, ourHomeSpace[100];

    getcwd(ourHomeSpace, 99);

    locDisk = toUpper(ourHomeSpace[0]);

    if (NewVideo) {
#ifdef ZENITH || BOTHTYPES
        if (cfg.modemData.IBM_or_clone) {
            StartVideo = video;
            ChangeStatus = statusline;
            StopVideo = nodie;
        } else {
            StartVideo = Zvideo;
            ChangeStatus = Zsline;
            StopVideo = Zreset_video;
        }
#else
        StartVideo = video;
        ChangeStatus = statusline;
        StopVideo = nodie;
#endif
    }
    else {
        StartVideo = nodie;
        ChangeStatus = nodie;
        StopVideo = nodie;
    }
    VideoInit();

    initDirList();

    ArcStartup(0);  /* dearc.sys ? */

	ArcStartup(1);  /* unzip.sys ? */

	ArcStartup(2);  /* unlharc.sys ? */

	ArcStartup(3);  /* zoo.sys ?   */
#ifdef BREAK_READY
#ifdef ZENITH || BOTHTYPES
    if (cfg.modemData.IBM_or_clone) {
    	printf("=> setup_nocccb()\n");
        setup_nocccb();
    }
    else
        ctrlbrk(Control_C);
#else
 	printf("=> setup_nocccb()\n");
    setup_nocccb();
#endif
#else
    ctrlbrk(Control_C);
#endif
}

void VideoInit()
{
    char work[60];
    extern char *VERSION;

    if (!NewVideo) return;
    sprintf(work, "Citadel:K2NE V%s  ", VERSION);
    (*StartVideo)(work);
	fakeBanner();
    ScrNewUser();
}

/************************************************************************/
/*      systemShutdown() system dependent shutdown code                 */
/************************************************************************/
void systemShutdown()
{
    outportb(cfg.modemData.mdm_ctrl, (inportb(cfg.modemData.mdm_ctrl) & 254));
    DisableModem();
}

#ifdef QTEST
/* sole routine to raise DTR */
void ResurrectDTR()
{
	  outportb(0x3fc, (inportb(0x3fc) | 1));
	  /* for Citadel:  change "0x3fc" to cfg.modemData.mdm_ctrl */
}
#endif
/************************************************************************/
/*      WhatDay() Returns what day it is (0=Sunday...)                  */
/************************************************************************/
int WhatDay()
{
    _AX = 0x2a00;
    geninterrupt(0x21);
    return _AL;
}


/**************************************************************************
 * These time sharing routines were co-written by  djt - Dave Trulli 
 * and <br>  - Brian Riley, for the Packet Radio MailBox System (c) 1986,
 * 1987. 1988, 1989. They may be freely used for non commercial applications
 * so longs as the source code is distributed with the creditd attached and
 * runtime documentation gives credit to the authors.
 **************************************************************************/
	

/*
	multi_init() - test for Multitasking, and use pid to set the
				   Multitasker flag to NONE or as appropriate <br>
*/
multi_init()
{
	extern int ismultitask();
	char *which_tasker = "DOS";

	switch(MultiTasker = ismultitask()) {
		case	DOUBLEDOS:
			which_tasker = "DoubleDos";
			break;
		case	DESQVIEW:
			which_tasker = "DesqView";
			break;
	}
	printf("  (%s), TimeShare %s activated\n",which_tasker,
				    	MultiTasker==0 ? "not" : "is");
}



/*
	ismultitask() - check which multitasker is running.
	                return appropriate values   -  djt
*/
ismultitask()
{
	union REGS regs;

		/* check for doubledos */
	regs.h.ah = 0xe4;
	int86(0x21,&regs,&regs);
	if ( regs.h.al == 1 || regs.h.al == 2 )
		return (DOUBLEDOS);

		/* check for desqview */
	regs.x.ax = 0x2b01;
	regs.x.cx = 0x4445;
	regs.x.dx = 0x5351;
	int86(0x21,&regs,&regs);
	if ( regs.h.al != 0xff )
		return (DESQVIEW);

		/* failed all above .. no multitasker */
	return (NONE);
}


/*
	tsoff() - turn timesharing off during critical operations, this gives
	          the task at hand exclusive control of the CPU .
	          *** CAUTION *** You MUST do a tson() after or dats all she
	          wrote for the other side!!!!   <br>
*/
tsoff()
{
	union REGS regs;
	switch(MultiTasker) {
		case	DOUBLEDOS:
			int86(0xfa,&regs,&regs);
			break;
		case	DESQVIEW:
			regs.x.ax = 0x101b;
			int86(0x15,&regs,&regs);
			break;
	}
}

/*
	tson() - turn timesharing back on after using tsoff()  <br> / djt
*/
tson()
{
	union REGS regs;
	switch(MultiTasker) {
		case	DOUBLEDOS:
			int86(0xfb,&regs,&regs);
		case	DESQVIEW:
			regs.x.ax = 0x101c;
			int86(0x15,&regs,&regs);
			break;
	}
}

/*
	giveaway() - give up cpu for a while each unit is a 55ms interval
	             for doubledos or the whole slice for the DESQVIEW  <br>
*/
giveaway(numslices)
int numslices;
{
	union REGS regs;
	switch(MultiTasker) {
		case DOUBLEDOS:
			regs.h.al = numslices;
			int86(0xfe,&regs,&regs);
			break;
		case DESQVIEW:
				regs.x.ax = 0x1000;
				int86(0x15,&regs,&regs);
			break;
	}
}

/*
 * The following series of functions were added in conjunction with the
 * release of Citadel:K2NE Version 6.01 in order to facilitate the use of
 * standard FOSSIL drivers in place of the old "mini-interpeter" code
 * originally used by Citadel to control the modem port and the actual
 * modem device.  The bottom-layer FOSSIL functions are found in the
 * library module LIBFOSSL.C and the functions which follow simply call
 * them as needed, setting up the parameters needed for each low-level
 * function in this module before "passing control."  This approach was
 * used in order to make it easier to modify/enhance this code, as well
 * as to (hopefully) make it more portable between platforms which have
 * (or may get in the future) FOSSIL support.  Theoretically, at least,
 * all you should have to modify when "porting" is the LIBFOSSL code,
 * since the functions in this module are intentionally written in
 * "very generic" C.  (91Oct28 - VAQ)
 */

fossilDTR(int mode) /* Assert/Suppress DTR; Assert if mode is TRUE, */
{                   /* and suppress if mode is FALSE.               */

 f_dtr(PORT_POINTER, mode);
 if (gotCarrier()) {
   if (cfg.slowModemDelay!=0 && mode==FALSE)  /* #DTR-DELAY governs this */
			delay( cfg.slowModemDelay * 100); /* little loop!            */
   }
/* printf("\nP is %d.\n", PORT_POINTER); */
 delay(65); /* A "pause for the cause." */
 }

shuttleDTR(int mode) /* Assert/Suppress DTR; Assert if mode is TRUE, */
{                   /* and suppress if mode is FALSE.               */

 f_dtr(SHUTTLE_POINTER, mode);
 if (gotCarrier()) {
   if (cfg.slowModemDelay!=0 && mode==FALSE)  /* #DTR-DELAY governs this */
			delay( cfg.slowModemDelay * 100); /* little loop!            */
   }
/* printf("\nP is %d.\n", PORT_POINTER); */
 delay(65); /* A "pause for the cause." */
 }


initFossil()  /* Initialize (activate) FOSSIL driver */
{
 clrscr();
 f_init(PORT_POINTER);
}

initShuttle()  /* Initialize (activate) FOSSIL driver */
{
 clrscr();
 f_init(SHUTTLE_POINTER);
}


sendStringViaFossil(char *command)  /* for modem control strings only! */
{
 do {
      normalFossilTransmit(*command++);
      while ( !(portStatus() & F_TSRE) ); /* wait for empty buffer!    */

#ifdef TEST
      if (!fastModem) pause(5); /* if modem is a klunker we slow down! */
#endif
	 } while (*command);
 normalFossilTransmit('\r');
 while ( !(portStatus() & F_TSRE) );
}

sendStringViaShuttle(char *command)  /* for modem control strings only! */
{
 do {
      normalShuttleTransmit(*command++);
      while ( !(shuttleStatus() & F_TSRE) ); /* wait for empty buffer!    */

	 } while (*command);
 normalShuttleTransmit('\r');
 while ( !(shuttleStatus() & F_TSRE) );
}


killFossil()                             /* disable FOSSIl driver          */
{
 f_exit(PORT_POINTER);
}


killShuttle()                             /* disable FOSSIl driver          */
{
 f_exit(SHUTTLE_POINTER);
}


recycleModem()                           /* recycle/reset modem params     */
{
 purgeFossilBuffs();                     /* Clean output/input buffers     */
 fossilDTR(1);                           /* Assert DTR and set baudrate to */
 baudSetter( minimum(4, cfg.sysBaud) );  /* lowest of sysBaud or 2400 bps  */
 sendStringViaFossil(modemResetString);  /* then sent reset-string, and    */
 sleep(1);                               /* stall for lazy modems.         */
 purgeFossilBuffs();                     /* Clean the buffers again, then  */
 sendStringViaFossil(modemStartupString);/* send modem init. string, then  */
 sleep(1);                               /* stall for lazy modems and then */
 purgeFossilBuffs();                     /* clean buffers and return.      */
}


setNetCallBaud(int x)
{
  int theRate=minimum(x, cfg.sysBaud);

  baudSetter(theRate);
}

int gotCarrier()                  /* Function to deterimine DCD state     */
{
 unsigned int status;

/* if (!onLine() && ringsToAnswer>0) doRingCounter(); */
 status = f_status(PORT_POINTER); /* Get COMport MCR status               */
 if (!(status & F_DCD)) return 0; /* Mask status byte with DCD mask and   */
 else return 1;                   /* return 0 if no DCD and 1 if DCD.     */
}


int gotShuttleCarrier()                  /* Function to deterimine DCD state     */
{
 unsigned int shutStatus;

/* if (!onLine() && ringsToAnswer>0) doRingCounter(); */
 shutStatus = f_status(SHUTTLE_POINTER); /* Get COMport MCR status               */
 if (!(shutStatus & F_DCD)) return 0; /* Mask status byte with DCD mask and   */
 else return 1;                   /* return 0 if no DCD and 1 if DCD.     */
}


baudSetter(int setBaudRate)
{
  int ourPort=PORT_POINTER;

/*  if (cfg.lockFossil==TRUE) return; */
  switch(setBaudRate) {
       	case 0:
           	f_baud(ourPort, F_B300|F_NORMAL);
			break;
		case 1:
           	f_baud(ourPort, F_B1200|F_NORMAL);
			break;
		case 2:
           	f_baud(ourPort, F_B2400|F_NORMAL);
            break;
		case 3:
           	f_baud(ourPort, F_B4800|F_NORMAL);
			break;
		case 4:
           	f_baud(ourPort, F_B9600|F_NORMAL);
            break;
		case 5:
		case 6:
			f_baud(ourPort, F_B19200|F_NORMAL);
			break;
        default:
			f_baud(ourPort, F_B1200|F_NORMAL);
			break;
		}
 return;
}

purgeFossilBuffs()
{
 int port=PORT_POINTER;

 doRTS(FALSE);               /* Shut down RTS so nothing else happens...    */
 f_purgei(port);             /* Get rid of data in the input port...        */
 if (netMasterFlag==TRUE) {  /* If we are in the networker we don't want to */
    doRTS(TRUE);             /* clear the output port (drives Xmodem nuts!) */
	return;                  /* so we enable RTS and return to calling fn.  */
	}
 f_purgeo(port);             /* Otherwise we get rid of data in output port */
 doRTS(TRUE);                /* then renable RTS and return to the function */
 return;                     /* which called us here to begin with!         */
}

unsigned int portStatus()    /* Returns COMport MCR status                  */
{
 return f_status(PORT_POINTER);
}


unsigned int shuttleStatus()
{
 return f_status(SHUTTLE_POINTER);
}

normalFossilTransmit(char c) /* Send a char assuming checking for buffer    */
{                            /* space already has been done by outMod().    */
							 /* THIS is the bottom layer in Citadel for     */
							 /* sending via the FOSSIL.  It directly calls  */
 f_tx(PORT_POINTER, c);		 /* f_tx(port,char) in LIBFOSSL.C.              */
}


normalShuttleTransmit(char c) /* Send a char assuming checking for buffer    */
{                            /* space already has been done by outMod().    */
							 /* THIS is the bottom layer in Citadel for     */
							 /* sending via the FOSSIL.  It directly calls  */
 f_tx(SHUTTLE_POINTER, c);		 /* f_tx(port,char) in LIBFOSSL.C.              */
}


int grabOneChar()				   /* interface layer to call one char */
{                                  /* from modem - checks first!       */
   unsigned int c=0;
   int returnvalue = 0;

		if (MIReady()) {               /* Check for char in input buffer */
            c = normalFossilReceive(); /* Pass to Level One rcv function */
       	    returnvalue = c;
	        }
   return(returnvalue);
}

int grabOneShuttleChar()				   /* interface layer to call one char */
{                                  /* from modem - checks first!       */
   unsigned int c=0;
   int returnvalue = 0;

		if (ShuttleReady()) {               /* Check for char in input buffer */
            c = normalShuttleReceive(); /* Pass to Level One rcv function */
       	    returnvalue = c;
	        }
   return(returnvalue);
}

int normalFossilReceive() /* bottom layer to receive a char from modem */
{                         /* ONLY call this via grabOneChar() */
 unsigned int c=0;

 c = f_rx(PORT_POINTER);
 return (c);
}

int normalShuttleReceive() /* bottom layer to receive a char from modem */
{                         /* ONLY call this via grabOneChar() */
 unsigned int c=0;

 c = f_rx(SHUTTLE_POINTER);
 return (c);
}

doRingCounter()
{
 char answerNow=FALSE;
 int guardValue=0;
 int limitWait, stallValue;

 stallValue = ringsToAnswer==1 ? 500 : 2000;
 if (ringsToAnswer==0) return;
 if (numberOfRings>ringsToAnswer+1) { /* An "OOPS" situation!    */
	numberOfRings=1;                  /* So we reset counters    */
    answerNow=FALSE;                  /* and flags.              */
	answerGuard=FALSE;
    f_purgei(PORT_POINTER);
	}
 if (answerGuard==TRUE) return;       /* bad time to do this!    */
 if (gotCarrier()) return;            /* remote system online    */
	 if (getRingFromModem(PORT_POINTER, stallValue)) {
			numberOfRings++;
            printf(" ***RING ");
		    doRinger();
/*			didRing=TRUE; */
			}
 else 	return;

 if (numberOfRings == ringsToAnswer) answerNow=TRUE;

 if (answerNow==TRUE) {
    answerNow=FALSE;
	sendStringViaFossil(modemAnswerString); /* generalize later! */
    sleep(1);
    limitWait=0;
    guardValue=0;
    do {
        limitWait++;
        if (gotCarrier()) {
			guardValue=1;
			continue;
			}
		sleep(1);
		} while (guardValue==0 && limitWait<30);


    numberOfRings=ringsToAnswer+1;
    if (guardValue==1) answerGuard=TRUE;
	}

 return;
}

/* #ifdef READY */
topStatBar()
{
/* int xpos, ypos; */
 char bannerTop[80];

 if (!onLine()) return;
 setmem(bannerTop, 80, 0);
/* xpos=wherex();
 ypos=wherey(); */
 window(1,1,80,1);
 textbackground(BLUE);
 textcolor(WHITE);
 clrscr();
 sprintf(bannerTop, "Room: %s  Attribs: %s%s%s%s%s%s",
       ( strlen(roomBuf.rbname) > 0 ) ? roomBuf.rbname : "",
       roomBuf.rbflags.PERMROOM ? "Perm|" : "Temp|",
       roomBuf.rbflags.PUBLIC ? "Public|" : "Private|",
       (roomBuf.rbflags.ANON) ? "Anon|" : "",
       (roomBuf.rbflags.READ_ONLY) ? "Read-Only|" : "Read/Write|",
       (roomBuf.rbflags.SHARED) ? "Networked|" : "",
       (roomBuf.rbflags.ISDIR) ? "Directory" : "");
 cprintf("%s", bannerTop);
 window(1,2,80,24);
 textbackground(BLACK);
/* gotoxy(xpos,ypos); */
 return;
}
/* #endif */