/************************************************************************/
/*                              Floors.c                                */
/*                      Floor handling code for Citadel-86              */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              Contents                                */
/************************************************************************/

char ShowNew;
char JustChecking;
char HasSkipped;

#ifndef ONLYTHENET
static char maim;
static char NotForgotten = TRUE;
static char FoundNew;
static int  GlobalFloor;
static int  MoveCount;

extern struct floor  *FloorTab;
extern rTable *roomTab;
extern CONFIG cfg;
extern aRoom  roomBuf;
extern paintBrush colTable;
extern logBuffer logBuf;
extern MSG_BUF   msgBuf;
extern int           thisRoom;
extern int           TopFloor;
extern char          *confirm;
extern char          outFlag;
extern char          onConsole;
extern char          *baseRoom;

extern int DispDirectoryRoom(), DispPrivateRoom(), DispAnonRoom(),
           DispNetworkRoom(), DispExterLinkedRoom(), DispCrossLinkedRoom();
extern int DispGatewayRoom();
#endif

/************************************************************************/
/*      FloorRunner() runs through all the rooms on a floor             */
/************************************************************************/
int FloorRunner(start, func)
int start, (*func)(int r);
{
#ifndef ONLYTHENET
    int rover, CurrentFloor;

    if (start == ERROR) return ERROR;
    CurrentFloor = roomTab[start].rtFlIndex;
                /* start with current room, go through table */
    for (rover = 0; rover < MAXROOMS; rover++) {
                /* deep breath ... should rewrite this, prime example of
                   programming via accretion. */
        if (  roomTab[rover].rtflags.INUSE &&
              roomTab[rover].rtFlIndex == CurrentFloor  &&
              (((roomTab[rover].rtgen == (logBuf.lbgen[rover] >> GENSHIFT) &&
              NotForgotten) ||
                (abs(roomTab[rover].rtgen -
                        (logBuf.lbgen[rover] >> GENSHIFT))
                             == FORGET_OFFSET && !NotForgotten))
              || (NotForgotten && aide &&
                     (cfg.BoolFlags.aideSeeAll || onConsole) &&
              (!roomTab[rover].rtflags.INVITE || onConsole)))) {
            if ((*func)(rover)) return rover;
        }
    }
    return ERROR;
#endif
}

/************************************************************************/
/*      NewRoom() Gets next new room in system (like GotoRoom())        */
/************************************************************************/
int NewRoom()
{
#ifndef ONLYTHENET
    int CurrentFloor, OldFloor, roomNo;

    OldFloor = thisFloor;
    CurrentFloor = -1;
    while (   (roomNo = FloorRunner(thisRoom, NSRoomHasNew)) == ERROR &&
              CurrentFloor != TopFloor     ) {
        thisRoom = FirstRoom(++CurrentFloor);
    }
    if (CurrentFloor == TopFloor)
        roomNo = 0;           /* no new-message rooms found */
    getRoom(roomNo);
    mPrintf("%s\n ", roomBuf.rbname);
	ScrNewUser();
    return !(OldFloor == roomTab[thisRoom].rtFlIndex);
#endif
}

/************************************************************************/
/*      FirstRoom() Gets first room of specified floor                  */
/************************************************************************/
int FirstRoom(FloorNo)
int FloorNo;
{
#ifndef ONLYTHENET
    int rover;

    for (rover = 0; rover < MAXROOMS; rover++) {
        if (     roomTab[rover].rtflags.INUSE &&
                 roomTab[rover].rtFlIndex == FloorNo    )
            return rover;
    }
    return ERROR;
#endif
}

/************************************************************************/
/*      DoFloor() handles fanout for floor commands                     */
/************************************************************************/
int DoFloors()
{
#ifndef ONLYTHENET
    char c;

    c = toUpper(iChar());
    switch (c) {
        case 'A': return FAide();
        case 'Z': FForget();                   break;
        case 'S':
        case 'G': FGotoSkip(c);                break;
        case 'K': FKnown(ONLY_FLOORS);         break;
        case 'C': FConfigure();                break;
        case '?': tutorial("floor.mnu", TRUE); break;
        default:
            return FALSE;       /* let main() handle this mistake       */
    }
    return TRUE;
#endif
#ifdef ONLYTHENET
	return FALSE;
#endif
}

/************************************************************************/
/*      FSkipped() Show skipped rooms and floors                        */
/************************************************************************/
void FSkipped()
{
#ifndef ONLYTHENET
    int rover, roomNo;

    JustChecking = FALSE;
    ShowNew = TRUE;
    mPrintf("\n Skipped rooms [%s]:\n ", FloorTab[thisFloor].FlName);
    FloorRunner(thisRoom, SkippedNewRoom);
    ShowNew = FALSE;
    JustChecking = TRUE;
    mPrintf("\n Floors with skipped rooms:\n ");
    for (rover = 1; rover < TopFloor; rover++) {
        roomNo = FirstRoom(rover);
        if (FloorRunner(roomNo, SkippedNewRoom) != ERROR)
            mPrintf(" [%s] ", FloorTab[rover].FlName);
    }
    JustChecking = FALSE;
    tableRunner(SkippedNewRoom, TRUE);
#endif
}

/************************************************************************/
/*      FForget() Forget floor (this is pretty crude code)              */
/************************************************************************/
void FForget()
{
#ifndef ONLYTHENET
    mPrintf("\bForget [%s]\n ", FloorTab[roomTab[thisRoom].rtFlIndex].FlName);
    if (!getYesNo(confirm)) return ;
    FloorRunner(thisRoom, Zroom);
    gotoRoom(cfg.codeBuf+cfg.bRoom /*baseRoom*/, 'S');
#endif
}

/************************************************************************/
/*      FConfigure() Change floor configuration value                   */
/************************************************************************/
void FConfigure()
{
#ifndef ONLYTHENET
    FloorMode = !FloorMode;
    mPrintf("\b%s mode\n ", (FloorMode) ? "FLOOR" : "Normal");
#endif
}

/************************************************************************/
/*      FGotoSkip() Skip an entire floor                                */
/************************************************************************/
void FGotoSkip(mode)
char mode;
{
#ifndef ONLYTHENET
    label floorName;
    int   floorNo, room1, newRoom, rover;

    outFlag = IMPERVIOUS;
    if (mode == 'S')
        mPrintf("kip [%s] goto ",
                            FloorTab[roomTab[thisRoom].rtFlIndex].FlName);
    else
        mPrintf("oto ");
    getNormStr("", floorName, NAMESIZE, ECHO);

    if (strLen(floorName) != 0) {
        if ((floorNo = FindFloor(floorName, TRUE)) == ERROR) {
            mPrintf(" ?no %s floor\n", floorName);
            return ;
        }
        room1 = FirstRoom(floorNo);
        if ((newRoom = FloorRunner(room1, FindAny)) == ERROR) {
            mPrintf(" No known rooms on %s floor\n", FloorTab[floorNo].FlName);
            return ;
        }
    }
    else {
        floorNo = thisFloor;
        for (rover = 0; rover < TopFloor; rover++)
            if (rover != floorNo && FloorTab[rover].FlInuse) {
                newRoom = FirstRoom(rover);
                if ((newRoom = FloorRunner(newRoom, NSRoomHasNew)) != ERROR)
                    break;
            }

        if (rover == TopFloor)
            newRoom = 0;
    }
    if (mode == 'S') FloorRunner(thisRoom, FSroom);
    gotoRoom(roomTab[newRoom].rtname, mode);
#endif
}

/************************************************************************/
/*      FindFloor() returns index for the given floor name              */
/************************************************************************/
static int FindFloor(name, doPartial)
label name;
char  doPartial;
{
#ifndef ONLYTHENET
    int rover;

    for (rover = 0; rover < TopFloor; rover++) {
        if (strCmpU(name, FloorTab[rover].FlName) == SAMESTRING &&
                       FloorTab[rover].FlInuse) {
            return rover;
        }
    }

    if (doPartial)
        for (rover = 0; rover < TopFloor; rover++) {
            if (FloorTab[rover].FlInuse &&
                        matchString(FloorTab[rover].FlName, name,
                                     lbyte(FloorTab[rover].FlName)) != NULL)
                return rover;
        }
    return ERROR;
#endif
}

/************************************************************************/
/*      FAide() Handles floor-oriented aide commands                    */
/************************************************************************/
int FAide()
{
#ifndef ONLYTHENET
    char c;

    if (!aide) return FALSE;    /* Indicates problem */
    mPrintf("ide ");
    c = toUpper(iChar());
    switch (c) {
        case 'C': CreateFloor();                 break;
        case 'D': DeleteFloors();                break;
        case 'M': MoveRooms();                   break;
        case 'K': KillFloor();                   break;
        case 'R': RenameFloor();                 break;
        case '?': tutorial("aideflr.mnu", TRUE); break;
        default: return FALSE;
    }
    return TRUE;
#endif
}

/************************************************************************/
/*      DeleteFloors() deletes empty floors                             */
/************************************************************************/
void DeleteFloors()
{
#ifndef ONLYTHENET
    int rover, looker, count = 0;

    mPrintf("elete empty floors\n ");
    sPrintf(msgBuf.mbtext, "Following empty floors deleted by %s: ",
                                                logBuf.lbname);
    for (rover = 1; rover < TopFloor; rover++) {
        if (FloorTab[rover].FlInuse) {
            for (looker = 0; looker < MAXROOMS; looker++) {
                if (    roomTab[looker].rtFlIndex == rover &&
                        roomTab[looker].rtflags.INUSE     )
                    break;
            }

            if (looker == MAXROOMS) {       /* Ah ha!  A kill! */
                count++;
                FloorTab[rover].FlInuse = FALSE;
                putFloor(rover);
                sPrintf(lbyte(msgBuf.mbtext), "[%s], ", FloorTab[rover].FlName);
            }
        }
    }
    if (count) {
        *(lbyte(msgBuf.mbtext) - 2) = '.';
        *(lbyte(msgBuf.mbtext) - 1) = 0;
    }
    aideMessage(FALSE);
#endif
}

/************************************************************************/
/*      MoveRooms() moves a series of rooms into the current floor      */
/************************************************************************/
void MoveRooms()
{
#ifndef ONLYTHENET
    int   CurrentRoom;
    char  *end;
    extern char callLogPosting[800];

    CurrentRoom = thisRoom;
    GlobalFloor = thisFloor;
    mPrintf("ove rooms\n ");
    MoveCount = 0;
    sPrintf(msgBuf.mbtext, "Following rooms moved to floor %s by %s: ",
                                FloorTab[thisFloor].FlName, logBuf.lbname);
    getList(MoveToFloor, "Rooms to move to this floor");
    getRoom(CurrentRoom);       /* MoveToFloor changes thisRoom & roomBuf */
    if (MoveCount != 0) {
        end = lbyte(msgBuf.mbtext);
        *(end - 2) = '.';
        *(end - 1) = 0;
        aideMessage(FALSE);
    strCpy(callLogPosting, msgBuf.mbtext);
	logMessage(19,"",FALSE);
    }
#endif
}

/************************************************************************/
/*      MoveToFloor() move a room to a floor                            */
/************************************************************************/
int MoveToFloor(name)
char *name;
{
#ifndef ONLYTHENET
    int roomNo;

    if ((roomNo = roomExists(name)) == ERROR) {
        mPrintf("No '%s' exists!\n ", name);
        return TRUE;
    }

    if (   roomNo == LOBBY     ||
           roomNo == MAILROOM  ||
           roomNo == AIDEROOM  ) {
        mPrintf("Can't move '%s' from main floor!\n ", name);
        return TRUE;
    }

    MoveCount++;
    getRoom(roomNo);
    roomBuf.rbFlIndex = GlobalFloor;
    putRoom(roomNo);
    noteRoom();
    sPrintf(lbyte(msgBuf.mbtext), "%s, ", formRoom(thisRoom, FALSE, FALSE));
    return TRUE;
#endif
}

/************************************************************************/
/*      RenameFloor() renames a floor                                   */
/************************************************************************/
void RenameFloor()
{
#ifndef ONLYTHENET
    label FloorName;
    int   ReturnNo;

    mPrintf("ename Floor\n ");
    if (thisFloor == 0) {
        mPrintf("\n Use CTDLCNFG.SYS!!\n ");
        return ;
    }
    if (!getXString("Name of floor", FloorName, NAMESIZE, NULL, NULL))
        return ;

    if ((ReturnNo = FindFloor(FloorName, FALSE)) != ERROR) {
        if (ReturnNo != thisFloor) {
            mPrintf("Floor '%s' already exists!\n ", FloorName);
            return;
        }
    }

    sPrintf(msgBuf.mbtext, "Floor %s renamed to %s by %s.",
                      FloorTab[thisFloor].FlName, FloorName, logBuf.lbname);
    strCpy(FloorTab[thisFloor].FlName, FloorName);
    putFloor(thisFloor);
    aideMessage(FALSE);
#endif
}

/************************************************************************/
/*      CreateFloor() creates a floor                                   */
/************************************************************************/
void CreateFloor()
{
#ifndef ONLYTHENET
    label FloorName;
    int   rover;

    mPrintf("reate Floor\n ");
    if (  thisRoom == LOBBY ||
          thisRoom == MAILROOM ||
          thisRoom == AIDEROOM  ) {
        mPrintf("Illegal here!\n ");
        return ;
    }

    if (!getXString("Name of new floor", FloorName, NAMESIZE, NULL, NULL))
        return ;

    if (FindFloor(FloorName, FALSE) != ERROR) {
        mPrintf("'%s' is in use!\n ", FloorName);
        return;
    }

    for (rover = 1; rover < TopFloor; rover++)
        if (!FloorTab[rover].FlInuse) break;

    if (rover == TopFloor) {
        FloorTab = (struct floor *) realloc(FloorTab,
                                      sizeof *FloorTab * ++TopFloor);
    }
    roomBuf.rbFlIndex = rover;
    FloorTab[rover].FlInuse = TRUE;
    strCpy(FloorTab[rover].FlName, FloorName);
    putFloor(rover);
    putRoom(thisRoom);
    noteRoom();
    sPrintf(msgBuf.mbtext, "Floor %s created by %s.", FloorName,
                                        logBuf.lbname);
    aideMessage(FALSE);
#endif
}

/************************************************************************/
/*      putFloor() Put a floor record out to disk                       */
/************************************************************************/
void putFloor(i)
int i;
{
#ifndef ONLYTHENET
    SYS_FILE name;
    FILE *fd;
    long r;
    extern char *R_W_ANY;

    makeSysName(name, "ctdlflr.sys", &cfg.floorArea);
    if ((fd = safeopen(name, R_W_ANY)) == NULL)
        crashout("Couldn't open floor file for update!");

    r = i * sizeof *FloorTab;
    fseek(fd, r, 0);

    if (fwrite(FloorTab + i, sizeof *FloorTab, 1, fd) != 1)
        crashout(" ?putFloor(): write failed!");

    fclose(fd);
#endif
}

/************************************************************************/
/*      KillFloor() Kills a floor                                       */
/************************************************************************/
void KillFloor()
{
#ifndef ONLYTHENET
    int CurrentFloor, CurrentRoom;

    mPrintf("ill Floor\n ");
    if (roomBuf.rbFlIndex == 0) {
        mPrintf("\n Illegal!\n ");
        return;
    }

    if (!getYesNo(confirm)) return;

    sPrintf(msgBuf.mbtext, "Floor %s killed by %s.",
                               FloorTab[thisFloor].FlName, logBuf.lbname);
    aideMessage(FALSE);

    CurrentFloor = thisFloor;   /* #define in CTDL.H */
    CurrentRoom  = thisRoom;

    maim = getYesNo("Move all rooms on this floor to the main floor");
    FloorRunner(thisRoom, MaimOrKill);
    FloorTab[CurrentFloor].FlInuse = FALSE;
    putFloor(CurrentFloor);
                            /* due to behavior of MaimOrKill */
    getRoom((!maim) ? LOBBY : CurrentRoom);
#endif
}

/************************************************************************/
/*      MaimOrKill() kills or moves a room to main floor                */
/************************************************************************/
int MaimOrKill(i)
int i;
{
#ifndef ONLYTHENET
    getRoom(i);
    if (maim) {
        roomBuf.rbFlIndex = 0;
    }
    else {
        roomBuf.rbflags.INUSE = FALSE;
    }
    putRoom(i);
    noteRoom();
#endif
}

/************************************************************************/
/*      FKnown() Handles the ticklish task of floor display             */
/************************************************************************/
void FKnown(mode)
char mode;
{
#ifndef ONLYTHENET
    int         rover, roomNo, loopy;
    extern int  DirAlign;
    extern char AlignChar;

    if (mode == FORGOTTEN) {
        if (logBuf.lbflags.FLOORS) {
            NotForgotten = FALSE;
            mPrintf("\n Floors with forgotten rooms:");
            ShowNew = 2;
            for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
                    if (FloorRunner(roomNo, FindAny) != ERROR) {
						shrtColor( (rover+1)%4 );
                        DispFloorName(rover);
                        FloorRunner(roomNo, DispRoom);
                        DirAlign = 0;
                    }
                }
            }

            NotForgotten = TRUE;
        }
        else {
            mPrintf("\n Forgotten public rooms:\n ");
            for (rover = 0; rover < MAXROOMS; rover++) {
                if (abs(roomTab[rover].rtgen -
                        (logBuf.lbgen[rover] >> GENSHIFT))
                             == FORGET_OFFSET && roomTab[rover].rtflags.PUBLIC)
                    mPrintf(" %s ", formRoom(rover, TRUE, TRUE));
            }
        }
    }
	else if (mode == 5)  /* directory */
	{
           ShowNew=FoundNew=0;
           mPrintf("\n Floors with directory rooms:");
		   for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
					if (FloorRunner(roomNo, FindAny) != ERROR)
					{
	                    DispFloorName(rover);
    	                FloorRunner(roomNo, DispDirectoryRoom);
        	            DirAlign = 0;
					}
                }
            }
			mPrintf("\n ");

	}
	else if (mode == 6)  /* private */
	{
         ShowNew=FoundNew=0;
         mPrintf("\n Floors with private rooms:");
         for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
					if (FloorRunner(roomNo, FindAny) != ERROR)
					{
	                    DispFloorName(rover);
    	                FloorRunner(roomNo, DispPrivateRoom);
        	            DirAlign = 0;
					}
                }
            }
			mPrintf("\n ");
	}
#ifndef BRIAN
	else if (mode == 7)  /* anonymous */
	{
         ShowNew=FoundNew=0;
         mPrintf("\n Floors with anonymous rooms:");
         for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
					if (FloorRunner(roomNo, FindAny) != ERROR)
					{
	                    DispFloorName(rover);
    	                FloorRunner(roomNo, DispAnonRoom);
        	            DirAlign = 0;
					}
                }
            }
			mPrintf("\n ");
	}
#endif
	else if (mode == 8)  /* networked */
	{
         ShowNew=FoundNew=0;
         mPrintf("\n Floors with networked rooms:");
         for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
					if (FloorRunner(roomNo, FindAny) != ERROR)
					{
	                    DispFloorName(rover);
    	                FloorRunner(roomNo, DispNetworkRoom);
        	            DirAlign = 0;
					}
                }
            }
			mPrintf("\n ");
	}

	else if (mode == 9)  /* external-net linked */
	{
         ShowNew=FoundNew=0;
         mPrintf("\n Floors with external-net linked rooms:");
         for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
					if (FloorRunner(roomNo, FindAny) != ERROR)
					{
	                    DispFloorName(rover);
    	                FloorRunner(roomNo, DispExterLinkedRoom);
        	            DirAlign = 0;
					}
                }
            }
			mPrintf("\n ");
	}

	else if (mode == 10)  /* cross-linked shared rooms */
	{
         ShowNew=FoundNew=0;
         mPrintf("\n Floors with cross-linked network rooms:");
         for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
					if (FloorRunner(roomNo, FindAny) != ERROR)
					{
	                    DispFloorName(rover);
    	                FloorRunner(roomNo, DispCrossLinkedRoom);
        	            DirAlign = 0;
					}
                }
            }
			mPrintf("\n ");
	}
/* #ifdef QTEST */
	else if (mode == 11)  /* cross-linked shared rooms */
	{
         ShowNew=FoundNew=0;
         mPrintf("\n Floors with Gateway rooms:");
         for (rover = 0; rover < TopFloor; rover++) {
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
					if (FloorRunner(roomNo, FindAny) != ERROR)
					{
	                    DispFloorName(rover);
    	                FloorRunner(roomNo, DispGatewayRoom);
        	            DirAlign = 0;
					}
                }
            }
			mPrintf("\n ");
	}
/* #endif */
    else if (mode != ONLY_FLOORS) {
        mPrintf("\n Rooms with unread messages on floor [%s]:\n ",
                                                FloorTab[thisFloor].FlName);
        ShowNew = TRUE;
		shrtColor(colTable.level1 /* A_RED */);
        FloorRunner(thisRoom, DispRoom);
        if (mode == INT_NOVICE || mode == NOT_INTRO) {
			shrtColor(colTable.level0 /* A_GREEN */);
            mPrintf("\n No unseen msgs in:\n ");
            ShowNew = FALSE;
			shrtColor(colTable.level2 /* A_BLUE */);
            FloorRunner(thisRoom, DispRoom);
			shrtColor(colTable.level0 /* A_GREEN */);
        }

        mPrintf("\n Other floors with unread messages:\n ");
/*		shrtColor(colTable.level1); */
        for (rover = 0; rover < TopFloor; rover++) {
			shrtColor( (rover+1)%4 );
            if (rover != roomBuf.rbFlIndex) {
                roomNo = FirstRoom(rover);
                if (FloorRunner(roomNo, RoomHasNew) != ERROR)
                    mPrintf(" [%s] ", FloorTab[rover].FlName);
            }
        }
    }
    else {
        mPrintf("nown Floors");
        AlignChar = ' ';
         for (loopy = 0, ShowNew = FoundNew = 1; loopy < 2;
                                loopy++, ShowNew--, FoundNew--) {
            doCR();
            mPrintf(loopy == 0 ? "Floors with unread messages:" :
                                  "Floors with no unread messages:");
			shrtColor(loopy==0 ? colTable.level1 /* A_RED */
						: colTable.level2 /* A_BLUE */);
            for (rover = 0; rover < TopFloor; rover++) {
				shrtColor( (rover%3) );
                if (FloorTab[rover].FlInuse) {
                    roomNo = FirstRoom(rover);
                    if (
                      (FloorRunner(roomNo, CheckFloor) != ERROR && loopy == 0)
                       ||
                      (FloorRunner(roomNo, CheckFloor) != ERROR && loopy == 1)
                      ) {
                        DispFloorName(rover);
                        FloorRunner(roomNo, DispRoom);
                        DirAlign = 0;
                    }
                }
            }
            shrtColor(colTable.level0 /* A_GREEN */);
        }
    }
#endif
}

/************************************************************************/
/*      DispFloorName() Displays a floor name with periods, etc...      */
/************************************************************************/
void DispFloorName(FloorNo)
int FloorNo;
{
#ifndef ONLYTHENET
    int i;
    extern int DirAlign;
    extern char AlignChar;

    mPrintf("\n [%s] ", FloorTab[FloorNo].FlName);
    DirAlign = 24;
    AlignChar = ' ';
    for (i = strLen(FloorTab[FloorNo].FlName); i < 21; i++)
        mPrintf(".");
#endif
}


/************************************************************************/
/*      These functions are used as arguments to FloorRunner            */
/************************************************************************/

/************************************************************************/
/*      RoomHasNew() TRUE if room has new messages                      */
/************************************************************************/
int RoomHasNew(i)
int i;
{
#ifndef ONLYTHENET
    return (    roomTab[i].rtlastMessage >
            logBuf.lbvisit[logBuf.lbgen[i] & CALLMASK] &&
            roomTab[i].rtlastMessage >= cfg.oldest     );
#endif
}

/************************************************************************/
/*      CheckFloor()                                                    */
/************************************************************************/
int CheckFloor(i)
int i;
{
#ifndef ONLYTHENET
    if ((RoomHasNew(i) && FoundNew) ||
        (!RoomHasNew(i) && !FoundNew))
        return TRUE;
    return FALSE;
#endif
}

/************************************************************************/
/*      NSRoomHasNew() TRUE if room has new messages and isn't skipped  */
/************************************************************************/
int NSRoomHasNew(i)
int i;
{
#ifndef ONLYTHENET
    if (!roomTab[i].rtflags.SKIP && RoomHasNew(i))
        return TRUE;

    if (roomTab[i].rtflags.SKIP) /* Kludge this in -- ugly but useful */
        HasSkipped = TRUE;
    return FALSE;
#endif
}

/************************************************************************/
/*      DispRoom() display room name                                    */
/************************************************************************/
int DispRoom(i)
int i;
{
#ifndef ONLYTHENET
    char HasNew;
    extern char shownHidden;

    HasNew = RoomHasNew(i);
    if (ShowNew == 2 || (HasNew && ShowNew == 1) || (!HasNew && !ShowNew)) {
        mPrintf(" %s ", formRoom(i, TRUE, TRUE));
/*        mPrintf(" %s", formRoom(i, TRUE, TRUE));  */
        if (!roomTab[i].rtflags.PUBLIC) shownHidden = TRUE;
    }
    return FALSE;
#endif
}

/************************************************************************/
/*      Zroom() Zforgets the room                                       */
/************************************************************************/
int Zroom(i)
int i;
{
#ifndef ONLYTHENET
    int r;

    if (     i == LOBBY    ||
             i == MAILROOM ||
             i == AIDEROOM      ) {
        return FALSE;
    }
    r = (roomTab[i].rtgen + FORGET_OFFSET) % MAXGEN;
    logBuf.lbgen[i] = r << GENSHIFT;
    return FALSE;
#endif
}

/************************************************************************/
/*      FSroom() Skips a room on a floor                                */
/************************************************************************/
int FSroom(i)
int i;
{
#ifndef ONLYTHENET
    roomTab[i].rtflags.SKIP = 1;     /* Set bit */
    return FALSE;
#endif
}

/************************************************************************/
/*      FindAny() Finds any known room on a floor                       */
/************************************************************************/
int FindAny(i)
int i;
{
    return TRUE;        /* My, that was easy... */
}

