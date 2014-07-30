/************************************************************************/
/*                              syscfg.c                                */
/*      configuration program for Citadel bulletin board system.  This  */
/* file contains the system dependent code!                             */
/************************************************************************/

#define SYSTEM_DEPENDENT
#define CONFIGURE

#include "ctdl.h"
#include "sys\stat.h"

/************************************************************************/
/*                              History                                 */
/*                                                                      */
/* 87Jan19 HAW  Created.                                                */
/************************************************************************/

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/* #    dirExists()             check to see if directory exists        */
/* #    doCommon()              handles common stuff                    */
/*      initSysSpec()           initialization for system dependencies  */
/*      SysDepIntegrity()       makes necessary checks for integrity    */
/*      sysSpecs()              System-dependent code for configure     */
/*                                                                      */
/*              # means local for this implementation                   */
/************************************************************************/

/************************************************************************/
/*              Statics                                                 */
/************************************************************************/
static int  necessary[13]   = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char interpCheck[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char modemSetup[100], modemReset[100], modemIdle[50], modemAnswer[50];
static char curDisk;
static char IBMcom = -1;

#define MAX_ROUTINE_CODE        30

struct routine {
    char code[MAX_ROUTINE_CODE];
    int  codeLength;
} ;

struct allRoutines {
    struct routine carrier,
                   hangup,
                   disable,
                   enable,
                   initPort,
                   checkBaud,
                   set300,
                   set1200,
                   set2400,
                   set4800,
                   set9600,
                   setHigher;
} ;

static struct allRoutines zenith = {
 { { 2, 0xED, 0, 1, 0x40, 9 }, 6 },
 { { 2, 0xEF, 0, 1, 0xFD, 6, 0xEF, 0, 8, 0x50, 11, 2, 6, 0xEF, 0, 9 }, 16 },
 { { LOADX, 0, ANDI, 0xFD, OUTP, 0xEF, 0, STOREX, 0, RET}, 10 },
 { { LOADX, 0, ORI, 0x02, OUTP, 0xEF, 0, STOREX, 0, RET}, 10 },
 { { 7 }, 1 },  /* Init port */
 { { 4, 0, 9 }, 3 },    /* Check baud */
 { { 4, 0x4E, 6, 0xEE, 0, 4, 0x76, 6, 0xEE, 0, 9 }, 11 },
 { { 4, 0x4E, 6, 0xEE, 0, 4, 0x78, 6, 0xEE, 0, 9 }, 11 },
 { { 4, 0x4E, 6, 0xEE, 0, 4, 0x7B, 6, 0xEE, 0, 9 }, 11 },
 { { 4, 0x4E, 6, 0xEE, 0, 4, 0x7C, 6, 0xEE, 0, 9 }, 11 },
 { { 4, 0x4E, 6, 0xEE, 0, 4, 0x7D, 6, 0xEE, 0, 9 }, 11 },
 { { 9 }, 1 }
 } ;

static struct allRoutines ibm1 = {
 { { 2, 0xFE, 3, 1, 0x80, 9 }, 6 },
 { { 4, 0, 6, 0xFC, 3, 8, 0x50, 4, 1, 6, 0xFC, 3, 9 }, 13 },
 { { LOADX, 0, ANDI, 0xFE, OUTP, 0xFC, 3, STOREX, 0, RET }, 10 },
 { { LOADX, 0, ORI,  0x01, OUTP, 0xFC, 3, STOREX, 0, RET }, 10 },
 { { 4, 0x83, 6, 0xFB, 3, 4, 0x80, 6, 0xF8, 3, 4, 1, 6, 0xF9, 3, 4, 1,
          6, 0xFC, 3, 4, 3, 6, 0xFB, 3, 7 }, 26 },
 { { 2, 0xFF, 3, 1, 1, 11, 1, 9}, 8 },
 { { 4, 0x83, 6, 0xFB, 3, 4, 0x80, 6, 0xF8, 3, 4, 1, 6, 0xF9, 3, 4, 3,
                                 6, 0xFB, 3, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 3, 4, 0x60, 6, 0xF8, 3, 4, 0, 6, 0xF9, 3, 4, 3,
                                 6, 0xFB, 3, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 3, 4, 0x30, 6, 0xF8, 3, 4, 0, 6, 0xF9, 3, 4, 3,
                                 6, 0xFB, 3, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 3, 4, 0x18, 6, 0xF8, 3, 4, 0, 6, 0xF9, 3, 4, 3,
                                 6, 0xFB, 3, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 3, 4, 0x0C, 6, 0xF8, 3, 4, 0, 6, 0xF9, 3, 4, 3,
                                 6, 0xFB, 3, 9 }, 21 },
 { { 9 }, 1 }
} ;

static struct allRoutines ibm2 = {
 { { 2, 0xFE, 2, 1, 0x80, 9 }, 6 },
 { { 4, 0, 6, 0xFC, 2, 8, 0x50, 4, 1, 6, 0xFC, 2, 9 }, 13 },
 { { LOADX, 0, ANDI, 0xFE, OUTP, 0xFC, 2, STOREX, 0, RET }, 10 },
 { { LOADX, 0, ORI,  0x01, OUTP, 0xFC, 2, STOREX, 0, RET }, 10 },
 { { 4, 0x83, 6, 0xFB, 2, 4, 0x80, 6, 0xF8, 2, 4, 1, 6, 0xF9, 2, 4, 1,
          6, 0xFC, 2, 4, 3, 6, 0xFB, 2, 7 }, 26 },
 { { 2, 0xFF, 2, 1, 1, 11, 1, 9}, 8 },
 { { 4, 0x83, 6, 0xFB, 2, 4, 0x80, 6, 0xF8, 2, 4, 1, 6, 0xF9, 2, 4, 3,
                                 6, 0xFB, 2, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 2, 4, 0x60, 6, 0xF8, 2, 4, 0, 6, 0xF9, 2, 4, 3,
                                 6, 0xFB, 2, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 2, 4, 0x30, 6, 0xF8, 2, 4, 0, 6, 0xF9, 2, 4, 3,
                                 6, 0xFB, 2, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 2, 4, 0x18, 6, 0xF8, 2, 4, 0, 6, 0xF9, 2, 4, 3,
                                 6, 0xFB, 2, 9 }, 21 },
 { { 4, 0x83, 6, 0xFB, 2, 4, 0x0C, 6, 0xF8, 2, 4, 0, 6, 0xF9, 2, 4, 3,
                                 6, 0xFB, 2, 9 }, 21 },
 { { 9 }, 1 }
} ;

/************************************************************************/
/*                External variable declarations in CONFG.C             */
/************************************************************************/
        /***** THESE ARE REQUIRED DEFINITIONS! ******/
char *R_W_ANY =   "r+b";
char *W_R_ANY =   "w+b";
char *READ_ANY =  "rb";
char *WRITE_ANY = "wb";
char *READ_TEXT = "rt";

FILE *modemInitFile;

        /*****  done  *****/

extern CONFIG cfg;       /* The configuration variable           */
extern MSG_BUF   msgBuf;


/************************************************************************/
/*      dirExists() check to see if the directory exists                */
/************************************************************************/
void dirExists(disk, theDir)
char disk, *theDir;
{
    struct stat buff;

    sPrintf(msgBuf.mbtext, "%c:%s", disk + 'A', theDir);
    if (strLen(theDir) == 0) return ;
    if (msgBuf.mbtext[strLen(msgBuf.mbtext)-1] == '\\')
        msgBuf.mbtext[strLen(msgBuf.mbtext)-1] = '\0';
    if (stat(msgBuf.mbtext, &buff) == 0) {
        if (!(buff.st_mode & S_IFDIR)) {
            sPrintf(msgBuf.mbtext, "%c:%s is not a directory!",
                  disk + 'A', theDir);
            illegal(msgBuf.mbtext);
        }
    }
    else {
        printf("\nThe directory '%s' doesn't exist.  Create it? ", 
                                                        msgBuf.mbtext);
        if (toUpper(simpleGetch()) == 'Y')
            if (mkdir(msgBuf.mbtext) == BAD_DIR)
                illegal("Couldn't make directory!");
    }
}

/************************************************************************/
/*      sysSpecs()  system specific configure stuff                     */
/************************************************************************/
int sysSpecs(line, offset, status)
char *line, *status;
int  offset;
{
    char cmd[90], var[90], string[90];
    int  i, arg2, arg, *intPtr;

    *status = TRUE;
    if (sscanf(line, "%s %s ", var, string)) {
        if (string[0] != '\"') {
            arg = atoi(string);
        }
               if (strCmp(var, "#SEARCHBAUD")    == SAMESTRING) {
            cfg.modemData.search_baud = arg;
        } else if (strCmp(var, "#IBM"       )    == SAMESTRING) {
            cfg.modemData.IBM_or_clone= arg;
        } else if (strCmp(var, "#COM"       )    == SAMESTRING) {
            if (arg == 1) {
                IBMcom = 1;
                cfg.modemData.modem_data = 0x3f8;
                cfg.modemData.modem_status = 0x3fd;
                cfg.modemData.line_status = 0x3fd;
                cfg.modemData.mdm_ctrl = 0x3fc;
                cfg.modemData.ln_ctrl = 0x3fb;
                cfg.modemData.ier = 0x3f9;
                cfg.modemData.com_vector = 12;
                cfg.modemData.PIC_mask = 0xef;
            }
            else if (arg == 2) {
                IBMcom = 2;
                cfg.modemData.modem_data = 0x2f8;
                cfg.modemData.modem_status = 0x2fd;
                cfg.modemData.line_status = 0x2fd;
                cfg.modemData.mdm_ctrl = 0x2fc;
                cfg.modemData.ln_ctrl = 0x2fb;
                cfg.modemData.ier = 0x2f9;
                cfg.modemData.com_vector = 11;
                cfg.modemData.PIC_mask = 0xf7;
            }
            else illegal("COM port can only currently be 1 or 2");
        } else if (strCmp(var, "#HELPAREA"  )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.homeArea, offset, HELP);
        } else if (strCmp(var, "#LOGAREA"  )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.logArea, offset, LOG);
        } else if (strCmp(var, "#ROOMAREA"  )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.roomArea, offset, ROOM);
        } else if (strCmp(var, "#MSGAREA"  )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.msgArea, offset, MSG);
        } else if (strCmp(var, "#MSG2AREA" )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.msg2Area, offset, MSG2);
        } else if (strCmp(var, "#NETAREA"  )    == SAMESTRING) {
          offset = doAreaCommon(var, line, &cfg.netArea, offset, NET_STUFF);
        } else if (strCmp(var, "#CALLAREA"  )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.call_log, offset, CALL);
            cfg.BoolFlags.Calllog = TRUE;
        } else if (strCmp(var, "#HOLDAREA"  )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.holdArea, offset, HOLD);
            cfg.BoolFlags.HoldOnLost = TRUE;
        } else if (strCmp(var, "#FLOORAREA" )    == SAMESTRING) {
            offset = doAreaCommon(var, line, &cfg.floorArea, offset, FLOORA);
        } else if (strCmp(var, "#NET_RECEPT_AREA")   == SAMESTRING) {
            anyArea(var, line, &cfg.receptArea.naDisk, cfg.receptArea.naDirname);
        } else if (strCmp(var, "#RESULT-300") == SAMESTRING) {
            offset = doResCodes(var, line, offset, ONLY_300);
        } else if (strCmp(var, "#RESULT-1200") == SAMESTRING) {
            offset = doResCodes(var, line, offset, BOTH_300_1200);
        } else if (strCmp(var, "#RESULT-2400") == SAMESTRING) {
            offset = doResCodes(var, line, offset, TH_3_12_24);
        } else if (strCmp(var, "#RESULT-4800") == SAMESTRING) {
            offset = doResCodes(var, line, offset, B_4);
        } else if (strCmp(var, "#RESULT-9600") == SAMESTRING) {
            offset = doResCodes(var, line, offset, B_5);
        } else if (strCmp(var, "#RESULT-YOUROWN") == SAMESTRING) {
            offset = doResCodes(var, line, offset, SetYourOwn);
        } else if (strCmp(var, "#RING") == SAMESTRING) {
            cfg.modemData.pRing = offset;
            readString(line, &cfg.codeBuf[offset], FALSE);
            while (cfg.codeBuf[offset]) /* step over string     */
                offset++;
            cfg.codeBuf[offset++] = '\0'; /* tie off with null    */
        } else if (stricmp(var, "#modemSetup"   ) == SAMESTRING) {
            readString(line, modemSetup, TRUE);
            strCat(modemSetup, "\r");
            interpCheck[I_INIT]++;
            unlink("initline.sys");
            modemInitFile=fopen("initline.sys", "wt");
			fprintf(modemInitFile, "%s", modemSetup);
			fclose(modemInitFile);
        } else if (stricmp(var, "#modemReset"   ) == SAMESTRING) {
            readString(line, modemReset, TRUE);
            strCat(modemReset, "\r");
            unlink("resetmdm.sys");
            modemInitFile=fopen("resetmdm.sys", "wt");
			fprintf(modemInitFile, "%s", modemReset);
			fclose(modemInitFile);
        } else if (stricmp(var, "#modemIdle"   ) == SAMESTRING) {
            readString(line, modemIdle, TRUE);
            strCat(modemIdle, "\r");
            unlink("idleline.sys");
            modemInitFile=fopen("idleline.sys", "wt");
			fprintf(modemInitFile, "%s", modemIdle);
			fclose(modemInitFile);
        } else if (stricmp(var, "#modemAnswer"   ) == SAMESTRING) {
            readString(line, modemAnswer, TRUE);
            strCat(modemAnswer, "\r");
            unlink("answrmdm.sys");
            modemInitFile=fopen("answrmdm.sys", "wt");
			fprintf(modemInitFile, "%s", modemAnswer);
			fclose(modemInitFile);
		} else if (strCmp(var, "#start"  ) == SAMESTRING) {
            sscanf(line, "%s %s %x ", cmd, var, &arg);
            printf("#start procedure '%s'\n", var);
            if        (strCmp(var, "HANGUP"    ) == SAMESTRING) {
                cfg.modemData.pHangUp     = offset;
                interpCheck[I_HANGUP]++;
            } else if (strCmp(var, "INITPORT"  ) == SAMESTRING) {
                cfg.modemData.pInitPort   = offset;
                interpCheck[I_INIT] += 2;
            } else if (strCmp(var, "CARRDETECT") == SAMESTRING) {
                cfg.modemData.pCarrDetect = offset;
                interpCheck[I_CARRDET]++;
            } else if (strCmp(var, "SET300"    ) == SAMESTRING) {
                cfg.modemData.pBauds[ONLY_300] = offset;
                interpCheck[I_SET300]++;
            } else if (strCmp(var, "SET1200"   ) == SAMESTRING) {
                cfg.modemData.pBauds[BOTH_300_1200] = offset;
                interpCheck[I_SET1200]++;
            } else if (strCmp(var, "SET2400"   ) == SAMESTRING) {
                cfg.modemData.pBauds[TH_3_12_24] = offset;
                interpCheck[I_SET2400]++;
            } else if (strCmp(var, "SET4800"   ) == SAMESTRING) {
                cfg.modemData.pBauds[B_4] = offset;
                interpCheck[I_SET4800]++;
            } else if (strCmp(var, "SET9600"   ) == SAMESTRING) {
                cfg.modemData.pBauds[B_5] = offset;
                interpCheck[I_SET9600]++;
            } else if (strCmp(var, "SET_HIGHER") == SAMESTRING) {
                cfg.modemData.pBauds[SetYourOwn] = offset;
                interpCheck[I_SETHI]++;
            } else if (strCmp(var, "CHECKBAUD" ) == SAMESTRING) {
                cfg.modemData.pCheckBaud  = offset;
                interpCheck[I_CHECKB]++;
            } else if (strCmp(var, "ENABLE"    ) == SAMESTRING) {
                cfg.modemData.pEnable     = offset;
                interpCheck[I_ENABLE]++;
            } else if (strCmp(var, "DISABLE"   ) == SAMESTRING) {
                cfg.modemData.pDisable     = offset;
                interpCheck[I_DISABLE]++;
            } else
                *status = FALSE;
        } else if (strCmp(var, "#code"   ) == SAMESTRING) {
            sscanf(line, "%s %s %x ", cmd, var, &arg);
            printf("#code '%s'\n", var);
            if        (strCmp(var, "LOAD"      ) == SAMESTRING) {
                cfg.codeBuf[offset++] = LOAD;
                intPtr = (int *) &cfg.codeBuf[offset];
                *intPtr = arg;
                offset += 2;
            } else if (strCmp(var, "ANDI"      ) == SAMESTRING) {
                cfg.codeBuf[offset++] = ANDI;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "ORI"       ) == SAMESTRING) {
                cfg.codeBuf[offset++] = ORI;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "XORI"      ) == SAMESTRING) {
                cfg.codeBuf[offset++] = XORI;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "STORE"     ) == SAMESTRING) {
                cfg.codeBuf[offset++] = STORE;
                intPtr = (int *) &cfg.codeBuf[offset];
                *intPtr = arg;
                offset += 2;
            } else if (strCmp(var, "LOADI"     ) == SAMESTRING) {
                cfg.codeBuf[offset++] = LOADI;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "LOADX"     ) == SAMESTRING) {
                cfg.codeBuf[offset++] = LOADX;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "RET"       ) == SAMESTRING) {
                cfg.codeBuf[offset++] = RET;
            } else if (strCmp(var, "INP"       ) == SAMESTRING) {
                cfg.codeBuf[offset++] = INP;
                intPtr = (int *) &cfg.codeBuf[offset];
                *intPtr = arg;
                offset += 2;
            } else if (strCmp(var, "OUTP"      ) == SAMESTRING) {
                cfg.codeBuf[offset++] = OUTP;
                intPtr = (int *) &cfg.codeBuf[offset];
                *intPtr = arg;
                offset += 2;
            } else if (strCmp(var, "PAUSEI"    ) == SAMESTRING) {
                cfg.codeBuf[offset++] = PAUSEI;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "ARRAY[]="  ) == SAMESTRING) {
                cfg.codeBuf[offset++] = STOREX;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "ARRAY[]"   ) == SAMESTRING) {
                cfg.codeBuf[offset++] = LOADX;
                cfg.codeBuf[offset++] = arg;
            } else if (strCmp(var, "OPR#"      ) == SAMESTRING) {
                cfg.codeBuf[offset++] = OPRNUMBER;

                /* reparse to pick up string: */
                sscanf(line, "%s %s \"%s\" %d %d",
                    cmd, var, string, &arg, &arg2
                );
         /* BDS was excellent.  C86 sucks.  Reparse by hand <sigh>: */
                for (i = strLen(line) - 1; line[i] != '"'; i--)
                    ;
                sscanf(line + ++i, " %d %d", &arg, &arg2);

                string[strLen(string) - 1] = '\0';  /* Bug kludge   */
                /* copy string into code buffer: */
                strCpy(&cfg.codeBuf[offset], string);
                while (cfg.codeBuf[offset]) /* step over string     */
                    offset++;
                offset++;
                cfg.codeBuf[offset++] = arg;  /* lower limit          */
                cfg.codeBuf[offset++] = arg2; /* upper limit          */
            } else if (strCmp(var, "OUTSTRING" ) == SAMESTRING) {
                cfg.codeBuf[offset++] = OUTSTRING;
                /* reparse to pick up string: */
    /*              sscanf(line, "%s %s \"%s\"", cmd, var, string);     */
    /* Reparse on our own (stupid scanf() function, it worked in BDS!): */
                readString(line, &cfg.codeBuf[offset], TRUE);
                while (cfg.codeBuf[offset]) /* step over string     */
                    offset++;
                cfg.codeBuf[offset++] = '\r'; /* add a CR             */
                cfg.codeBuf[offset++] = '\0'; /* tie off with null    */
            } else /*printf("?--no code '%s'!\n", var); */
                    *status = FALSE;
        }
        else *status = FALSE;
    }
    return offset;
}

#define makeN(x)        x + cfg.codeBuf

/************************************************************************/
/*      SysDepIntegrity() makes necessary checks for integrity          */
/************************************************************************/
char SysDepIntegrity(offset)
int *offset;
{
    char bad = FALSE;

    if (cfg.modemData.IBM_or_clone == -1) {
        printf("You didn't specify if this is IBM or Zenith!\n");
        bad = TRUE;
    }

    if (cfg.modemData.IBM_or_clone && IBMcom == -1) {
        printf("Need a COM setting for PClones!\n");
        bad = TRUE;
    }

    if (bad == FALSE) cfg.FOSSIL_PORT = (IBMcom-1);

    if (!necessary[HELP]) {
        printf("The help stuff was not fully defined\n");
        bad = TRUE;
    }
    else dirExists(cfg.homeArea.saDisk, makeN(cfg.homeArea.saDirname));

    if (!necessary[LOG ]) {
        printf("The log stuff was not fully defined\n");
        bad = TRUE;
    }
    else dirExists(cfg.logArea.saDisk, makeN(cfg.logArea.saDirname));

    if (!necessary[ROOM]) {
        printf("The room stuff was not fully defined\n");
        bad = TRUE;
    }
    else dirExists(cfg.roomArea.saDisk, makeN(cfg.roomArea.saDirname));

    if (!necessary[MSG ]) {
        printf("The msg stuff was not fully defined\n");
        bad = TRUE;
    }
    else dirExists(cfg.msgArea.saDisk, makeN(cfg.msgArea.saDirname));

    if (!necessary[FLOORA]) {
        printf("The floor stuff was not fully defined\n");
        bad = TRUE;
    }
    else dirExists(cfg.floorArea.saDisk, makeN(cfg.floorArea.saDirname));

    if (!necessary[MSG2] && cfg.BoolFlags.mirror) {
        printf("The mirror message stuff was not fully defined\n");
        bad = TRUE;
    }
    if (cfg.BoolFlags.mirror)
         dirExists(cfg.msg2Area.saDisk, makeN(cfg.msg2Area.saDirname));

    if (!necessary[NET_STUFF] && cfg.BoolFlags.netParticipant) {
        printf("The net stuff was not fully defined\n");
        bad = TRUE;
    }
    else if (cfg.BoolFlags.netParticipant) {
        dirExists(cfg.netArea.saDisk, makeN(cfg.netArea.saDirname));
        dirExists(cfg.receptArea.naDisk, cfg.receptArea.naDirname);
    }

    if (!necessary[CALL] && cfg.BoolFlags.Calllog) {
        printf("The call stuff was not fully defined\n");
        bad = TRUE;
    }
    else if (cfg.BoolFlags.Calllog)
        dirExists(cfg.call_log.saDisk, makeN(cfg.call_log.saDirname));

    if (necessary[HOLD])
        dirExists(cfg.holdArea.saDisk, makeN(cfg.holdArea.saDirname));

    if (bad)
        illegal("See above.");
    else {
        *offset = setInterps(*offset,
                      (!cfg.modemData.IBM_or_clone) ? &zenith :
                      (IBMcom == 1) ? &ibm1 : &ibm2);
    }

    return TRUE;
}

/************************************************************************/
/*      doResCodes() common work for result codes                       */
/************************************************************************/
int doResCodes(var, line, offset, which)
char *line, *var;
int offset, which;
{
    readString(line, &cfg.codeBuf[offset], FALSE);
    cfg.modemData.idFromModem++;
    cfg.modemData.pResults[which] = offset;
    while (cfg.codeBuf[offset]) offset++;
    cfg.codeBuf[offset++] = '\0'; /* tie off with null    */
    return offset;
}

/************************************************************************/
/*      doAreaCommon() handles common stuff                             */
/************************************************************************/
int doAreaCommon(var, line, area, offset, which)
char     *line, *var;
SYS_AREA *area;
int      offset, which;
{
    int old;

    anyArea(var, line, &area->saDisk, cfg.codeBuf + offset);
    area->saDirname = old = offset;
    while (cfg.codeBuf[offset]) /* step over string     */
        offset++;

    if (strchr(cfg.codeBuf + old, '\\') != NULL)
        illegal("Directory name cannot have a '\\' in it!");

    if (old != offset) {
        cfg.codeBuf[offset++] = '\\';
        cfg.codeBuf[offset] = 0;
    }
    offset++;
    necessary[which]++;
    return offset;
}

/************************************************************************/
/*      anyArea() handles common stuff                                  */
/************************************************************************/
void anyArea(var, line, disk, target)
char     *line, *var, *target, *disk;
{
    readString(line, target, FALSE);

    MSDOSparse(target, disk);
}

/************************************************************************/
/*      initSysSpec() initialization for system dependencies            */
/************************************************************************/
void initSysSpec()
{
    char curDir[80];
    int i;

    cfg.modemData.IBM_or_clone = -1;
    getcwd(curDir, 79);
    curDisk = curDir[0] - 'A';
    cfg.modemData.idFromModem = 0;
    for (i = 0; i < 4; i++)
        cfg.modemData.pResults[i] = 0;
    cfg.modemData.pRing = 0;
}

/************************************************************************/
/*      setInterps() makes sure all interpreter routines are set        */
/************************************************************************/
setInterps(offset, interps)
struct allRoutines *interps;
int offset;
{
    if (!interpCheck[I_HANGUP]) {
        cfg.modemData.pHangUp     = offset;
        offset = copyRoutine(offset, &interps->hangup);
    }

    switch (interpCheck[I_INIT]) {
    case 0:
        illegal("Must either have an INITPORT routine or a #modemSetup!");
    case 3:
        illegal("Cannot have both an INITPORT routine and a #modemSetup!");
    case 1:
        cfg.modemData.pInitPort   = offset;
        offset = copyRoutine(offset, &interps->initPort);
        strCpy(cfg.codeBuf + offset, modemSetup);
        while (cfg.codeBuf[offset]) offset++;
        cfg.codeBuf[++offset] = 9;      /* Set RET command */
        offset++;
    }

    if (!interpCheck[I_CARRDET]) {
        cfg.modemData.pCarrDetect = offset;
        offset = copyRoutine(offset, &interps->carrier);
    }

    if (!interpCheck[I_SET300]) {
        cfg.modemData.pBauds[ONLY_300] = offset;
        offset = copyRoutine(offset, &interps->set300);
    }

    if (!interpCheck[I_SET1200]) {
        cfg.modemData.pBauds[BOTH_300_1200] = offset;
        offset = copyRoutine(offset, &interps->set1200);
    }

    if (!interpCheck[I_SET2400]) {
        cfg.modemData.pBauds[TH_3_12_24] = offset;
        offset = copyRoutine(offset, &interps->set2400);
    }

    if (!interpCheck[I_SETHI]) {
        cfg.modemData.pBauds[SetYourOwn] = offset;
        offset = copyRoutine(offset, &interps->setHigher);
    }

    if (!interpCheck[I_CHECKB]) {
        cfg.modemData.pCheckBaud  = offset;
        offset = copyRoutine(offset, &interps->checkBaud);
    }
    if (!interpCheck[I_ENABLE]) {
        cfg.modemData.pEnable = offset;
        offset = copyRoutine(offset, &interps->enable);
    }

    if (!interpCheck[I_DISABLE]) {
        cfg.modemData.pDisable = offset;
        offset = copyRoutine(offset, &interps->disable);
    }
}

int copyRoutine(offset, thisOne)
struct routine *thisOne;
int offset;
{
    int i;

    for (i = 0; i < thisOne->codeLength; i++)
        cfg.codeBuf[offset++] = thisOne->code[i];
    return offset;
}

/************************************************************************/
/*      MSDOSparse() parses a string                                    */
/************************************************************************/
void MSDOSparse(theDir, drive)
char *theDir, *drive;
{
    if (theDir[1] == ':') {
        *drive = toUpper(theDir[0]) - 'A';
        strCpy(theDir, theDir+2);
    }
    else {
        *drive = curDisk;
    }
}
