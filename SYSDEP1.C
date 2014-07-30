/************************************************************************/
/*                              sysdep1.c                               */
/*                                                                      */
/*      This is the repository of most of the system dependent code     */
/*      in Citadel.  We hope, pray, and proselytize, at least.          */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 87Apr01 HAW  File tagging completed; bug for .RD/.RE fixed.          */
/* 86Dec14 HAW  Reorganized into areas.                                 */
/* 86Nov25 HAW  Created.                                                */
/************************************************************************/

#define SYSTEM_DEPENDENT
#define TIMER_FUNCTIONS_NEEDED

#include "ctdl.h"
#include "sys\stat.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*              AREAS:                                                  */
/*      bigDirectory()          gets an "extended" directory            */
/* #    bigDirs()               work function for bigDirectory()        */
/*      CitGetFileList()        gets list of files                      */
/*      freeFileList()          Free file list                          */
/*      getArea()               get an area from the sysop              */
/*      goodArea()              get good area                           */
/*      homeSpace()             takes us to our home space              */
/*      netGetArea()            get area for dumping networked file(s)  */
/*      netSetNewArea()         set area for dumping networked file(s)  */
/*      prtNetArea()            makes human readable form of NET_AREA   */
/*      setSpace()              goto specified "area"                   */
/* #    realSetSpace()          does work of setSpace, others           */
/* #    MSDOSparse()            parses a string for MSDOS filename      */
/* #    fileType()              gets file type for MSDOS                */
/*      sysGetSendFiles()       specify where to send files from        */
/*      updFiletag()            updates a filetag                       */
/*      sysRoomLeft()           how much room left in net recept area   */
/* #    getSize()               gets size of a file                     */
/*      sysSendFiles()          system dep stuff for sending files      */
/* #    doSendWork()            does work of sysSendFiles()             */
/*              BAUD HANDLER:                                           */
/* #    check_CR()              scan input for carriage returns         */
/*      Find_baud()             does flip flop search for baud          */
/*      getNetBaud()            check for baud of network caller        */
/*              CONSOLE HANDLING:                                       */
/*      getCh()                 bottom-level console-input filter       */
/*      KBReady()               returns TRUE if a console char is ready */
/*      mputChar()              Do our own for some Console output      */
/*                                                                      */
/*              # == local for this implementation only                 */
/************************************************************************/
#define SETDISK         14      /* MSDOS change default disk function   */

#define NO_IDEA         0
#define SINGLE_FILE     1
#define IS_DIR          2
#define AMB_FILE        3

/************************************************************************/
/*      Globals -- there shouldn't be anything here but statics and     */
/*      externs.                                                        */
/************************************************************************/
/* These MUST be defined! */

char *R_W_ANY     = "r+b";
char *READ_ANY    = "rb";
char *READ_TEXT   = "rt";
char *APPEND_TEXT = "a+";
char *APPEND_ANY  = "a+b";
char *A_C_TEXT    = "a+";
char *WRITE_TEXT  = "wt";
char *W_R_ANY     = "w+b";
char *WRITE_ANY   = "wb";
int  netPassBaud;
DIR_EXTRA GblOther;

/* Here's the rest of the goo */
struct timePacket {
        long tPday, tPhour, tPminute, tPsecond, tPmilli;
} ;

extern logBuffer logBuf;         /* Log buffer of a person       */
extern aRoom     roomBuf;
extern MSG_BUF      msgBuf;
extern CONFIG    cfg;            /* Lots an lots of variables    */
extern char onConsole;                  /* Who's in control?!?          */
extern char whichIO;                    /* CONSOLE or MODEM             */
extern char anyEcho;
extern char echo;
extern char modStat;
extern char echoChar;
extern char haveCarrier;
extern long netBytes;
extern char outFlag;
extern char *strFile;
extern char *indexTable;
extern char *whatBaudRate, baudLOCK;
extern int  thisRoom;

/************************************************************************/
/* Section 3.4. AREAS:                                                  */
/*   The model of Citadel includes a provision for "directory rooms."   */
/* A directory room is defined as the ability to look in on some section*/
/* of the host system's file section.  In order to avoid tying the      */
/* directory structure of Citadel to any particular operating system, an*/
/* "abstraction" has been implemented.  Each room that has been desig-  */
/* nated as a directory will have an "area" associated with it.  This   */
/* "area" is dependent on each implementation; access to "areas" is     */
/* through routines located in this module.  Therefore, the abstract    */
/* directory model of Citadel is in the main code modules, and should   */
/* not require any changes from port to port.  The only changes neces-  */
/* sary should be in this file (SYSDEP.C), where the porter must decide */
/* upon and implement a mapping of how a directory room peeks in on his */
/* or her file system.                                                  */
/*   Basically, the routines are fairly simple in purpose.              */
/*                                                                      */
/*      "And if pigs had wings they could fly!"                         */
/************************************************************************/

/************************************************************************/
/* MSDOS version:                                                       */
/*    MSDOS has a "tree" directory structure, one tree for each disk    */
/* on the system.  The system remembers for each tree that there is a   */
/* "current directory" associated with it.  For this implementation,    */
/* directory rooms may either peek in on the current directory of any   */
/* disk on the system, or any subdirectory of the current directory of  */
/* any disk.  Directories above the current directory of any disk cannot*/
/* be accessed via directory rooms.  However, network area handlers do  */
/* allow access.                                                        */
/************************************************************************/

char        locDir[100] = "";
char        ourHomeSpace[100];
char        locDisk;


/************************************************************************/
/*      CitGetFileList() Get a list of files from the current "area"    */
/*      Returns the # of files listed that fit the given mask           */
/************************************************************************/
int CitGetFileList(mask, list, rmax)
char           *mask;
struct dirList list[];
int            rmax;
{
    struct ffblk   FlBlock;
    int            done;
    extern char    *monthTab[13];
    char           *w, *work, *sp, again, duplicate;
    struct dirList *fp;
    int            fileCount = 0, i;
    struct stat    buff;

    w = work = GetDynamic(strLen(mask) + 1);
    strCpy(work, mask);
    fp = list;

    do {
        again = (sp = strchr(work, ' ')) != NULL;      /* space is separator */
        if (again) {
            *sp = 0;
        }
        /* Do all scanning for illegal requests here */
        if (strchr(work, ':')  != NULL ||
            strchr(work, '\\') != NULL)
            continue;

        if (stat(work, &buff) == 0)
            if (buff.st_mode & S_IFCHR)
                continue;


        done = findfirst(work, &FlBlock, 0);
        while (!done) {
            for (i = 0, duplicate = FALSE; i < fileCount; i++) {
                if (strCmpU(list[i].unambig, FlBlock.ff_name) == SAMESTRING)
                    duplicate = TRUE;
            }
            if (!duplicate) {
                if (fileCount >= rmax) {
                    mPrintf("too many files!\n ");
                    return fileCount;
                }
                fp->unambig = (char *) GetDynamic(strLen(FlBlock.ff_name) + 1);
                strCpy(fp->unambig, FlBlock.ff_name);
                fp->otherStuff.Fdate = GetDynamic(8);
                sPrintf(fp->otherStuff.Fdate, "%d%s%02d",
                     ((FlBlock.ff_fdate & 0xfe00) >> 9) + 80,
                    monthTab[(FlBlock.ff_fdate & 0x1e0) >> 5],
                    FlBlock.ff_fdate & 0x001f);
                fp->otherStuff.Fsize = FlBlock.ff_fsize;
                fp++, fileCount++;
            }
            done = findnext(&FlBlock);
        }
        if (again) work = sp+1;
    } while (again);
    free(w);
    return fileCount;
}

/************************************************************************/
/*      freeFileList() Frees the file list generated by CitGetFileList  */
/************************************************************************/
void freeFileList(list, count)
struct dirList list[];
int count;
{
    int i;

    for (i = 0; i < count; i++) {
        free(list[i].unambig);
        free(list[i].otherStuff.Fdate);
    }
}

/************************************************************************/
/*      getArea() get area to assign to a directory room from sysop     */
/*      returns FALSE if problems or abort                              */
/************************************************************************/
char getArea(roomData)
aRoom *roomData;
{
    char dir[200], dir1[200], drive;
    char storeMember[100], oldMember[100], oldMemberNumber[4];
    SYS_FILE filename;
	FILE *alterFiles, *tempalterFiles;
    extern char *DirFileName, alterNet;
    extern struct any_list DirBase;

    if (!goodArea("directory", dir, &drive, TRUE)) {
        return FALSE;
    }

    strCpy(dir1 + 2, dir);
    dir1[0] = drive;
    dir1[1] = ':';
    if (alterNet==TRUE) {
		homeSpace();
		tempalterFiles=fopen("linkdirs.tmp", "w+t");
        sprintf(storeMember, "%03d: %s\n", thisRoom, dir1);
		fprintf(tempalterFiles, "%s", storeMember);
		if (access("linkdirs.sys",0)==0) {
			alterFiles=fopen("linkdirs.sys", "rt");
			do {
				fgets(oldMember, 100, alterFiles);
				strncpy(oldMemberNumber, oldMember, 3);
				oldMemberNumber[3]='\0';
				if ((strncmp(oldMemberNumber, storeMember, 3)!=0)
					 && !feof(alterFiles) )
						fprintf(tempalterFiles, "%s", oldMember);
        		} while (!feof(alterFiles));
			}
		fclose(alterFiles);
		fclose(tempalterFiles);
		unlink("linkdirs.sys");
		rename("linkdirs.tmp", "linkdirs.sys");
		}

    if (alterNet==FALSE) makeSysName(filename, DirFileName, &cfg.roomArea);
    if (alterNet==TRUE) return TRUE;
	else    return AddElement(&DirBase, filename, roomData->rbArea, dir1);
}

/************************************************************************/
/*      goodArea() Gets a valid path from the sysop. Drive should be    */
/*                  set already.                                        */
/************************************************************************/
static goodArea(prompt, dir, drive, fullPath)
char *prompt, *dir, *drive, fullPath;
{
    int  c;
    char dir_x[150];
    char *rm_dir=" in room directory names!";
    
    while (TRUE) {
        if (!getXString(prompt, dir_x, 149, "", ""))
            return FALSE;

        MSDOSparse(dir_x, drive);

        if (!fullPath && strchr(dir_x, '\\') != NULL)
            mPrintf("No '\\'s%s",rm_dir);
        else if (!fullPath && strchr(dir_x, '.') != NULL)
            mPrintf("No suffixes%s",rm_dir);
        else {
            c = fileType(dir_x, *drive);
            switch (c) {
            case IS_DIR:
                strCpy(dir, dir_x);
                return TRUE;
            case NO_IDEA:
                if (strLen(dir_x) != 0) {
                    mPrintf("%s doesn't exist. ", dir_x);
                    if (getYesNo("Create it")) {
                        DoBdos(SETDISK, toUpper(*drive) - 'A');
                        if (mkdir(dir_x) == BAD_DIR) {
                            mPrintf("ERROR!");
                            homeSpace();
                        }
                        else {
                            homeSpace();
                            strCpy(dir, dir_x);
                            return TRUE;
                        }
                    }
                }
                else {
                    strCpy(dir, dir_x);
                    return TRUE;
                }
            break;
            default:
                mPrintf("Not a directory!\n ");
            }
        }
    }
}

/************************************************************************/
/*      homeSpace() takes us home!                                      */
/************************************************************************/
void homeSpace()
{
    if (strLen(locDir) != 0) {
        chdir(locDir);
        locDir[0] = 0;
    }
    DoBdos(SETDISK, locDisk - 'A');
    chdir(ourHomeSpace);
}

/************************************************************************/
/*      netGetArea() Get area to store a file from networking           */
/************************************************************************/
netGetArea(file_data, ambiguous)
struct fl_req *file_data;
char          ambiguous;
{
    char goodname;
    FILE *temp;

    if (!goodArea("directory to store received file(s) in",
                   file_data->flArea.naDirname,
                   &file_data->flArea.naDisk, TRUE)) return FALSE;

    file_data->flArea.naDisk -= 'A';
    if (!ambiguous) {
        realSetSpace(file_data->flArea.naDisk, file_data->flArea.naDirname);
        do {
            if (!getXString("name the file will be stored under on this system",
                              file_data->filename, NAMESIZE,
                              file_data->roomfile, file_data->roomfile)) {
                homeSpace();
                return FALSE;
            }
            temp = safeopen(file_data->filename, READ_TEXT);
            if (temp != NULL) {
                fclose(temp);
                mPrintf("'%s' already exists - ",
                               file_data->filename);
                mPrintf("will be overwritten by networking.");
                goodname = getYesNo("OK to do this");
            }
            else goodname = TRUE;
        } while (!goodname);
        homeSpace();
    }
    else {
        mPrintf("Vague requests may cause overwriting.\n ");
        file_data->filename[0] = 0;
    }
    return TRUE;
}

/************************************************************************/
/*      prtNetArea() human readable form of a NET_AREA                  */
/************************************************************************/
char *prtNetArea(netArea)
NET_AREA *netArea;
{
    static char temp[105];

    sPrintf(temp, "%c:%s", netArea->naDisk + 'A', netArea->naDirname);
    return temp;
}

/************************************************************************/
/*      netSetNewArea() Set area to store a file from networking        */
/************************************************************************/
char netSetNewArea(file_data)
NET_AREA *file_data;
{
    return realSetSpace(file_data->naDisk, file_data->naDirname);
}

/************************************************************************/
/*      realSetSpace() does work of setSpace                            */
/************************************************************************/
char realSetSpace(disk, dir)
char disk, *dir;
{
    DoBdos(SETDISK, disk);

    getcwd(locDir, 99);

    if (strLen(dir) != 0 && chdir(dir) == BAD_DIR) {
        homeSpace();
        return FALSE;
    }

    return TRUE;
}

/************************************************************************/
/*      setSpace() moves us to an area                                  */
/************************************************************************/
char setSpace(roomData)
aRoom *roomData;
{
    char	dir[150], drive;
    char	*no_dir = "?Directory not present!";
    extern 	struct any_list DirBase;

    if (findListName(&DirBase,roomData->rbArea) == NULL) {
        mPrintf("%s (internal error)\n ",no_dir);
        return FALSE;
    }
    strCpy(dir, findListName(&DirBase, roomData->rbArea));
    MSDOSparse(dir, &drive);
    if (!realSetSpace(toUpper(drive) - 'A', dir)) {
        mPrintf("%s\n ",no_dir);
        return FALSE;
    }
    return TRUE;
}

/************************************************************************/
/*      printArea(x) prints the area associated with this room          */
/************************************************************************/
void writeArea(rightNow, roomData, buf)
aRoom *roomData;
char rightNow, *buf;
{
    extern struct any_list DirBase;

    if (rightNow)
        mPrintf("%s\n ", findListName(&DirBase, roomData->rbArea));
    else {
        if (strLen(findListName(&DirBase, roomData->rbArea)) != 0)
            sPrintf(buf, "'%s'", findListName(&DirBase, roomData->rbArea));
        else
            sPrintf(buf, "'%c:'", locDisk);
    }
}

/************************************************************************/
/*      MSDOSparse() parses a string                                    */
/************************************************************************/
void MSDOSparse(theDir, drive)
char *theDir, *drive;
{
    if (theDir[1] == ':') {
        *drive = toUpper(theDir[0]);
        strCpy(theDir, theDir+2);
    }
    else {
        *drive = locDisk;
    }
}

/************************************************************************/
/*      fileType() gets file type for MSDOS                             */
/************************************************************************/
static fileType(theDir, theDrive)
char theDrive, *theDir;
{
    FILE *fd;

    if (strchr(theDir, '*') != NULL ||
        strchr(theDir, '?') != NULL) {
        return AMB_FILE;
    }
    if (realSetSpace(toUpper(theDrive) - 'A', theDir)) {
        homeSpace();
        return IS_DIR;
    }
    DoBdos(SETDISK, toUpper(theDrive) - 'A');
    if ((fd = safeopen(theDir, READ_TEXT)) != NULL) {
        fclose(fd);
        DoBdos(SETDISK, locDisk - 'A');
        return SINGLE_FILE;
    }
    DoBdos(SETDISK, locDisk - 'A');
    return NO_IDEA;
}

/************************************************************************/
/*      sysGetSendFiles() where to find files to send to another system */
/************************************************************************/
char sysGetSendFiles(sendWhat)
struct fl_send *sendWhat;
{
    if (!getXString("what files to send", sendWhat->snArea.naDirname, 100,
                                               NULL, ""))
        return FALSE;

    MSDOSparse(sendWhat->snArea.naDirname, &sendWhat->snArea.naDisk);
    if (fileType(sendWhat->snArea.naDirname,
                 sendWhat->snArea.naDisk) == NO_IDEA) {
        if (getYesNo("Neither file nor directory found. Send anyways"))
            return TRUE;
        return FALSE;
    }
    return TRUE;
}

/************************************************************************/
/*      sysRoomLeft() how much room left in net recept area             */
/************************************************************************/
long sysRoomLeft()
{
    long temp, temp2, temp3;

    temp = (long) ( (long) cfg.sizeArea * 1024);
    netBytes = 0l;
    doSendWork("*.*", getSize);
    diskSpaceLeft(getdisk() + 'A', &temp3, &temp2);
    return (minimum(temp - netBytes, temp2));
}

/************************************************************************/
/*      sysSendFiles() system dep stuff for sending files               */
/************************************************************************/
void sysSendFiles(sendWhat)
struct fl_send *sendWhat;
{
    char   temp[100], *last;
    label  mask;

    strCpy(temp, sendWhat->snArea.naDirname);
    switch (fileType(sendWhat->snArea.naDirname, sendWhat->snArea.naDisk)) {
        case IS_DIR:
            strCpy(mask, "*.*");
            break;
        case SINGLE_FILE:
        case AMB_FILE:
            if ((last = strrchr(temp, '\\')) == NULL) {
                strCpy(mask, temp);
                temp[0] = 0;
            }
            else {
                *last = 0;
                strCpy(mask, last + 1);
            }
            break;
        case NO_IDEA:
        default:
            sPrintf(temp, "Couldn't do anything with '%s'.",
                                              sendWhat->snArea.naDirname);
            netResult(temp);
            return;
    }
    if (!realSetSpace(toUpper(sendWhat->snArea.naDisk)-'A', temp)) {
        killConnection();
        crashout("Send file fatal error!");
    }
    doSendWork(mask, netSendFile);
    homeSpace();
}

/************************************************************************/
/*      doSendWork() does work of sysSendFiles()                        */
/************************************************************************/
void doSendWork(filename, fn)
char *filename;
int (*fn)(char *fn);
{
    int   fileCount;
    struct dirList  list[DIR_SIZE];

    if ((fileCount = CitGetFileList(filename, list, DIR_SIZE)) != 0) {
        wild2Card(list, fileCount, fn, "");
        freeFileList(list, fileCount);
    }
}


/************************************************************************/
/* Section 3.5. BAUD HANDLER:                                           */
/*    The code in here has to discover what baud rate the caller is at. */
/* For some computers, this should be ridiculously easy.                */
/************************************************************************/

#define NO_GOOD         0
#define CR_CAUGHT       1
#define NET_CAUGHT      2
/************************************************************************/
/*      check_CR() Checks for CRs from the data port for half a second. */
/************************************************************************/
static check_CR()
{
    struct timePacket ff;
    int c;

    setTimer(&ff);

    while (milliTimeSince(&ff) < 50) {
        if (MIReady())  {
            switch ((c=getMod())) {
            case '\r':
                return CR_CAUGHT;
            case 7:
                if (cfg.BoolFlags.netParticipant) return NET_CAUGHT;
            default: printf("%x\n", c);
            }
        }
    }
    return NO_GOOD;
}

#define idStrings(i)   (cfg.codeBuf + cfg.modemData.pResults[i])

static char *rates[] = {
        "300", "1200", "2400", "4800", "9600", "14400", "19200", "Unknown"
} ;

/************************************************************************/
/*      Find_Baud() Finds the baud from sysop and user supplied data.   */
/************************************************************************/
char Find_baud(whatRate)
char **whatRate;
{
    char noGood = NO_GOOD;
    int  Time = 0;
    int  baudRunner;                    /* Only try for 60 seconds      */
	static char *Bauds[] = {"300", "1200", "2400", "4800", "9600", "14400", "19200"};
    extern FILE *netLog;
    extern int  byteRate;
	extern char noInit;

	if (noInit) return TRUE;

    pause(100);             /* Dramatic pause */
	baudRunner = getFromModem(cfg.modemData.modem_data==0x3f8 ? 0 : 1);
    netPassBaud = baudRunner;
	*whatRate = rates[baudRunner];

	byteRate = (atoi(rates[baudRunner])) / 10;

    baudSetter(baudRunner);

    return TRUE;
}

#ifdef OLD_WAY
/************************************************************************/
/*      getModemId() Try to read baud id from modem                     */
/************************************************************************/
#define BA_BUF_SIZE     50
int getModemId()
{
    char c, buffer[BA_BUF_SIZE];         /* Hopefully, overkill */
    int i;
	int answerBufferLength;
    extern FILE *netLog;
    struct timePacket ff;

    setTimer(&ff);

    i = 0;
    while (milliTimeSince(&ff) < 100) {
        if (MIReady())  {
            if ((c = getMod()) == '\r') {
                buffer[i] = 0;
                if ((strCmpU(buffer, cfg.codeBuf + cfg.modemData.pRing) == SAMESTRING)
					||
					(strCmpU(buffer, "3") == SAMESTRING)  /* no carrier */
					||
					(strCmpU(buffer, "4") == SAMESTRING)) /* error */
                                   /* Ignore */
                    i = 0;
                else
                    break;
            }
            else {
                buffer[i++] = c;
                if (i >= BA_BUF_SIZE - 4) {     /* Fudge factor */
                    i = 0;
                }
            }
        }
    }
    buffer[i] = 0;
    for (i = 0; i < 6; i++) {
        if (strLen(idStrings(i)) != 0)
            if (strCmpU(buffer, idStrings(i)) == SAMESTRING)
                return i;
    }
	answerBufferLength = strlen(buffer);
    if (answerBufferLength > 0) /* only report this if non-zero result */
	    splitF(netLog, "Link:  %d character%sin buffer at connect.\n",
			 answerBufferLength, (answerBufferLength==1) ? " " : "s ");
    return ERROR;
}
#endif


/************************************************************************/
/*      getNetBaud()  gets baud of network caller -- refer to SysDep.doc*/
/************************************************************************/
char getNetBaud()
{
    extern FILE *netLog;
    int         Time, baudRunner;
    extern int  byteRate;
    char        laterMessage[100];
    char        found = FALSE, notFinished;
    extern char inNet;

        /* If anytime answer, then we already have baud rate */
#ifdef IDEAL
    if (inNet == ANY_CALL) return TRUE;
#endif

    sPrintf(laterMessage,
 "BUSY for another %d minutes; please call back.\n",
                                                timeLeft());

    if (cfg.modemData.idFromModem) {
        if ((baudRunner = netPassBaud) != ERROR) {
            found = TRUE;
	        }
        else found = FALSE;
   		}

    pause(50);         /* Pause a half second */

    if (found) {

        byteRate = (atoi(rates[baudRunner])) / 10;
        for (Time = 0; gotCarrier() && Time < 20; Time++) {
            if (check_for_init(FALSE)) return TRUE;
            if (cfg.BoolFlags.debug) splitF(netLog, ".\n");
        }
        if (gotCarrier()) {
            outFlag = IMPERVIOUS;
            mPrintf(laterMessage);
        }
    }
    else {
        /* while (MIReady())   inp(); */
		purgeFossilBuffs();		 /* Clear garbage        */

		if (baudLOCK) {
			byteRate = (atoi(whatBaudRate)) / 10;
			for (baudRunner=0; rates[baudRunner]!=whatBaudRate; baudRunner++) {
				;
				}
			}
/* K2NE TEST */
        for (Time = 0; gotCarrier() && Time < 60; Time++) {

		if (baudLOCK) {
			baudLOCK = FALSE;

			if (check_for_init(FALSE)) return TRUE;
			}
        else {

             for (notFinished = TRUE, baudRunner = 0;
                                gotCarrier() && notFinished;) {
/* printf("setting baud to %s\n", rates[baudRunner]); */
                byteRate = (atoi(rates[baudRunner])) / 10;

				baudSetter(baudRunner);
#ifdef OLDFOSSIL
			    switch(byteRate) {
       				case 30:
           				f_baud(cfg.modemData.modem_data==0x3f8 ? 0 : 1, F_B300|F_NORMAL);
						break;
					case 120:
			           	f_baud(cfg.modemData.modem_data==0x3f8 ? 0 : 1, F_B1200|F_NORMAL);
						break;
					case 240:
			           	f_baud(cfg.modemData.modem_data==0x3f8 ? 0 : 1, F_B2400|F_NORMAL);
            			break;
					}
#endif
                if (check_for_init(FALSE)) return TRUE;
                if (cfg.BoolFlags.debug) splitF(netLog, ".\n");
                notFinished = !(baudRunner == cfg.sysBaud);
                baudRunner++;
	            }
			}
        }
    }
    if (!gotCarrier()) splitF(netLog, "Lost carrier\n");
    killConnection();
    recycleModem();
    return FALSE;
}

/************************************************************************/
/* Section 3.3. CONSOLE HANDLING:                                       */
/*    These functions are responsible for handling console I/O.         */
/************************************************************************/


/************************************************************************/
/*      mputChar()                                                      */
/************************************************************************/
void mputChar(c)
char c;
{
    extern char NewVideo;

    if ((c == '\0') || (c == BELL && cfg.BoolFlags.noChat && !onConsole))
        return;
    if (!(whichIO == CONSOLE || onConsole) && !anyEcho)
        return;
    if (c != ESC &&
      (echo == BOTH || (whichIO == CONSOLE && (echo != NEITHER || echoChar)))) {
#ifdef ZENITH || BOTHTYPES
       if (cfg.modemData.IBM_or_clone && NewVideo) vputch(c);
       else DoBdos(6, c);
#else
       if (NewVideo)  vputch(c);
       else DoBdos(6, c);
#endif
       if (c == '\n')
       	   mputChar('\r');

    }
}

/************************************************************************/
/* 3.11. File Comment handling                                          */
/*   These four functions handle file comments.  In DOS, there is no    */
/* built-in way to keep file comments; therefore, we keep a file in     */
/* each subdirectory named FILEDIR.TXT which contains the file comments.*/
/************************************************************************/

static FILE *fileTags;                  /* For the file tags            */
static char *flDir = "filedir.txt";
static char tagFound = FALSE;


/************************************************************************/
/*      StFileComSearch() Start File Comment search: see SYSDEP.DOC.    */
/************************************************************************/
int StFileComSearch()
{
    fileTags = safeopen(flDir, READ_TEXT);
    tagFound = TRUE;
    return TRUE;
}

/************************************************************************/
/*      FindFileComment() find file comment for file: see SYSDEP.DOC.   */
/************************************************************************/
int FindFileComment(fileName)
char *fileName;
{
    char *c;

    if (specCmpU(fileName, msgBuf.mbtext) == SAMESTRING) return TRUE;

    if (tagFound && fileTags != NULL) {
        fgets(msgBuf.mbtext, MAXTEXT-10, fileTags);
	    }

    tagFound = FALSE;
    if (fileTags != NULL) {
        while (specCmpU(fileName, msgBuf.mbtext) > SAMESTRING) {
            if ((c=fgets(msgBuf.mbtext, MAXTEXT-10, fileTags)) == NULL)
                break;
	        }

        if (c == NULL)
            rewind(fileTags);
        else if (specCmpU(fileName, msgBuf.mbtext) == SAMESTRING)
            tagFound = TRUE;
	    }
    if (tagFound) {
/*        if ((c = strchr(msgBuf.mbtext, '\n')) != NULL)
            *c = 0; */
        return TRUE;
	    }
    return FALSE;
}

/************************************************************************/
/*      EndFileComment() end session of reading file comments.          */
/************************************************************************/
void EndFileComment()
{
    if (fileTags != NULL) fclose(fileTags);
    fileTags = NULL;
}

/************************************************************************/
/*      updFiletag()  updates a file tag                                */
/************************************************************************/
void updFiletag(fileName, desc)
char *fileName, *desc;
{
    FILE        *updFd, *temp;
    static char *tmpName = "a45u8a7.$$$";
    char        line[500], *c;

	extern logBuffer logBuf; /* Who be it? */

    if ((updFd = safeopen(flDir, READ_TEXT)) == NULL) {
        if ((updFd = safeopen(flDir, WRITE_TEXT)) == NULL)
            mPrintf("Filetag error!\n ");
        else {
            fprintf(updFd, "%s %s\n",
							fileName, desc);
            fclose(updFd);
        }
        return;
    }
    else {
        temp = safeopen(tmpName, WRITE_TEXT);
        line[0] = 0;
        while ((c=fgets(line, 500, updFd)) != NULL) {
            if (specCmpU(fileName, line) <= SAMESTRING) break;
            fprintf(temp, line);
        }
        fprintf(temp, "%s %s\n", fileName, desc);
        if (c != NULL && specCmpU(fileName, line) != SAMESTRING)
            fprintf(temp, line);

        while ((c=fgets(line, 500, updFd)) != NULL)
            fprintf(temp, line);
        fclose(updFd);
        fclose(temp);
        unlink(flDir);
        rename(tmpName, flDir);
    }
}

