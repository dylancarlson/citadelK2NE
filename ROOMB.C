/************************************************************************/
/*                              roomb.c                                 */
/*              room code for Citadel bulletin board system             */
/************************************************************************/

/************************************************************************/
/*                              History                                 */
/*                                                                      */
/* 87Mar30 HAW  Fix for carrier loss in renameRoom, Invite only room    */
/*              bug, add <L>ogin to novice menu for unlogged            */
/* 86Aug16 HAW  Kill history in here due to space problems.             */
/* 85Jan16 JLS  Fix getText so console starting CR creates blank msg.   */
/* 84Jun28 JLS  Enhancement: Creator of a room is listed in Aide>.      */
/* 84Apr04 HAW  Start upgrade to BDS 1.50a.                             */
/* 83Feb26 CrT  bug in makeRoom when out of rooms fixed.                */
/* 83Feb26 CrT  matchString made caseless, normalizeString()            */
/* 83Feb26 CrT  "]" directory prompt, user name before prompts          */
/* 82Dec06 CrT  2.00 release.                                           */
/* 82Nov02 CrT  Cleanup prior to V1.2 mods.                             */
/* 82Nov01 CrT  Proofread for CUG distribution.                         */
/* 82Mar27 dvm  conversion to v. 1.4 begun                              */
/* 82Mar25 dvm  conversion for TRS-80/Omikron test started              */
/* 81Dec21 CrT  Log file...                                             */
/* 81Dec20 CrT  Messages...                                             */
/* 81Dec19 CrT  Rooms seem to be working...                             */
/* 81Dec12 CrT  Started.                                                */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      coreGetYesNo()          core for conGetYesNo() and getYesNo()   */
/*      conGetYesNo()           prompts for a yes/no response from CON  */
/*      editText()              handles the end-of-message-entry menu   */
/*      findRoom()              find a free room                        */
/*      getNumber()             prompt user for a number, limited range */
/*      getString()             read a string in from user              */
/*      getText()               reads a message in from user            */
/*      getYesNo()              prompts for a yes/no response           */
/*      givePrompt()            gives usual "THISROOM>" prompt          */
/*      indexRooms()            build RAM index to ctdlroom.sys         */
/*      initialArchive()        does initial archive of a room          */
/*      insertParagraph()       inserts paragraph into message          */
/*      makeRoom()              make new room via user dialogue         */
/*      matchString()           search for given string                 */
/*      noteRoom()              enter room into RAM index               */
/*      renameRoom()            sysop special to rename rooms           */
/*      replaceString()         string-substitute for message entry     */
/*      searchForRoom()         auxilary to addToList()                 */
/*                                                                      */
/*      # -- operating system dependent function.                       */
/************************************************************************/

#define WARN_MSG \
"This forces you to re-enter all systems sharing; are you sure?"

/************************************************************************/
/*                  External variable declarations in ROOMB.C           */
/************************************************************************/
char *public_str  = " Public";
char *private_str = " Private";
char *perm_str    = "Permanent";
char *temp_str    = "Temporary";
char exChar       = '?';
char *on          = "on";
char *off         = "off";
char *no          = "no";
char *yes         = "yes";
char ShType;
int k2neLocalOnlyFlag;

/************************************************************************/
/*                  External variable definitions for ROOMB.C           */
/************************************************************************/
extern struct floor     *FloorTab;
extern aRoom     roomBuf;        /* Room buffer                  */
extern rTable    *roomTab;       /* RAM index                    */
extern FILE      *roomfl;        /* Room file descriptor         */
extern CONFIG    cfg;            /* Other variables              */
extern paintBrush colTable;      /* the ANSI rainbow             */
extern MSG_BUF   msgBuf;         /* Message buffer               */
extern MSG_BUF   tempMess;  /* For held messages            */
extern logBuffer logBuf;         /* Person buffer                */
extern NetBuffer netBuf;
extern NetTable  *netTab;
extern int              masterCount;
extern int              thisRoom;       /* Current room                 */
extern int              thisLog;
extern char             remoteSysop;
extern char             outFlag;        /* Output flag                  */
extern char             loggedIn;       /* Logged in?                   */
extern char             haveCarrier;    /* Have carrier?                */
extern char             onConsole;      /* How about on Console?        */
extern char             whichIO;        /* Where is I/O?                */
extern char             *baseRoom;
extern char             heldMess;
extern char             echo;
extern char             echoChar;
extern char             *confirm;
extern char				justLostCarrier, justOut;
extern char				sleepFlag, Profile, FileDescribe, RoomDescribe;
extern char				specialPrompt, shortStuff, fastOut, jumpOut;
extern char             alterNet, alterLinkName[NAMESIZE];
extern char             manualNet, autoNet, infoBannerCut;
extern int 				dumpDeadWood;
extern int				userMessages; /* running count of messages saved */
extern int              linkerNumber; /* count of external-linked msgs   */

/************************************************************************/
/*      conGetYesNo() prompts for a yes/no response from CONSOLE only   */
/************************************************************************/
char conGetYesNo(prompt)
char *prompt;
{
    return coreGetYesNo(prompt, TRUE);
}

/************************************************************************/
/*      coreGetYesNo() prompts for a yes/no response given fn for echo  */
/************************************************************************/
char coreGetYesNo(prompt, consoleOnly)
char *prompt;
int  consoleOnly;
{
    int  toReturn, lidCatcher=0;
    char (*inputFn)(void);
    int  (*outputFn)(char *format, ...);
#define write_CR()      (!consoleOnly) ? doCR() : printf("\n")

    inputFn  = (consoleOnly) ? getCh  : iChar;
    outputFn = (consoleOnly) ? printf : mPrintf;

    for (write_CR(), toReturn = ERROR; toReturn == ERROR && onLine(); ) {
        outFlag = IMPERVIOUS;
        (*outputFn)("%s? (Y/N): ", prompt);

        switch (toUpper((*inputFn)())) {
	        case 'Y': toReturn      = TRUE ;  break;
    	    case 'N': toReturn      = FALSE;  break;
			default:  lidCatcher++;           break;
        	}
        write_CR();

		if (lidCatcher > 15) { /* fixes nasty nasty bug! */
			outFlag = OUTOK;
			toReturn = FALSE;
			dumpDeadWood = FALSE; /* keep all our kludges happy! */
            }
	    }
    outFlag = OUTOK;
    return   toReturn;
}

/************************************************************************/
/*      editText() handles the end-of-message-entry menu.               */
/*      return TRUE  to save message to disk,                           */
/*             FALSE to abort message, and                              */
/*             ERROR if user decides to continue                        */
/************************************************************************/
int editText(buf, lim)
char *buf;
int lim;
{
#ifndef NOTONLYTHENET
	int findStart, startPoint, indexStart, lidCatcher=0, editLimit;
	char *dummy, foundSpace;
	extern char callLogPosting[800];
    extern char *ALL_LOCALS, *WRITE_LOCALS;

	editLimit=(FileDescribe) ? 500 :
			    RoomDescribe ? 500 :MAXTEXT;
    do {
        outFlag = IMPERVIOUS;
        mPrintf("\n [%d used; %d remain]", strLen(buf), editLimit-strLen(buf));
	    mPrintf("\n [%s]%s", /* tell what we are doing here! */
			RoomDescribe==TRUE ? "Room Information" :
            FileDescribe==TRUE ? "File Description" :
			Profile==TRUE ? "User Information" : roomTab[thisRoom].rtname,
            expert ? " Entry cmd: " : " Edit command: ");
        switch (toUpper(iChar())) {
        case 'A':
            mPrintf("bort\n ");
            if (getYesNo(confirm)) {
                return FALSE;
            }
            break;
        case 'C':
            mPrintf("ontinue");
            doCR();

/* K2NE 88Oct09 */
            findStart = strLen(buf) - 20;
			if (findStart < 1) startPoint = 0;
				else startPoint = findStart;
            indexStart = startPoint;
            /* Now let's find the first SPACE char in the last 20 or so */
			foundSpace = FALSE;
			while (!foundSpace) {
                if (buf[indexStart] != ' ') indexStart++;
				if ((indexStart == (strLen(buf) - 1) ) && (foundSpace == FALSE) )
					break;
				if (buf[indexStart] == ' ') {
					foundSpace = TRUE;
					startPoint = indexStart+1;
					}
				}
        	mPrintf("\n ...");
            dummy = &buf[startPoint];
            buf[strLen(buf)-1] = '\0';
			mPrintf("%s", dummy);
/* K2NE 88Oct09 */

            return ERROR;
        case 'I':
			if (FileDescribe==TRUE || RoomDescribe==TRUE) {
				mPrintf("\n?");
				break;
				}
            mPrintf("nsert paragraph break\n ");
            insertParagraph(buf, lim);
            break;
        case 'P':
			mPrintf("\bPrint msg\n \n");
            outFlag = OUTOK;
            shrtColor(colTable.level1 /* A_RED */);
            if (RoomDescribe==FALSE && Profile==FALSE && FileDescribe==FALSE) mPrintf(formHeader());
			shrtColor(colTable.level0 /* A_GREEN */);
            if (RoomDescribe==FALSE && Profile==FALSE) doCR();
            mFormat(buf);
            break;

        case 'R':
            mPrintf("eplace string\n ");
            replaceString(buf, lim);
            break;
        case 'S':
            mPrintf("ave msg.\n ");

            if (  FileDescribe==FALSE && RoomDescribe==FALSE &&
				(roomBuf.rbflags.SHARED) && /* K2NE FIDO FLAG */
                cfg.BoolFlags.netParticipant &&
                loggedIn &&
                (strLen(msgBuf.mbaddr) == 0) &&
                logBuf.lbflags.NET_PRIVS) {
                	if (getYesNo("Save as net msg"))
                    	if (!netInfo()) break;
	            	}
			if ( thisRoom == MAILROOM && !loggedIn ) {
				strCpy(callLogPosting, "Mail to Sysop (unlogged)");
				logMessage(19,"",FALSE);
				}
			userMessages++;
            return TRUE;
        case 'H':
			if (FileDescribe==TRUE || RoomDescribe==TRUE) {
				mPrintf("\n?");
				break;
				}
            mPrintf("old for later\n ");
            if (heldMess) {
                mPrintf("Msg already being held!\n ");
                break;
            }
            mPrintf("Msg held\n ");
            movmem(&msgBuf, &tempMess, sizeof msgBuf);
            heldMess = TRUE;
            return FALSE;
        case '?':
            tutorial("edit.mnu", TRUE);
            if ((roomBuf.rbflags.SHARED || thisRoom == MAILROOM) &&
                cfg.BoolFlags.netParticipant &&
                loggedIn &&
                logBuf.lbflags.NET_PRIVS)
                mPrintf(" <N>etwork/save this msg\n ");
            break;
        case 'N':
			if (FileDescribe==TRUE) {
				mPrintf("\n?");
				break;
				}
            if ((roomBuf.rbflags.SHARED || thisRoom == MAILROOM) &&
                cfg.BoolFlags.netParticipant &&
                loggedIn &&
                logBuf.lbflags.NET_PRIVS) {
                mPrintf("\bSave as networked msg\n ");
                if (!netInfo()) break;
                return TRUE;
            }
		case 'L':
			if (FileDescribe==TRUE) {
				mPrintf("\n?");
				break;
				}
			if (thisRoom == MAILROOM
				||
				!roomBuf.rbflags.SHARED
				||
				!loggedIn
				||
				!cfg.BoolFlags.netParticipant
  				||
                !roomBuf.rbflags.ALTER_NET
				||
				!logBuf.lbflags.NET_PRIVS) {
				mPrintf (" ? Not here!\n ");
				break;
				}
			mPrintf("\bLocal-only save msg.\n ");
			clearNetFields();
			userMessages++;
			return TRUE;
        default:
			lidCatcher++;
            if (!expert) {
                tutorial("edit.mnu", TRUE);
                if ((roomBuf.rbflags.SHARED || thisRoom == MAILROOM) &&
                    cfg.BoolFlags.netParticipant &&
                    loggedIn &&
					FileDescribe==FALSE && Profile==FALSE && RoomDescribe==FALSE
							 &&
                    logBuf.lbflags.NET_PRIVS)
                    mPrintf(" <N>etwork/save this msg\n ");
            }
            else mPrintf(" ? (Type '?' for menu)\n \n");
            break;
        }
    } while (onLine() && lidCatcher < 15);

    if (loggedIn)
        SaveInterrupted(&msgBuf);

    return FALSE;
#endif
}

/************************************************************************/
/*      findRoom() returns # of free room if possible, else ERROR       */
/************************************************************************/
int findRoom()
{
#ifndef NOTONLYTHENET
    int roomRover;

    for (roomRover = 0;  roomRover < MAXROOMS;  roomRover++) {
        if (roomTab[roomRover].rtflags.INUSE == 0) return roomRover;
    }
    return ERROR;
#endif
}

/************************************************************************/
/*      getNumber() prompts for a number in (bottom, top) range.        */
/************************************************************************/
long getNumber(prompt, bottom, top)
char  *prompt;
long bottom;
long top;
{
#ifndef NOTONLYTHENET
    long try;
    char numstring[NAMESIZE];


    do {
        getString(prompt, numstring, NAMESIZE, FALSE, ECHO);
        try     = atol(numstring);

		if (dumpDeadWood == TRUE) {
			dumpDeadWood = FALSE;
			try = 79l;
			}
		if (shortStuff==TRUE && strlen(numstring)==0) {
			fastOut=TRUE;
			shortStuff=FALSE;
			break;
			}
        if (try < bottom)  mPrintf("Must be at least %ld\n", bottom);
        if (try > top   )  mPrintf("Can't exceed %ld\n", top);
    } while ((try < bottom ||  try > top) && onLine());
    shortStuff=FALSE;
    if (fastOut==TRUE) {
		fastOut=FALSE;
		jumpOut=TRUE;
		return 0l;
		}
    else return  (long) try;
#endif
}

/************************************************************************/
/*      getString() gets a string from the user.                        */
/************************************************************************/
void getString(prompt, buf, lim, QuestIsSpecial, doEcho)
char *prompt;
char *buf;
char doEcho;
int  lim;       /* max # chars to read */
char QuestIsSpecial;    /* Return immediately on '?' input?             */
{
#ifndef NOTONLYTHENET
    extern int crtColumn;
    char c, oldEcho;
    int  i;

    outFlag = IMPERVIOUS;

    if (strLen(prompt) > 0) {
        if (shortStuff==FALSE) doCR();
/*      mPrintf("Enter %s\n : ", prompt); */
		mPrintf("%s%s%s",
			specialPrompt ? "" : "Enter ",
			prompt,
			specialPrompt ? ": " : "\n : ");
    }

    oldEcho = echo;
    if (!doEcho) {
        echo     = NEITHER;
        echoChar = specialPrompt ? '.': 'X';
    }

    i   = 0;
    outFlag = OUTOK;
    while (
         c = iChar(),
         c        != NEWLINE
         && i     <  lim
         && onLine()
    ) {

        /* handle delete chars: */
        if (c == BACKSPACE) {
            oChar(' ');
            oChar(BACKSPACE);
            if (i > 0) i--;
            else  {
                oChar(' ');
                oChar(BELL);
            }
        } else   buf[i++] = c;

        if (i >= lim) {
            oChar(BELL);
            oChar(BACKSPACE); i--;
        }

		if (justLostCarrier) {
            runHangup();
			break;
			}

        /* kludge to return immediately on single '?': */
        if (QuestIsSpecial && *buf == exChar)   {
            doCR();
            break;
        }
    }
    crtColumn = 1;
    echo    = oldEcho;
    buf[i]  = '\0';
#endif
}

/************************************************************************/
/*      getText() reads a message from the user                         */
/*      Returns TRUE if user decides to save it, else FALSE             */
/************************************************************************/
char getText(uploading)
char uploading;
{
#ifndef NOTONLYTHENET
    extern PROTO_TABLE Table[];
    extern char *R_SH_MARK;
/*	extern int   askForCRC; */

    char c, beeped = FALSE;
    int  i, toReturn, lim, msg_c;
	FILE *in_msg;

    msgBuf.mbtext[-1] = NEWLINE;

    if (uploading == ASCII) {
		shrtColor(colTable.level1 /* A_RED */);
        if (!expert && alterNet==FALSE) {
            tutorial("entry.blb", TRUE);
            mPrintf("Enter message (end with empty line)");
        }
        outFlag = OUTOK;
        if (Profile==FALSE && RoomDescribe==FALSE) doCR();
        if (alterNet==TRUE) {
			echo = BOTH;
        	printf("Importing: #%d from %s.",
						linkerNumber,
						alterLinkName);
			echo = NEITHER;
            }
        if (Profile==FALSE && FileDescribe==FALSE && RoomDescribe==FALSE)
									 mPrintf(formHeader());
        shrtColor(colTable.level0 /* A_GREEN */);
        if (RoomDescribe==FALSE && Profile==FALSE && FileDescribe==FALSE) doCR();

        if (msgBuf.mbtext[0]) {
            outFlag = OUTOK;
            mFormat(msgBuf.mbtext);
            outFlag = OUTOK;
            doCR();
        }
        outFlag = OUTOK;
        if (FileDescribe==TRUE) lim=499;
		else    lim=MAXTEXT-1;
    }
    else {
        if (!expert)
            tutorial(Table[uploading].UpBlbName, TRUE);
        if (!getYesNo("Ready for transfer"))
            return FALSE;
        masterCount = 0;

/****** The HOOK is in the lines which follow, bounded by the next pair
of long comment-lines.  We need functions to:
         1. jump to our doTempShell with a dummy filename to receive
			the incoming message stream.
		 2. "Size" the tentative text to make sure it does not exceed
			MAXTEXT.
		 3. Read the file into the message-edit-buffer.  The function to
			do this already exists in the code - it is "merely" a matter
			of calling it with the appropriate bounds to make sure that
			the resulting edit buffer is within legal size.
		 4. Jump to the editText() function to wrap it up!
****/

/*************<This segment will be replaced by "our stuff.">**************/
#ifdef K2NE_MSG_IN  /* this segment is disabled under normal compiles */
         if (Reception(uploading, putBufChar) != TRAN_SUCCESS)
            return FALSE;
#endif

/***************************************************************************/
/* First we do the actual incoming message upload, shelling to doTempShell */
/* for the actual work involved.                                           */
/***************************************************************************/
#ifndef K2NE_MSG_IN /* this segment is active under normal compiles */

		homeSpace(); /* let's make sure we know where we are! */
		unlink("msg_in.tmp"); /* A fresh start, just like Chapter 11! */

		doTempShell(TRUE, uploading, "msg_in.tmp", FALSE);

		in_msg=fopen("msg_in.tmp", "r");
        if (in_msg == NULL) {
            fclose(in_msg);
			unlink("msg_in.tmp");
			return FALSE;
            }

            /* If 'in_msg' returns nonNULL then we have a file.
               Presumably this file contains the user's message.
               Now, we are going to pull it into the buffer, stripping
			   the high bit as we go.  All of this nastiness SHOULD
			   be handled virtually automatically by putBufChar(). */

		rewind(in_msg);
		do {
			msg_c = getc(in_msg);
			} while ((putBufChar(msg_c) != ERROR)
						&&
					 (msg_c != CPMEOF)
						&&
					 (msg_c != EOF) );

        fclose(in_msg);
		unlink("msg_in.tmp");

/*		return FALSE;   and that SHOULD do it! */
#endif
/**************************************************************************/

    }

	if (alterNet==TRUE) return TRUE;

    do {
        i = strLen(msgBuf.mbtext);
        if (uploading == ASCII)
            while (
                !(
                    (c=iChar()) == NEWLINE   &&
                    (msgBuf.mbtext[i-1] == NEWLINE || i == 0 || FileDescribe)
                )
                && i < lim
                && onLine()
                && alterNet==FALSE
            ) {
				if (dumpDeadWood == TRUE) {
					dumpDeadWood = FALSE;
					return FALSE; /* kludge alert! */
					}
				if (sleepFlag == TRUE) {
                    sleepFlag = FALSE;
					return FALSE; /* kludge alert! */
					}
                if (c != BACKSPACE) {
                    if (c != 0) msgBuf.mbtext[i++]   = c;
                    if (i > MAXTEXT - 80 && !beeped) {
                        beeped = TRUE;
                        oChar(BELL);
                    }
                }
                else {
                     /* handle delete chars: */
                    oChar(' ');
                    oChar(BACKSPACE);
                    if (i > 0 && msgBuf.mbtext[i-1] != NEWLINE)   i--;
                    else                                oChar(BELL);
                }

                msgBuf.mbtext[i] = 0;   /* null to terminate message     */

                if (i == lim)   mPrintf(" buffer overflow\n ");
            }
        if (FileDescribe) {
			msgBuf.mbtext[i++]=' ';
			msgBuf.mbtext[i]=0;
			}
        if (alterNet==TRUE) toReturn = TRUE;
        else toReturn = editText(msgBuf.mbtext, lim);


        uploading = ASCII;
    } while ((toReturn == ERROR)  &&  onLine());
    if (toReturn == TRUE) {             /* Filter null messages         */
        toReturn = FALSE;
        for (i = 0; msgBuf.mbtext[i] != 0 && !toReturn; i++)
            toReturn = (msgBuf.mbtext[i] > ' ' && msgBuf.mbtext[i] < 127);
    }
    return  toReturn;
#endif
}

/************************************************************************/
/*      getYesNo() prompts for a yes/no response                        */
/************************************************************************/
char getYesNo(prompt)
char *prompt;
{
	dimColor(colTable.level0 /* A_GREEN */);
    return coreGetYesNo(prompt, FALSE);
}

/************************************************************************/
/*      givePrompt() prints the usual "CURRENTROOM>" prompt.            */
/************************************************************************/
void givePrompt()
{
 int thex, they;

    if (justOut==TRUE || autoNet==TRUE || manualNet==TRUE) {
		justOut=FALSE;
        manualNet=FALSE;
		ScreenUser();

	    infoBannerCut=TRUE;
		doInfobanner();

		return;
		}

    outFlag = IMPERVIOUS;
    doCR();
    ScreenUser();
    if (!loggedIn)
        mPrintf("<L>ogin ");
    if (!expert) {
        mPrintf("<G>oto%s%s <H>elp",
     (thisRoom == MAILROOM || loggedIn ||
                            cfg.BoolFlags.unlogReadOk)  ? " <N>ew"   : "",
     (thisRoom == MAILROOM || loggedIn ||
                            cfg.BoolFlags.unlogEnterOk) ? " <E>nter" : "");
        doCR();
    }
	shrtColor(thisRoom == MAILROOM ? colTable.level1 /* A_RED */ :
			  thisRoom == LOBBY ? colTable.level2 /* A_BLUE */ :
			  thisRoom == AIDEROOM ? colTable.level1 /* A_RED */ :
			  colTable.level2 /* A_BLUE */);


/* CHECKPOINT */
    if (FloorMode || !expert) mPrintf("[%s] ", FloorTab[thisFloor].FlName);
    mPrintf("%s ", formRoom(thisRoom, FALSE, TRUE));
    shrtColor(colTable.level0 /* A_GREEN */);

    if (strCmp(roomBuf.rbname, roomTab[thisRoom].rtname) != SAMESTRING) {
/*        printf("thisRoom=%d, rbname=-%s-, rtname=-%s-\n", thisRoom,
                roomBuf.rbname, roomTab[thisRoom].rtname); */
        crashout("Dependent vars mismatch!");
    }
    outFlag = OUTOK;
}

/************************************************************************/
/*      indexRooms() -- build RAM index to CTDLROOM.SYS, by CITADEL, to */
/*      delete empty rooms.                                             */
/************************************************************************/
void indexRooms()
{
    int  goodRoom, slot;

    for (slot = 0;  slot < MAXROOMS;  slot++) {
        if (roomTab[slot].rtflags.INUSE == 1) {
            goodRoom = FALSE;
            if (roomTab[slot].rtlastMessage > cfg.oldest ||
                roomTab[slot].rtflags.PERMROOM == 1) {
                goodRoom    = TRUE;
            }

            if (!goodRoom) {
                getRoom(slot);
                roomBuf.rbflags.INUSE    = 0;
                roomBuf.rbflags.ISDIR    = 0;
                roomBuf.rbflags.PERMROOM = 0;
                roomBuf.rbflags.INUSE    = 0;
                putRoom(slot);
                strCat(msgBuf.mbtext, roomBuf.rbname);
                strCat(msgBuf.mbtext, "> ");
                noteRoom();
            }
        }
    }
}

/************************************************************************/
/*   insertParagraph()   inserts paragraph (CR/Space) into message      */
/*                       (By Jay Johnson of The Phoenix)                */
/************************************************************************/
void insertParagraph(buf, lim)
char *buf;
int lim;
{
#ifndef NOTONLYTHENET
    char oldString[2*SECTSIZE];
    char *loc, *textEnd;
    char *pc;
    int length;

    for (textEnd = buf, length = 0; *textEnd; length++, textEnd++);
    if (lim - length < 3) {
        mPrintf("?Overflow!\n ");
        return;
    }
    getString("string",oldString,(2*SECTSIZE),FALSE,ECHO);
    if ((loc=matchString(buf, oldString, textEnd)) == NULL) {
        mPrintf("?not found.\n ");
        return;
    }
    for (pc=textEnd; pc>=loc; pc--) {
       *(pc+2) = *pc;
    }
    *loc++ = '\n';
    *loc = ' ';
#endif
}

/************************************************************************/
/*      makeRoom() constructs a new room via dialogue with user.        */
/************************************************************************/
void makeRoom()
{
#ifndef NOTONLYTHENET
    label nm, oldName;
    int  i, CurrentFloor;
	extern char callLogPosting[800];
/*  extern char deFault, deFaultnm[NAMESIZE];  NEW STUFF for LATER! */
	char holdLine[20];
    CurrentFloor = thisFloor;

    /* update lastMessage for current room: */
    logBuf.lbgen[thisRoom]      = roomBuf.rbgen << GENSHIFT;

	baseRoom = cfg.codeBuf + cfg.bRoom; /* KLUGE til I find WHY!!! #### <br>*/

    strCpy(oldName, roomBuf.rbname);
    if ((thisRoom = findRoom()) == ERROR) {
        indexRooms();   /* try and reclaim an empty room        */
        if ((thisRoom = findRoom()) == ERROR) {
            mPrintf(" ?no room!");
            /* may have reclaimed old room, so: */
            if (roomExists(oldName) == ERROR)   strCpy(oldName, baseRoom);
            getRoom(roomExists(oldName));
            return;
        }
    }
/*  Next few lines is for code to prompt for new room stuff (flags)
 *  if (deFault == FALSE) {
 *		deFault = TRUE;
 *      sprintf(nm, deFaultnm);
 *		}
 *  else
 */
	getNormStr("name for new room", nm, NAMESIZE, ECHO);
    if (strLen(nm) == 0) {
        if (roomExists(oldName) == ERROR)   strCpy(oldName, baseRoom);
        getRoom(roomExists(oldName));
        return ;
    }

    if (roomExists(nm) >= 0) {
        mPrintf(" '%s' already exists.\n", nm);
        /* may have reclaimed old room, so: */
        if (roomExists(oldName) == ERROR)   strCpy(oldName, baseRoom);
        getRoom(roomExists(oldName));
        return;
    }
    if (!expert)   tutorial("newroom.blb", TRUE);

    zero_struct(roomBuf.rbflags);
    roomBuf.rbflags.INUSE    = TRUE;

    if (getYesNo(" Public room")) roomBuf.rbflags.PUBLIC = TRUE;
    else                               roomBuf.rbflags.PUBLIC = FALSE;

    mPrintf("'%s', a %s room",
        nm,
        roomBuf.rbflags.PUBLIC == 1  ?  "public"  :  "private"
    );

    if(!getYesNo("Install it")) {
        /* may have reclaimed old room, so: */
        if (roomExists(oldName) == ERROR)   strCpy(oldName, baseRoom);
        getRoom(roomExists(oldName));
        return;
    }

    strCpy(roomBuf.rbname, nm);
    for (i = 0;  i < MSGSPERRM;  i++) {
        roomBuf.msg[i].rbmsgNo   = 0l;   /* mark all slots empty */
        roomBuf.msg[i].rbmsgLoc  = 0 ;
    }
    roomBuf.rbgen = (roomTab[thisRoom].rtgen + 1) % MAXGEN;
    roomBuf.rbFlIndex = CurrentFloor;

    noteRoom();                         /* index new room       */
    RoomSys(thisRoom);
    putRoom(thisRoom);

    /* update logBuf: */
    logBuf.lbgen[thisRoom]      = roomBuf.rbgen << GENSHIFT;
    sPrintf(msgBuf.mbtext, "%s created by %s", formRoom(thisRoom, FALSE, FALSE),
                                                        logBuf.lbname);
    aideMessage(FALSE);
    strCpy(callLogPosting, msgBuf.mbtext);
	logMessage(19,"",FALSE);
    roomTab[thisRoom].rtlastNet = 0l;
	homeSpace();
    sprintf(holdLine, "roominfo.%d", thisRoom);
	unlink(holdLine); /* deletes old room info file whether there or not! */
	editRoomInfoFile();
#endif
}

/************************************************************************/
/*      matchString() searches for match to given string.  Runs backward*/
/*      through buffer so we get most recent error first.               */
/*      Returns loc of match, else ERROR                                */
/************************************************************************/
char *matchString(buf, pattern, bufEnd)
char *buf, *pattern, *bufEnd;
{
#ifndef NOTONLYTHENET
    char *loc, *pc1, *pc2;
    char foundIt;

    for (loc = bufEnd, foundIt = FALSE;  !foundIt && --loc >= buf;) {
        for (pc1 = pattern, pc2 = loc,  foundIt = TRUE ;  *pc1 && foundIt;) {
            if (! (toLower(*pc1++) == toLower(*pc2++)))   foundIt = FALSE;
        }
    }

    return   foundIt  ?  loc  :  NULL;
#endif
}

/************************************************************************/
/*      getNormStr() gets a string and deletes leading                  */
/*                                        & trailing blanks etc.        */
/************************************************************************/
void getNormStr(prompt, s, size, doEcho)
char *s, *prompt;
char doEcho;
int  size;
{
#ifndef NOTONLYTHENET
    char *pc;

/* printf("getNormStr:size=%d\n", size);        */
    getString(prompt, s, size, FALSE, doEcho);
    pc = s;
    if (!gotCarrier() && !onConsole) return;
    /* find end of string   */
    while (*pc)   {
        if (*pc < ' ')   *pc = ' ';   /* zap tabs etc... */
        pc++;
    }

    /* no trailing spaces: */
    while (pc>s  &&  isSpace(*(pc-1))) pc--;
    *pc = '\0';

    /* no leading spaces: */
    while (*s == ' ') {
        for (pc=s;  *pc;  pc++)    *pc = *(pc+1);
    }

    /* no double blanks */
    for (;  *s;)   {
        if (*s == ' '   &&   *(s+1) == ' ')   {
            for (pc=s;  *pc;  pc++)    *pc = *(pc+1);
        }
        else s++;
    }
#endif
}

/************************************************************************/
/*      noteRoom() -- enter room into RAM index array.                  */
/************************************************************************/
void noteRoom()
{
#ifndef NOTONLYTHENET
    int   i;
    MSG_NUMBER last;

    last = 0l;
    for (i = 0;  i < ((thisRoom == MAILROOM) ? MAILSLOTS : MSGSPERRM);  i++)  {
        if (roomBuf.msg[i].rbmsgNo > last &&
            roomBuf.msg[i].rbmsgNo <= cfg.newest) {
            last = roomBuf.msg[i].rbmsgNo;
        }
    }
    roomTab[thisRoom].rtlastMessage = last           ;
    strCpy(roomTab[thisRoom].rtname, roomBuf.rbname) ;
    roomTab[thisRoom].rtgen         = roomBuf.rbgen  ;
    movmem(&roomBuf.rbflags, &roomTab[thisRoom].rtflags,
                                             sizeof roomBuf.rbflags);
    roomTab[thisRoom].rtShareType = roomBuf.rbShareType;
    roomTab[thisRoom].rtFlIndex   = roomBuf.rbFlIndex;
#endif
}

/************************************************************************/
/*      renameRoom() is sysop special fn                                */
/*      Returns:        TRUE on success else FALSE                      */
/************************************************************************/
char renameRoom()
{
#ifndef NOTONLYTHENET
    logBuffer lBuf;
    int c, r;
    static char *huh = "?\n ";
#ifdef BRIAN
    char buffer[200], wasShared;
#else
    char buffer[200], wasShared, wasAnon, wasReadOnly, wasAlterNet;
#endif
    extern char *APrivateRoom;
    char doAideMessage = 0;     /* Counter to determine if aide msg needed */

    if (                                /* clearer than "thisRoom <= AIDEROOM"*/

#ifndef QTEST
        ( thisRoom == LOBBY && !(onConsole || remoteSysop) )
        ||
        thisRoom == MAILROOM
        ||
        thisRoom == AIDEROOM

#else
		thisRoom < 3         /* but this is more to the point and shorter! */
#endif

    ) {
        mPrintf("? Not here!\n ");
        return FALSE;
    }

    wasAlterNet = roomBuf.rbflags.ALTER_NET;
    wasReadOnly = roomBuf.rbflags.READ_ONLY;
    wasShared = roomBuf.rbflags.SHARED;
#ifndef BRIAN
    wasAnon = roomBuf.rbflags.ANON;
#endif
    initLogBuf(&lBuf);

    formatSummary(buffer);
    sPrintf(msgBuf.mbtext, "%s, formerly ", formRoom(thisRoom, FALSE, FALSE));
    strCat(msgBuf.mbtext, buffer);

    c = 0;      /* Init */
    while (c != 'X' && onLine()) {
        mPrintf("\n Room edit cmd: ");
        c = toUpper(iChar());
        switch (c) {
            case 'X':
                mPrintf("\bExit room editing\n ");
                break;
            case 'A':
                if (!SomeSysop()) {
                    mPrintf(huh);
                    break;
                }
                doAideMessage++;
                mPrintf("rchive status\n ");
                roomBuf.rbflags.ARCHIVE = getYesNo("Activate room archiving");
                if (roomBuf.rbflags.ARCHIVE) {
                    getString("filename", buffer, 198, FALSE, ECHO);
                    if (!addArchiveList(thisRoom, buffer)) {
                        roomBuf.rbflags.ARCHIVE = 0;
                    }
                    else {
                        initialArchive(buffer);
                    }
                }
                break;
            case 'B':
                if (!SomeSysop() || !cfg.BoolFlags.netParticipant) {
                    mPrintf(huh);
                    break;
                }
                mPrintf("ackbone/Host/Normal setting\n ");
                if (!roomBuf.rbflags.SHARED) {
                    mPrintf("Not a shared room!\n ");
                    break;
                }
                mPrintf("Make this room <B>ackbone, <H>ost, or <N>ormal? ");
                r = toUpper(iChar());
                if (r != 'B' && r != 'H') break;
                roomBuf.rbShareType = (r == 'B') ? BACKBONE : REG_HOST;
                mPrintf((r == 'B') ? "ackbone" : (r == 'H') ? "ost" : "ormal");
                doCR();
                if (r == 'B') {
                    ShType = REG_HOST;
                    roomBuf.rbShareType = BACKBONE;
                    getList(knownHosts, "Systems that are Hosts");
                    ShType = ACTIVE_BACKBONE;
                    getList(knownHosts,
                        "Systems that you will be an Active Backbone for");
                    ShType = PASS_BACKBONE;
                    getList(knownHosts,
                        "Systems that you will be a Passive Backbone for");
                }
                else if (r == 'H') {
                    ShType = BACKBONE;
                    roomBuf.rbShareType = REG_HOST;
                    getList(knownHosts,
                        "Systems that are Backbones for this room");
                }
                else {
                    roomBuf.rbShareType = PEON;
                }
                if (r != 'N') {
                    ShType = PEON;
                    getList(knownHosts,
                        "Systems that should be returned to PEON status");
                }
                break;
            case 'M':
                if (!aide) {
                    mPrintf(huh);
                    break;
                }
                mPrintf("oderator setting\n ");
                if (!getXString("moderator", buffer, NAMESIZE,
                      "no moderator", ""))
                    break;
                if (strLen(buffer) != 0)
                    if (findPerson(buffer, &lBuf) == ERROR) {
                        mPrintf("No '%s' found\n ", buffer);
                        break;
                    }
                doAideMessage++;
                strCpy(roomBuf.rbmoderator, buffer);
                break;
            case 'N':
                mPrintf("ame change\n ");
                getNormStr("New room name", buffer, NAMESIZE, ECHO);
                r = roomExists(buffer);
                if (r >= 0  &&  r != thisRoom) {
                     mPrintf("A '%s' exists!\n", buffer);
                } else {
                    strCpy(roomBuf.rbname, buffer);  /* also in room itself  */
                    doAideMessage++;
                }
                break;
            case 'D':
                if (!SomeSysop()) {
                    mPrintf(huh);
                    break;
                }
                doAideMessage++;
                mPrintf("irectory status\n ");
                roomBuf.rbflags.ISDIR = getYesNo("Activate directory");
                if (roomBuf.rbflags.ISDIR) {
                    if ((roomBuf.rbflags.ISDIR = getArea(&roomBuf)))
                        roomBuf.rbflags.PERMROOM = TRUE;
                    else break;
                }
                else break;
            case 'U':
                if (!SomeSysop() || !roomBuf.rbflags.ISDIR) {
                    mPrintf(huh);
                    break;
                }
                doAideMessage++;
                if (c == 'U') mPrintf("/D room\n ");
                roomBuf.rbflags.UPLOAD = getYesNo("Allow uploads");
                roomBuf.rbflags.DOWNLOAD = getYesNo("Allow downloads");
                if (!roomBuf.rbflags.UPLOAD && !roomBuf.rbflags.DOWNLOAD)
                    mPrintf("???\n ");
                break;
            case 'T':
                mPrintf("emporary room\n ");
                if (roomBuf.rbflags.ISDIR)
                    mPrintf("File rooms ALWAYS permanent\n ");
                else {
                    roomBuf.rbflags.PERMROOM = !getYesNo("Room is temporary");
                    doAideMessage++;
                }
                break;
            case 'P':
                doAideMessage++;
                mPrintf("rivate setting\n ");
                roomBuf.rbflags.PUBLIC = !getYesNo("Make room private");
                if (!roomBuf.rbflags.PUBLIC) {
                    if (getYesNo("Cause non-aide users to forget room")) {
                        if (!wasShared || getYesNo(WARN_MSG)) {
                            roomBuf.rbgen = (roomBuf.rbgen +1) % MAXGEN;
                  logBuf.lbgen[thisRoom] = (logBuf.lbgen[thisRoom] & CALLMASK) +
                                              (roomBuf.rbgen << GENSHIFT);
                            roomTab[thisRoom].rtgen = roomBuf.rbgen;
                         }
                    }
                }
                break;
            case 'S':
                if (!SomeSysop() || !cfg.BoolFlags.netParticipant) {
                    mPrintf(huh);
                    break;
                }
                mPrintf("hared room\n ");
                roomBuf.rbflags.SHARED = getYesNo("Network (shared) room");
                if (roomBuf.rbflags.SHARED) {
                    if (!wasShared)
                        doAideMessage++;
                    getList(addToList, wasShared ?
                                "Systems to add to network list for this room" :
                                "Systems to network this room with");
                    if (wasShared)
                        getList(killFromList,
                                  "Systems to take off network list");
                    roomBuf.rbflags.AUTO_NET =
                        getYesNo("Should messages automatically be networked");
                    if (roomBuf.rbflags.AUTO_NET)
                        roomBuf.rbflags.ALL_NET =
                             getYesNo("Even for users without net privileges");
                }
                break;
            case 'L':
                mPrintf("ure users to room\n ");
                if (roomBuf.rbflags.PUBLIC && !roomBuf.rbflags.INVITE) {
           mPrintf("\n This is a public room!\n ");
                    break;
                }
                getList(makeKnown, "Users to be invited");
                break;
            case 'O':
                mPrintf("nly Invitational\n ");
                doAideMessage++;
                if ((roomBuf.rbflags.INVITE = getYesNo("Invitation only room")))
                {
                    roomBuf.rbflags.PUBLIC = FALSE;
                    if (getYesNo("Cause non-aide users to forget room")) {
                        if (!wasShared || getYesNo(WARN_MSG)) {
                            roomBuf.rbgen = (roomBuf.rbgen +1) % MAXGEN;
                  logBuf.lbgen[thisRoom] = (logBuf.lbgen[thisRoom] & CALLMASK) +
                                              (roomBuf.rbgen << GENSHIFT);
                            roomTab[thisRoom].rtgen = roomBuf.rbgen;
                         }
                    }
                    getList(makeKnown, "Users to be invited");
                }
                break;
#ifndef BRIAN
            case 'I':
                mPrintf("nnominate status\n ");
                roomBuf.rbflags.ANON = getYesNo("Anonymous room");
                if (roomBuf.rbflags.ANON != wasAnon)
                    doAideMessage++;
                break;
#endif
			case 'R':
				if (!SomeSysop()) {
                    mPrintf(huh);
                    break;
					}
				mPrintf("ead-only status\n ");
				roomBuf.rbflags.READ_ONLY = getYesNo("Read-Only room");
				if (roomBuf.rbflags.READ_ONLY != wasReadOnly)
					doAideMessage++;
				break;
			case 'E':
				if (!SomeSysop()) {
					mPrintf(huh);
					break;
					}
				mPrintf("xternal Net link enable\n ");
				roomBuf.rbflags.ALTER_NET =
					getYesNo("Enable external net");
                if (!roomBuf.rbflags.ALTER_NET) break;
                alterNet = getYesNo("Link directory");
                if (roomBuf.rbflags.ALTER_NET) {
                    if ((roomBuf.rbflags.ALTER_NET = getArea(&roomBuf)))
                        roomBuf.rbflags.PERMROOM = TRUE;
                    else {
						alterNet=FALSE;
						roomBuf.rbflags.ALTER_NET = FALSE;
						mPrintf("\n DirLink error.  Net-link aborted.");
						break;
						}
                	}
				if (roomBuf.rbflags.SHARED)
					roomBuf.rbflags.ALT_LINKED=
						getYesNo("Link external net to Citadel net");
                if (roomBuf.rbflags.ALTER_NET != wasAlterNet)
					doAideMessage++;
                alterNet=FALSE;
				break;
			case 'G':
				mPrintf("ateway enable\n ");
				roomBuf.rbflags.IS_GATEWAY=FALSE;
				if (roomBuf.rbflags.ALTER_NET==FALSE) {
					mPrintf("\n Room is not net-linked.");
					break;
					}
                if (roomBuf.rbflags.SHARED==FALSE ||
							roomBuf.rbflags.ALT_LINKED==FALSE) {
                    mPrintf("CitaNet linkage not in place.");
					break;
					}
                roomBuf.rbflags.IS_GATEWAY=
					getYesNo("Make this a GATEWAY room");
				break;
			case 'C':
				if (roomBuf.rbflags.SHARED && roomBuf.rbflags.ALTER_NET) {
    				mPrintf("ross-link to Citadel net");
					roomBuf.rbflags.ALT_LINKED = getYesNo("Confirm");
					}
				else mPrintf("\b?");
				break;
            case 'V':
                mPrintf("alues\n %s is %s.", roomBuf.rbname,
                                                   formatSummary(buffer));
                break;
            case 'W':
                mPrintf("ithdraw Invitations\n ");
                if (roomBuf.rbflags.PUBLIC && !roomBuf.rbflags.INVITE) {
           mPrintf("\n This is a public room!\n ");
                    break;
                }
                getList(makeUnknown, "Users to be removed");
                break;
            case 'Z':
                if (!SomeSysop() || !cfg.BoolFlags.netParticipant ||
                             !roomBuf.rbflags.ISDIR) {
                    mPrintf(huh);
                    break;
                }
                doAideMessage++;
                mPrintf("\bNet Downloadable\n ");
                roomBuf.rbflags.NO_NET_DOWNLOAD =
                          !getYesNo("Room accessible to network");
                break;
            default:
                if (expert) {
                    mPrintf("?\n ");
                   /* break */ return FALSE;
                }
            case '?':
                tutorial(SomeSysop() ? "rooms.mnu" : "rooma.mnu", TRUE);
                break;
        }
    }

    noteRoom();
    putRoom(thisRoom);

    sPrintf(lbyte(msgBuf.mbtext), ", has been edited to %s, ",
                                   formRoom(thisRoom, FALSE, FALSE));
    if (doAideMessage && !onConsole) {
        formatSummary(lbyte(msgBuf.mbtext));
        sPrintf(lbyte(msgBuf.mbtext), ", by %s.", logBuf.lbname);

        aideMessage(FALSE);
    }
    killLogBuf(&lBuf);
    return TRUE;
#endif
}

/************************************************************************/
/*      formatSummary() formats a summary of the current room           */
/************************************************************************/
char *formatSummary(buffer)
char *buffer;
{
#ifndef NOTONLYTHENET
    sPrintf(buffer, "a%s, ",
            roomBuf.rbflags.INVITE ? "n Invitation only" :
            roomBuf.rbflags.PUBLIC ? public_str : private_str);

    sPrintf(lbyte(buffer), "%s",
            roomBuf.rbflags.PERMROOM ? perm_str : temp_str);

    if (roomBuf.rbflags.ARCHIVE)
        sPrintf(lbyte(buffer), ", Archived ('%s')", findArchiveName(thisRoom));
#ifndef BRIAN
    if (roomBuf.rbflags.ANON)
        strCat(buffer, ", Anonymous");
#endif
    if (roomBuf.rbflags.READ_ONLY)
		strCat(buffer, ", Read-Only");
	if (roomBuf.rbflags.ALTER_NET) {
		strCat(buffer, ", external-net enabled");
        strCat(buffer, (roomBuf.rbflags.IS_GATEWAY) ? ", Gateway enabled"
													: ", non-Gateway");
		} /* after debugging move to next condition! */
	if (roomBuf.rbflags.ALT_LINKED)
		strCat(buffer, ", external-net<=>Citadel-Net linked");
    if (roomBuf.rbflags.SHARED) {
        strCat(buffer, ", Shared");
        if (roomBuf.rbflags.AUTO_NET) {
            strCat(buffer, " (autonet for ");
            strCat(buffer, roomBuf.rbflags.ALL_NET ? "all users)" :
                                                "net-priv users)");
        }
    }

    if (roomBuf.rbflags.ISDIR) {
        strCat(buffer, ", Directory (");
		if (onConsole || remoteSysop) {
    	    dirString(lbyte(buffer), &roomBuf);
        	strCat(buffer, ", ");
            }

        if (roomBuf.rbflags.UPLOAD)
            strCat(buffer, "uploads, ");

        if (roomBuf.rbflags.DOWNLOAD)
            strCat(buffer, "downloads, ");

        sPrintf(lbyte(buffer), "%snet downloadable",
                roomBuf.rbflags.NO_NET_DOWNLOAD ? "not " : "");

        strCat(buffer, ")");
    }
    strCat(buffer, " room");

    if (strLen(roomBuf.rbmoderator))
        sPrintf(lbyte(buffer), " (Moderator is %s)", roomBuf.rbmoderator);

    return buffer;
#endif
}

/************************************************************************/
/*      getList() get a list of names and process them                  */
/************************************************************************/
void getList(fn, prompt)
int (*fn)(label data);
char *prompt;
{
     label buffer;

     mPrintf("%s (Empty line to end)", prompt);
     doCR();
     do {
         mPrintf(": ");
         getNormStr("", buffer, NAMESIZE, ECHO);
         if (strLen(buffer) != 0)
             if (!(*fn)(buffer)) break;
     } while (strLen(buffer) != 0);
}

/************************************************************************/
/*      replaceString() corrects typos in message entry                 */
/************************************************************************/
#define REPLACE_SIZE    2000
void replaceString(buf, lim)
char *buf;
int  lim;
{
#ifndef NOTONLYTHENET
    char oldString[2*SECTSIZE];
    char newString[REPLACE_SIZE];
    char *loc, *textEnd;
    char *pc;
    int  incr, length, oldLen, newLen;
                                                  /* find terminal null */
    textEnd = lbyte(buf);
    length = strLen(buf);

    getString("string",      oldString, (2*SECTSIZE), FALSE, ECHO);
    oldLen = strLen(oldString);
    if ((loc=matchString(buf, oldString, textEnd)) == NULL) {
        mPrintf("?not found.\n ");
        return;
    }

    getString("replacement", newString, REPLACE_SIZE, FALSE, ECHO);
    newLen = strLen(newString);
    if ( (newLen - oldLen)  >=  lim - length) {
        mPrintf("?Overflow!\n ");
        return;
    }

    /* delete old string: */
    for (pc=loc,
    incr=strLen(oldString);
    *pc=*(pc+incr);     /* Compiler generates a warning for this line */
    pc++);
    textEnd -= incr;

    /* make room for new string: */
    for (pc=textEnd, incr=strLen(newString);  pc>=loc;  pc--) {
        *(pc+incr) = *pc;
    }

    /* insert new string: */
    for (pc=newString;  *pc;  *loc++ = *pc++);
#endif
}

/************************************************************************/
/*      initialArchive() Does initial archive of a room                 */
/************************************************************************/
void initialArchive(fn)
char *fn;
{
#ifndef NOTONLYTHENET
    int        msgRover;
    char       *TmpMsg;
    MSG_NUMBER number;

    TmpMsg = GetDynamic(strLen(msgBuf.mbtext) + 1);
    strCpy(TmpMsg, msgBuf.mbtext);
    mPrintf("Starting archive\n ");
    for (msgRover = 0; msgRover < MSGSPERRM; msgRover++) {
        number = roomBuf.msg[msgRover].rbmsgNo;
        if (number > cfg.oldest && number < cfg.newest) {
            msgToDisk(fn, roomBuf.msg[msgRover].rbmsgNo,
                          roomBuf.msg[msgRover].rbmsgLoc);
            mPrintf("%ld\n ", roomBuf.msg[msgRover].rbmsgNo);
        }
    }
    strCpy(msgBuf.mbtext, TmpMsg);
    free(TmpMsg);
#endif
}

/************************************************************************/
/*      knownHosts() handles setting systems as hosts                   */
/************************************************************************/
int knownHosts(name)
char *name;
{
#ifndef NOTONLYTHENET
    int slot, i;

    if ((slot = searchNameNet(name)) == ERROR) {
        mPrintf("No '%s' known", name);
        doCR();
        return TRUE;
    }

    getNet(slot);

    if ((i = searchForRoom()) == ERROR) {
        mPrintf("%s not sharing this room with you.", name);
        doCR();
        return TRUE;
    }

    netBuf.netRooms[i].mode = ShType;

    putNet(slot);
    return TRUE;
#endif
}

/************************************************************************/
/*      addToList() Adds a system to a room networking list             */
/************************************************************************/
int addToList(name)
char *name;
{
#ifndef NOTONLYTHENET
    int slot, i, temp, gen;

    if (name[0] == '?') {
        writeNet(FALSE);
        return TRUE;
    }

    if ((slot = searchNameNet(name)) == ERROR) {
        mPrintf("No '%s' known", name);
        doCR();
        return TRUE;
    }

    getNet(slot);

    if (searchForRoom() != ERROR) {
        mPrintf("Already netting this room with %s", name);
        doCR();
        return TRUE;
    }

    for (i = 0; i < SHARED_ROOMS; i++) {
        if ((netBuf.netRooms[i].srgen & 0x8000) != 0) { /* Salvage attempt */
            temp = netBuf.netRooms[i].srslot & 0x7fff;
            gen  = netBuf.netRooms[i].srgen & 0x7fff;

            if (roomTab[temp].rtgen != gen ||   /* No longer exists!    */
                roomTab[temp].rtflags.SHARED == 0) {    /* Not netting  */
                break;
            }
        }
        else
            break;
    }

    if (i == SHARED_ROOMS) {
        mPrintf("**Already sharing %d rooms with %s", SHARED_ROOMS, name);
        return TRUE;
    }

    netBuf.netRooms[i].srslot   = thisRoom;
    netBuf.netRooms[i].lastMess = cfg.newest;
    netBuf.netRooms[i].srgen    = roomBuf.rbgen + (unsigned) 0x8000;
    copy_array(netBuf.netRooms, netTab[slot].netTRooms);

    putNet(slot);
    return TRUE;
#endif
}

/************************************************************************/
/*      searchForRoom() Checks to see if the current room is in the     */
/*      current node's room sharing list                                */
/************************************************************************/
int searchForRoom()
{
#ifndef NOTONLYTHENET
    int i;
    int temp, gen;

    for (i = 0; i < SHARED_ROOMS; i++) {
        if ((netBuf.netRooms[i].srgen & 0x8000) != 0) {
            temp = netBuf.netRooms[i].srslot & 0x7fff;
            gen  = netBuf.netRooms[i].srgen & 0x7fff;

            if (temp == thisRoom && gen == roomBuf.rbgen) {
                return i;
            }
        }
    }
    return ERROR;

#endif
}

char getXString(prompt, target, targetSize, CR_str, dft)
char *prompt, *target, *dft;
int  targetSize;
char *CR_str;           /* Null if empty C/R indicates abort entry      */
{
    char ourPrompt[100];

    sPrintf(ourPrompt, "%s (", prompt);
    if (CR_str != NULL && strLen(CR_str) != 0)
        sPrintf(lbyte(ourPrompt), "C/R = '%s', ", CR_str);

    strCat(ourPrompt, "ESCape aborts)");
    exChar = ESC;
    getString(ourPrompt, target, targetSize, TRUE, ECHO);
    exChar = '?';
    if (!onLine()) return FALSE;        /* Lost carrier */
    if (target[0] == ESC) return FALSE;
    if (CR_str == NULL && target[0] == 0) return FALSE;
    else if (target[0] == 0) strCpy(target, dft);
    return TRUE;
}


int makeKnown(user)
char *user;
{
    return doMakeWork(user, roomBuf.rbgen);
}

int makeUnknown(user)
char *user;
{
    return doMakeWork(user, (roomBuf.rbgen + (MAXGEN-1)) % MAXGEN);
}

int doMakeWork(user, val)
char *user;
int val;
{
    logBuffer lBuf;
    int              target;

    initLogBuf(&lBuf);
    if ((target = findPerson(user, &lBuf)) == ERROR)
        mPrintf("'%s' not found\n ", user);
    else {
        lBuf.lbgen[thisRoom] = (val << GENSHIFT) + MAXVISIT - 1;
        putLog(&lBuf, target);
    }
    killLogBuf(&lBuf);
    return TRUE;
}

int killFromList(sysName)
char *sysName;
{
#ifndef NOTONLYTHENET
    int i, slot;

    if ((slot = searchNameNet(sysName)) == ERROR) {
        mPrintf("No '%s' known", sysName);
        doCR();
        return TRUE;
    }

    getNet(slot);

    if ((i = searchForRoom()) == ERROR) {
        mPrintf("Not netting this room with %s", sysName);
        doCR();
        return TRUE;
    }

    netBuf.netRooms[i].srgen = 0;

    putNet(slot);
    return TRUE;
#endif
}

makeLinkedRoomList()
{
#ifndef NOTONLYTHENET
 static int roomIndPtr;
 FILE *theRoomList;

 roomIndPtr=2;
 unlink("roomlink.sys");
 theRoomList=fopen("roomlink.sys", "w+t");
 fprintf(theRoomList, "%s\n\n", "AlterNet Linked Rooms");
 do {
    getRoom(roomIndPtr);
	if (roomBuf.rbflags.ALTER_NET == 1 && roomBuf.rbflags.INUSE)
		fprintf(theRoomList, "%03d: %s\n", thisRoom, roomBuf.rbname);
	roomIndPtr++;
	} while (roomIndPtr < MAXROOMS);
 fclose(theRoomList);
#endif
}