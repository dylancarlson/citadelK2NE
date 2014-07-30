/************************************************************************/
/*                              sysarc.c                                */
/*                                                                      */
/*      This file contains the system dependent code for deARCing       */
/*      files for download.                                             */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 88Jun08 HAW  Created.                                                */
/* 88Dec12 VAQ  Patched for external protocol drivers.                  */
/* 89Mar12 VAQ  Extensively modified to add ZIPfile support.            */
/************************************************************************/

#define SYSTEM_DEPENDENT

#include "ctdl.h"
#include "sys\stat.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*              3.12 SEA ARC handling:                                  */
/*                                                                      */
/*              # == local for this implementation only                 */
/*              "SEA" is Systems Enhancement Associates                 */
/************************************************************************/

/************************************************************************/
/*      Globals -- there shouldn't be anything here but statics and     */
/*      externs.                                                        */
/************************************************************************/
static char ARCwork[80], ZIPwork[80], LHARCwork[80], ZOOwork[80];
static char TDirBuffer[120];

extern void  (*StopVideo)();
extern char  ARCready, ZIPready, LHARCready, UNZOOready;
extern aRoom roomBuf;
extern MSG_BUF msgBuf;
extern CONFIG  cfg;
extern logBuffer logBuf;

/************************************************************************/
/*      ArcStartup() initialize for deARCing  mode 0:ARC  mode 1:ZIP    */
/*                   and mode 2:LHARC (*.LZH files)                     */
/************************************************************************/
void ArcStartup(mode)
int mode;
{
#ifndef ONLYTHENET
    FILE *fd;
    char *c;
    SYS_FILE name;
 	char *archiver="dearc.sys", *zipper="unzip.sys", *lharcer="unlharc.sys";
    char *unzoo="unzoo.sys";
    makeSysName(name,
	 mode==0 ? archiver : mode==1 ? zipper : mode==2 ? lharcer : unzoo,
		  &cfg.roomArea);
    if ((fd = fopen(name, "r")) == NULL) {
        if (mode==0) ARCready = FALSE;
		else if (mode==1) ZIPready = FALSE;
		else if (mode==2) LHARCready = FALSE;
        else UNZOOready = FALSE;
        return;
    }

    if (fgets(mode==0 ? ARCwork : mode==1 ? ZIPwork : mode==2 ?
			LHARCwork : ZOOwork, 49, fd) == NULL) {
        fclose(fd);
        if (mode==0) ARCready = FALSE;
		  else if (mode ==1) ZIPready = FALSE;
		  else LHARCready = FALSE;
        return;
    }

    if ((c = strchr(mode==0 ? ARCwork : mode==1 ? ZIPwork : mode==2 ?
		LHARCwork : ZOOwork, '\n')) != NULL)
        *c = 0;

    if (mode==0) ARCready = TRUE;
	else if (mode==1) ZIPready = TRUE;
	else LHARCready = TRUE;

    fclose(fd);
#endif
}

/************************************************************************/
/*      MakeTempDir() Create and move into a temporary directory        */
/************************************************************************/
void MakeTempDir()
{
#ifndef ONLYTHENET
    static char *seed = "8ys6%d";
    int i = 0;

    sPrintf(TDirBuffer, seed, i++);
    while (access(TDirBuffer, 0) != -1)
        sPrintf(TDirBuffer, seed, i++);

    if (mkdir(TDirBuffer) != 0)
        mPrintf("MKDIR error!\n ");
    chdir(TDirBuffer);
    getcwd(TDirBuffer, 120);
#endif
}

/************************************************************************/
/*      SendCompressedFiles() get Filename, send contents specified     */
/************************************************************************/
void SendCompressedFiles(protocol, mode)
int mode;   /* 0 for ZIP, 1 for ARC, 2 for LHARC, 3 for ZOO */
char protocol;
{
#ifndef ONLYTHENET
    SYS_FILE name;
    char     *temp, *c, stringer[100];
    struct stat statbuff;
    int         NumFiles;
    extern long netBytes;

/* put download blocker here to handle downloading from within
   compressed files -- if true return at once */

    if (logBuf.lbflags.lflag6==TRUE) return;
	sPrintf(stringer, "file to send from");
	getNormStr(stringer, name, SIZE_SYS_FILE, ECHO);

    if (strLen(name) == 0) return;

    if (!setSpace(&roomBuf)) return;

    temp = getcwd(NULL, 100);
    sPrintf(msgBuf.mbtext, "%s\\%s", temp, name);
    homeSpace();

    if (access(msgBuf.mbtext, 4) == -1) {
        strCat(msgBuf.mbtext,
/*		  mode ? ".ARC" : ".ZIP"); */
          mode==0 ? ".ZIP" : mode==1 ? ".ARC" : mode==2 ? ".LZH" : ".ZOO");

        if (access(msgBuf.mbtext, 4) == -1) {
            mPrintf("%s not found.\n ", name);
            free(temp);
            homeSpace();
            return;
        }
    }

    stat(msgBuf.mbtext, &statbuff);
    if (statbuff.st_mode & S_IFCHR ||
        statbuff.st_mode & S_IFDIR ||
        !(statbuff.st_mode & S_IREAD)) {
        mPrintf("%s not found.\n ", name);
        free(temp);
        homeSpace();
        return;
    }

    c = msgBuf.mbtext + sPrintf(msgBuf.mbtext, "%s %s\\%s ",
/*    		mode ? ARCwork : ZIPwork, temp, name); */
			mode==0 ? ZIPwork : mode==1 ? ARCwork :
				mode==2 ? LHARCwork : ZOOwork, temp, name);
    sPrintf(stringer, "un%s mask", /* mode ? "ARC" : "ZIP"); */
			mode==0 ? "ZIP" : mode==1 ? "ARC" : mode==2 ? "LHARC" : "ZOO");
	getNormStr(stringer, c, 100, ECHO);

    if (strLen(c) == 0) strCpy(c, "*.*");

    free(temp);         /* Don't need this any longer, so kill it now */

    MakeTempDir();      /* Create and drop into */

    mPrintf("Wait...\n ");
    (*StopVideo)();
    system(msgBuf.mbtext);
    VideoInit();

    netBytes = 0l;

    if ((NumFiles = wildCard(getSize, "*.*", FALSE, "")) <= 0) {
        mPrintf("File match error.\n ");
    }
    else {
        if (netBytes > statbuff.st_size && !getYesNo(
  "It would be faster to download the compressed file; continue anyway")) {
        }
        else if (TranAdmin(protocol, NumFiles)) {
			if (protocol != ASCII) {
				if (protocol !=1 && protocol != 3)
					doTempShell(FALSE, protocol, "*.*", FALSE);
				else mPrintf
					("\n Pick a BATCH protocol and try again.\n ");
				}
            else TranSend(ASCII, transmitFile, "*.*", "", FALSE);
            }
       /* now we clean up "just in case"....*/
        chdir(TDirBuffer);
        wildCard(unlink, "*.*", FALSE, "");
    }
    homeSpace();
    rmdir(TDirBuffer);
#endif
}
