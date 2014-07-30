/************************************************************************/
/*                              netcall.c                               */
/*                                                                      */
/*      Networking call functions.                                      */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 88Dec23 <br> removed WXMODEM                                         */
/* 86Aug20 HAW  History not maintained due to space problems.           */
/************************************************************************/

#include "ctdl.h"
#include "sys\stat.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                 External variable declarations in NET.C              */
/************************************************************************/
char                    inReceive;
int						sessionMessageCount;

extern int				fieldSuppress; /* Net_Switch stuff */
extern char             *SR_Sent, callLogPosting[800];
extern char             *pollCall;
extern FILE             *netMisc;
extern FILE             *netLog;
extern AN_UNSIGNED      RecBuf[SECTSIZE + 5];
extern int              counter, linkLayerCount;
extern int              callSlot;
extern label            callerName, callerId;
extern char				suppressAideMsg;
char                    checkNegMail;
extern char             processMail;
extern char             *VERSION;

char             normId(), getNetMessage();
AN_UNSIGNED      inp();

extern CONFIG    cfg;            /* Lots an lots of variables    */
extern logBuffer logBuf;         /* Person buffer                */
extern aRoom     roomBuf;        /* Room buffer                  */
extern rTable    *roomTab;
extern MSG_BUF   msgBuf;
extern NetBuffer netBuf;
extern NetTable  *netTab;
extern int       thisNet;
extern char      onConsole;
extern char      loggedIn;       /* Is we logged in?             */
extern char      outFlag;        /* Output flag                  */
extern char      haveCarrier;    /* Do we still got carrier?     */
extern char      modStat;        /* Needed so we don't die       */
extern char      TrError;
char *chMailTemplate = "chkMail.$$$";

/************************************************************************/
/*      caller() we've called and got carrier, so let's goferit         */
/************************************************************************/
void caller()
{
    ITL_InitCall();             /* initialize the ITL layer */

    setmem(SR_Sent, SHARED_ROOMS, 0);

    inReceive = FALSE;
    processMail = FALSE;
    checkNegMail = FALSE;
	fieldSuppress = TRUE; /* Net_Switch stuff */
    splitF(netLog, "\nLink: Connected.\n");

    caller_stabilize();

    if (!haveCarrier) return ;  /* Abort */

    linkLayerCount++;
    splitF(netLog, "Session: Stabilized [Job N%d]\n", linkLayerCount);
    sendId();
    if (!haveCarrier) return ;  /* Abort */

    /* call to ITL_optimize() was here when WXMODEM still around */

    sendStuff(FALSE, TRUE);

    startTimer(0);
    while (gotCarrier() && chkTimeSince(0) < 10) ;

    killConnection();
    doResults();

    splitF(netLog, "Session: Call to %s finished at ", netBuf.netName);
	purgeFossilBuffs();
    delay(20);
}

/************************************************************************/
/*      sendStuff() assume role of sender                               */
/************************************************************************/
void sendStuff(reversed, SureDoIt)
char reversed, SureDoIt;
{
    if (SureDoIt && callSlot != ERROR) {
        SendPwd();
        if (!haveCarrier) return ;  /* Abort */

        if (netBuf.nbflags.normal_mail) {
            sendMail();
            if (!haveCarrier) return ;  /* Abort */
            checkMail();
        }
        if (!haveCarrier) return ;  /* Abort */

        if (netBuf.nbflags.room_files)
            askFiles();
        if (!haveCarrier) return ;  /* Abort */

        sendSharedRooms();
        if (!haveCarrier) return ;  /* Abort */

        if (netBuf.nbflags.send_files)
            doSendFiles();
        if (!haveCarrier) return ;  /* Abort */


        roleReversal(reversed);
        if (!haveCarrier) return ;  /* Abort */
    }

    sendHangUp();

    pollCall[thisNet]--;        /* Don't set polled flag unless stable call */
}

/************************************************************************/
/*      SendPwd() Send system password                                  */
/************************************************************************/
void SendPwd()
{
    struct cmd_data cmds;

    if (callSlot == ERROR) return;
    if (netBuf.TheirPwd[0] != 0) {      /* only send if need to -- gets */
        zero_struct(cmds);              /* us around a bug in pre net 1.10*/
        strCpy(cmds.fields[0], netBuf.TheirPwd);        /* versions     */
        cmds.command = SYS_NET_PWD;
        sendNetCommand(&cmds, "system pwd");
    }
}

/************************************************************************/
/*      roleReversal() hokay, into drag                                 */
/************************************************************************/
void roleReversal(reversed)
char reversed;
{
    struct cmd_data cmds;

    if (reversed) return ;
    if (!netBuf.nbflags.local && !netBuf.nbflags.spine) return ;

    splitF(netLog, "Network: Reversing roles\n");

    zero_struct(cmds);
    cmds.command = ROLE_REVERSAL;
    if (!sendNetCommand(&cmds, "role reversal"))
        return;

    rcvStuff(TRUE);
    reply(GOOD, "");
}

/************************************************************************/
/*      caller_stabilize() Tries to stabilize the call --               */
/*                         baud is already set                          */
/************************************************************************/
void caller_stabilize()
{
    int tries, x1, x2, x3;

    pause(20);  /* a brief respite! WAS 100 THEN 20*/
	purgeFossilBuffs();                /* Clear garbage        */

/****** special patch for binkley delays - whatakludge!!! ***************/
/* #ifdef NET_BINKLEY */
    outMod(7);  /* if Citadel shut him up! */

if (netBuf.nbflags.uses_frontend) {
    for (tries = 0; tries < 5 &&gotCarrier(); tries++) {
/*		outMod(7); */
        for (startTimer(0); chkTimeSince(0) < 3l;)
			;
		outMod(7);
		}
	purgeFossilBuffs(); /* flushes modemINbuffer - clear out garbage
							    * remote may have sent from Binkley */
/* #endif */
	}
	purgeFossilBuffs();
    for (tries = 0; tries < 25 && gotCarrier(); tries++) { /* WAS 20 */
        if (cfg.BoolFlags.debug) splitF(netLog, ".");
        outMod(7);
        outMod(13);
        outMod(69);
        for (startTimer(0); chkTimeSince(0) < 2l && !MIReady();)
            ;

        if (MIReady()) {
            x1 = receive(2);
            x2 = receive(2);
            if (x2 != ERROR) x3 = receive(2);
            if (x1 == 248 &&
                x2 == 242 &&
                x3 == 186) {
                outMod(ACK);
                if (cfg.BoolFlags.debug)
                    splitF(netLog, "ACKing, Call stabilized\n");
                return;
            }
            else if (cfg.BoolFlags.debug)
                splitF(netLog, "%d %d %d\n", x1, x2, x3);
		purgeFossilBuffs();
        }
    }
    splitF(netLog, "Link: Couldn't stabilize; dropping link @");
    purgeFossilBuffs();
    killConnection();
    fossilDTR(FALSE);
    delay(50);
	purgeFossilBuffs();
	recycleModem();
	purgeFossilBuffs();
}

/************************************************************************/
/*      sendId() Sends ID to the receiver                               */
/************************************************************************/
void sendId()
{
	fieldSuppress = TRUE; /* Net_Switch stuff */
    if (ITL_Send(STARTUP)) {
	    mTrPrintf("%s", cfg.codeBuf + cfg.nodeId  );
    	mTrPrintf("%s", cfg.codeBuf + cfg.nodeName);
/*        mTrPrintf("vaq%s", VERSION); */
    	if (ITL_Send(FINISH))
	        return;
    }
    no_good("Couldn't transfer ID to %s!", TRUE);

}
/************************************************************************/
/*      sendMail()  send normal mail to receiver                        */
/************************************************************************/
void sendMail()
{
    struct cmd_data cmds;
    int             nor_mail;

    if (!gotCarrier()) {
        modStat = haveCarrier = FALSE;
        return ;
    }

    splitF(netLog, "Network: Sending mail\n");

    zero_struct(cmds);
    cmds.command = NORMAL_MAIL;
    if (!sendNetCommand(&cmds, "normal mail"))
        return;

    if (!ITL_Send(STARTUP)) {
        no_good("Couldn't start Mail transfer to %s!", TRUE);
        killConnection();
        return;
    }

	fieldSuppress=FALSE; /* keep Net_Switch fields in MAIL only */
    nor_mail = s_m_n();            /* Send normal mail     */
	fieldSuppress=TRUE;

    if (gotCarrier()) {
        splitF(netLog, "Network: Sent %d message%s\n",
			 nor_mail, nor_mail != 1 ? "s" : "");
        sessionMessageCount=sessionMessageCount+nor_mail;
        netBuf.nbflags.normal_mail = FALSE;
        ITL_Send(FINISH);
    }
}

/************************************************************************/
/*      checkMail()  negative acknowledgement on netMail>               */
/************************************************************************/
void checkMail()
{
    struct cmd_data cmds;
    SYS_FILE fileNm;
    extern char *WRITE_ANY;

    if (!gotCarrier()) {
        return;
    }

    splitF(netLog, "Network: Requesting remote-node mail check\n");

    makeSysName(fileNm, chMailTemplate, &cfg.netArea);

    zero_struct(cmds);
    cmds.command = CHECK_MAIL;
    if (!sendNetCommand(&cmds, "check mail")) {
        return;
    }

    if (ITL_Receive(fileNm, FALSE) == ITL_SUCCESS)
        checkNegMail = TRUE;        /* Call readNegMail() later */
}

/************************************************************************/
/*      readNegMail() reads and processes negative acks                 */
/************************************************************************/
void readNegMail()
{
    label author, target, context, node;
    char tempReply[480], tempPath[480];
    int whatLog;
    logBuffer lBuf;
    int  sigChar;
    SYS_FILE fileNm;
    extern char *READ_ANY;
	int q;
	char next[480];

    makeSysName(fileNm, chMailTemplate, &cfg.netArea);
    if ((netMisc = safeopen(fileNm, READ_ANY)) == NULL) {
        no_good("Couldn't open negative ack file from %s.", TRUE);
        return ;
    }
    initLogBuf(&lBuf);

    getRoom(MAILROOM);
    sigChar = getc(netMisc);
    while (sigChar != NO_ERROR && sigChar != EOF && sigChar != EOF) {
	    strcpy(tempReply, msgBuf.mbmsgreply);
		strcpy(tempPath, msgBuf.mbmsgpath);
        setmem(next, strlen(next), '\0');
        q=findNodeId(cfg.codeBuf+cfg.nodeId, next, msgBuf.mbmsgreply);
        q=findNetName(next, next);

		zero_struct(msgBuf);

        getNetStr(author, NAMESIZE);
        getNetStr(target, NAMESIZE);
        getNetStr(context, NAMESIZE);
        switch (sigChar) {
            case NO_RECIPIENT:
            strCpy(msgBuf.mbauth, "Citadel");
		    if (tempPath[0])
		      strCpy(msgBuf.mbauth, "FlashNET");

             if (q==1) strCpy(msgBuf.mbaddr, next);
		     strCpy(msgBuf.mbmsgpath, tempReply);
             strCpy(msgBuf.mbto, author);
             getRecipient(&lBuf, &whatLog);
             if (strLen(author) > 0) {
                    sPrintf(msgBuf.mbtext,
"Your netMail to '%s' (%s) failed.  Not found on %s.",
                    target, context, callerName);

                    putMessage();
					if (whatLog!=ERROR) noteMessage(&lBuf, whatLog);
                    break;
                }
            case UNKNOWN:
                zero_struct(msgBuf);
                sPrintf(msgBuf.mbtext,
"Problems with netMail: author=-%s-,target=-%s-,context=-%s-",
                                          author, target, context);
                netResult(msgBuf.mbtext);
                break;
            case BAD_FORM:
                sPrintf(msgBuf.mbtext, "Bad netMail sent to %s.", callerName);
                netResult(msgBuf.mbtext);
                break;
            default:
                sPrintf(msgBuf.mbtext, "Bad sigChar=%d.", sigChar);
                netResult(msgBuf.mbtext);
                break;
        }
        sigChar = getc(netMisc);
    }
    fclose(netMisc);
    unlink(fileNm);
    killLogBuf(&lBuf);
}

/************************************************************************/
/*      sendSharedRooms() Sends all shared rooms to receiver            */
/************************************************************************/
void sendSharedRooms()
{
    char *send1, *send2, *send3, doit;
    int  commnd;
    extern char *R_SH_MARK, *NON_LOC_NET, *LOC_NET, inNet;
    int rover;

    for (rover = 0; rover < SHARED_ROOMS; rover++) {
        if (!gotCarrier()) {
            modStat = haveCarrier = FALSE;
            return;
        }
        if (isSharedRoom(thisNet, rover) && roomValidate(thisNet, rover) &&
                        SR_Sent[rover] != 1) {
            if (roomTab[netRoomSlot(rover)].rtlastNet >
                                  netBuf.netRooms[rover].lastMess ||
                   ((roomTab[netRoomSlot(rover)].rtShareType == BACKBONE &&
                    netBuf.netRooms[rover].mode != PEON) &&
                    (inNet != ANYTIME_NET))  /* No room-poll during AnyTime */
                ) { /* also set ^^^ in NETRCV.C to kill poll during AnyTime */
                getRoom(netRoomSlot(rover));
                doit = TRUE;
                send1 = R_SH_MARK;
                send2 = send3 = NULL;
                switch (roomBuf.rbShareType) {
                case PEON:
                        commnd = NET_ROOM;
                        break;
                case REG_HOST:          /* Talking to backbones not here */
                        if (netBuf.netRooms[rover].mode != PEON)
                            doit = FALSE;
                        else {
                            commnd = NET_ROOM;
                            send2 = NON_LOC_NET;
                        }
                        break;
                case BACKBONE:
                        switch (netBuf.netRooms[rover].mode) {
                        case PEON:
                                commnd = NET_ROOM;
                                send2 = NON_LOC_NET;
                                break;
                        case ACTIVE_BACKBONE:
                        case REG_HOST:
                                if (netBuf.nbflags.local)
                                    commnd = NET_ROOM;
                                else
                                    commnd = NET_ROUTE_ROOM;
                                send2 = NON_LOC_NET;
                                send3 = LOC_NET;
                                break;
                        case PASS_BACKBONE:
                                if (!inReceive) doit = FALSE;
                                else if (netBuf.nbflags.local)
                                    commnd = NET_ROOM;
                                else
                                    commnd = NET_ROUTE_ROOM;
                                send2 = NON_LOC_NET;
                                send3 = LOC_NET;
                                break;
                        default: crashout("shared rooms: #2");
                        }
                        break;
                default: crashout("shared rooms: #1");
                }
                if (doit) {
                    findAndSend(commnd, send1, send2, send3, rover,
                                RoomSend, roomBuf.rbname, RoomReceive);
                    SR_Sent[rover] = 1;
                }
            }
        }
    }
    DoVirtuals();
}

/************************************************************************/
/*      findAndSend() Finds and sends specified messages                */
/************************************************************************/
void  findAndSend(commnd, send1, send2, send3, rover, MsgSender, roomName,
MsgReceiver)
int   commnd, rover;
label roomName;
char  *send1, *send2, *send3;
int   (*MsgSender)(int r, char *d1, char *d2, char *d3),
      (*MsgReceiver)(int r);
{
    struct cmd_data cmds;
    char mess[140];
    int  tempcount;                        /* K2NE...........*/
    extern char *netRoomTemplate, *WRITE_ANY, ThisNetRoom[40];

    zero_struct(cmds);
    cmds.command = commnd;
    strCpy(cmds.fields[0], roomName);
    if (commnd != ERROR)
        if (!sendNetCommand(&cmds, "shared rooms")) {
            strCpy(mess, "%s reports: ");
            strCat(mess, RecBuf + 1);
            no_good(mess, FALSE);
            return ;
        }

    if (!ITL_Send(STARTUP)) {
        no_good("Couldn't start WC for room sharing: %s",
                                                       FALSE);
        return;
    }
    tempcount = (*MsgSender)(rover, send1, send2, send3);
    ITL_Send(FINISH);
/*  splitF(netLog, "Network:  Sending %s ", roomBuf.rbname);
    splitF(netLog, "(%d messages).\n", tempcount); */

	if (tempcount > 0)
/*		splitF(netLog, "Network:  Sent %d message%s for \"%s.\"\n",
				 tempcount, tempcount > 1 ? "s" : "", ThisNetRoom); */

		splitF(netLog, "Network: Sent [%s, %d]\n", ThisNetRoom, tempcount);
		sessionMessageCount=sessionMessageCount+tempcount;

    if (commnd == NET_ROUTE_ROOM) {
        (*MsgReceiver)(rover);
    }
}
#ifdef NET_K2NE
char testFileMake=FALSE;
FILE *testFileptr;
#endif

int RoomReceive(rover)
int rover;
{
    recNetMessages(rover, roomBuf.rbname, netRoomSlot(rover));
}
char ThisNetRoom[40];  /* K2NE */

int RoomSend(rover, send1, send2, send3)
int rover;
char *send1, *send2, *send3;
{
    int        msgRover, tempcount = 0;
    MSG_NUMBER HiSent = 0l;
    char testFile[100];

	strCpy(ThisNetRoom,roomBuf.rbname);   /* K2NE */
/*  splitF(netLog, "Network: Sending %s ", roomBuf.rbname);  */

#ifdef NET_K2NE
	sPrintf(testFile, "tempnet.$$$");
	unlink("tempnet.$$$");
	testFileptr = fopen(testFile, "at");
	testFileMake = TRUE; /* FALSE;    K2NE ----- FEATURE DISABLED!!! */
#endif

    for (msgRover = 0; msgRover < MSGSPERRM; msgRover++) {
        if (netBuf.netRooms[rover].lastMess <
                    roomBuf.msg[msgRover].rbmsgNo) {
            if (findMessage(roomBuf.msg[msgRover].rbmsgLoc,
                         roomBuf.msg[msgRover].rbmsgNo))
                if (strCmpU(msgBuf.mbaddr, send1) == SAMESTRING ||
                    strCmpU(msgBuf.mbaddr, send2) == SAMESTRING ||
                    strCmpU(msgBuf.mbaddr, send3) == SAMESTRING) {
                    tempcount++;
                    printMessage(roomBuf.msg[msgRover].rbmsgLoc,
                         roomBuf.msg[msgRover].rbmsgNo, TRUE);
                    HiSent = max(HiSent, roomBuf.msg[msgRover].rbmsgNo);
                }
        }
    }
#ifdef NET_K2NE
	system("dsz sx tempnet.$$$");
#endif
    if (HiSent != 0l) {
        netBuf.netRooms[rover].lastMess = HiSent;
        netTab[thisNet].netTRooms[rover].lastMess = HiSent;
    }
    SR_Sent[rover] = 1;
#ifdef NET_K2NE
	fclose(testFileptr);
	unlink("tempnet.$$$");
	testFileMake = FALSE;
#endif
    return tempcount;
}

/************************************************************************/
/*      doSendFiles() send files to a victim                            */
/************************************************************************/
void doSendFiles()
{
    extern char       *READ_ANY;
    struct   fl_send  theFiles;
    SYS_FILE          sdFile;
    char              temp[8];
    FILE              *fd;

    sPrintf(temp, "%d.sfl", thisNet);
    makeSysName(sdFile, temp, &cfg.netArea);
    if ((fd = safeopen(sdFile, READ_ANY)) == NULL) {
        sPrintf(msgBuf.mbtext, "SendFile error for %s!",
                                              netBuf.netName);
        netResult(msgBuf.mbtext);
        netBuf.nbflags.send_files = FALSE;
    }
    else {
        while (getSLNet(theFiles, fd) && haveCarrier) {
            sysSendFiles(&theFiles);
        }
        fclose(fd);
        if (haveCarrier) {  /* if no carrier, was an error during transmit */
            unlink(sdFile);
            netBuf.nbflags.send_files = FALSE;
        }
    }

}

/************************************************************************/
/*      netSendFile() send a file to another system via net             */
/************************************************************************/
int netSendFile(fn)
char *fn;
{
    extern char     *READ_ANY;
    struct cmd_data cmds;
    char            mess[140];
    struct stat     stat_buff;

    if (stat(fn, &stat_buff) == -1) {
        sPrintf(msgBuf.mbtext, "Couldn't open %s!", fn);
        netResult(msgBuf.mbtext);
        return;
    }

    splitF(netLog, "Network: Sending file \"%s\"\n", fn);
    zero_struct(cmds);
    cmds.command = SEND_FILE;

    strCpy(cmds.fields[0], fn);
    sPrintf(cmds.fields[1], "%ld",
                    (stat_buff.st_size + SECTSIZE - 1) / SECTSIZE);
    sPrintf(cmds.fields[2], "%ld", stat_buff.st_size);

    if (!sendNetCommand(&cmds, "send file")) {
        if (haveCarrier) {
            strCpy(mess, "%s reports: ");
            strCat(mess, RecBuf + 1);
            no_good(mess, FALSE);
        }
    }
    else {
        SendHostFile(fn);
        if (haveCarrier) {
            sPrintf(msgBuf.mbtext, "%s sent to %s.", fn, netBuf.netName);
            netResult(msgBuf.mbtext);
        }
    }
}

/************************************************************************/
/*      askFiles() ask for file(s) from caller                          */
/************************************************************************/
void askFiles()
{
    label    data2;
    SYS_FILE dataFl;
    char     mess[130];
    char     ambiguous;
    FILE     *temp;
    struct   cmd_data cmds;
    struct   fl_req file_data;
    extern char *READ_ANY, *WRITE_ANY;

    if (!gotCarrier()) {
        modStat = haveCarrier = FALSE;
        return ;
    }

    sPrintf(data2, "%d.rfl", thisNet);
    makeSysName(dataFl, data2, &cfg.netArea);
    temp = safeopen(dataFl, READ_ANY);
    if (temp == NULL) {
        no_good("Couldn't open room request file for %s", FALSE);
        netBuf.nbflags.room_files = FALSE;
    }
    else {
        while (fread(&file_data, sizeof (file_data), 1, temp) == 1 &&
                                 gotCarrier()) {
            if (netSetNewArea(&file_data.flArea)) {
                zero_struct(cmds);
                ambiguous = !(strchr(file_data.roomfile, '*') == NULL &&
                              strchr(file_data.roomfile, '?') == NULL);
                cmds.command = (!ambiguous) ? R_FILE_REQ : A_FILE_REQ;

                strCpy(cmds.fields[0], file_data.room);
                strCpy(cmds.fields[1], file_data.roomfile);
                splitF(netLog, "Network: Requesting \"%s\" in %s\n", file_data.roomfile,
                                                        file_data.room);
                if (!sendNetCommand(&cmds,
                 (!ambiguous) ? "single file request" :
                                "multiple file request")) {
                    strCpy(mess, "%s reports: ");
                    strCat(mess, RecBuf + 1);
                    no_good(mess, FALSE);
                }
                else {
                    if (ambiguous)
                        multiReceive(&file_data);
                    else {
                        ITL_Receive(file_data.filename, FALSE);
                        sPrintf(msgBuf.mbtext,
                        "File '%s' received from %s (stored in DIR %s).",
                        file_data.filename,
                        netBuf.netName,
                        prtNetArea(&file_data.flArea));

                        netResult(msgBuf.mbtext);
                    }
                }
            }
            homeSpace();
        }
        fclose(temp);
        if (gotCarrier()) {
            unlink(dataFl);
            netBuf.nbflags.room_files = FALSE;
        }
        else haveCarrier = modStat = FALSE;
    }
}

/************************************************************************/
/*      multiReceive() Receive multiple files                           */
/************************************************************************/
void multiReceive(file_data)
struct   fl_req *file_data;
{
    char        first = 1;
    extern char *WRITE_ANY;

    sPrintf(msgBuf.mbtext,
 "Files received from %s in response to request for %s from %s: ",
        netBuf.netName, file_data->roomfile, file_data->room);
    do {
        ITL_Receive(NULL, FALSE);
        if (!gotCarrier()) return;
        if (RecBuf[0] == 0) {          /* Last file name       */
            sPrintf(lbyte(msgBuf.mbtext), " (stored in DIR %s).",
                                         prtNetArea(&file_data->flArea));
            netResult(msgBuf.mbtext);
            return;
        }
        if (!first)
            strCat(msgBuf.mbtext, ", ");
        else
            first = FALSE;
        strCat(msgBuf.mbtext, RecBuf);
        ITL_Receive(RecBuf, FALSE);
    } while (1);
}

/************************************************************************/
/*      sendNetCommand() Sends a command to the receiver                */
/************************************************************************/
char sendNetCommand(cmds, error)
struct cmd_data *cmds;
char            *error;
{
    char errMsg[100];
    int  count;

    if (!ITL_Send(STARTUP)) {
        sPrintf(errMsg, "Link failure for %s (system: %%s).", error);
        no_good(errMsg, TRUE);
        killConnection();
        return FALSE;
    }
    sendITLchar(cmds->command);
    for (count = 0; count < 4; count++) {
        if (cmds->fields[count][0]) {
            mTrPrintf("%s", cmds->fields[count]);
        }
    }
    sendITLchar(0);
    ITL_Send(FINISH);
    if (cmds->command == HANGUP && !inReceive) return TRUE;
    ITL_Receive(NULL, FALSE);
    if (RecBuf[0] == BAD || !gotCarrier()) return FALSE;
    return TRUE;
}

/************************************************************************/
/*      sendHangUp() Send hangup command to receiver                    */
/************************************************************************/
void sendHangUp()
{
    struct cmd_data cmds;

    if (!gotCarrier()) {
        modStat = haveCarrier = FALSE;
        return ;
    }
    zero_struct(cmds);
    cmds.command = HANGUP;
    sendNetCommand(&cmds, "HANGUP");
}

/************************************************************************/
/*      no_good() does error messages                                   */
/************************************************************************/
void no_good(str, hup)
char *str;
char hup;
{
    sPrintf(msgBuf.mbtext, str, netBuf.netName);
    if (hup) {
        killConnection();
    }
	if (!suppressAideMsg)
		netResult(msgBuf.mbtext);
	suppressAideMsg = FALSE;
}

/************************************************************************/
/*      s_m_n() Send mail normal (non-route mail)                       */
/************************************************************************/
int s_m_n()
{
    FILE     *ptrs;
    label    fntemp;
    SYS_FILE fn;
    int      messCount = 0;
    struct   netMLstruct buf;
    extern char *READ_ANY;

    sPrintf(fntemp, "%d.ml", thisNet);
    makeSysName(fn, fntemp, &cfg.netArea);
    if ((ptrs = safeopen(fn, READ_ANY)) == NULL) {
        sPrintf(msgBuf.mbtext, "No mail to send to %s?", netBuf.netName);
        netResult(msgBuf.mbtext);
        return 0;
    }

    while (getMLNet(ptrs, buf) && TrError == TRAN_SUCCESS) {
        printMessage(buf.ML_loc, buf.ML_id, TRUE);
        messCount++;
    }
    fclose(ptrs);
    if (TrError == TRAN_SUCCESS) {
        unlink(fn);
        return messCount;
    }
    killConnection();
    splitF(netLog, "\nLink: Unsuccessful transfer of mail\n");
    return 0;
}

/************************************************************************/
/*      SendHostFile() Send a file to the other system                  */
/************************************************************************/
void SendHostFile(fn)
char *fn;
{
    int  data, success;
    FILE *fd;
    extern char *READ_ANY;

    success = ((fd = safeopen(fn, READ_ANY)) != NULL);
    if (ITL_Send(STARTUP)) {
        if (!success) mTrPrintf("Bad file.");
        else {
            while ((data = fgetc(fd)) != EOF && data != -1)
                if (!sendITLchar(data)) break;
        }
        ITL_Send(FINISH);
    }
    if (success) fclose(fd);
    else {
        sPrintf(msgBuf.mbtext, "Couldn't open %s for %s.",
                                                           fn, netBuf.netName);
        netResult(msgBuf.mbtext);
    }
}
