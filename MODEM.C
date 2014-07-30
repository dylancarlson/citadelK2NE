/*****
 *                              modem.c
 *
 *              modem code for Citadel bulletin board system
 *      NB: this code is rather machine-dependent:  it will typically
 *      need some twiddling for each new installation.
 *                            82Nov05 CrT
 *
 *                 now this file is mostly for upper layer modem
 *              handling and the protocols, so it may need
 *              no fiddling.
 *                            88May07 HAW
 *****/

/*
 *                              history
 *
 * 85Nov09 HAW  Warning bell before timeout.
 * 85Oct27 HAW  Cermetek support eliminated.
 * 85Oct18 HAW  2400 support.
 * 85Sep15 HAW  Put limit in ringSysop().
 * 85Aug17 HAW  Update for gotCarrier().
 * 85Jul05 HAW  Insert fix code (Brian Riley) for 1200 network.
 * 85Jun11 HAW  Fix readFile to recognize loss of carrier.
 * 85May27 HAW  Code for networking time out.
 * 85May06 HAW  Code for daily timeout.
 * 85Mar07 HAW  Stick in Sperry PC mods for MSDOS.
 * 85Feb22 HAW  Upload/download implemented.
 * 85Feb20 HAW  IMPERVIOUS flag implemented.
 * 85Feb17 HAW  Baud change functions installed.
 * 85Feb09 HAW and Sr.  Chat bug analyzed by Sr.
 * 85Jan16 JLS  fastIn modified for CR being first character from modem.
 * 85Jan04 HAW  Code added but not tested for new WC functions.
 * 84Sep12 HAW  Continue massacre of portability -- bye, pMIReady.
 * 84Aug30 HAW  Wheeee!!  MS-DOS time!!
 * 84Aug22 HAW  Compilation directive for 8085 chips inserted.
 * 84Jul08 JLS & HAW ReadFile() fixed for the 255 rollover.
 * 84Jul03 JLS & HAW All references to putCh changed to putChar.
 * 84Jun23 HAW & JLS Local unused variables zapped.
 * 84Mar07 HAW  Upgrade to BDS 1.50a begun.
 * 83Mar01 CrT  FastIn() ignores LFs etc -- CRLF folks won't be trapped.
 * 83Feb25 CrT  Possible fix for backspace-in-message-entry problem.
 * 83Feb18 CrT  fastIn() upload mode cutting in on people.  Fixed.
 * 82Dec16 dvm  modemInit revised for FDC-1, with kludge for use with
 *              Big Board development system
 * 82Dec06 CrT  2.00 release.
 * 82Nov15 CrT  readfile() & sendfile() borrowed from TelEdit.c
 * 82Nov05 CrT  Individual history file established
*/

#include "ctdl.h"

/*
 *                              Contents
 *
 *      BBSCharReady()          returns true if user input is ready
 *      ClearWX()               finishes a WXMODEM transmission
 *      CommonPacket()          reads a block
 *      CommonWrite()           writes a block to wherever
 *      FlowControl()           flow control handler for WXMODEM
 *      GenTrInit()             general init for individual protocols
 *      getMod()                bottom-level modem-input   filter
 *      iChar()                 top-level user-input function
 *      initTransfers()         initial data buffers of protocols
 *      interact()              chat mode
 *      JumpStart()             gets protocol reception going
 *      MIReady()               check MS-DOS interrupt for data
 *      modIn()                 returns a user char
 *      modemInit()             top-level initialize-all-modem-stuff
 *      oChar()                 top-level user-output function
 *      Reception()             receive data via protocol
 *      recWX()                 receive data via WXMODEM
 *      recWXchar()             receive a WXMODEM char, stripped of DLE
 *      recXYmodem()            receive data via X or Y MODEM
 *      ringSysop()             signal chat-mode request
 *      runHangup()             hides the hangup code
 *      SendCmnBlk()            sends a WX/X/Y/MODEM block
 *      sendWCChar()            send file with WC-protocol handshaking
 *      sendWXchar()            send a char for WXMODEM
 *      sendWXModem()           send data with WXMODEM protocol
 *      sendYMChar()            send file with YMODEM protocol
 *      SummonSysop()           rings bell for ^T
 *      Transmission()          handles protocol transmission
 *      WXResponses()           handles NAK/ACK XON/XOFF for WXMODEM
 *      XYBlock()               common routine for X & Y mode
 *      XYClear()               finished X or Y MODEM transmission
 *      YMHdr()                 YMODEM BATCH header handler
*/

/*****
 *    External variable declarations in MODEM.C
 *****/
char justLostCarrier = FALSE;   /* Modem <==> rooms connection          */
char newCarrier      = FALSE;   /* Just got carrier                     */
char onConsole;                 /* Who's in control?!?                  */
char sleepFlag       = FALSE;   /* snore! 								*/
int dumpDeadWood	 = FALSE;   /* a kludge for getText					*/
int  outPut = NORMAL;
char          modStat;          /* Whether modem had carrier LAST time  */
                                /* you checked.                         */
char *whatBaudRate;             /* some modem stuff                     */
char CallSysop = FALSE;         /* Call sysop on user logout            */

char whichIO = CONSOLE;         /* CONSOLE or MODEM                     */
#ifdef NEED_VISIBLE
char visibleMode;               /* make non-printables visible?         */
#endif
static char captureOn = FALSE;
static FILE *cptFile;
char haveCarrier;               /* set if DCD == TRUE                   */
char textDownload;              /* read host files, TRUE => ASCII       */
char echo;                      /* Either NEITHER, CALLER, or BOTH      */
char echoChar;                  /* What to echo with if echo == NEITHER */
int  TransProtocol;             /* Transfer protocol value              */
int  upDay;                     /* Day system was brought up            */
char nextDay;                   /* Come down tomorrow rather than today?*/
int  timeCrash;
char anyEcho = TRUE, notFlag;
char warned, itsGone, windowReport, specialBump, blockAllBlurbs;
char bannerNote[80];
char Shuttle, telnet, killScreen;
long netBytes;
long TempBytes;
/* Block transfer variables */
TransferBlock Twindow[4];

#define blk     Twindow[0]

int  CurWindow,     /* For sequence #, init to 1 */
     TrBlock,       /* For block #, init to 1 */
     StartWindow,   /* First block of current Twindow, init = 1 */
     TrCount,       /* Byte accumulator counter, init to 0 */
     LastSent,      /* Last seq block sent to receiver, regardless of ACK */
     CurYBufSize,   /* Size of current YMODEM block to send     */
     GlobalHeader;  /* Starting char of transfer (YMODEM)       */

char DoCRC,         /* True if doing CRC */
     DLinkError,    /* For conveying errors during WXMODEM */
     TrError,       /* True only on fatal error */
     DLEsignal;     /* True only when an WXMODEM read involved a DLE */

AN_UNSIGNED *DataBuf;
int         TrCksm;     /* Checksum variable for transmissions */

static char *msg[] = {
    "NO ERROR",
    "BAD DLE",
    "EARLY SYN",
    "DATA TIMEOUT",
    "BAD CRC",
    "BAD CHECKSUM",
    "BAD SECTOR COMPLEMENT",
    "SYNCH ERROR",
    "WRITE ERROR",
    "CARRIER LOSS"
};

struct dfree disk;

#define WindowFull()    IsSent(0) && IsSent(1) && IsSent(2) && IsSent(3)
#define WindowEmpty()   IsDone(0) && IsDone(1) && IsDone(2) && IsDone(3)
#define IsDone(x)       (IsAcked(x) || NotUsed(x))
#define IsSent(x)       (Twindow[x].status == SENT)
#define IsAcked(x)      (Twindow[x].status == ACKED)
#define NotUsed(x)      (Twindow[x].status == NOT_USED)

/*
 * This table presupposes that XMODEM is defined as 1, YMODEM as 2, WXMODEM
 * as 3.  Used only by CommonPacket().
 */
static int time_table[] = { 0, 1, 1, 15 } ;
/*****
 *    External variable definitions for MODEM.C
 *****/
extern MSG_BUF   msgBuf;                /* Message buffer               */
extern CONFIG    cfg;                   /* Configuration variables      */
extern logBuffer logBuf;                /* Log buffer of a person       */
extern FILE      *upfd;
extern char      prevChar;       /* previous char                */
extern char      outFlag;        /* output flag                  */
extern char      ExitToMsdos;    /* Kill program flag            */
extern int       exitValue, jumper, holdx, holdy;
                /* net stuff vars should go here */
extern char      inNet, manualNet, autoNet, justOut;
extern FILE      *netLog;
                /* bloooooop! */

extern int              acount, CITCOLOR, timeOnLine, oldDayVal;
extern int				numberOfRings, evMode, SHUTTLE_POINTER;
extern int              timeForBlank;
#define AUDIT           9000
extern char             audit[AUDIT];
extern PROTO_TABLE      Table[];
extern char             blankVariable; /* , didRing; */
extern char             ansi, sentNotice, shortSession, callLogPosting[800];
extern char             doorLogFlag, siege, AideNetTrigger, chatFlag;
extern char				runningAsDoor, loggedIn, frontEnd, PrintBanner;
extern char             answerGuard, isShuttle, shuttleUser, shuttleStyle[10];
extern char             modemIdleString[50];
extern void    	        (*StopVideo)(void);
extern rTable           *roomTab;
extern paintBrush colTable;      /* the ANSI rainbow             */

/*
 *              The principal dependencies:
 *
 *  iChar   modIn                                   outMod
 *          modIn   getMod  getCh   mIReady kBReady outMod  carrDetect
 *                  getMod
 *                          getCh
 *                                  mIReady
 *                                          kBReady
 *                                                          carrDetect
 *
 *  oChar                                           outMod
 *                                                  outMod
 * (This is out of date.  HAW)
*/


/*****
 *    BBSCharReady() returns TRUE if char is available from user
 *    NB: user may be on modem, or may be sysop in CONSOLE mode
 *****/
char BBSCharReady()
{
    return (((haveCarrier && whichIO == MODEM) && MIReady()) ||
           (whichIO == CONSOLE  &&   KBReady()));
}
#ifdef WXMODEM_avl
/*****
 *    ClearWX() finishes a WXMODEM transmission
 *****/
int ClearWX()
{
    int rover, BlockRover, TempSent, SendEOT;

    if (!gotCarrier())
        return CARR_LOSS;

    outMod(EOT);        /* Forces us to wait until output buffer is flushed */
    while (!WindowEmpty()) {
        WXResponses();
        for (BlockRover = (LastSent + 1) % 4, TempSent = LastSent, SendEOT=0;
             BlockRover != LastSent; BlockRover = (BlockRover + 1) % 4) {
            if (Twindow[BlockRover].status != SECTOR_READY) break;
            SendEOT++;
            SendCmnBlk(WXMDM, Twindow + BlockRover, sendWXchar, SECTSIZE);
            Twindow[BlockRover].status = SENT;
            TempSent = BlockRover;
        }
        LastSent = TempSent;

        if (!gotCarrier())
            return CARR_LOSS;

        if (SendEOT) outMod(EOT);
    }

    for (rover = 1; rover < MAX_WX_ERRORS; rover++) {
        if (!gotCarrier())
            return CARR_LOSS;

        if (receive(3) != ACK) outMod(EOT);
        else return TRAN_SUCCESS;
    }

    return TRAN_FAILURE;
}
#endif

/*****
 *    CommonPacket() reads a block of data (XMDM, YMDM, WXMDM)
 *****/
int CommonPacket(type, size, recFn, Sector)
char type;
int  size, (*recFn)(int t), *Sector;
{
    int  comp, cksm, i, c, hi, lo, time;
    CRC_TYPE crc;
    /*
     * Format:
     *
     * <SOH | STX><Sec#><w Sec#><size bytes of data><checksum or CRC>
     *
     * SOH | STX has already been received by the caller.  type is used for
     * protocol specific problems, as follows.
     *
     * WXMDM:
     * 1. When SYN is detected without DLEsignal == TRUE, this indicates a bad
     *    packet problem, so that must be checked.
     */

    time = time_table[type];
    *Sector = (*recFn)(time);   /* Get Sector # */
    if (type == WXMDM && *Sector == SYN && !DLEsignal) return EARLY_SYN;
    comp = (*recFn)(time);      /* Get Sector #'s complement */

    for (i = cksm = 0; i < size; i++) { /* Get data block */
        if ((c = (*recFn)(time)) == ERROR) {
            if (!gotCarrier())
                return CARR_LOSS;
            return DATA_TIMEOUT;
	        }
        DataBuf[i] = c;
        cksm = (c + cksm) & 0xFF;
        if (!gotCarrier())
            return CARR_LOSS;
    	}

    hi = (*recFn)(time);        /* Get cksm or hi byte of CRC */
    if (DoCRC) {
        lo = (*recFn)(time);    /* Get lo byte of CRC */
        crc = (hi << 8) + lo;
        if (*Sector + comp == 256)      /* Validations... */
            if (crc != calcrc(DataBuf, size)) {
                return BAD_CRC;
            }
    }
    else {
        if (hi != cksm)
            return BAD_CKSM;
    }
    if (*Sector + comp != 0xFF) { /* Check this to make sure, too */
        return BAD_SEC_COMP;
    }

    return NO_ERROR;
}

/*****
 *    CommonWrite() writes a block of data to wherever
 *****/
int CommonWrite(WriteFn, size)
int size, (*WriteFn)(int c);
{
    int i;

    for (i = 0; i < size; i++)
        if ((*WriteFn)(DataBuf[i]) == ERROR)
            return WRITE_ERROR;
     return NO_ERROR;
}

/*****
 *    GenTrInit() General protocol initializations
 *****/
void GenTrInit()
{
    int i;

    for (i = 0; i < 4; i++) Twindow[i].status = NOT_USED;
    CurWindow  = TrBlock   = StartWindow = 1;
    TrCount    = LastSent  = TrCksm      = 0;
    DLinkError = DLEsignal = FALSE;
    TrError    = TRAN_SUCCESS;
    DoCRC      = TRUE;
}

/*****
 *    getMod() is bottom-level modem-input routine
 *      kills any parity bit
 *      rubout                   -> backspace
 *      CR                       -> newline
 *      other nonprinting chars  -> blank
 *    Returns: result
 ******/
char getMod()  /* modified to support Binkley 'YOOHOO' signal */
{
		if (PrintBanner) return inp();
		else return inp() & 0x7F;
/*      return inp() & 0x7F; */
}


char getShuttleMod()
{
		return inpShuttle();
}



/*****
 *    getSize() gets size of a file
 *    NOTE: This should be converted to use stat instead.
 *****/
int getSize(fileName)
char *fileName;
{
    FILE *fd;
    long temp;

    if ((fd = safeopen(fileName, "rb")) == NULL)
        return FALSE;
    totalBytes(&temp, fd);
    netBytes += temp;
    fclose(fd);
    return TRUE;
}

/*****
 *    iChar() is the top-level user-input function -- this is the
 *    function the rest of Citadel uses to obtain user input
 *****/
char iChar()
{
    char  c;

    if (justLostCarrier)   return 0;    /* ugly patch   */

    c = cfg.filter[modIn() & 0x7f];


/* AUDIT CODE */
audit[acount] = c;
acount = (acount + 1) % AUDIT;

    switch (echo) {
    case BOTH:
        if (haveCarrier) {
            if (c == '\n') {
                doCR();
            }
            else
                outMod(c);
        }
        mputChar(c);     /* Let putChar decide if it should go on console */
        break;
    case CALLER:
        if (whichIO == MODEM) {
            if (c == '\n') {
                doCR();
            }
            else {
                outMod(c);
            }
        } else {
            mputChar(c);
        }
        break;
    case NEITHER:
        if (echoChar != '\0') {
            if (whichIO == MODEM) {
                if (c == '\n') doCR();
                else if (c <= ' ') outMod(c);
                else           outMod(echoChar);
            }
            else {
                if (c == '\n') doCR();
                else if (c <= ' ') mputChar(c);
                else           mputChar(echoChar);
            }
        }
        break;
    }
    return(c);
}

/*****
 *    initTransfers() initial data buffers for protocol transfers
 *****/
void initTransfers()
{
    int i;

    for (i = 1; i < 4; i++) {
        Twindow[i].buf = GetDynamic(SECTSIZE);
    }
    DataBuf = blk.buf = GetDynamic(YM_BLOCK_SIZE);
}

/*****
 *    interact() allows the sysop to interact directly with
 *    whatever is on the modem.   dvm 9-82
 *    Telnet> (shuttle) functions added for V6.05.  vaq/smw/mab 3-93
 *****/
void interact(ask)
char ask;
{
    char c = 0, lineEcho, lineFeeds, localEcho;
    char last = 0;
	char *tempPrompt, tempWork[80];
    extern char *APPEND_TEXT;
/* #ifdef OLDCHAT */
/*    printf(" Direct modem-interaction mode\n"); */
    if (ask==FALSE) {

#ifdef NOSHUTTLE
	    printf(" Direct modem-interaction mode\n");
        lineEcho    = conGetYesNo("Echo to modem"     );
        localEcho   = conGetYesNo("Echo keyboard"     );
        lineFeeds   = conGetYesNo("Echo CR as CRLF"   );
#else
        lineEcho=localEcho=FALSE;
		lineFeeds=TRUE;
#endif
		}

    if (ask) lineEcho = localEcho = lineFeeds = TRUE;

    if (ask) printf("CHAT mode: <ESC> to exit\n");
	if (ask) {
		doCR();
		mPrintf("Chat engaged.");
		doCR();
		mPrintf(">");
		}
    /* incredibly ugly code.  Rethink sometime: */
    while (c == shuttleUser ? 400 : SPECIAL) {
        c = 0;
        if (MIReady()) {
            c = getMod();
/*            if (c == SPECIAL && !ask) c = 0; */
            if (c != '\r') c = cfg.filter[c];
            if (c != '\r') {
                if (lineEcho && c != ESC)   outMod(c);
                if (gotCarrier() && !shuttleUser)  interOut(c);
            } else {
                if (!lineFeeds) {
                    if (lineEcho) {
						outMod('\r');
                        outMod('>');
						}
					if (ask) {
	                    interOut('\n');
						interOut('>');
						}
                } else {
                    if (lineEcho) {
                        outMod('\r');
                        outMod('\n');
						outMod('>');
                    }
                   	interOut('\n');
					if (ask) {
						interOut('>');
						}
                }
            }
			if (shuttleUser) normalShuttleTransmit(c);
        }
		else if (!isShuttle && !(f_status(cfg.FOSSIL_PORT) & F_DCD)) {
            DisableModem();
			return;
			}

		else if (shuttleUser && !gotShuttleCarrier() ) {
			shuttleDTR(FALSE);
			return;
			}

		else if (shuttleUser && ShuttleReady() ) {
            c = getShuttleMod();
/*            if (c == SPECIAL) c = 0; */
            if (c != '\r') c = cfg.filter[c];

            if (c != '\r') {
                if (whichIO!=CONSOLE) outMod(c);
                interOut(c);
                }
			else {
				interOut('\n');
				if (whichIO!=CONSOLE) {
					outMod('\r');
					if (termLF) outMod('\n');
					}
/*              outMod('\n'); */
				}
			}

        else if (KBReady()) {
            if ((c  = getCh() & 0x7f) == '\r') c = '\n';
            if (c==SPECIAL && !shuttleUser) break;
            if (c == CPT_SIGNAL) {
                captureOn = !captureOn;
                if (captureOn) {
                    if ((cptFile = safeopen("chat.txt", APPEND_TEXT)) == NULL) {
                        printf("\nCHAT.TXT file error!\n");
                        captureOn = FALSE;
                    }
                    else {
                        printf("\nCHAT.TXT open\n");
                        mPrintf(
               "\n CHAT recorder ON.\n ");
                    }
                }
                else {
                    fclose(cptFile);
                    printf("\nCapture done\n");
                    mPrintf(
                 "\n CHAT recorder OFF.\n ");
                }
            }
			if (c == 4) {
				mPrintf("Stand by...\n ");
				printf("SHELL -- \"exit\" returns.\n");
	            (*StopVideo)();
	            if ( (tempPrompt=getenv("PROMPT"))==0) sprintf(tempWork, "PROMPT=(Citadel resident)$_$p$g");
				else sprintf(tempWork, "PROMPT=(Citadel resident)$_%s", tempPrompt);
				putenv(tempWork);

				system(""); /* behold, a Shell! */

            	setmem(tempWork, strlen(tempWork), '\0');
				sprintf(tempWork, "PROMPT=%s", tempPrompt);
				putenv(tempWork);
                VideoInit();
				}
            if (c != NEWLINE) {
                if (localEcho)  interOut(c);
                if (c != ESC && c != '\\')   {
					if (!shuttleUser) outMod(c);
					else normalShuttleTransmit(c);
					}
                else {
                    if (last == '\\') {
					if (!shuttleUser) outMod(c);
					else normalShuttleTransmit(c);
                    c = 0;
                    }
                }
                last = c;
            } else {
                if (!lineFeeds) {
                    if (localEcho) {
						interOut('\r');
						interOut('>');
						}
                    if (!shuttleUser) outMod('\r'); else normalShuttleTransmit('\r');
					if (!ask) outMod('>');
                } else {
                    if (localEcho) {
                        interOut('\n');
						interOut('>');
                    }
                    if (!shuttleUser) {
						outMod('\r');
	                    outMod('\n');
    					}
					else {
						normalShuttleTransmit('\r');
/*						normalShuttleTransmit('\n'); */
						}

					if (ask) outMod('>');
					}
            }
        }
        else DoTimeouts();


/*    if (isShuttle && !(f_status(cfg.FOSSIL_PORT) && F_DCD)) c=SPECIAL; */
    if ((isShuttle && !shuttleUser) && !gotCarrier()) break;
	}
    if (captureOn) {
        captureOn = FALSE;
        fclose(cptFile);
    }
    if ((shuttleUser ? !gotShuttleCarrier() : !gotCarrier()) /*&& whichIO == CONSOLE*/)
        DisableModem();
}

/*****
 *    interOut() interact I/O bottleneck
 *****/
void interOut(c)
char c;
{
    if (!shuttleUser) mputChar(c);
	else {
		if (c=='\n' || c=='\r') mputChar('\n');
		else putchar(c);
		}
    if (captureOn)
        fputc(c, cptFile);
}

/*****
 *      JumpStart() Generic just start for Reception
 *****/
char JumpStart(tries, timeout, Starter, t1, t2, Method, WriteFn)
int  t1, t2;
int  tries, timeout, Starter;
char (*Method)(int (*wrt)(int c));
int  (*WriteFn)(int c);
{
    int StartTries;

    for (StartTries = 0; StartTries < tries; StartTries++) {
        if (!gotCarrier())
            return CARR_LOSS;

        outMod(Starter);
        GlobalHeader = receive(timeout);
        if (GlobalHeader == t1 || GlobalHeader == t2) {
            return (*Method)(WriteFn);
        }

        if (GlobalHeader == CAN)
            return CANCEL;

        if (GlobalHeader == EOT) {
            outMod(ACK);
            return TRAN_SUCCESS;        /* zero length data */
        }
    }
    return NO_LUCK;
}

/*
 *      modemInit() is responsible for all modem-related initialization
 *      at system startup
 *      Globals modified:       haveCarrier     visibleMode
 *                              whichIO         modStat
 *                              ExitToMsDos     justLostCarrier
 *      modified 82Dec10 to set FDC-1 SIO-B clock speed at
 *      300 baud         -dvm
*/

void modemInit()
{
    SpecialMessage("Modem initialization");
    msgBuf.Ooops        = '\n';

    modemClose();
/*    rawModemInit(); */
    initFossil();
    recycleModem();
/*    firstModemInit(); RILEY CODE should make this OBSOLETE */
    haveCarrier = modStat = gotCarrier();
/*    SpecialMessage(""); */
}

/*
 * modIn() toplevel modem-input function
 *   If DCD status has changed since the last access, reports
 *   carrier present or absent and sets flags as appropriate.
 *   In case of a carrier loss, waits 20 ticks and rechecks

 *   carrier to make sure it was not a temporary glitch.
 *   If carrier is newly received, returns newCarrier = TRUE;  if
 *   carrier lost returns 0.  If carrier is present and state
 *   has not changed, gets a character if present and
 *   returns it.  If a character is typed at the console,
 *   checks to see if it is keyboard interrupt character.  If
 *   so, prints short-form console menu and awaits next
 *   keyboard character.
 * Globals modified:    carrierDetect   modStat         haveCarrier
 *                      justLostCarrier whichIO         ExitToMsDos
 *                      visibleMode
 * Returns:     modem or console input character,
 *              or above special values
*/

#define MAX_TIME        210l            /* Time out is 210 seconds      */

char roomLevelFlag, versionTag[20];  /* global for status bar stuff */
AN_UNSIGNED modIn()
{
    AN_UNSIGNED logVal = 0;
    char c, localthing;
    char signal = FALSE;
	char ourDir[80], ourDrive;
  	char *whatRate;
    int roomCount, i;
    long int byteFree;

/*	char advertiseLine[140];
    int advertisePosition=0; */

	extern char *curBaud;
#ifdef BRIAN
	extern int  exitValue, CONTRAST;
#else
	extern int  exitValue, CONTRAST, tradeWars;
#endif
	extern aRoom     roomBuf;
	extern char ExitToMsdos, MenuFlag;
	extern char *baseRoom, *confirm, *VERTAG, noInit;

    windowReport=FALSE;
    notFlag=FALSE;

    if (!onLine() && CallSysop)
        SummonSysop();

    startTimer(0);
    while (TRUE) {
		if (noInit) whichIO = MODEM;
        if ((whichIO==MODEM)
				&&
			( (c=gotCarrier()) != modStat) || noInit) {
            /* carrier changed   */
            if ((c) || noInit || runningAsDoor) {  /* carrier present   */
				answerGuard=TRUE;
				if (noInit || runningAsDoor) {
#ifndef BRIAN
					if (!gotCarrier() && !onConsole) doWarsReset();
#endif
					noInit = FALSE;
                    warned          = FALSE;
                    haveCarrier     = TRUE;
                    modStat         = c;
                    newCarrier      = FALSE;
					itsGone         = FALSE;
                    justLostCarrier = FALSE;
    				roomLevelFlag   = TRUE;
					gate_keeper();
					logMessage(BAUD, cfg.whatRate, FALSE);
					if (frontEnd) getBinkleyBaud();
					ScrNewUser();
                    if (doorLogFlag) {
						expert = TRUE;
						HelpIfPresent("dooruser.blb");
						expert = FALSE;
						if (siege) doSecureLogin();
						}
					if (runningAsDoor) {
						expert = TRUE;
                        HelpIfPresent("citdoor.blb");
						expert = FALSE;
						}
					if (frontEnd) {
						expert = TRUE;
						greeting();
						expert = FALSE;
						if (siege) doSecureLogin();
						}
			        doorLogFlag = FALSE; /* bye-bye flag! */
				} else if (Find_baud(&whatRate) && !noInit && !runningAsDoor) {
					gate_keeper(); /* does all sysdata.usr checking on DCD */
                    printf("***CONNECT %s\n", whatRate);
					numberOfRings=0; /* gotCarrier sets answerGuard on DCD */
					justOut=FALSE;   /* This will reset all of the net flags */
					autoNet=FALSE;   /* that we use to control the netWindow */
					manualNet=FALSE; /* code; make sure prompts> are working */
					purgeFossilBuffs();
                    warned          = FALSE;
					onConsole       = FALSE;
                    haveCarrier     = TRUE;
                    modStat         = c;
                    newCarrier      = TRUE;
                    justLostCarrier = FALSE;
                    purgeFossilBuffs();
					itsGone         = FALSE;
					chatFlag        = FALSE;
					tradeWars       = FALSE;
    				roomLevelFlag   = TRUE;
	                logMessage(BAUD, whatRate, FALSE);
					whatBaudRate = whatRate;
				   	logMessage(16,"",FALSE);   /* carrier detect to log */
                    purgeFossilBuffs();
					ScrNewUser();
					if (strcmp(whatRate, "300")==SAMESTRING) {
						mPrintf("\n Sorry, no 300 baud connects allowed.\n");
						runHangup();
						purgeFossilBuffs();
						fakeBanner();
						}
	                }
                else {
                    runHangup();
                    purgeFossilBuffs();
					fakeBanner();
					}

	            return(0);
    	        }
		   else {
                pause(100);                 /* confirm it's not a glitch */
                if (!gotCarrier()) {    /* check again */
                    printf("\n***DISCONNECT\n");
					blockAllBlurbs=TRUE;
					if (tradeWars) doWarsReset();
					if(CONTRAST>127) {
						CONTRAST=CONTRAST-128;
						chatFlag=FALSE;
						ScrNewUser();
						}
				   	logMessage(17,"",FALSE);   /* lost carrier to log */
                    logMessage(CARRLOSS, "", logVal);
                    killConnection();           /* This should clear garp */
					recycleModem();
					purgeFossilBuffs();
					fakeBanner();
                    justLostCarrier = TRUE;
					dumpDeadWood = TRUE;
					itsGone=TRUE;
					haveCarrier = FALSE;
                    startTimer(1);      /* start anytime net timer */
					if (frontEnd) specialExit();
					purgeFossilBuffs();
					answerGuard=FALSE;
                    return(0);
                }
            }
        }

        if (MIReady()) {
            if (haveCarrier) {
                c = getMod();
                if (whichIO == MODEM)   return c;
            }
        } else {
        	giveaway(1);	/* multi-user time sharing */
        }


        if (!onLine() && killScreen==FALSE && blankVariable==TRUE
				&& chkTimeSince(0) >= timeForBlank) screenSaver();

        if (killScreen==TRUE) {
/*			setmem(advertiseLine,140,32); */
			if (ringDetect(cfg.FOSSIL_PORT) ) resurrect();

#ifdef SAVER-OPTION


            else {
              advertiseLine[advertisePosition+25]='C';
			  advertiseLine[advertisePosition+26]='i';
			  advertiseLine[advertisePosition+27]='t';
			  advertiseLine[advertisePosition+28]='a';
			  advertiseLine[advertisePosition+29]='d';
			  advertiseLine[advertisePosition+30]='e';
			  advertiseLine[advertisePosition+31]='l';
			  advertiseLine[advertisePosition+32]=':';
			  advertiseLine[advertisePosition+33]='K';
			  advertiseLine[advertisePosition+34]='2';
			  advertiseLine[advertisePosition+35]='N';
			  advertiseLine[advertisePosition+36]='E';
 			  advertiseLine[110]=0;
			  shrtColor(colTable.level0 /* A_GREEN */);
              cprintf("%s\r", advertiseLine+35);
			  pause(2);
			  advertisePosition++;
			  if (advertisePosition>110) advertisePosition=0;
			}


#endif

			}



        if (!onLine()) doRingCounter();

        if (!haveCarrier && !loggedIn && AideNetTrigger) doNetNow();
		if (sentNotice==FALSE &&
			   loggedIn &&
			   shortSession &&
			   (chkTimeSince(3) + 300 > timeOnLine)) {
			sentNotice=TRUE;
            mPrintf("\n %c*** Only 5 minutes remain for this call!!\n ", BELL);
			return 0;
			}
		if (loggedIn && shortSession && (chkTimeSince(3) > timeOnLine)) {
			strCpy(callLogPosting, "Session cancelled - time exceeded");
			mPrintf("\n %cTime limit exceeded!\n ", BELL);
			logMessage(19,"",FALSE);
			terminate(TRUE,TRUE);
			return 0;
			}
        if (KBReady()) {
            c = getCh();
			if ( killScreen==TRUE) {
				resurrect();
				}
            if (whichIO == CONSOLE) return(c);
            else {
                switch (toUpper(c)) {
#ifndef ONLYTHENET
                    case SPECIAL:
do_special:
                        printf("CONSOLE mode\n ");
						if (CONTRAST>127) {
							CONTRAST=CONTRAST-128;
							chatFlag=FALSE;
							ScrNewUser();
						}
                        whichIO = CONSOLE;
                        if (!gotCarrier()) {
							sendStringViaFossil(modemIdleString);
                            DisableModem();
/* BATCH FILE HOOK to DISABLE WATCHKIT goes HERE */
                            if ( (access("ONLOCAL.BAT", 00)) != -1)
											system("ONLOCAL.BAT >nul");
							whatRate = NULL;
		                    logMessage(BAUD, whatRate, FALSE);
							onConsole = TRUE;
							roomLevelFlag = TRUE;
							tradeWars       = FALSE;
							justOut=FALSE;
							autoNet=FALSE;
							manualNet=FALSE;
                            ScrNewUser();
						}
						setUp(FALSE);
                        warned          = FALSE;
                        return 0;
#endif
#ifndef BRIAN
#ifndef ONLYTHENET
                    case CON_NEXT:
                        if (!onLine()) goto do_special;
                        CallSysop = !CallSysop;
                        printf("\nSysop call toggle is %s\n",
                                        CallSysop ? "ON" : "OFF");
                        ScrNewUser();
                        break;
#ifdef UNSHRINK
					case 5:   /* an information window */
						savetext(1,1,80,25);
						window (16,7,60,15);
						upWin();
						textbackground(YELLOW);
						textcolor(BLACK);
						clrscr();
						window (17,8,59,14);
						textbackground(WHITE);
						clrscr();
						cprintf(" GENERAL SYSTEM INFORMATION\n");
						cprintf(" Software is %s\n", versionTag);
						cprintf(" Last User: %s\n", cfg.lastCaller[0]
										 ? cfg.lastCaller : "not recorded");
						cprintf(" Messages: %ld        Highest: %lu\n",
                                   cfg.newest-cfg.oldest +1, cfg.newest);
					    for (roomCount = i = 0; i < MAXROOMS; i++)
        						if (roomTab[i].rtflags.INUSE) roomCount++;
						cprintf(" Message-file:%5dK     Rooms: %d in use\n",
								cfg.maxMSector / (1024 / MSG_SECT_SIZE),
								roomCount);
                        cprintf(" Log size: %d accounts\n", cfg.MAXLOGTAB);
						getcwd(ourDir, 80);
                        ourDrive=ourDir[0];
						getdfree(ourDrive-'A'+1, &disk);
						byteFree=(long) disk.df_avail * (long) disk.df_bsec *
                                     (long) disk.df_sclus;
						cprintf(" Home Area is %s [%ld bytes free]",
									ourDir, byteFree);
                        window(1,1,80,24);
						textbackground(BLACK);
						if (!onLine()) {
							getch();
							downWin();
							restoretext(1,1,80,25);
							gotoxy(holdx, holdy);
            				}
                        break;
#endif
                    case 1:   /* what's to be done? */
                        if (onLine()) break;
/* #ifdef EVENTSTATUS */
						evMode=0;
						savetext(16,11,61,16);
						upWin();
                        window(16, 11, 61, 16);
						textbackground(RED);
						clrscr();
						textcolor(YELLOW);
/* #endif */
                        EventShow();
/* #ifdef EVENTSTATUS */
                        window(1,1,80,24);
						textbackground(BLACK);
						getch();
                        window(1,1,80,24);
						textbackground(BLACK);
						downWin();
						restoretext(16,11,61,16);
						gotoxy(holdx, holdy);
                        break;
/* #else
    					getPermission();
						doTotals();
						return 0;
 #endif */
#endif
#endif
/*
 * The strange method that IBM uses to detect keypress of a FUNCTION key
 * makes for the strange code that follows for function-keys in the code
 * below.  It's a kludge - it's ugly - it's cheating - but it works!
*/
#ifndef ONLYTHENET


                    case 2:
                        screenSaver();
                        break;


					case ';': /* F1 -- backup log files */
						if (!onLine()) {
/*				      		printf("Backup log files\n "); */
	                        backSysLog();
/* 					        printf("\n\n"); */
							doTotals();
/*							fakeBanner(); */
							roomLevelFlag=TRUE;
							return 0;
                            }
						break;
#endif
					case '<': /* F2  -- force Anytime Net */
						if (!onLine()) {
							manualNet=TRUE;
							doNetNow();
#ifdef NEWSTANDBY
							window(1,1,80,24);
						    textbackground(BLACK);
							specialBump=TRUE;
 							doTotals();
							gotoxy(holdx, holdy);
							specialBump=FALSE;
							roomLevelFlag = FALSE;
							ScrNewUser();
							fakeBanner();
							onConsole=FALSE;
#else
							doTotals();
							roomLevelFlag = FALSE;
							ScrNewUser();
							shrtColor(colTable.level2 /* A_BLUE */);
							printf("\n%s> ",roomBuf.rbname);
#endif
							return 0;
    						}
						break;

#ifndef BRIAN
#ifndef ONLYTHENET
					case '=': /* F3 -- view net node list/status */
						if (!onLine()) {
							notFlag=TRUE;
/* #ifdef NETSTATUS */
/* #ifdef NOISE */
							upWin();
/* #endif */
							savetext(10, 4, 64, (cfg.netSize/2)+13);
/*							window( 10, 4, 64, (cfg.netSize/2)+13 );
							textbackground(BLUE);
							clrscr();
 */
							window( 11, 5, 62, (cfg.netSize/2)+11 );
							textbackground(LIGHTMAGENTA);
							clrscr();

							window( 12, 6, 63, (cfg.netSize/2)+12 );
							textcolor(RED);
							textbackground(LIGHTGRAY);
							clrscr();

							windowReport=TRUE;
/* #endif */
    	    		        writeNet(TRUE);
/* #ifdef NETSTATUS */
							windowReport=FALSE;

						    window(12, (cfg.netSize/2)+12, 63, (cfg.netSize/2)+12);
						    cprintf(" A \"*\" indicates work pending.  [Press any key]");

							getch(); /* grab a key from the console */
							window(1,1,80,24);
						    textbackground(BLACK);

/*							getch(); */ /* grab a key from the console */
/* #ifdef NOISE*/
							downWin();
/* #endif */
							notFlag=FALSE;
							gotoxy(holdx, holdy);
							restoretext(10, 4, 64, (cfg.netSize/2)+13);
#ifdef NONETSTATUS
							getPermission();
							doTotals();
							return 0;
#endif
							}
                		break;

  					case '>': /* Function Key F4 */
						if (!onLine() && !gotCarrier()) {
							expert = TRUE;
                        	printf("\n");
							logReader("calllog.sys");
							getPermission();
							expert = FALSE;
							doTotals();
							return 0; /* E.T. phone home */
							}
						break;

					case '?': /* F5 and "?" do double-duty here! */
						if (!onLine()) {
							doConsoleHelp("fkeys.blb");
							gotoxy(holdx, holdy);
							}
						break;
#endif
					case '@': /* F6 to read the FIDO summary */
						if (!onLine() && !gotCarrier()) {
							expert = TRUE;
                        	printf("\n");
							if (HelpIfPresent("fidoday.hlp")) getPermission();
							expert = FALSE;
							doTotals();
							return 0; /* E.T. phone home */
							}
						break;
					case 'C': /* Key <F9> */
						if (!onLine() && !gotCarrier()) {
							whichIO = CONSOLE;
							expert = TRUE;
							if (toUpper(c)=='C') {
                        		printf("\n");
								tutorial("netlog.sys",3);
								}
							else dayList();
							getPermission();
							expert = FALSE;
							whichIO = MODEM;
							doTotals();
							return 0; /* E.T. phone home */
							}
						break;
#endif
#ifndef ONLYTHENET
					case 'D': /* Key <F10> */
						if (onLine() && gotCarrier()) {
							mPrintf("\n ");
                            if (!HelpIfPresent("chatcall.blb"))
								mPrintf("The Sysop wants to chat.\n ");
							return 0;
							}
						break;
#ifdef QTEST
					case 'A':
						if (ansi==FALSE && !onLine() && !gotCarrier()) {
							CITCOLOR = CITCOLOR+16;
							if (CITCOLOR > 127) CITCOLOR = CITCOLOR-128;
							VideoInit();
/*							doConsoleHelp("fkeys.blb"); */
							doTotals();
							return 0;
							}
						break;

					case 'B':  /* 'F8' response */
						if (ansi==FALSE && !onLine() && !gotCarrier()) {
							jumper = CITCOLOR;
							CITCOLOR++;
							if ((CITCOLOR-jumper)==16) {
								CITCOLOR=jumper;
								}
							if (CITCOLOR%16==0) CITCOLOR=CITCOLOR-16;
							VideoInit();
/*							doConsoleHelp("fkeys.blb"); */
							doTotals();
							return 0;
							}
						break;
#endif
#endif /* ONLYTHENET */
					case '-': /* ALT X return to MS DOS */
						if (!onLine() && !gotCarrier()) {
							whichIO = CONSOLE;
							onConsole = TRUE;
/* #ifdef UNSHRINK */
							savetext(1,1,80,25);
							upWin();
							window(26, 7, 49, 9);
							textbackground(RED);
							clrscr();
							textcolor(YELLOW+BLINK);
                            cprintf("\n Exit to DOS <Y/N>? ");
							do {
	                            localthing=toupper(getch());
								textcolor(YELLOW);
								if (localthing!= 'Y' && localthing!='N') {
									sound(150); delay(100); nosound();
									}
								} while (localthing!= 'Y' && localthing!='N');
       	                    cprintf("%c", localthing);
							if (localthing=='Y') {
								cprintf("es");
								delay(750);
					            ExitToMsdos = TRUE;
					            exitValue   = SYSOP_EXIT;
#ifdef TRIAL
                                clrscr();
								cprintf("\n       ");
								doSignOff();
#endif /* for TRIAL */
								return FALSE;
								}
							else {
								cprintf("o");
/*								delay(400); */
								downWin();
								window(1,1,80,24);
								textbackground(BLACK);
								restoretext(1,1,80,25);
								}
#ifdef NOEXITWINDOW
							mPrintf("\n Exit to DOS\n ");
   					        if (getYesNo(confirm)) {
					            ExitToMsdos = TRUE;
					            exitValue   = SYSOP_EXIT;
					            return FALSE;
								}
#endif
							whichIO = MODEM;
							onConsole = FALSE;
/* #ifndef UNSHRINK
							return 0;
   #endif */
       						}
						break;
#ifndef ONLYTHENET
					case 4:  /* CTRL D "twit" key */
					case 11: /* CTRL K "twit" key fallthru */
						if (onLine()) {
							expert = TRUE;
						    strCpy(callLogPosting,
								   "Session cancelled from Console");
							logMessage(19,"",FALSE);
							HelpIfPresent((c == 4) ?
									 "dtwitkey.blb" : "ktwitkey.blb");
							sleep(2);
							runHangup();
							expert = FALSE;
							}
						break;

					case 22: /* toggle visible Mail> on CTRL-V */
						cfg.SeeMail = !cfg.SeeMail;
						if (onLine()) printf("***Mail> %s***\n",
										 cfg.SeeMail ? "VISIBLE" : "HIDDEN");
                        ScrNewUser();
						break;

  			        case 25: /* CTRL Y -- toggle Chat mode */
		                cfg.BoolFlags.noChat = !cfg.BoolFlags.noChat;
            			if (onLine()) printf("[Chat mode %s] ",
			                           cfg.BoolFlags.noChat ? "OFF" : "ON");
			            ScrNewUser();
			            break;
#endif
                }
            }
        }

        if (DoTimeouts()) return 0;

        /* check for no input.  (Short-circuit evaluation, remember!) */
        if (whichIO==MODEM  &&  haveCarrier  &&
                 chkTimeSince(0) >= MAX_TIME) {
            mPrintf("Sleeping? Call again :-)");
			sleepFlag = TRUE;
            runHangup();
            logVal = 't';
			return; /* K2NE added - TEST */
        }
        else if (whichIO == MODEM &&
                 haveCarrier &&
                 chkTimeSince(0) == MAX_TIME - 10) {
            if (!signal)
                oChar(BELL);
            signal = TRUE;
        }
    }
}

/*****
 *    oChar() is the top-level user-output function
 *      sends to modem port and console both
 *      does conversion to upper-case etc as necessary
 *      in "debug" mode, converts control chars to uppercase letters
 *    Globals modified:       prevChar
 *****/
void oChar(c)
char c;
{
    prevChar = c;                       /* for end-of-paragraph code    */
    if (outFlag != OUTOK &&             /* s(kip) mode                  */
        outFlag != IMPERVIOUS)
        return;

    if (c == NEWLINE)   c = ' ';        /* doCR() handles real newlines */

    /* show on console              */

    if (outPut == DISK)
        putc(c, upfd);
    else {
        if (haveCarrier)
            (*Table[TransProtocol].method)(c);
        if (TransProtocol == ASCII)
            mputChar(c);
    }
}

/*****
 *    Reception() Reads data, trying to use specified protocol.
 *    Note: This only handles XMODEM, WXMODEM, and YMODEM.
 *    Due to the multiple authors of these protocols, this code is
 *    a mess.  I do da best I can, wid da leetle brain giben me.
 *****/
char Reception(protocol, WriteFn)
char protocol;
int  (*WriteFn)(int c);
{
    char RetVal;

    if (!gotCarrier())
        return CARR_LOSS;

    GenTrInit();

    if (protocol == XMDM) {

        if ((RetVal = JumpStart((inNet != NON_NET) ? 1 : 4, 10, 'C', SOH,
                                                SOH, recXYmodem, WriteFn))
                        != NO_LUCK)
            return RetVal;
        DoCRC = FALSE;
        if ((RetVal = JumpStart(6, 10, NAK, SOH, SOH, recXYmodem, WriteFn))
                        != NO_LUCK)
            return RetVal;
    }
                /* Extrapolated from C. Forsberg's doc and WC's doc. */
    return NO_START;
}


/*****
 *    recXYmodem() Receives data via YMODEM SINGLE
 *    (May also work with XMODEM...)
 *****/
char recXYmodem(WriteFn)     /* Supports YMODEM SINGLE mode only */
int (*WriteFn)(int c);
{
    char AbortTransmission = FALSE;
    int  SectorResult,              /* Result of packet read */
         Sector,                    /* Sector # received */
         LastReceived = 0,          /* Last sector received */
         TotalErrors = 0,           /* Total errors for this transmission */
         CurrentErrors = 0,         /* Total errors for block */
         SOH_val,                   /* SOH indicates 128, STX 1024 */
         BufSize;                   /* In conjunction with SOH_val */
    /*
     * Assume that the initial SOH or STX has been received.  An initial EOT
     * should also be handled by the startup function.
     */

    SOH_val = GlobalHeader;         /* Set by startup function, is global */

    do {
        BufSize = (SOH_val == SOH) ? SECTSIZE : YM_BLOCK_SIZE;
        SectorResult = CommonPacket(YMDM, BufSize, receive, &Sector);
        if (SectorResult == NO_ERROR) {
            if (Sector == (LastReceived + 1) % 256) {
				doRTS(FALSE);
                SectorResult = CommonWrite(WriteFn, BufSize);
                doRTS(TRUE);
                LastReceived++;
                /* if (inNet == NON_NET)
                    printf("Block %d received (try %d, %d total errors)\r",
                                                                LastReceived,
                                                                CurrentErrors,
                                                                TotalErrors); */
            }
            else if (Sector != LastReceived)
                SectorResult = SYNCH_ERROR;
        }
        if (SectorResult != NO_ERROR) {
            TotalErrors++;
/*            if (inNet == NON_NET)
                printf("\nError: %s (%d)\n", msg[SectorResult],
                                                       CurrentErrors+1); */
            switch (SectorResult) {
                case SYNCH_ERROR:
                case WRITE_ERROR:
                    outMod(CAN);
                    outMod(CAN);    /* Fatal error -- cancel transmission */
                case CARR_LOSS:         /* Don't bother with CANs here */
                    AbortTransmission = TRUE;
/*                    if (inNet == NON_NET)
                        printf("\nAborting: %s error\n",
                                            msg[SectorResult]); */
                    break;
                default:            /* Some normal problem */
                    outMod(NAK);
                    if (CurrentErrors++ >= 9) {
                        AbortTransmission = TRUE;
                        /* if (inNet == NON_NET)
           					printf("\nAborting: 10 consecutive errors\n"); */
                    }
            }
        }
        else {
            outMod(ACK);
            CurrentErrors = 0;
        }
        if (!AbortTransmission) {
            do
                SOH_val = receive(10);
            while (SOH_val != EOT && SOH_val != SOH && SOH_val != CAN &&
                     SOH_val != STX);
        }
    } while (SOH_val != EOT && !AbortTransmission && SOH_val != CAN);
    if (!AbortTransmission && SOH_val != CAN) {
        outMod(ACK);    /* Ack the EOT, indicates end of file */
/*		doRTS(FALSE);
		purgeFossilBuffs();
		doRTS(TRUE); */
        return TRAN_SUCCESS;
    }
    else return TRAN_FAILURE;
}

/*****
 *  ringSysop() signals a chat mode request.  Exits on input from
 *  modem or keyboard.
 *****/
#define RING_LIMIT 6
void ringSysop()
{
#ifndef ONLYTHENET
    char answered;
    int  i, ring;

    mPrintf("\n Ringing sysop.\n ");

    answered = FALSE;
    for (ring = 0; ring < RING_LIMIT && !answered && gotCarrier(); ring++) {
        for (i=0; !BBSCharReady() && !KBReady(); i = ++i % 7)  {
            /* play shave-and-a-haircut/two bits... as best we can: */
            oChar(BELL);
            pause(cfg.shave[i]);
            if (i == 6) ring++;
        }
        if (BBSCharReady() || KBReady()) answered = TRUE;
    }

    if (KBReady())   {
        getCh();
        whichIO = CONSOLE;
        interact(TRUE);
        whichIO = MODEM;
    }
    else if (ring >= RING_LIMIT) {
        cfg.BoolFlags.noChat = TRUE;
        mPrintf("\n Sysop not around...\n ");
    }
    else modIn();
#endif
}

/*****
 *    runHangup() This code does a hangup, then resets for next
 *                carrier
 *****/
void runHangup()
{

	extern int tradeWars;

	if (tradeWars == TRUE) {
		strCpy(callLogPosting, "Level 8 Exit");
		logMessage(19,"",FALSE);
		doWarsReset();
		}

    modemClose();
/*    rawModemInit(); */
	purgeFossilBuffs();
}

/*****
 *    SendCmnBlk() Sends a WX/X/Y Modem block
 *****/
void SendCmnBlk(type, block, SendFn, size)
int size;
char type;
TransferBlock *block;
char (*SendFn)(int c);
{
    int rover;

/*	delay(75);   MAYBE */
/*    if (inNet == NON_NET) printf("Sending block %d\r", block->ThisBlock); */
    (*SendFn)((size == SECTSIZE) ? SOH : STX);
    (*SendFn)(block->ThisBlock & 0xFF);
    (*SendFn)(~(block->ThisBlock & 0xff));
    for (rover = 0; rover < size; rover++) {
        (*SendFn)(block->buf[rover]);
#ifdef WXMODEM_avl
        if (type == WXMDM) WXResponses();
#endif
        if (!gotCarrier()) {
            TrError = CARR_LOSS;
            return ;
        }

    }

        /*
         * Handle CRC/Checksum stuff.
         */
    if (DoCRC) {
        (*SendFn)(((block->ThisCRC & 0xff00) >> 8));
    }
    (*SendFn)(block->ThisCRC & 0xff);
}

/*****
 *      sendWCChar() sends a file using Ward Christensen's protocol.
 *      (i.e., compatable with xModem, modem7, modem2, YAM, ... )
 *****/
int sendWCChar(c)
int c;         /* character to output to MODEM */
{
    if (TrError != TRAN_SUCCESS)
        return FALSE;

    blk.buf[TrCount++] = c;
    TrCksm = (TrCksm + c) & 0xFF;
    if (TrCount != SECTSIZE)
        return TRUE;

    blk.ThisBlock = TrBlock;
    blk.ThisCRC   = (DoCRC) ? calcrc(blk.buf, SECTSIZE) : TrCksm;

    return (XYBlock(XMDM, SECTSIZE));
}


/************************************************************************/
/*      SummonSysop() rings the sysop for ^T                            */
/************************************************************************/
void SummonSysop()
{
#ifndef ONLYTHENET
    int i;

    CallSysop = FALSE;
    DisableModem();
    printf("SYSOP: System available!  Hit space!\n");
    for (i = 0; i < 12 && !KBReady(); i++) {
        onConsole = TRUE;
        printf("%c", BELL);
        onConsole = FALSE;
        startTimer(0);
        while (!KBReady() && chkTimeSince(0) < 10l)
            ;
    }

    if (KBReady()) {
        getCh();
        printf("CONSOLE mode\n ");
        whichIO = CONSOLE;
        setUp(FALSE);
        ScrNewUser();
        warned          = FALSE;
    }
    else {
        printf("No answer.  System back on MODEM.\n");
        EnableModem();
    }
    givePrompt();
#endif
}

/************************************************************************/
/*      Transmission()  Starts protocols up.                            */
/*      Note: This only handles XMODEM, WXMODEM, and YMODEM.            */
/************************************************************************/
char Transmission(protocol, mode)      /* Transmission handler */
char protocol;
char mode;              /* STARTUP or FINISH? */
{
    int Errors, m;

    if (!gotCarrier() && protocol != ASCII) {
        return CARR_LOSS;
    }

    if (protocol == ASCII) return TRAN_SUCCESS;

    if (mode == STARTUP) {
        GenTrInit();
        for (Errors = 0; Errors < ERRORMAX; Errors++) {
            m = receive(MINUTE);
            switch (m) {
                case CAN:
					/* if (inNet == NON_NET) printf("Xmit CANCELLED.\n"); */
                    return CANCEL;
                case ERROR:
                    /* if (inNet == NON_NET) printf("Xmit not started.\n"); */
                    return NO_START;
                case NAK:
                    TransProtocol = XMDM;
                    DoCRC = FALSE;
                    /* if (inNet == NON_NET) printf("XMODEM Transmission.\n"); */
                    return TRAN_SUCCESS;
                case 'C':
                    /* if (inNet == NON_NET) printf("XMODEM-CRC Transmission.\n"); */
                    TransProtocol = XMDM;
                    return TRAN_SUCCESS;
            }
        }
        return NO_START;    /* If we make it out of the loop, error */
    }
    else {
        /* if (inNet == NON_NET) printf("\n"); */
        if (TrError != TRAN_SUCCESS)
            return TrError;

        if (TransProtocol == YMDM && TrCount < SECTSIZE)
            CurYBufSize = SECTSIZE;

        while (TrCount != 0)
        {
            (*Table[TransProtocol].method)(' ');
        }

        if (Table[TransProtocol].CleanUp != NULL)
            return (*Table[TransProtocol].CleanUp)();

        TransProtocol = ASCII;  /* Return to normal */

        return TRAN_SUCCESS;
    }
}

/************************************************************************/
/*      XYBlock() handles common work of XMODEM and YMODEM              */
/************************************************************************/
char XYBlock(mode, size)
int size, mode;
{
    int i, m;

    for (i = 0; i < ERRORMAX; i++) {
        SendCmnBlk(mode, &blk, outMod, size);
/*		purgeFossilBuffs(); */
        m = receive(MINUTE);
        if (m == ACK || m == CAN || !gotCarrier()) break;
	    }
    TrCksm  = TrCount = 0;
    TrBlock++;
    if (m == ACK)
        return TRUE;
    TrError = TRAN_FAILURE;
    /* if (inNet == NON_NET) printf("Aborting\n "); */
    return FALSE;
}

/************************************************************************/
/*      XYClear() finishes XMODEM and YMODEM transmission               */
/************************************************************************/
int XYClear()
{
    int i, m;

    for (i = 0; gotCarrier() && i < ERRORMAX; i++) {
        outMod(EOT);
        if ((m = receive(MINUTE)) == ACK || !haveCarrier)
            return TRAN_SUCCESS;
        if (m == CAN)
            return TRAN_FAILURE;
    }
    return TRAN_FAILURE;
}


screenSaver()
{
 if (onLine()) return;

 holdx=wherex();
 holdy=wherey();
 savetext(1,1,80,25);
 window(0,0,79,25);
 /* StopVideo(); */
 system("cls");
 killScreen=TRUE;
/* didRing=FALSE; */
 startTimer(0);
}

/*
 while (getOut==FALSE) {
	if (kbhit()) getOut=TRUE;
    if (ringDetect(cfg.FOSSIL_PORT)  ) getOut=TRUE;
    }
*/

resurrect()
{
 killScreen=FALSE;
/*  didRing=FALSE; */
 /* VideoInit(); */
  gotoxy(holdx,holdy);
 restoretext(1,1,80,25);
  gotoxy(holdx,holdy);
 window(0,0,79,24);
  gotoxy(holdx,holdy);
 ScrNewUser();
  gotoxy(holdx,holdy);
 startTimer(0);
  gotoxy(holdx,holdy);
}
