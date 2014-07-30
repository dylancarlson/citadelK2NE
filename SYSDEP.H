/************************************************************************/
/*                              SysDep.H                                */
/*      #include file for all Citadel C files; contains system          */
/*      dependent code.                                                 */
/************************************************************************/

#define TURBO_C

#ifdef TURBO_C

#include "stdlib.h"
#include "ctype.h"
#include "dos.h"
#include "dir.h"
#include "string.h"
#include "mem.h"
#include "io.h"
#include "conio.h"

#define ANSI_PROTOTYPING

#endif

#define SYSTEM_CLOCK
#define DeARC_SUPPORT

/************************************************************************/
/* These constants, etc. are only for use by SYSDEP.C and other source  */
/* files that contain code that depends on the computer system in use.  */
/* Access is via #define in the appropriate source files.               */
/************************************************************************/


/*
 * Required! The following MUST be defined:
 *   MSG_SECT_SIZE -- size, in bytes, of a (psuedo) "sector" on disk
 *                    for the message file
 *   SIZE_SYS_FILE -- longest length, in bytes, of a "system" (i.e., *.SYS,
 *                    *.BLB, *.MNU, *.HLP, and net files) file plus 1 (for
 *                    the NULL byte.  Variables using this constant are used for
 *                    defining a name of a file and then opening it.
 *   MSG_NUMBER  -- just what is* a message number
 *   SECTOR_ID   -- just what to use to identify a sector
 *   AN_UNSIGNED -- an unsigned quantity, very preferably 8 bits
 *   ROOM_MSG    -- this needs* to be 16 bits, for a kludge in listRooms()
 *   DATA_BLOCK[MSG_SECT_SIZE] -- a block of msg data from disk
 *   CRC_TYPE    -- sufficient to hold a XMODEM CRC value
 *   SYS_FILE    -- a variable used in makeSysName() that holds the "name" of
 *                  a system file (see SIZE_SYS_FILE).  This is passed to
 *                  safeopen().  Normally, this would be just a string using
 *                  the SIZE_SYS_FILE constant, but I can see situations in
 *                  which a struct would be more appropriate.  If you have*
 *                  to go with a struct, I suggest making safeopen() a macro
 *                  that takes the address of the name (see description of
 *                  safeopen for its function).  I.e...
 *  #define safeopen(x, y)    ourOpen(&x, y)
 *   ROOM_AREA   -- definition of an area attached to a room
 *   NET_AREA    -- definition of an area accessible to net functions
 *   SYS_AREA    -- definition of an area that may be occupied by a Citadel
 *                  system file.
 *   MODEM_DATA  -- all necessary modem data for this implementation
 *
 */
#define MSG_SECT_SIZE   128
#define SIZE_SYS_FILE   31              /* Should be enough             */

#define LOCKFILE        "ctdllock.sys"

typedef unsigned long MSG_NUMBER;       /* Msg number for PClone 32 bits*/
typedef unsigned      SECTOR_ID;        /* Sector ID -- 16 bits         */
typedef unsigned char AN_UNSIGNED;
typedef unsigned int  ROOM_MSG;
typedef unsigned char DATA_BLOCK[MSG_SECT_SIZE];
typedef unsigned      CRC_TYPE;
typedef char          SYS_FILE[SIZE_SYS_FILE];
typedef unsigned long MULTI_NET_DATA;
typedef unsigned long  UNS_32;
typedef unsigned short UNS_16;

typedef int ROOM_AREA;          /* This might carry the # of the room   */

typedef struct {
    char        naDisk;         /* Disk on MS-DOS                       */
    char        naDirname[100]; /* Directory anywhere on system         */
} NET_AREA;

typedef struct {
    char        saDisk;         /* Disk on MS-DOS                       */
    int         saDirname;      /* Points into cfg.codeBuf              */
} SYS_AREA;

typedef struct {
    char search_baud,           /* Flip flop search for baud?           */
         idFromModem,           /* Get baud from modem?                 */
         IBM_or_clone;          /* Is this a PClone                     */
    int  modem_status,          /* MSR location                         */
         modem_data,            /* Data location                        */
         line_status,           /* LSR location                         */
         mdm_ctrl,              /* MCR location                         */
         ln_ctrl,               /* LCR location                         */
         ier,                   /* Interrupt Enable location            */
         com_vector,            /* Com vector                           */
         PIC_mask,              /* For the Interrupt Controller         */
                    /* routine offsets in codeBuf: */
         pBauds[6],             /* Indexes into here defined later      */
         pHangUp,               /* hang up phone                        */
         pCarrDetect,           /* detect carrier                       */
         pEnable,               /* Enable modem after disabling         */
         pDisable,              /* Disable modem                        */
         pResults[6],           /* Result codes for 300, etc.           */
         pRing,                 /* Result code for RING                 */
         pInitPort,             /* initialize modem port                */
         pCheckBaud;            /* find out what baud rate is           */
} MODEM_DATA;

typedef struct {
    char *Fdate;
    long Fsize;
} DIR_EXTRA;

#define BOOLEAN_FLAG(x)         unsigned x : 1


/* Optional gunk for this port only. */
#define mvToHomeDisk(x)         DoBdos(14, (x)->saDisk)
#define simpleGetch()           DoBdos(7, 0)

#define printArea(x)            writeArea(TRUE, x, NULL);

#define makeSysName(x, y, z)    sPrintf(x, "%c:%s%s", (z)->saDisk + 'a',\
                                  cfg.codeBuf + (z)->saDirname, y)

#define makeVASysName(x, y)     sPrintf(x, "virtual\\%s", y)

/* #define callLogActive(x)        (x)->saDisk >= 100 */

#define dirString(x, r)         writeArea(FALSE, r, x)

#define zero_struct(x)  setmem(&x, sizeof x, 0)
#define zero_array(x)   setmem(x, sizeof x, 0)
#define copy_struct(src, dest)  movmem(&src, &dest, sizeof src)
#define copy_array(src, dest)   movmem(src, dest, sizeof src)
#define copy_ptr(src, dest, s)  movmem(src, dest, (sizeof src[0]) * s)

/* #define EnableModem()   interpret(cfg.modemData.pEnable) */
#define EnableModem()      fossilDTR(1)
/* #define DisableModem()  interpret(cfg.modemData.pDisable) */
#define DisableModem()     fossilDTR(0)

#define RoomSys(x)      roomBuf.rbArea = x

#define CreateVAName(fn, slot, dir, num) \
                sPrintf(fn, "virtual\\%d\\%s\\%ld", slot, dir, num)

#define mAnyPrintf      mTrPrintf
/************************************************************************/
/* These constants, etc. are only for use by SYSDEP.C and other source  */
/* files that contain code that depends on the computer system in use.  */
/* Access is via #define in the appropriate source files.               */
/************************************************************************/

#ifdef SYSTEM_DEPENDENT

/*      stuff for the mini-INTRPreter that drives the modem:            */
/* opcodes for the interpreter: */
#define ANDI            1
#define INP             2
#define LOAD            3
#define LOADI           4
#define ORI             5
#define OUTP            6
#define OUTSTRING       7
#define PAUSEI          8
#define RET             9
#define STORE          10
#define XORI           11
#define STOREX         12
#define LOADX          13
#define OPRNUMBER      14
#define TOBDC          15
#define TODEC          16

#endif

#ifdef CONFIGURE

#define HELP            0
#define LOG             1
#define ROOM            2
#define MSG             3
#define MSG2            4
#define NET_STUFF       5
#define CALL            6
#define HOLD            7
#define FLOORA          8

#define I_HANGUP        0
#define I_INIT          1
#define I_CARRDET       2
#define I_SET300        3
#define I_SET1200       4
#define I_SET2400       5
#define I_SETHI         6
#define I_CHECKB        7
#define I_ENABLE        8
#define I_DISABLE       9
#define I_SET4800       10
#define I_SET9600       11

        /* Hmmmmmm.  This stuff shouldn't be here, actually. */

#define R_SH_MARK       "&&"    /* Rather than external char    */
#define NON_LOC_NET     "%%"

#endif
        /* Or this.  Fix this someday */

#ifdef V_ADMIN

#define R_SH_MARK       "&&"    /* Rather than external char    */
#define NON_LOC_NET     "%%"
#define LOC_NET         "++"

#endif

#define BAD_DIR         -1      /* chdir() return value */

#define strCmp          strcmp
#define strLen          strlen
#define sPrintf         sprintf
#define toUpper         toupper
#define toLower         tolower
#define strCpy          strcpy
#define strCat          strcat
#define isAlpha         isalpha
#define strnCmp         strncmp
#define isSpace         isspace

#define KBReady()       kbhit()
#define getCh           getch

#define totalBytes(x, fd)       *(x) = filelength(fileno(fd))
#define DoBdos(x, y)    bdos(x, y, 0)

#define unopenSize(x, y) *(x) = GblOther.Fsize

#undef inp

/* end of this file */
