/************************************************************************/
/*                              Ctdl.h                                  */
/*      #include file for all Citadel C files.                          */
/*      Now includes only #defines and structs.                         */
/************************************************************************/

#include "stdio.h"
#include "sysdep.h"

/************************************************************************/
/* 85Oct16 HAW  Add code for OFFICE-STUFF parameter.                    */
/* 85Aug29 HAW  Install code to allow double msg files for autobackup.  */
/* 85Jun19 HAW  Implant exit values so batch files can be made useful.  */
/* 85May27 HAW  Start adding networking gunk.                           */
/* 85May22 HAW  MAXLOGTAB now sysop selectable.                         */
/* 85May06 HAW  Add daily bailout parameter.                            */
/* 85May05 HAW  Add SYSDISK parameter.                                  */
/* 85Mar20 HAW  Add timestamp code.                                     */
/* 85Feb21 HAW  Add directory names.                                    */
/* 85Feb20 HAW  Implement IMPERVIOUS flag.                              */
/* 85Feb18 HAW  Insert global variables for baud search.                */
/* 85Jan20 HAW  Insert code to read from system clock.                  */
/* 84Aug30 HAW  Begin conversion to MS-DOS                              */
/*                                                                      */
/*                              #defines                82Dec10 dvm     */
/*                                                                      */
/* #defs supported in current system:                                   */
/*                                                                      */
/* #def TEST for special small-scale test version                       */
/************************************************************************/

#define NAMESIZE       20       /* length of room names                 */
#define SECTSIZE      128       /* Size of a sector (XMODEM)            */
#define YM_BLOCK_SIZE 1024      /* Size of a sector (YMODEM)            */

typedef char label[NAMESIZE];   /* Semi-generic                         */

/* Citadel programs use readSysTab() and writeSysTab() to write an      */
/* image of the external variables in RAM to disk, and later restore    */
/* it.  The image is stored in ctdlTabl.sys .  If ctdlTabl.sys is lost, */
/* confg.com will automatically reconstruct the hard way when invoked,  */
/* and write a new ctdlTabl.sys out when finished.  CtdlTabl.sys is     */
/* always destroyed after reading, to minimize the possibility of       */
/* reading an out-of-date version.  In general, the technique works     */
/* well and saves time and head-banging on bootup.  You should,         */
/* however, note carefully the following caution:                       */
/*  o  Whenever you change the declarations in Ctdl.h you should:       */
/*   -->  destroy the current ctdlTabl.sys file                         */
/*   -->  recompile and reload all citadel programs which access        */
/*        ctdlTabl.sys -- currently citadel.com & configur.com          */
/*   -->  use configur.com to build a new ctdlTabl.sys file             */
/*                                                                      */
/* If you ignore these warnings, little pixies will prick you in your   */
/* sleep for the rest of your life.                                     */
/************************************************************************/

/* universal ANSI color definitions for Alan's code a/o 90Feb04  (vaq)  */
#ifdef ANSI
	#define BRIGHT 1
	#define DIM 0
	#define SAME -1
	#define A_BLACK 0
	#define A_RED 1
	#define A_GREEN 2
	#define A_BROWN 3
	#define A_BLUE 4
	#define A_MAGENTA 5
	#define A_CYAN 6
	#define A_WHITE 7
#endif

struct MiscBool {
    BOOLEAN_FLAG(HoldOnLost);   /* Save entries when carrier lost?      */
    BOOLEAN_FLAG(mirror);       /* mirror msg option?                   */
    BOOLEAN_FLAG(unlogEnterOk); /* TRUE if OK to enter messages anon    */
    BOOLEAN_FLAG(unlogReadOk);  /* TRUE if unlogged folks can read mess */
    BOOLEAN_FLAG(unlogLoginOk); /* TRUE if spontan. new accounts ok.    */
    BOOLEAN_FLAG(nonAideRoomOk);/* TRUE general folks can make rooms    */
    BOOLEAN_FLAG(noMail);       /* TRUE if mail is not allowed          */
    BOOLEAN_FLAG(noChat);       /* TRUE if not accepting chats          */
    BOOLEAN_FLAG(netParticipant);/* TRUE if participating in the net    */
    BOOLEAN_FLAG(longHaul);     /* TRUE if a long haul system           */
    BOOLEAN_FLAG(aideSeeAll);   /* TRUE if aides see private rooms      */
    BOOLEAN_FLAG(debug);        /* TRUE for debug phase                 */
    BOOLEAN_FLAG(Calllog);      /* TRUE if we want calllog on           */
    BOOLEAN_FLAG(NetDft);       /* TRUE if new users get net privs      */
} ;

                /* Let's begin by defining the configuration struct.    */
                /* This is part of the contents of ctdltabl.sys         */
typedef struct {

    SECTOR_ID maxMSector;       /* Max # of sectors (simulated)         */

    MSG_NUMBER oldest;          /* 32-bit ID# of first message in system*/
    MSG_NUMBER newest;          /* 32-bit ID# of last  message in system*/

    int  nodeName;              /* Offsets in codeBuf                   */
    int  nodeTitle;
    int  nodeId;
    int  bRoom;
    int  MainFloor;             /* Main floor name                      */

    int  netPrefix, netSuffix;
    NET_AREA receptArea;        /* Area to accept files sent via net here */
    int  sizeArea;              /* How much room to allow for same (K)  */
    int  maxFileSize;           /* In K                                 */

    label SysopName;
    int  cryptSeed;

    MODEM_DATA modemData;

    char filter[128];           /* input character translation table    */

    SYS_AREA homeArea,          /* Location: Help files                 */
             msgArea,           /* Message file                         */
             msg2Area,          /* Mirror message file                  */
             logArea,           /* Log file                             */
             roomArea,          /* Room file                            */
             netArea,           /* Net files                            */
             call_log,          /* The call log                         */
             floorArea,         /* The floor file                       */
             holdArea;          /* Held messages (lost carrier)         */

    char sysBaud;               /* What's our baud rate going to be?    */

    int  netSize;               /* How many on the net?                 */

    int EvNumber;

/*      stuff to distinguish the various Citadel programs               */
#define CITADEL         0       /* principal program                    */
#define xxxxx           1       /* unused                               */
#define NET             2       /* network downloader                   */
#define ARCHIVER        3       /* backup program       (future)        */
#define CONFIGUR        4
#define UTILITY         5
    char    weAre;              /* set first thing by main()            */
    int     paramVers;

/*                      Stuff to size system with:                      */
/* WARNING!!! if you expand MAXROOMS beyond 128, horrible, horrible     */
/* things will happen to the userlog entries, which will no longer      */
/* fit in the assigned 256-byte records and will overwrite things       */
/* with normally undesirable results. 82Nov10CrT                        */


    int MAXLOGTAB,              /* number of log entries supported      */
        MailSlots,
        MsgsPerrm,
        MaxRooms,
        NetArchRooms,
        SharedRooms;


/*              Stuff nowadays usually in bdscio.h:                     */
/* values for functions to return: */
#define TRUE            1
#define FALSE           0
#define ERROR          -1

#define SAMESTRING      0       /* value for strcmp() & friend          */

#define PTR_SIZE        (sizeof (char *))         /* could cause problems */


/*                      Stuff for rooms:                                */
#define LOBBY           0       /* Lobby> is >always< room 0.           */
#define MAILROOM        1       /* Mail>  is >always< room 1.           */
#define AIDEROOM        2       /* Aide> is >always< room 2.            */

#define MAXCODE       800
    unsigned char codeBuf[MAXCODE];/* buffer for configuration routines */
    unsigned char scratch[40];  /* scratch space for config routines    */

    AN_UNSIGNED shave[8];       /* shave-and-a-haircut/2 bits pauses    */

    int  catChar;               /* Location of next write in msg file   */
    SECTOR_ID catSector;

    int  sizeLTentry;           /* contains size of a logTab entry      */
    char sysPassword[80];       /* Remote sysop                         */
	char lastCaller[NAMESIZE+1];/* last caller name						*/
	char whatRate[10];			/* last Baudrate of caller				*/
	int	SeeMail;				/* See mail in Mailroom					*/

/* K2NE NEW CODE on 91May13 */
    int LogTry;                 /* How many bad logins before CLICK!        */
	int Ansi;                   /* 0 for normal, 1 for ANSI console         */
    int modemSpeed;             /* 0 for slow, 1 for cpu-rate               */
    int netLog;                 /* 1 to log nets, 0 for no net log          */
    int neverChat;              /* 1 = remote sysops page a chat; 0 for all */
    int doorsOk;                /* 1 = new users get door privs             */
	int limitSession;           /* # of minutes per logged modem session    */
/* New for V6.01 */
    int FOSSIL_PORT;            /* 0 for COM1, 1 for COM2                   */
    int slowModemDelay;         /* 0 by default, otherwise set by sysop     */
    int lockFossil;             /* 0 by default, 1 if set by sysop          */
/* New for V6.02 */
#ifdef NEW_PARMS
    long DeadTime;              /* 1800 by default                          */
	long AnyNetLength;          /* 180 by default                           */
#endif
/* end K2NE NEW CODE */

    struct MiscBool BoolFlags;  /* Buncha flags                         */
} CONFIG;            /* And that's all of the variables we want to save */

/************************************************************************/
/*                                Room data                             */
/************************************************************************/
#define MAXGEN         32       /* six bits of generation => 64 of them */
#define FORGET_OFFSET  (MAXGEN / 2)     /* For forgetting rooms         */

#define UN_STACK        40      /* stack of rooms */

#define MSG_BULK        (MSGSPERRM * sizeof (theMessages))

#define RB_SIZE         (sizeof(roomBuf) - (PTR_SIZE * 1))
#define RB_TOTAL_SIZE   (RB_SIZE + MSG_BULK)

struct rflags {                 /* Room flags                           */
    BOOLEAN_FLAG(INUSE);        /* Room in use?                         */
    BOOLEAN_FLAG(PUBLIC);       /* Room public?                         */
    BOOLEAN_FLAG(ISDIR);        /* Room directory?                      */
    BOOLEAN_FLAG(PERMROOM);     /* Room permanent?                      */
    BOOLEAN_FLAG(SKIP);         /* Room skipped? (temporary for user)   */
    BOOLEAN_FLAG(UPLOAD);       /* Can room be uploaded to?             */
    BOOLEAN_FLAG(DOWNLOAD);     /* Can room be downloaded from?         */
    BOOLEAN_FLAG(SHARED);       /* Is this a shared room?               */
    BOOLEAN_FLAG(ARCHIVE);      /* Is this room archived somewhere?     */
    BOOLEAN_FLAG(ANON);         /* All messages anonymous?              */
    BOOLEAN_FLAG(NO_NET_DOWNLOAD); /* Accessible via the net for download? */
    BOOLEAN_FLAG(INVITE);
    BOOLEAN_FLAG(AUTO_NET);
    BOOLEAN_FLAG(ALL_NET);
    BOOLEAN_FLAG(READ_ONLY);    /* was rflag8 - now only Sysop writes   */
    BOOLEAN_FLAG(ALTER_NET);    /* was rflag9 - AlterNet Linked Room    */
    BOOLEAN_FLAG(ALT_LINKED);   /* was rflag10 - AltNet->CitNet link    */
    BOOLEAN_FLAG(IS_GATEWAY);   /* was rflag11 - AltNet<->CitNet link   */
    BOOLEAN_FLAG(IS_ANSI_ROOM); /* was rflag12 - Room supports ANSI msg */
} ;

typedef struct {                /* The summation of a room              */
    AN_UNSIGNED   rtgen;        /* generation # of room                 */
    struct rflags rtflags;      /* public/private flag etc              */
    label         rtname;       /* name of room                         */
    MSG_NUMBER    rtlastMessage;/* # of most recent message in room     */
    MSG_NUMBER    rtlastNet;    /* Highest outgoing net message         */
    char          rtShareType;  /* What type of sharing for this room?  */
    int           rtFlIndex;    /* Index into the floors                */
} rTable ;                      /* And see ROOMA.C for declaration      */

typedef struct {
    MSG_NUMBER rbmsgNo;     /* every message gets unique#           */
    SECTOR_ID rbmsgLoc;     /* sector message starts in             */
} theMessages;

typedef struct {                /* The appearance of a room:            */
    AN_UNSIGNED   rbgen;        /* generation # of room                 */
    struct rflags rbflags;      /* same bits as flags above             */
    label         rbname;       /* name of room                         */
    ROOM_AREA     rbArea;       /* area this room is attached to        */
    label         rbmoderator;  /* moderator for this room, if any      */
    char          rbShareType;  /* room share type                      */
    int           rbFlIndex;    /* index into the floors                */
    theMessages *msg;
} aRoom ;

/************************************************************************/
/*                            userlog stuff                             */
/************************************************************************/
#define CRYPTADD        117    /*                                      */

#define LB_SIZE         (sizeof (logBuf) - (PTR_SIZE * 2))
#define MAIL_BULK       (MAILSLOTS * sizeof (theMessages))
#define GEN_BULK        (MAXROOMS * sizeof (AN_UNSIGNED))
#define LB_TOTAL_SIZE   (LB_SIZE + MAIL_BULK + GEN_BULK)

struct lflags {                 /* Flags for person in log              */
    BOOLEAN_FLAG(FLOORS);       /* Unused for now                       */
    BOOLEAN_FLAG(LFMASK);       /* Linefeeds?                           */
    BOOLEAN_FLAG(EXPERT);       /* Expert?                              */
    BOOLEAN_FLAG(AIDE);         /* Vice-Grand-Poobah?                   */
    BOOLEAN_FLAG(L_INUSE);      /* Is this slot in use?                 */
    BOOLEAN_FLAG(TIME);         /* Send time to user of msg creation?   */
    BOOLEAN_FLAG(OLDTOO);       /* Print out last oldmessage on <N>ew?  */
    BOOLEAN_FLAG(NET_PRIVS);    /* User have net privileges?            */
    BOOLEAN_FLAG(RUGGIE);       /* Juvenile? Future fun-ness maybe      */
    BOOLEAN_FLAG(SUBSYSTEM_OK); /* User authorized to use the "*" cmd   */
    BOOLEAN_FLAG(lflag2);       /* TRUE if user is BANNED from DOORS    */
    BOOLEAN_FLAG(lflag3);       /* ANSI graphics on if true (doors too) */
    BOOLEAN_FLAG(lflag4);       /* PAUSE between messages flag          */
    BOOLEAN_FLAG(lflag5);       /* VERBOSE message-headers flag         */
    BOOLEAN_FLAG(lflag6);       /* true if BANNED from file downloads   */
    BOOLEAN_FLAG(lflag7);       /* true if user is NOT validated        */
    BOOLEAN_FLAG(lflag8);       /* true to turn message numbers OFF     */
    BOOLEAN_FLAG(lflag9);       /* true if username is UNLISTED         */
} ;



#define MAXVISIT        8       /* #visits we remember old newestLo for */

#define GENSHIFT        3       /* Where the generation # is            */
#define CALLMASK        7       /* For finding last visit               */

typedef struct {                /* The appearance of a user:            */
    AN_UNSIGNED   lbnulls;      /* #nulls, lCase, lFeeds                */
    struct lflags lbflags;      /* LFMASK, EXPERT, AIDE, INUSE, etc.    */
    AN_UNSIGNED   lbwidth;      /* terminal width                       */
    char          credit;       /* Credit for long distance calls       */
    label         lbname;       /* caller's name                        */
    label         lbpw;         /* caller's password                    */
    int           lbfwd;        /* Index for mail forwarding            */
    char          lbNetGen;     /* net gen.  -1 indicates no forwarding */
    MSG_NUMBER    lbvisit[MAXVISIT];/* newestLo for this and 7 prev. visits */
    AN_UNSIGNED   *lbgen;/* 5 bits gen, 3 bits lastvisit       */
    theMessages *lbMail;
} logBuffer ;

typedef struct {                /* Summation of a person:               */
    int   ltpwhash;             /* hash of password                     */
    int   ltnmhash;             /* hash of name                         */
    int   ltlogSlot;            /* location in userlog.buf              */
    MSG_NUMBER ltnewest;        /* last message on last call            */
} LogTable ;                    /* And see LOG.C for declaration        */

/************************************************************************/
/*                      ansi table stuff                                */
/************************************************************************/


typedef struct {        /* Principal color levels defined     */
	short int level0;   /* 0 main text stuff and messages     */
	short int level1;   /* 1 headers and helps, mail and aide */
	short int level2;   /* 2 directories and other goodies    */
	short int level3;   /* 3 some minor prompts               */
	short int level4;   /* 4 rest reserved for later use      */
	short int level5;   /* 5         "      "    "    "       */
	short int level6;   /* 6         "      "    "    "       */
	short int level7;   /* 7         "      "    "    "       */
} paintBrush ;

/************************************************************************/
/*                      terminal stuff                                  */
/************************************************************************/
#define SPECIAL         27      /* <ESC>        console escape char     */
#define CON_NEXT        20      /* ^T           console request char    */

#define DIR_SIZE        600

struct dirList {                /* For listing files on disk            */
   char *unambig;
   DIR_EXTRA otherStuff;
} ;

struct any_list {
    int             roomNo;
    char            *archName;
    struct any_list *next;
} ;


/************************************************************************/
/*                      message stuff                                   */
/************************************************************************/
#define MAXTEXT         7500    /* maximum chars in edit buffer         */
#define MAXWORD         256     /* maximum length of a word             */
#define IDIOT_TRIGGER   8       /* Idiot trigger                        */

#define HELD 3

typedef struct {                /* This is what a msg looks like        */
    int  Ooops;
    char mbtext[MAXTEXT]  ;     /* buffer text is edited in             */

    int  mbheadChar       ;     /* start of message                     */
    SECTOR_ID     mbheadSector; /* start of message                     */

    label mbauth ;              /* name of author                       */
    label mbdate ;              /* creation date                        */
    label mbtime ;              /* creation time                        */
    label mbId   ;              /* local number of message              */
    label mboname;              /* short human name for origin system   */
    label mborig ;              /* US xxx xxx xxxx style ID             */
    label mbroom ;              /* creation room                        */
    label mbsrcId;              /* message ID on system of origin       */
    label mbto   ;              /* private message to                   */
    label mbaddr ;              /* address of system for net routing    */
	label mbcompnode;           /* AB - for mail forwarding             */
	char  mbmsgpath[482];       /* AB - for more mail forwarding        */
    char  mbmsgreply[482];      /* AB - auto reply path                 */
} MSG_BUF;

/* values for showMess routine */
#define NEWoNLY         0
#define OLDaNDnEW       1
#define OLDoNLY         2
#define GLOBALnEW       3

/* definitions for determination of net message display */
#define ALL_MESSAGES    0
#define LOCAL_ONLY      1

struct mBuf {
    DATA_BLOCK    sectBuf;
    int           thisChar;
    SECTOR_ID     thisSector;
    int           oldChar;
    SECTOR_ID     oldSector;
} ;

/************************************************************************/
/*                      modem stuff                                     */
/************************************************************************/


#define NEWCARRIER   0x01       /* returned to main prog on login       */

#define CPT_SIGNAL      18      /* ^R                                   */

#define MODEM           0       /* current user of system is            */
#define CONSOLE         1       /* one of these                         */

/*  output XON/XOFF etc flag... */
#define OUTOK           0       /* normal output                        */
#define OUTPAUSE        1       /* a pause has been requested           */
#define OUTNEXT         2       /* quit this message, get the next      */
#define OUTSKIP         3       /* stop current process                 */
#define OUTPARAGRAPH    4       /* skip to next paragraph               */
#define IMPERVIOUS      5       /* make current output unstoppable      */
#define NET_CALL        6       /* net call detected - only banner      */

#define NEITHER         0       /* don't echo input at all              */
#define CALLER          1       /* echo to caller only --passwords etc  */
#define BOTH            2       /* echo to caller and console both      */

#define NO_ECHO         0       /* Echo input as X's                    */
#define ECHO            1       /* Echo input   (getString())           */


/************************************************************************/
/*                      event stuff                                     */
/************************************************************************/
#define SUNDAYS         1
#define MONDAYS         2
#define TUESDAYS        4
#define WEDNESDAYS      8
#define THURSDAYS       16
#define FRIDAYS         32
#define SATURDAYS       64
#define ALL_DAYS        127     /* 0x7F */

#define TYPREEMPT       0
#define TYNON           1
#define TYQUIET         2

#define CLNET           0
#define CLEXTERN        1
#define CLREL           2
#define CL_DL_TIME      3
#define CL_ANYTIME_NET  4

#define ALL_NETS        ~(0l)
#define MAX_NET         32
#define NO_NETS         0l

typedef struct {
    int            EvDur,               /* Event duration       */
                   EvWarn;              /* Event warning pointer */
    unsigned char  EvClass,             /* Event Class          */
                   EvType;              /* Event Type           */
    MULTI_NET_DATA EvExitVal;           /* Event Exit value     */
    int            EvMinutes;           /* From midnight        */
} EVENT;

/************************************************************************/
/*                      net stuff                                       */
/************************************************************************/
                        /* SYSBAUD constants    */
#define ONLY_300        0       /* 300 baud only                        */
#define BOTH_300_1200   1       /* +1200 baud                           */
#define TH_3_12_24      2       /* +2400 baud                           */
#define B_4             3       /* +4800 baud                           */
#define B_5             4       /* +9600 baud                           */
#define SetYourOwn      5       /* +roll your own -- this may go away   */

                        /* ITL constants        */
#define ITL_SUCCESS     0
#define ITL_BAD_TRANS   1
#define ITL_NO_OPEN     2

        /* Network request codes        */
#define HANGUP          0       /* Terminate networking                 */
#define NORMAL_MAIL     1       /* Send normal Mail                     */
#define R_FILE_REQ      2       /* Request a single file                */
#define A_FILE_REQ      3       /* Request a number of files            */
#define NET_ROOM        5       /* Send a shared room                   */
#define CHECK_MAIL      6       /* Check for recipient validity         */
#define SEND_FILE       7       /* Send a file to another system        */
#define NET_ROUTE_ROOM  8       /* Send a routed shared room            */

#ifdef NEW_NET                  /* Citadel:K2NE Reserved list           */
#define                 20
#define                 21
#define                 22
#define                 23
#define                 24
#endif                          /* End of Citadel:K2NE Reserved list    */


#define ITL_PROTOCOL    100     /* Switch to different protocol         */
#define ROLE_REVERSAL   201     /* Reverse roles                        */
#define SYS_NET_PWD     202     /* System password stuff                */

#define BAD             0       /* Reply Codes: this indicates bad      */
#define GOOD            1       /* And this indicates good              */

                        /* These refer to negative ack mail     */
#define NO_ERROR        0       /* No error (ends transmission)         */
#define NO_RECIPIENT    1       /* No recipient found                   */
#define BAD_FORM        2       /* Something's wrong                    */
#define UNKNOWN         99      /* Something's REALLY wrong (eek!)      */

#define PEON            0
#define REG_HOST        1
#define BACKBONE        2       /* Kinda like a hub     */
#define PASS_BACKBONE   3       /* Used in "mode" only, not rbSharedType */
#define ACTIVE_BACKBONE 4

                        /* These used with ITL_PROTOCOL command         */
#define XM_ITL          "0"
#define YM_ITL          "1"
#define WXM_ITL         "2"

#define NET_GEN         32

#define NON_NET         0
#define NORMAL_NET      1
#define ANYTIME_NET     2
#define ANY_CALL        3

struct nflags {                 /* Any and all reasons to call this node*/
    BOOLEAN_FLAG(normal_mail);  /* Outgoing normal mail?                */
    BOOLEAN_FLAG(in_use);       /* Is this record even in use?          */
    BOOLEAN_FLAG(room_files);   /* Any file requests?                   */
    BOOLEAN_FLAG(local);        /* Is this node local?                  */
    BOOLEAN_FLAG(spine);        /* Will we be a spine?                  */
    BOOLEAN_FLAG(send_files);
    BOOLEAN_FLAG(is_spine);     /* Is that system a spine?              */
    BOOLEAN_FLAG(uses_frontend);/* Needs special treatment when called  */
    BOOLEAN_FLAG(telnet_ok);    /* users with * access can dial to it   */
    BOOLEAN_FLAG(flag6);
    BOOLEAN_FLAG(flag7);
    BOOLEAN_FLAG(flag8);
    BOOLEAN_FLAG(flag9);
    BOOLEAN_FLAG(flag10);
    BOOLEAN_FLAG(flag11);
    BOOLEAN_FLAG(flag12);
    BOOLEAN_FLAG(flag13);
    BOOLEAN_FLAG(flag14);
    BOOLEAN_FLAG(flag15);
    BOOLEAN_FLAG(flag16);
} ;

struct shared_room {
    MSG_NUMBER lastMess;        /* Highest net message in this room     */
    unsigned   srgen;           /* High bit of gen is used flag         */
    unsigned   srslot;
    int        mode;            /* Local Peon, R. H., Backbone...       */
} ;

#define NT_SIZE         (sizeof (*netTab) - (PTR_SIZE * 2))
#define NB_SIZE         (sizeof (netBuf)  - (PTR_SIZE * 2))
#define SR_BULK         (SHARED_ROOMS * sizeof (struct shared_room))
#define NA_BULK         (NET_ARCH_ROOMS * sizeof (struct shared_room))
#define NT_TOTAL_SIZE   (NT_SIZE + SR_BULK + NA_BULK)
#define NB_TOTAL_SIZE   (NB_SIZE + SR_BULK + NA_BULK)

typedef struct {
    label          netId;        /* Node id      */
    label          netName;      /* Node name    */
    label          OurPwd;
    label          TheirPwd;
    MULTI_NET_DATA MemberNets;
    struct nflags  nbflags;      /* Flags        */
    char           baudCode;     /* Baud code for this node   */
    char           nbGen;        /* Generation value for this node */
    char           access[40];   /* For alternative access       */
    struct shared_room *netRooms;
    struct shared_room *nbArchRooms;
} NetBuffer;

typedef struct {
    int            ntnmhash;
    int            ntidhash;
    struct nflags  ntflags ;
    MULTI_NET_DATA ntMemberNets;
    struct shared_room *netTRooms;
    struct shared_room *ntArchRooms;
} NetTable;

struct cmd_data {               /* Commands for networking              */
    AN_UNSIGNED command;
    char        fields[4][NAMESIZE];
} ;

struct netMLstruct {
    MSG_NUMBER ML_id;
    SECTOR_ID  ML_loc;
} ;

struct fl_req {
    label room;
    label roomfile;
    NET_AREA flArea;
    label filename;
} ;

struct fl_send {
    NET_AREA snArea;
    label sFilename;
} ;


/************************************************************************/
/*                      Floor data structures                           */
/************************************************************************/
                                /* Display modes for floor summaries    */
#define INT_EXPERT      0       /* First display for experts            */
#define INT_NOVICE      1       /* First display for novices            */
#define ONLY_FLOORS     2       /* Floors only, no rooms 'tall.         */
#define NOT_INTRO       3       /* 'K' is done.                         */
#define FORGOTTEN       4       /* Forgotten rooms list                 */

struct floor {
    label FlName;
    char  FlInuse;
} ;

/************************************************************************/
/*                      Exit values for errorlevels                     */
/************************************************************************/
#define SYSOP_EXIT             0        /* "Normal"     */
#define RECURSE_EXIT           1
#define CRASH_EXIT             2
#define REMOTE_SYSOP_EXIT      3

/************************************************************************/
/*                      Useful psuedo functions                         */
/************************************************************************/
#define onLine()        (haveCarrier    ||   onConsole)
#define wrNetId(x)      ((strCmpU(x, ALL_LOCALS) != 0) ? x : WRITE_LOCALS)

#define putMLNet(f,b)   if (fwrite(&b, sizeof(b), 1, f) != 1)\
                                         crashout("putMLNet crash")
#define getMLNet(f,b)   (fread(&b, sizeof(b), 1, f) == 1)

#define putSLNet(b, f)  if (fwrite(&b, sizeof(b), 1, f) != 1)\
                                crashout("putSLNet crash")
#define getSLNet(b, f)  (fread(&b, sizeof(b), 1, f) == 1)

#define TheSysop()      (aide && onConsole)
#define SomeSysop()     (TheSysop() || remoteSysop)

#ifdef OLD_STYLE

#define needToCall(i, x)   (netTab[i].ntflags.in_use && \
          !netTab[i].ntflags.is_spine && \
          (netTab[i].ntMemberNets & x) && ((netTab[i].ntflags.normal_mail ||\
          netTab[i].ntflags.room_files || \
netTab[i].ntflags.send_files) || roomsShared(i) || (netTab[i].ntflags.spine && \
                                                (!inNet || pollCall[i] == 1))))

#endif

#define setNeedsProcessing(i)   netBuf.netRooms[i].srslot |= 0x8000
#define chkNeedsProcessing(i)   (netBuf.netRooms[i].srslot & 0x8000)
#define resetNeedsProcessing(i) netBuf.netRooms[i].srslot &= 0x7FFF

#define isSharedRoom(h, i)      (netTab[h].netTRooms[i].srgen & 0x8000)

#define resetNetRoomFlag(i)     netBuf.netRooms[i].srgen &= 0x7FFF;\
                                netTab[thisNet].netTRooms[(i)].srgen &= 0x7FFF;

#define netRoomSlot(i)          (netBuf.netRooms[i].srslot & 0x7FFF)

#define netTabRoomSlot(h,i)     (netTab[h].netTRooms[i].srslot & 0x7FFF)

#define netGen(i)               (netBuf.netRooms[i].srgen & 0x7FFF)
#define roomValidate(h, i)  (roomTab[netTabRoomSlot(h, i)].rtgen == netGen(i) \
&& roomTab[netTabRoomSlot(h, i)].rtflags.INUSE \
&& roomTab[netTabRoomSlot(h, i)].rtflags.SHARED)

#define INTERVALS 8             /* Half second intervals                */

#define minimum(x,y)    ((x) < (y) ? (x) : (y))
/* #define max(x,y)        ((x) > (y) ? (x) : (y)) */

#define initLogBuf(x)   (x)->lbgen = (AN_UNSIGNED *) GetDynamic(GEN_BULK),\
                        (x)->lbMail = (theMessages *)\
                                                GetDynamic(MAIL_BULK)

#define killLogBuf(x)   free((x)->lbgen), free((x)->lbMail)

#define initRoomBuf(x)  (x)->msg = (theMessages *)\
         GetDynamic(max(MAIL_BULK, MSG_BULK))
#define killRoomBuf(x)  free((x)->msg)

#define initNetBuf(x)   (x)->netRooms = (struct shared_room *)\
                           GetDynamic(sizeof (*(x)->netRooms) * SHARED_ROOMS),\
                        (x)->nbArchRooms = (struct shared_room *)\
                      GetDynamic(sizeof (*(x)->nbArchRooms) * NET_ARCH_ROOMS)

#define killNetBuf(x)   free((x)->netRooms)

#define NumElems(x)     (sizeof (x)) / (sizeof (x[0]))

#define copyLogBuf(x, y)  movmem(x, y, LB_SIZE),\
                          movmem((x)->lbMail, (y)->lbMail, MAIL_BULK),\
                          movmem((x)->lbgen, (y)->lbgen, GEN_BULK)

/************************************************************************/
/*                      Call log stuff                                  */
/************************************************************************/
#define BAUD            0       /* This message concerns baud rate      */
#define L_IN            1       /*  "      "       "     login          */
#define L_OUT           2       /*  "      "       "     logout         */
#define CARRLOSS        3       /*  "      "       "     carr-loss      */
#define FIRST_IN        4       /*  "      "       "     init           */
#define LAST_OUT        5       /*  "      "       "     close-down     */
#define EVIL_SIGNAL     6       /*  "      "       "     user errors    */
#define CRASH_OUT       7       /*  "      "       "     crash down     */
#define INTO_NET        8       /*  "      "       "     net entry      */
#define OUTOF_NET       9       /*  "      "       "     net exit       */
#define CHAT_FLAG      10       /*  "      "       "     chat page      */

/************************************************************************/
/*                      Transfer protocol constants                     */
/************************************************************************/
#define ASCII           0
#define XMDM            1
#define YMDM            2
#define WXMDM           3

#define MAX_WX_ERRORS   10
#define ERRORMAX        10       /* How many errors to suffer through   */
#define RETRYMAX        10
#define MINUTE          60       /* How long is a minute?               */

#define STARTUP         1        /* Code to start a transfer            */
#define FINISH          2        /* Code to cleanup a transfer          */

        /* Reception startup error values */
#define TRAN_SUCCESS    0       /* Successful transfer                  */
#define NO_LUCK         1       /* Never return this to caller          */
#define CANCEL          2       /* Session encountered a CAN            */
#define NO_START        3       /* Transfer never even started!         */
#define TRAN_FAILURE    4       /* Something blew...                    */

        /* Internally used values, not returned to caller       */
#define NO_ERROR        0
#define BAD_DLE         1
#define EARLY_SYN       2
#define DATA_TIMEOUT    3
#define BAD_CRC         4
#define BAD_CKSM        5
#define BAD_SEC_COMP    6
#define SYNCH_ERROR     7       /* Fatal */
#define WRITE_ERROR     8       /* Fatal */
#define CARR_LOSS       9       /* Fatal */

/* Internal WXMODEM enumerations for transmission window */
#define NOT_USED        0
#define SECTOR_READY    1
#define SENT            2
#define ACKED           3

/* ASCII characters: */
#define SOH             1
#define STX             2
#define CNTRLC          3
#define EOT             4
#define ACK             6
#define BELL            7
#define BACKSPACE       8
#define CNTRLI          9       /* aka tab                              */
#define TAB             9       /* aka ^I                               */
#define NEWLINE        10       /* "linefeed" to philistines.           */
#define CNTRLl         12       /* Sysop privileges                     */
#define CNTRLO         15
#define DLE            16
#define XON            17
#define XOFF           19       /* control-s                            */
#define NAK            21
#define SYN            22
#define CAN            24
#define CNTRLZ         26
#define CPMEOF     CNTRLZ
#define ESC            27       /* altmode                              */
#define CRC_START     'C'       /* CRC Mode for WC                      */
#define DEL          0x7F       /* delete char                          */

#define IS_NUMEROUS     0x01
#define IS_DL           0x02
#define NEEDS_FIN       0x04
#define RIGAMAROLE      0x08
#define NOT_AVAILABLE   0x10
#define NEEDS_HDR       0x20    /* True only for file transfer          */

typedef struct {
                                    /* Calculate time to transfer */
    void (*TimeCalc)(long size, long *seconds);
    int  flags;                 /* Bit map - see above */
    char *name;
    char *MsgTran;
    char *BlbName;
    char *UpBlbName;
    int  (*method)(int c);
    int  BlockSize;
    int  (*SendHdr)(long fileSize, char *fileName);
    int  (*CleanUp)(void);
} PROTO_TABLE;

typedef struct {
    int ThisBlock;      /* Block # of this block */
    AN_UNSIGNED *buf;
    CRC_TYPE ThisCRC;        /* So we only calculate once */
    char status;        /* Init these to NOT_USED */
} TransferBlock;

#define NORMAL          0
#define DISK            1

/************************************************************************/
/*                      SEA ARC reading structure                       */
/************************************************************************/
typedef struct {
    char name [13];     /* file name */
    UNS_32 size; /* size of compressed file */
    UNS_16 date;        /* file date*/
    UNS_16 time;        /* file time */
    UNS_16 crc;         /* cyclic redundancy check */
    UNS_32 length;       /* true file length */
  } ARCbuf;

/************************************************************************/
/*                      Finger saving defines                           */
/************************************************************************/
#define termWidth       logBuf.lbwidth
#define termNulls       logBuf.lbnulls
#define termLF          logBuf.lbflags.LFMASK
#define expert          logBuf.lbflags.EXPERT
#define aide            logBuf.lbflags.AIDE
#define sendTime        logBuf.lbflags.TIME
#define oldToo          logBuf.lbflags.OLDTOO
#define FloorMode       logBuf.lbflags.FLOORS
#define SubSystemOK     logBuf.lbflags.SUBSYSTEM_OK
#define thisFloor       roomBuf.rbFlIndex
#define userPause       logBuf.lbflags.lflag4

#define MAILSLOTS       cfg.MailSlots
#define MSGSPERRM       cfg.MsgsPerrm
#define MAXROOMS        cfg.MaxRooms
#define SHARED_ROOMS    cfg.SharedRooms
#define NET_ARCH_ROOMS  cfg.NetArchRooms

#define LVL_0 colTable.level0;
#define LVL_1 colTable.level1;
#define LVL_2 colTable.level2;
#define LVL_3 colTable.level3;

        /*
         * This is icky, and should be a lesson to all would-be
         * header writers.
         */

#include "ctdlvirt.h"
#include "ctdlansi.h"
#include "ansisys.h"

/* NEW for FOSSIL stuff */
#include "fossil.h"

/* And that's it for this file */
