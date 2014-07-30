/************************************************************************/
/*                              libtabl.c                               */
/*                       Code to handle CTDLTABL.SYS                    */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 87Jan20 HAW  Integrity stuff for portability.                        */
/* 86Apr24 HAW  Modified for fwrite() and fread().                      */
/* 85Nov15 HAW  Created.                                                */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      readSysTab()            restores system state from ctdltabl.sys */
/*      common_read()           bottleneck for reading                  */
/*      writeSysTab()           saves state of system in CTDLTABL.SYS   */
/*      GetDynamic()            allocation bottleneck                   */
/************************************************************************/
CONFIG    cfg;                   /* A buncha variables           */
LogTable  *logTab;               /* RAM index of pippuls         */
NetTable  *netTab;               /* RAM index of nodes           */
rTable    *roomTab;              /* RAM index of rooms           */
EVENT     *EventTab = NULL;
char             *indexTable = "ctdlTabl.sys";
struct floor     *FloorTab;
int              TopFloor;
static char *msg1 = "?old ctdlTabl.sys!";

static struct {
    int checkMark;                      /* rudimentary integrity        */
    int cfgSize;                        /* sizeof cfg                   */
    int logTSize;                       /* logtab size                  */
    int endMark;                        /* another integrity check      */
} integrity;

extern char *R_W_ANY;

                        /* These two should change from major release to */
#define CHKM    7       /* major release        */
#define ENDM    8       /* CHKM was 6 and ENDM was 7)

/************************************************************************/
/*      readSysTab() restores state of system from CTDLTABL.SYS         */
/*      returns:        TRUE on success, else FALSE                     */
/*        destroys CTDLTABL.TAB after read, to prevent erroneous re-use */
/*        in the event of a crash.                                      */
/*                                                                      */
/*      MS-DOS fun: Here's the map --                                   */
/*      Word 1 == sizeof cfg                                            */
/*      Word 2 == sizeof logTab                                         */
/*      Word 3 == sizeof roomTab                                        */
/*      Word 4 -- thru x == cfg contents                                */
/*      x -- y == logTab                                                */
/*      y -- z == roomTab                                               */
/*      z -- a == netTab                                                */
/*      EOF                                                             */
/************************************************************************/
char readSysTab(kill, showMsg)
char kill, showMsg;
{
    FILE *fd;
    extern char *READ_ANY;
    int         rover;
    long        bytes;
    SYS_FILE    name;
    char        caller;

    caller = cfg.weAre;

    if ((fd = fopen(indexTable, READ_ANY)) == NULL) {
        if (showMsg) printf("?no %s!", indexTable);    /* Tsk, tsk! */
        return(FALSE);
    }

    if (fread(&integrity, sizeof integrity, 1, fd) != 1) {
        if (showMsg) printf(msg1);
        return FALSE;
    }

    if (     integrity.checkMark != CHKM ||
             integrity.endMark != ENDM ||
             integrity.cfgSize != sizeof cfg) {
        if (showMsg) printf(msg1);
        return(FALSE);
    }

    if (!common_read(&cfg, (sizeof cfg), 1, fd, showMsg))
        return FALSE;

                /* Allocations for dynamic parameters */
    logTab = (LogTable *) GetDynamic(integrity.logTSize);

    roomTab = (rTable *) GetDynamic(MAXROOMS * (sizeof (*roomTab)));

    if (cfg.netSize)
        netTab = (NetTable *) GetDynamic(sizeof (*netTab) * cfg.netSize);
    else
        netTab = NULL;

    if (cfg.EvNumber)
        EventTab  = (EVENT *)
                        GetDynamic(sizeof (*EventTab) * cfg.EvNumber);

                                 /* "- 1" is kludge */
    if (integrity.logTSize != sizeof (*logTab) * cfg.MAXLOGTAB) {
        if (showMsg) printf(msg1);
        return(FALSE);
    }

    if (!common_read(logTab, integrity.logTSize, 1, fd, showMsg))
        return FALSE;

    if (!common_read(roomTab, (sizeof (*roomTab)) * MAXROOMS, 1, fd, showMsg))
        return FALSE;

    if (cfg.netSize) {
        for (rover = 0; rover < cfg.netSize; rover++) {
            if (!common_read(&netTab[rover], NT_SIZE, 1, fd, showMsg))
                return FALSE;

            netTab[rover].netTRooms =
                                (struct shared_room *) GetDynamic(SR_BULK);
            netTab[rover].ntArchRooms =
                                (struct shared_room *) GetDynamic(NA_BULK);

            if (!common_read(netTab[rover].netTRooms, SR_BULK, 1, fd, showMsg))
                return FALSE;

            if (!common_read(netTab[rover].ntArchRooms, NA_BULK, 1, fd,
                                                        showMsg))
                return FALSE;
        }
    }

    if (cfg.EvNumber) {
        if (!common_read(EventTab, (sizeof(*EventTab) * cfg.EvNumber), 1, fd,
                                                showMsg))
        return FALSE;
    }

    fclose(fd);

    makeSysName(name, "ctdlflr.sys", &cfg.floorArea);

    if ((fd = fopen(name, R_W_ANY)) == NULL) {
        if (caller != CONFIGUR) {
            if (showMsg) printf("No floor table!");
            return FALSE;
        }
    }
    else {
        totalBytes(&bytes, fd);
        FloorTab = (struct floor *) GetDynamic((int) bytes);
        if (fread(FloorTab, (int) bytes, 1, fd) != 1) {
            if (showMsg) printf("problem reading floor tab");
            fclose(fd);
            if (caller != CONFIGUR) return FALSE;
        }
        else {
            fclose(fd);
            TopFloor = (int) bytes/sizeof(*FloorTab);
        }
    }

    if (kill) unlink(indexTable);
#ifndef NO_CRYPT
    crypte(cfg.sysPassword, sizeof cfg.sysPassword, 0);
#endif
    return(TRUE);
}

/************************************************************************/
/*      common_read() reads in from file the important stuff            */
/*      returns:        TRUE on success, else FALSE                     */
/************************************************************************/
static int common_read(block, size, elements, fd, showMsg)
void *block;
char showMsg;
int  size;
int  elements;
FILE *fd;
{
    if (size == 0) return TRUE;
    if (fread(block, size, elements, fd) != 1) {
        if (showMsg) printf(msg1);
        return FALSE;
    }
    return TRUE;
}

/************************************************************************/
/*      writeSysTab() saves state of system in CTDLTABL.SYS             */
/*      returns:        TRUE on success, else ERROR                     */
/*      See readSysTab() to see what the CTDLTABL.SYS map looks like.   */
/************************************************************************/
int writeSysTab()
{
    FILE          *fd;
    extern char   *WRITE_ANY;
    int           rover;
#ifdef MULTI_USER
    do {
		mPrintf("\n Files busy.  Please wait...");
		} while (access("roomlock.cit",0)==0));
#endif

    if ((fd = fopen(indexTable, WRITE_ANY)) == NULL) {
        printf("?can't make %s", indexTable);
        return(ERROR);
    }

    /* Write out some key stuff so we can detect bizarreness: */
    integrity.checkMark = CHKM;
    integrity.endMark = ENDM;
    integrity.cfgSize = sizeof cfg;
    integrity.logTSize = sizeof (*logTab) * cfg.MAXLOGTAB;

    fwrite(&integrity, (sizeof integrity), 1, fd);

#ifndef NO_CRYPT
    crypte(cfg.sysPassword, sizeof cfg.sysPassword, 0);
#endif
    fwrite(&cfg, (sizeof cfg), 1, fd);
#ifndef NO_CRYPT
    crypte(cfg.sysPassword, sizeof cfg.sysPassword, 0);
#endif

    fwrite(logTab, (sizeof(*logTab) * cfg.MAXLOGTAB), 1, fd);
    fwrite(roomTab, (sizeof (*roomTab)) * MAXROOMS, 1, fd);
    for (rover = 0; rover < cfg.netSize; rover++) {
        fwrite(&netTab[rover], NT_SIZE, 1, fd);
        fwrite(netTab[rover].netTRooms, SR_BULK, 1, fd);
        fwrite(netTab[rover].ntArchRooms, NA_BULK, 1, fd);
    }
    if (cfg.EvNumber)
        fwrite(EventTab, (sizeof(*EventTab) * cfg.EvNumber), 1, fd);

    fclose(fd);
    return(TRUE);
}

/************************************************************************/
/*      GetDynamic() does mallocs with error checking                   */
/************************************************************************/
char *GetDynamic(size)
unsigned size;
{
    char *temp;
    void *calloc();

    if (size == 0) return NULL; /* Simplify code                */
    temp = calloc(1, size);     /* 0 out all allocated blocks           */
/* printf("Requested %d bytes, received address %p\n", size, temp); */
    if (temp == NULL) {
        printf("Request for %u bytes failed.\n", size);
        crashout("Memory failure -- I need more memory!");
    }
    return temp;
}

/************************************************************************/
/*      openFile() opens one of the .sys files.                         */
/************************************************************************/
void openFile(filename, fd)
char *filename;
FILE **fd;
{
                /* We use fopen here rather than safeopen for link reasons */
    /* open message file */
    if ((*fd = fopen(filename, R_W_ANY)) == NULL) {
        printf("?no %s", filename);
        exit(SYSOP_EXIT);
    }
}
