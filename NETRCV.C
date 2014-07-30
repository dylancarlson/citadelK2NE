/************************************************************************/
/*                              netrcv.c                                */
/*                                                                      */
/*      Networking functions for reception.                             */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
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
char netDebug = FALSE;
char            processMail;
char            PosId;
char			suppressAideMsg;
char            callerVersion[12], processedVersion[40];
int             gotVersion;
#ifdef NET_BUG
extern FILE *localcheck;
#endif

extern char             *SR_Sent, callLogPosting[800];
extern FILE             *netLog, *netMisc;
extern AN_UNSIGNED      RecBuf[];
extern int              counter, linkLayerCount;
extern int              callSlot;
extern char             checkNegMail;
extern char             inReceive;
extern label            normed, callerName, callerId;

char             normId(), getNetMessage();
char             callOut();
AN_UNSIGNED      inp();
char             *GetDynamic();

char *netRoomTemplate = "room%d.$$$";

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
extern char      WCError;
extern int       thisRoom;
extern int       thisLog;
extern int       fieldSuppress;  /* Net_Switch stuff             */
extern char      Net_Monitor;    /* FIDO Gateway stuff           */

/************************************************************************/
/*      called() We've been called, so let's handle it                  */
/************************************************************************/
void called()
{
    int  yr, hr, mins, dy;
    char *mn;
    extern long EndAnytime;
	extern char inNet;

    ITL_InitCall();             /* Initialize the ITL layer */

    setmem(SR_Sent, SHARED_ROOMS, 0);

    inReceive = TRUE;
	fieldSuppress = TRUE;
    getCdate(&yr, &mn, &dy, &hr, &mins);
    splitF(netLog, "Link: Caller detected (%d:%02d)\n", hr, mins);
    processMail = checkNegMail = FALSE;

    if (!called_stabilize()) return ;

	linkLayerCount++;
    splitF(netLog, "Session: Stabilized [Job N%d]\n", linkLayerCount);
	if (EndAnytime!=-1l)
	  inNet=ANYTIME_NET;

    getId();
    if (!haveCarrier) return;

    rcvStuff(FALSE);

    getCdate(&yr, &mn, &dy, &hr, &mins);
    splitF(netLog, "Session: Finished, disconnecting from %s (%d:%02d)\n",
                       callerName, hr, mins);
    sleep(2);
    killConnection();
    doResults();
    splitF(netLog, "Presentation: Intake completed.\n\n");
}

/************************************************************************/
/*      rcvStuff() receive stuff, like it says.                         */
/************************************************************************/
void rcvStuff(reversed)
char reversed;
{
    label           tempNm;
    struct cmd_data cmds;

    PosId = (callSlot == ERROR) ? FALSE : (netBuf.OurPwd[0] == 0);

    do {
        getNextCommand(&cmds);
        switch (cmds.command) {
            case HANGUP:                                        break;
            case NORMAL_MAIL:    getMail(&cmds);                break;
            case A_FILE_REQ:
            case R_FILE_REQ:     netFileReq(&cmds);             break;
            case NET_ROOM:       netRoomReq(&cmds);             break;
            case ROLE_REVERSAL:  reqReversal(&cmds, reversed);  break;
            case CHECK_MAIL:     reqCheckMail(&cmds);           break;
            case SEND_FILE:      reqSendFile(&cmds);            break;
            case NET_ROUTE_ROOM: netRRReq(&cmds);               break;
            case SYS_NET_PWD:    netPwd(&cmds);                 break;
/*          case ITL_PROTOCOL:   ITL_rec_optimize(&cmds);       break; */
            default:             sPrintf(tempNm, "'%d' unknown.", cmds.command);
                                 reply(BAD, tempNm);            break;
        }
    } while (gotCarrier() && cmds.command != HANGUP);
}

/************************************************************************/
/*      netPwd() process proposed net password                          */
/************************************************************************/
void netPwd(cmds)
struct cmd_data *cmds;
{
    if (callSlot != ERROR) {
        PosId = !strCmpU(cmds->fields[0], netBuf.OurPwd);
        if (!PosId) {
            splitF(netLog, "Link: Bad pwd: -%s-\n", cmds->fields[0]);
            sPrintf(msgBuf.mbtext, "%s sent bad password -%s-.",
                callerName, cmds->fields[0]);

            netResult(msgBuf.mbtext);
        }
    }
    reply(GOOD, "");
}


int flashNet;

/************************************************************************/
/*      doResults() processes results of receiving thingies and such.   */
/************************************************************************/
void doResults()
{
    flashNet=thisNet;
	putNet(thisNet);
    if (processMail)
	  readMail(TRUE, inMail);
    getNet(flashNet);
	thisNet=flashNet;  /* new */
    if (callSlot == ERROR)      /* If didn't know this node, don't      */
        return ;                /* bother with anything else            */
    if (checkNegMail)          readNegMail();
	getNet(flashNet);  /* new */
    doNetRooms();
    putNet(thisNet);
    UpdVirtStuff();             /* Just in case. */
	purgeFossilBuffs();			/* In case there's a modem mess. */
}

/************************************************************************/
/*      getId() Gets nodeId and nodeName from caller.                   */
/************************************************************************/
void getId()
{
    char *secRunner;
    extern int byteRate;
    SYS_FILE fn;
    FILE *upfd;
    extern char *APPEND_ANY;

    if (!haveCarrier) return;
    strCpy(RecBuf, "Test message.");
    ITL_Receive(NULL, FALSE);
    if (!gotCarrier()) {
        return ;
    }
    strncpy(callerId, RecBuf, NAMESIZE - 1);
    secRunner = RecBuf;
    while (*secRunner != 0) secRunner++;
    secRunner++;
    strncpy(callerName, secRunner, NAMESIZE - 1);
#ifdef NEW
    secRunner+=10;
	strncpy(callerVersion, secRunner, 12);
#endif
    normId(callerId, normed);
/*    procVersion(); */
    if (strLen(callerName) == 0 || strLen(callerId) == 0) {
        splitF(netLog, "Link: getId invalid data, disconnecting.\n\n");
        killConnection();
        makeSysName(fn, "getid.sys", &cfg.netArea);
        if ((upfd = safeopen(fn, APPEND_ANY)) != NULL) {
            fwrite(RecBuf, SECTSIZE, 1, upfd);
            fclose(upfd);
        }
    }
    if ((callSlot = searchNet(normed)) == ERROR) {
        sPrintf(msgBuf.mbtext, "New caller: %s (%s)", callerName, callerId);
        netResult(msgBuf.mbtext);
    }

    splitF(netLog, "Link: Call from %s (%s) @ %d\n", callerName, callerId,
                                                        byteRate * 10);
/*
    if (gotVersion==TRUE)
	   splitF(netLog, "Network: Remote running %s.\n", processedVersion);
*/
}

/************************************************************************/
/*      getNextCommand() Gets next command from caller                  */
/************************************************************************/
void getNextCommand(cmds)
struct cmd_data *cmds;
{
    zero_struct(*cmds);

    ITL_Receive(NULL, FALSE);
    if (!gotCarrier()) {
        return ;
    }
    grabCommand(cmds, RecBuf);
}

/************************************************************************/
/*      grabCommand() Pulls network cmds out of specified buffer        */
/************************************************************************/
void grabCommand(cmds, sect)
char *sect;
struct cmd_data *cmds;
{
    int fcount = 0;

    cmds->command = sect[0];
    if (cfg.BoolFlags.debug)
        splitF(netLog, "Cmd byte: %d\n", cmds->command);
    sect++;
    while (sect[0] > 0 && fcount < 4) {
        strncpy(cmds->fields[fcount], sect, NAMESIZE - 1);
        cmds->fields[fcount][NAMESIZE - 1] = 0;
        fcount++;
        while (*sect != 0) sect++;
        sect++;
    }
}

/************************************************************************/
/*      reply() Replies to caller                                       */
/************************************************************************/
void reply(state, reason)
char state;
char *reason;
{
/* splitF(netLog, "Network: Replying %d: %s\n", state, reason); */
	suppressAideMsg=FALSE;
    if (!ITL_Send(STARTUP)) {
		suppressAideMsg=TRUE;
		purgeFossilBuffs();
        no_good("Couldn't send reply to %s!", TRUE);
        return;
    }
	purgeFossilBuffs();
    sendITLchar(state);
    if (state == BAD) {
        mTrPrintf("%s", reason);
        if (cfg.BoolFlags.debug)
            splitF(netLog, "Replying BAD: %s\n", reason);
    }
	purgeFossilBuffs();
    sendITLchar(0);
    ITL_Send(FINISH);
}

/************************************************************************/
/*      reqReversal() reverses roles of networking                      */
/************************************************************************/
void reqReversal(cmds, reversed)
char reversed;
struct cmd_data *cmds;
{
    if (cfg.BoolFlags.debug) splitF(netLog, "Network: Role reversal\n");

    if (reversed) {
        reply(BAD, "Synch error on Reversal!");
        return ;
    }

    reply(GOOD, "");
    if (callSlot == ERROR)      /* Forces a "null" role reversal        */
        zero_struct(netBuf.nbflags);

    sendStuff(TRUE, PosId);
}

/************************************************************************/
/*      reqCheckMail() does negative acks on netMail                    */
/************************************************************************/
void reqCheckMail(cmds)
struct cmd_data *cmds;
{
    splitF(netLog, "Transport: Checking Mail\n");

    if (!processMail) {
        reply(BAD, "No mail to check!");
        return ;
    }

    reply(GOOD, "");

    if (ITL_Send(STARTUP)) {
        readMail(FALSE, targetCheck);
        sendITLchar(NO_ERROR);
        ITL_Send(FINISH);
    }
}

/************************************************************************/
/*      targetCheck() checks for existence of recipients                */
/************************************************************************/
void targetCheck()
{
    char      sigChar;
    int       logNo, j;
    logBuffer lBuf;
	char next[480];

    setmem(next, strlen(next), '\0');

    initLogBuf(&lBuf);
    if (!msgBuf.mbto[0]) {            /* Tsk, tsk!!! */
        sigChar = BAD_FORM;
    }
    else {
        logNo = findPerson(msgBuf.mbto, &lBuf);
		j=0;
		j=findNodeId(cfg.codeBuf+cfg.nodeId, next, msgBuf.mbmsgpath);
        if (logNo == ERROR && hash(msgBuf.mbto) != hash("Sysop")
			&& (!msgBuf.mbmsgpath[0] ||  (j==-1 || !next[0]) ) ) {
            sigChar = NO_RECIPIENT;
        }
        else {
            killLogBuf(&lBuf);
            return ;
        }
    }
    sendITLchar(sigChar);
    mTrPrintf(msgBuf.mbauth);
    mTrPrintf(msgBuf.mbto);
    mTrPrintf("%s @ %s", msgBuf.mbdate, msgBuf.mbtime);
    killLogBuf(&lBuf);
}


/************************************************************************/
/*      doNetRooms() integrates temporary files into data base          */
/************************************************************************/
void doNetRooms()
{
    label temp2;
    SYS_FILE tempNm;
    char  *addr;
    int   rover;
    extern char *READ_ANY, *LOC_NET, *NON_LOC_NET;

	Net_Monitor=FALSE;
    for (rover = 0; rover < SHARED_ROOMS; rover++) {
        if (chkNeedsProcessing(rover)) {
            sPrintf(temp2, netRoomTemplate, netRoomSlot(rover));
            makeSysName(tempNm, temp2, &cfg.netArea);
            if ((netMisc = safeopen(tempNm, READ_ANY)) == NULL) {
                sPrintf(msgBuf.mbtext,
                             "Couldn't open temproom file (%s) from %s!",
                                   tempNm, netBuf.netName);
                return;
            }
            getRoom(netRoomSlot(rover));
            if (roomBuf.rbShareType == BACKBONE) {
                if (netBuf.netRooms[rover].mode == PEON)
                    addr = LOC_NET;
                else    /* Hosts and other Backbones generate nonlocalnet */
                    addr = NON_LOC_NET;
            }
            else if (roomBuf.rbShareType == REG_HOST) {
                if (netBuf.netRooms[rover].mode == PEON)
                    addr = LOC_NET;
                else    /* Must be a backbone, hosts don't talk here */
                    addr = NON_LOC_NET;
            }
            else addr = "";
            while (getNetMessage(TRUE)) {
                strCpy(msgBuf.mbaddr, addr);
                if (strCmpU(cfg.nodeId + cfg.codeBuf, msgBuf.mborig)
                                                            != SAMESTRING) {
        			if (roomBuf.rbflags.IS_GATEWAY==TRUE) Net_Monitor=TRUE;
													else Net_Monitor=FALSE;
                    putMessage();
					Net_Monitor=FALSE;
                    noteMessage(NULL, ERROR);
                	}
            	}
            if (strLen(addr) != 0) {
                netBuf.netRooms[rover].lastMess = cfg.newest;
            }
            fclose(netMisc);
            unlink(tempNm);
            resetNeedsProcessing(rover);
        }
    }
}

/************************************************************************/
/*      getMail() Grabs mail from caller                                */
/************************************************************************/
void getMail(cmds)
struct cmd_data *cmds;
{
    SYS_FILE tempNm;

    splitF(netLog, "Network: Receiving mail from %s\n", callerName);
    makeSysName(tempNm, "tempmail.$$$", &cfg.netArea);
    if (ITL_Receive(tempNm, TRUE) == ITL_SUCCESS) {
        processMail = TRUE;
    }
}

/************************************************************************/
/*      reqSendFile() Receive a sent file                               */
/************************************************************************/
void reqSendFile(cmds)
struct cmd_data *cmds;
{
    label fn;
    long  proposed;
    int count;
    extern char *READ_ANY, *WRITE_ANY;

    if (netSetNewArea(&cfg.receptArea)) {
        proposed = atol(cmds->fields[2]);
        if (sysRoomLeft() < proposed) {
            reply(BAD, "No room for file.");
            homeSpace();
            return;
        }
        reply(GOOD, NULL);
        count = 0;
        strCpy(fn, cmds->fields[0]);
        while (access(fn, 0) != -1) {
            sPrintf(fn, "a.%d", count++);
        }
        splitF(netLog, "Network: Receiving \"%s\"\n", cmds->fields[0]);
        ITL_Receive(fn, FALSE);
        homeSpace();
        if (haveCarrier) {
            if (strCmp(fn, cmds->fields[0]) == SAMESTRING)
                sPrintf(msgBuf.mbtext, "%s received from %s.", fn, callerName);
            else
                sPrintf(msgBuf.mbtext, "%s (saved as %s) received from %s.",
                          cmds->fields[0], fn, callerName);
    	    netResult(msgBuf.mbtext);
	        }
    }
    else {
        reply(BAD, "System error");
    }
}

/************************************************************************/
/*      netFileReq() Handles request for file transfer                  */
/************************************************************************/
void netFileReq(cmds)
struct cmd_data *cmds;
{
    int  roomSlot;
    char reason[50];
    extern char *READ_ANY;

    splitF(netLog, "Network: File request: \"%s\" in %s\n", cmds->fields[1],
                                            cmds->fields[0]);

    if ((roomSlot = roomExists(cmds->fields[0])) == ERROR   ||
                    !roomTab[roomSlot].rtflags.ISDIR  ||
                    roomTab[roomSlot].rtflags.NO_NET_DOWNLOAD ||
                    !roomTab[roomSlot].rtflags.DOWNLOAD) {
        sPrintf(reason, "Room %s does not exist.", cmds->fields[0]);
        reply(BAD, reason);
        return;
    }

    getRoom(roomSlot);

    if (cmds->command == A_FILE_REQ) {
        reply(GOOD, "");
        sPrintf(msgBuf.mbtext, "Files sent to %s from %s: ",
                         callerName, roomBuf.rbname);
        wildCard(netMultiSend, cmds->fields[1], TRUE, "");
        if (ITL_Send(STARTUP)) {
            mTrPrintf("");
            ITL_Send(FINISH);
        }
    }
    else {
        if (!setSpace(&roomBuf)) {
            sPrintf(reason, "Couldn't open DIR for %s.", cmds->fields[0]);
            reply(BAD, reason);
            return;
        }

        if (access(cmds->fields[1], 4) == -1) {
            sPrintf(reason, "No '%s' in %s.", cmds->fields[1],
                                                       cmds->fields[0]);
            reply(BAD, reason);
            homeSpace();
            return;
        }

        reply(GOOD, "");

        SendHostFile(cmds->fields[1]);
        sPrintf(msgBuf.mbtext,
            "%s downloaded from %s during networking by %s.",
            cmds->fields[1], formRoom(thisRoom, FALSE, FALSE), callerName);
        homeSpace();
    }
    netResult(msgBuf.mbtext);
}

/************************************************************************/
/*      netRRReq() Handle request for room sharing routing              */
/************************************************************************/
void netRRReq(cmds)
struct cmd_data *cmds;
{
    char reason[80];
    int arraySlot, VirtSlot, RoomSlot;
    extern char *R_SH_MARK, *NON_LOC_NET, *LOC_NET;

    if ((RoomSlot = roomRoutable(cmds->fields[0], reason, &arraySlot)) !=
                                                ERROR) {
        getRoom(netRoomSlot(arraySlot));

        reply(GOOD, "");

        recNetMessages(arraySlot, cmds->fields[0], RoomSlot);

        findAndSend(ERROR, R_SH_MARK, LOC_NET,
             roomBuf.rbShareType == BACKBONE ? NON_LOC_NET : NULL, arraySlot,
                                RoomSend, roomBuf.rbname, RoomReceive);
    }
    else if ((VirtSlot = VirtualShared(thisNet, cmds->fields[0])) != ERROR) {
        reply(GOOD, "");
        if (!RecVirtualRoom(VirtSlot)) {
            sPrintf(reason, "RecVirtualRoom failure: %s.", cmds->fields[0]);
            no_good(reason, FALSE);
        }
        findAndSend(ERROR, NULL, NULL, NULL, VirtSlot, SendVirtual,
                                      cmds->fields[0], RecVirtualRoom);
    }
    else {
        reply(BAD, reason);
        return ;
    }
}

/************************************************************************/
/*      netRoomReq() Handles request for room networking                */
/************************************************************************/
void netRoomReq(cmds)
struct cmd_data *cmds;
{
    char reason[80];
    int  arraySlot, VirtSlot, RoomSlot;

    if ((RoomSlot = roomRoutable(cmds->fields[0], reason, &arraySlot))
                                                        != ERROR) {
        reply(GOOD, "");
        recNetMessages(arraySlot, cmds->fields[0], RoomSlot);
    }
    else if ((VirtSlot = VirtualShared(thisNet, cmds->fields[0])) != ERROR) {
        reply(GOOD, "");
        if (!RecVirtualRoom(VirtSlot)) {
            sPrintf(reason, "RecVirtualRoom failure: %s.", cmds->fields[0]);
            no_good(reason, FALSE);
        }
    }
    else {
        reply(BAD, reason);
    }
}

void recNetMessages(arraySlot, name, slot)
char *name;
int arraySlot, slot;
{
    SYS_FILE fileNm, temp;
    char reason[60];

/*  splitF(netLog, "Network: Receiving %s\n", name);  */
    sPrintf(temp, netRoomTemplate, slot);
    makeSysName(fileNm, temp, &cfg.netArea);
    switch (ITL_Receive(fileNm, FALSE)) {
    case ITL_SUCCESS:
/*	    splitF(netLog, "Network: Received and/or checked \"%s.\"\n", name);    K2NE */
		splitF(netLog, "Network: Checked [%s]\n", name);
        setNeedsProcessing(arraySlot);          break;
    case ITL_NO_OPEN:
        sPrintf(reason, "File error for %s", name);
        reply(BAD, reason);
        break;
    case ITL_BAD_TRANS:
        break;
    }
	purgeFossilBuffs();
}

int roomRoutable(name, reason, arraySlot)
char *name, *reason;
int  *arraySlot;
{
    int slot;

    if (!PosId) {
        sPrintf(reason, "No can do for '%s'", name);
        return ERROR;
    }

    if ((slot = roomExists(name)) == ERROR) {
        sPrintf(reason, "'%s' doesn't exist", name);
        return ERROR;
    }

    if (roomTab[slot].rtflags.SHARED == 0) {
        sPrintf(reason, "'%s' is not a net room", name);
        return ERROR;
    }

    if (callSlot == ERROR || ((*arraySlot) = chkSharing(slot)) == ERROR) {
        sPrintf(reason, "'%s' is not netting with you", name);
        return ERROR;
    }

    return slot;
}

/************************************************************************/
/*      chkSharing() See if given room slot is networking with system   */
/*                   that has just called                               */
/************************************************************************/
int chkSharing(i)
int i;
{
    int rover;

    for (rover = 0; rover < SHARED_ROOMS; rover++) {
        if (netBuf.netRooms[rover].srgen & 0x8000 &&
                             netRoomSlot(rover) == i &&
                             netGen(rover) == roomTab[i].rtgen)
            return rover;
    }
    return ERROR;
}

/************************************************************************/
/*      netMultiSend() Send files via WC                                */
/************************************************************************/
int netMultiSend(fn)
char *fn;
{
    struct stat stat_buff;
    long Sectors;

    if (!gotCarrier()) return ;
    if (stat(fn, &stat_buff) == -1) {
        sPrintf(msgBuf.mbtext, "Error: Requested file (%s) not found",
                                        fn);
        netResult(msgBuf.mbtext);
        return;
    }

    strCat(msgBuf.mbtext, fn);
    strCat(msgBuf.mbtext, " ");
    Sectors     = ((stat_buff.st_size + 127) / SECTSIZE);
    if (ITL_Send(STARTUP)) {
        mTrPrintf("%s", fn);
        mTrPrintf("%ld", Sectors);
        ITL_Send(FINISH);
    }
    SendHostFile(fn);
}

#ifdef NEW
procVersion()
{
 char preTag[4], postTag[6];

 gotVersion=FALSE;

 setmem(preTag, 4, 0);
 setmem(postTag, 7, 0);
 setmem(processedVersion, 40, 0);

 strncpy(preTag, callerVersion, 3);
 strncpy(postTag, callerVersion+3, 5);
 if (strCmpU(preTag, "vaq")==SAMESTRING) {
	gotVersion=TRUE;
    sprintf(processedVersion, "Citadel:K2NE Version %s", postTag);
	}
 return;
}
#endif