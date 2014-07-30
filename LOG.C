/************************************************************************/
/*                              log.c                                   */
/*                                                                      */
/*         userlog code for  Citadel bulletin board system              */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 85Nov15 HAW  MS-DOS library implemented.                             */
/* 85Aug31 HAW  Fix <.ep> problem.                                      */
/* 85Aug17 HAW  Update to onLine().                                     */
/* 85Aug10 HAW  Fix so system doesn't go out to disk on short pwds.     */
/* 85Jul26 HAW  Kill noteLog(), insert anti-hack code in newPW().       */
/* 85Jun13 HAW  Tweak code for networking stuff.                        */
/* 85Mar13 HAW  Moved zapLogFile() and logInit and logSort into confg.c.*/
/* 85Jan19 HAW  Fix terminate() so room prompt isn't tossed at modem.   */
/* 85Jan19 HAW  New Users are now directed to type ".help POLICY".      */
/* 85Jan19 HAW  Move findPerson() into file.                            */
/* 84Dec15 HAW  Fix bug that allowed discovery of private rooms.        */
/* 84Aug30 HAW  Now we roll into the 16-bit world.                      */
/* 84Jun23 HAW&JLS  Eliminating unused local variables using CRF.       */
/* 84Jun19 JLS  Fixed terminate so that Mail> doesn't screw up SYSOP.   */
/* 84Apr04 HAW  Started upgrade to BDS C 1.50a.                         */
/* 83Feb27 CrT  Fixed login-in-Mail> bug.                               */
/* 83Feb26 CrT  Limited # new messages for new users.                   */
/* 83Feb18 CrT  Null pw problem fixed.                                  */
/* 82Dec06 CrT  2.00 release.                                           */
/* 82Nov03 CrT  Began local history file & general V1.2 cleanup         */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      doInviteDisplay()       displays invited users                  */
/*      findPerson()            load log record for named person        */
/*      login()                 is menu-level routine to log caller in  */
/*      newPW()                 is menu-level routine to change a PW    */
/*      newUser()               menu-level routine to log a new caller  */
/*      PWSlot()                returns CTDLLOG.buf slot password is in */
/*      slideLTab()             support routine for sorting logTab      */
/*      storeLog()              store data in log                       */
/*      strCmpU()               strcmp(), but ignoring case distinctions*/
/*      terminate()             menu-level routine to exit system       */
/************************************************************************/

/************************************************************************/
/*                External variable declarations in LOG.C               */
/************************************************************************/
int              thisSlot;              /* logTab slot logBuf found via */
char             loggedIn = FALSE;      /* Global have-caller flag      */
char             prevChar;              /* for EOLN/EOParagraph stuff   */
static char      pwChangeCount;         /* Anti-hack variable           */
int              flip, extension, limit, counter, logTries = 0;
char			 newLogMethod, barBlock, specHook;
char             fullFileName[20], filename[15];
char nameThisGuy[80];
char *thisOne; /* makes name found in findPartPerson() a global         */

/************************************************************************/
/*                External variable definitions for LOG.C               */
/************************************************************************/
extern MSG_BUF      msgBuf;          /* Message buffer      */
extern logBuffer logBuf;         /* Log buffer of a person       */
extern paintBrush colTable;      /* the ANSI rainbow             */
extern LogTable *logTab;           /* RAM index of pippuls         */
extern CONFIG cfg;               /* Configuration variables      */
extern rTable *roomTab;          /* RAM index of rooms           */
extern aRoom  roomBuf;           /* Room buffer                  */
extern MSG_BUF   tempMess;
extern char          heldMess;
extern FILE          *logfl;            /* log file descriptor          */
extern int           thisLog;           /* entry currently in logBuf    */
extern char          outFlag;           /* Output skip flag             */
extern char          whichIO;           /* Where IO's going...          */
extern char          haveCarrier;       /* Do we still got carrier?     */
extern char          echo;              /* Who gets what                */
extern char          onConsole;         /* Where we get stuff from      */
extern int           thisRoom;          /* The room we're in            */
#ifdef BRIAN
extern int           exitValue;     	/* Kludge alert!         */
#else
extern int           exitValue, tradeWars;     /* Kludge alert!         */
#endif
extern char          shortSession, toDoors, justOut, blockAllBlurbs;
extern char			 specialPrompt;
extern int           nrPosts;           /* counts daily message total   */
extern int			 userMessages, nrCalls; /* counts messages & calls  */
extern int			 notDoorUser;       /* logrecord flag for door priv */
extern int           timeOnLine;        /* session time limit if used   */
extern int           tryToLogIn;        /* how many hack tries to login */
extern char			 siege, siegeResult;/* very private system login    */
extern char          ANSI_GRAPHICS;     /* used as a flag               */
extern char          human, sentNotice, itsGone;
extern char          *userEnterName;    /* for UNIX-style login         */
extern char          unixStyle;         /* ditto */
extern char          mailTag;           /* flag for findPartPerson()    */
extern char          stickingAround;    /* is user a .terminate-stay    */
                                        /* or really a new caller?      */
extern long          Dl_Limit;          /* mins available for downloads */


void doRegister(int userLogPosition);


/************************************************************************/
/*      doInviteDisplay() shows invited users                           */
/************************************************************************/
void doInviteDisplay()
{
    logBuffer lBuf;         /* Log buffer of a person       */
    int rover = 0;
	int bumpTop= 3, bumpIt=1;

	doCR();                           	/* We start of neatly...      */

	if (termWidth<51)       bumpTop=1; /* This sets the width of the */
 	else if (termWidth<73)  bumpTop=2; /* display when Citadel types */
 	else if (termWidth<95)  bumpTop=3; /* out the list of users who  */
 	else if (termWidth<117) bumpTop=4; /* left info files depending  */
 	else                    bumpTop=5; /* on caller's term width.    */


    initLogBuf(&lBuf);
    mPrintf("%-24s ",logBuf.lbname);
    outFlag = OUTOK;
    for (; outFlag == OUTOK && rover < cfg.MAXLOGTAB; rover++) {
        if (rover != thisLog) {
            getLog(&lBuf, rover);
            if (lBuf.lbflags.L_INUSE &&
                    (roomBuf.rbgen == (lBuf.lbgen[thisRoom] >> GENSHIFT) ||
                     roomBuf.rbflags.PUBLIC &&
                      (lBuf.lbgen[thisRoom] >> GENSHIFT)
                            != (roomBuf.rbgen + FORGET_OFFSET) % MAXGEN)) {
/*                mPrintf(", %s", lBuf.lbname); */

		        bumpIt++;
				mPrintf("%-24s ", lBuf.lbname);
        		if (bumpIt==bumpTop) {
				doCR();
				bumpIt=0;
				}

            }
        }
    }
/*    mPrintf("."); */
	doCR();
    killLogBuf(&lBuf);
}

/************************************************************************/
/*      findPerson() loads log record for named person.                 */
/*      RETURNS: ERROR if not found, else log record #                  */
/************************************************************************/
int findPerson(name, lBuf)
char                *name;
logBuffer    *lBuf;
{
    int  h, i, foundIt, logNo;

#ifdef NEWUSERBIO
	lBuf=(logBuffer *) GetDynamic(sizeof(/* logBuffer */ lBuf ));
#endif

    if (strCmpU(name, "Citadel") != SAMESTRING
		&& strCmpU(name, "FlashNET") != SAMESTRING) {
        h   = hash(name);
        for (foundIt = i = 0;  i < cfg.MAXLOGTAB && !foundIt;  i++) {

            if (logTab[i].ltnmhash == h) {
                getLog(lBuf, logNo = logTab[i].ltlogSlot);
                if (lBuf->lbflags.L_INUSE &&
                        strCmpU(name, lBuf->lbname) == SAMESTRING) {
                    foundIt = TRUE;
                }
            }
        }
    }
    else foundIt = FALSE;

#ifdef NEWUSERBIO
	free(lBuf);
#endif

    if (!foundIt)    return ERROR;
    else             return logNo;
}

/************************************************************************/
/*      login() is the menu-level routine to log someone in             */
/************************************************************************/
extern void (*StopVideo)(void);

void login(password)
char *password;    /* TRUE if parameters follow    */

{
#ifndef ONLYTHENET
    char temp[30], *m, *month;                 /* K2NE */
    int  foundIt, year, day, hours, minutes;   /* K2NE */
    FILE *funFd;
    SYS_FILE funFile;

	FILE *loghistory;
	char historyName[20];
    char wasExpertSetting;


    extern char *READ_ANY, *LCHeld, shownHidden; /* , *userHeldBaud; */
	extern int dumpDeadWood;

    foundIt =    ((PWSlot(password, /*load = */TRUE)) != ERROR);
    pwChangeCount = 1;
	siegeResult = TRUE;

    if (unixStyle && foundIt && *password && !onConsole) {
		if (strCmpU(userEnterName, logBuf.lbname)!=SAMESTRING)
		   foundIt=FALSE;
		   }
    if (foundIt && *password) {

        /* update userlog entries: */

        loggedIn     = TRUE;
        setUp(TRUE);
		wasExpertSetting=expert;

		/* Get today's date and time in Civilian form (human=TRUE) */
        getCdate(&year, &month, &day, &hours, &minutes);
		civTime(&hours, &m);                /* K2NE */
		human=TRUE;
        mPrintf(" Today is %s.\n", formDate());

        /* LASTON.# contains user's name, priv level and last call
		   date and time. If it exists, we show it now. */

        expert=TRUE;
		sprintf(historyName, "laston.%d", thisLog);
        if (access(historyName, 0) != -1) tutorial(historyName, FALSE);
		else doCR();
        expert=wasExpertSetting;

#ifdef OLD_WAY
		mPrintf(" %s logged in at %d:%02d%s.\n",logBuf.lbname, hours, minutes, m);  /* K2NE */
        mPrintf(" %s\n", aide ? "(SysAide)" : "");


/* tried to replace with updateLastOnFile() */
		sprintf(historyName, "laston.%d", thisLog);

		unlink(historyName);

        loghistory=fopen(historyName, "w+t");
		human=TRUE;
        fprintf(loghistory,
				" User: %s [%s]\n Last on: %s at %d:%02d%s.\n \n ",
				 logBuf.lbname,
				 strCmpU(logBuf.lbname, cfg.SysopName)==SAMESTRING
					 ? "Sysop" : aide ? "System Aide" : "Normal User",
				     formDate(), hours, minutes, m);
        fclose(loghistory);

#endif



/* now we grab the # of calls for this user, increment it and show it! */
        if (access("logcalls.sys", 00) != -1 && !onConsole) doUserCalls();
		else {
			mPrintf("Console login recorded.");
			doCR();
			}


        if (strCmpU(cfg.lastCaller, logBuf.lbname)!=SAMESTRING)
            	stickingAround=FALSE;
        if (!onConsole) {
			if (stickingAround==FALSE) {
				nrCalls++;
				/* if (aide ) */ mPrintf(" Caller #%d for today.\n", nrCalls);
				}
			else mPrintf (" Welcome back!\n");
			}
		startTimer(3);
		sentNotice = FALSE;
		userMessages = 0;
	    outFlag = IMPERVIOUS;

#ifdef OLD_WAY
        if (!onConsole) {
  	    if (access("logdates.sys", 00) != -1) doUserDate();
			}
#endif



/* now we update the LASTON file for this user */

		sprintf(historyName, "laston.%d", thisLog);
		unlink(historyName); /* erase the old file if it is there! */

        loghistory=fopen(historyName, "w+t");
		human=TRUE;
        fprintf(loghistory,
				" User: %s [%s]\n Last on: %s at %d:%02d%s.\n \n ",
				 logBuf.lbname,
				 strCmpU(logBuf.lbname, cfg.SysopName)==SAMESTRING
					 ? "Sysop" : aide ? "System Aide" : "Normal User",
				     formDate(), hours, minutes, m);
        fclose(loghistory);  /* LASTON.# is now updated for thisLog = # */











		if (cfg.lastCaller[0] && !stickingAround)
    		mPrintf(" Last caller: %s.\n", cfg.lastCaller);
        outFlag = OUTOK;
		dumpDeadWood=FALSE;
		unlink("JOURNAL.CAP");
	    if ( (access("logged.bat", 00)) != -1) {  /* check to see if batch */
                                                 /* files are in place    */
				if (gotCarrier()) system("logged.bat >nul");
				}
        ScrNewUser();
        shrtColor(colTable.level0 /* A_GREEN */);
        HelpIfPresent(termWidth > 40 ? "notice.blb" : "notice40.blb");
        shrtColor(colTable.level2 /* A_BLUE */);

/* #ifdef QTEST */
/*
 * This section of code will read files BULLETIN.### (000 thru 024)
 * to the user if the files exist, prompting continuation after
 * each file.  This is to "cover" the sysop who want his users to
 * read file(s) at each login.
 */

 limit=25;
 counter=0;
 strcpy(filename,"bulletin.");
 while (counter<limit) {
    	extension=counter;
	    sprintf(fullFileName, "%s%03d",filename,extension);
	    counter++;
   	    shrtColor(counter%2 ? colTable.level1 : colTable.level2);
	    if (!HelpIfPresent(fullFileName)) continue;
		if (counter == limit) break;
		else {
			if (!getYesNo("Read more")) break;
			}
        }
/* #endif */

/*
 * Next section of code reads LOGTEXT.### files to user if
 * they exist and if this is the first call of the day for
 * the current user.  The function "dayCheck(user)" returns
 * TRUE if this is the first call of the day for the user.
 */

 if (dayCheck(logBuf.lbname)==TRUE) {
 	limit=25;
 	counter=0;
 	strcpy(filename,"logtext.");
 	while (counter<limit) {
 		extension=counter;
		sprintf(fullFileName, "%s%03d",filename,extension);
		counter++;
    	shrtColor(counter%2 ? colTable.level1 : colTable.level2);
		if (!HelpIfPresent(fullFileName)) continue;
		if (counter==limit) break;
		else if (!getYesNo("Read more")) break;
		}
    }
 else mPrintf("\n Welcome back!");

/* CtdlK2NE V5.17:
 * We check the account to see if the user has download privs and if so
 * we tell him how many minutes of downloading is permitted this call.
 */

        if (logBuf.lbflags.lflag6==FALSE) {
			mPrintf("\n Download limit: ");
 			if (Dl_Limit != -1l) mPrintf("%ld minutes.\n", Dl_Limit);
 			else 	mPrintf("no limit.\n");
            }

		doCR();
        logMessage(L_IN, logBuf.lbname, FALSE);

        showMessages(NEWoNLY, FALSE, ALL_MESSAGES, "", "", "");

        listRooms(expert ? INT_EXPERT : INT_NOVICE);
/*        if (shownHidden && !expert)
            mPrintf("\n \n * = hidden room\n "); */

        if (!expert)
		    mPrintf("\n \n ! = external-net room\n%s",
			   (shownHidden==TRUE) ? " * = hidden room\n " : " ");

        outFlag = OUTOK;
        if (
            (
                logBuf.lbMail[MAILSLOTS-1].rbmsgNo >
                (logBuf.lbvisit[   logBuf.lbgen[MAILROOM] & CALLMASK   ])
            )
            &&
            logBuf.lbMail[MAILSLOTS-1].rbmsgNo > cfg.oldest
            &&
            thisRoom != MAILROOM
        )   {
			shrtColor(colTable.level2 /* A_BLUE */);
            mPrintf("\n  * You have private mail in Mail> *\n ");
        }

        if (cfg.BoolFlags.HoldOnLost) {
            sPrintf(temp, LCHeld, thisLog);
            makeSysName(funFile, temp, &cfg.holdArea);
            if ((funFd = safeopen(funFile, READ_ANY)) != NULL) {
                fread(&tempMess, sizeof tempMess, 1, funFd);
#ifndef NO_CRYPT
                crypte(&tempMess, sizeof tempMess, thisLog);
#endif
                fclose(funFd);
                unlink(funFile);
                outFlag = OUTOK;
                tutorial("holdbuff.blb", TRUE);
                heldMess = TRUE;
            }
        }
        if (shortSession) {
			if (expert) doCR();
			shrtColor(colTable.level0);
			mPrintf("\n For this call you have %d minutes.\n ",
				timeOnLine/60);
			}

#ifdef NEW_WAY
	   doCR();
	   dumpRoom(FALSE);
#endif

    } else {
        /* discourage password-guessing: */
        if (strLen(password) > 1 && whichIO == MODEM) {
            if (logTries++) pause(2000);
			if (logTries > tryToLogIn-1) {
                HelpIfPresent("badhack.blb");
				terminate(TRUE,TRUE);
				return;
				}
			}
        if (!cfg.BoolFlags.unlogLoginOk  &&  whichIO == MODEM)  {
			if (siege || unixStyle) {
				siegeResult = FALSE;
				return;
				}
            if (!HelpIfPresent("unlog.blb"))
                mPrintf(" No record -- leave message to 'sysop' in Mail> when requested.\n ");
				if (getYesNo(" Enter as new user")) {
					newLogMethod=TRUE;
					newUser();
					if (dumpDeadWood==TRUE) return;
					}

        } else {
            if (getYesNo(" No record: Enter as new user")) {
                newUser();
            }
        }
    }
#endif
}

/************************************************************************/
/*      newPW() is menu-level routine to change one's password          */
/*      since some Citadel nodes run in public locations, we avoid      */
/*      displaying passwords on the console.                            */
/************************************************************************/
void newPW()
{
    char oldPw[NAMESIZE];
    char pw[NAMESIZE];
    int  goodPW;

    /* save password so we can find current user again: */
    if (!loggedIn) {
        mPrintf("\n How?\n ");
        return ;
    }
    strcpy(oldPw, logBuf.lbpw);
    storeLog();
    do {
        echo    = CALLER;
        getNormStr(" new password", pw, NAMESIZE, NO_ECHO);
        echo    = BOTH;

        /* check that PW isn't already claimed: */
        goodPW = (PWSlot(pw,/* load = */TRUE) == ERROR  &&  strLen(pw) >= 2);

        if (pwChangeCount == 0) {
            mPrintf("Wait...\n");
            pause(3000);                    /* Discourage hacking       */
        }
        else pwChangeCount--;

        if (!goodPW) mPrintf("\n Poor password\n ");

    } while (!goodPW && (haveCarrier || whichIO==CONSOLE));

    doCR();
    PWSlot(oldPw, /*load = */TRUE);     /* reload old log entry             */
    pw[NAMESIZE-1] = 0x00;              /* insure against loss of carrier:*/

    if (goodPW  &&  strLen(pw) > 1) {   /* accept new PW:               */
        strcpy(logBuf.lbpw, pw);
        logTab[0].ltpwhash      = hash(pw);
        storeLog();
    }

    mPrintf("\n %s\n pw: ", logBuf.lbname);
    echo = CALLER;
    mPrintf("%s\n ", logBuf.lbpw);
    echo = BOTH;
}

/************************************************************************/
/*      newUser() prompts for name and password                         */
/************************************************************************/
void newUser()
{
#ifndef ONLYTHENET
    logBuffer   lBuf;
    char        *temp;
    char        fullnm[NAMESIZE], tmp[30];
    SYS_FILE    checkHeld;
    char        pw[NAMESIZE-1], tempCommand[50];
    int         good, g, h, i, ourSlot;
    MSG_NUMBER  low;
    char work[60];

    extern char *VERSION;
    extern char *LCHeld;
    extern char callLogPosting[800], siegeResult;
	extern int  dumpDeadWood;


	if (newLogMethod && !onConsole) {
   		HelpIfPresent("apply.blb");
	    gotoRoom("Mail", 'R');
		siegeResult = TRUE;
		dumpDeadWood = FALSE;
		doEnter(FALSE, 'M');
		}

    if (itsGone==TRUE) goto hook_toLeave; /* sometimes ya just gotta */

    configure(FALSE);   /* make sure new users configure reasonably     */

    if (!expert)   tutorial("password.blb", TRUE);

    do {
        /* get name and check for uniqueness... */
        do {
            getNormStr("Name", fullnm, NAMESIZE, ECHO);
            if ((temp = strchr(fullnm, '@')) != NULL) *temp = 0;
            good = TRUE;
            initLogBuf(&lBuf);
            if (findPerson(fullnm, &lBuf) != ERROR)
                good = FALSE;
            h = hash(fullnm);
            if (
                h == 0          /* "HUH?" --HAW 84Aug31                 */
                ||
                h == hash("Citadel")
				||
				h ==  hash("FlashNET")
                ||
                h == hash("Sysop")
            ) {
                good = FALSE;
            }
            /* lie sometimes -- hash collision !=> name collision */
            if (!good) mPrintf("We already have a %s\n", fullnm);
        } while (!good  &&  (haveCarrier || whichIO==CONSOLE));

        /* get password and check for uniqueness...     */
        do {
            echo        = CALLER;
            getNormStr("password",  pw, NAMESIZE-1, ECHO);
            echo        = BOTH  ;

            h    = hash(pw);
            for (i = 0, good = strLen(pw) > 1;
                 i < cfg.MAXLOGTAB && good;
                 i++) {
                if (h == logTab[i].ltpwhash) good = FALSE;
            }
            if (h == 0)   good = FALSE;
            if (!good) {
                mPrintf("\n Poor password\n ");
            }
        } while( !good  &&  (haveCarrier || whichIO==CONSOLE));

        mPrintf("\n name: %s", fullnm);
        mPrintf("\n password: ");
        echo = CALLER;
        mPrintf("%s\n ", pw);
        echo = BOTH;
    } while (!getYesNo("OK") && (haveCarrier || whichIO==CONSOLE));

    if (strlen(fullnm) < 3) goto hook_toLeave;
	strcpy(nameThisGuy, fullnm);
	if (whichIO==MODEM && !checkBanned(fullnm)) {
       goto hook_toLeave;
	   }


    if ((haveCarrier || whichIO == CONSOLE)) {
        nrCalls++;
        logMessage(L_IN, fullnm, '+');

        sPrintf(msgBuf.mbtext, "New user [%s] logged in.", fullnm);
        aideMessage(FALSE);


        /* kick least recent caller out of userlog and claim entry:     */
        ourSlot             = logTab[cfg.MAXLOGTAB-1].ltlogSlot;


        slideLTab(0, cfg.MAXLOGTAB-1);
        logTab[0].ltlogSlot = ourSlot;
        thisLog = ourSlot;

        /* copy info into record:       */
        strcpy(logBuf.lbname, fullnm);
        strcpy(logBuf.lbpw, pw);
        logBuf.lbflags.L_INUSE   = TRUE;




        logBuf.lbflags.SUBSYSTEM_OK=FALSE;   /* cannot use the "*" cmd.    */
        logBuf.lbflags.lflag2 = notDoorUser; /* true if BANNED from doors  */
        logBuf.lbflags.lflag3 = FALSE;       /* TRUE for ANSI graphics     */
        logBuf.lbflags.lflag4 = FALSE;       /* TRUE for Pause after msg   */
        logBuf.lbflags.lflag5 = FALSE;       /* TRUE for verbose headers   */
        logBuf.lbflags.lflag6 = FALSE;       /* TRUE if user can't D/load  */
		logBuf.lbflags.lflag7 = FALSE;       /* FALSE if user is validated */
        if (newLogMethod==TRUE && whichIO==MODEM)
        logBuf.lbflags.lflag7 = TRUE;        /* TRUE for UNvalidated user  */
        logBuf.lbflags.lflag8 = FALSE;       /* TRUE to turn msg #s OFF    */
        logBuf.lbflags.lflag9 = FALSE;       /* TRUE if UNLISTED username  */
        logBuf.lbflags.RUGGIE =         /* Tie this to cfg netPrivs option */
				!cfg.BoolFlags.NetDft;       /* TRUE if NO LinkedNet Privs */
        logBuf.lbflags.NET_PRIVS = cfg.BoolFlags.NetDft;
        logBuf.credit            = 0;           /* No L-D credit        */
        logBuf.lbfwd = logBuf.lbNetGen = -1;    /* No forwarding        */
        low = cfg.newest-50;
        if (cfg.oldest - low < 0x8000)   low = cfg.oldest;
        for (i=1;  i<MAXVISIT;  i++)   logBuf.lbvisit[i]= low;
        logBuf.lbvisit[                               0]= cfg.newest;
        logBuf.lbvisit[                    (MAXVISIT-1)]= cfg.oldest;

        /* initialize rest of record:   */
        for (i = 0;  i < MAXROOMS;  i++) {
            if (roomTab[i].rtflags.PUBLIC == 1) {
                g = (roomTab[i].rtgen);
                logBuf.lbgen[i] = (g << GENSHIFT) + (MAXVISIT-1);
            } else {
                /* set to one less */
                g = (roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN;
                logBuf.lbgen[i] = (g << GENSHIFT) + (MAXVISIT-1);
            }
        }
        for (i = 0;  i < MAILSLOTS;  i++)  {
            logBuf.lbMail[i].rbmsgLoc = 0l;
            logBuf.lbMail[i].rbmsgNo  = cfg.oldest -1;
        }

        /* fill in logTab entries       */
        logTab[0].ltpwhash      = hash(pw)         ;
        logTab[0].ltnmhash      = hash(fullnm)     ;
        logTab[0].ltlogSlot     = thisLog          ;
        logTab[0].ltnewest      = logBuf.lbvisit[0];

        /* special kludge for Mail> room, to signal no new mail:   */
        roomTab[MAILROOM].rtlastMessage =
                                logBuf.lbMail[MAILSLOTS-1].rbmsgNo;

        loggedIn = TRUE;

        storeLog();

        ScrNewUser();

        if (cfg.BoolFlags.HoldOnLost) {
            sPrintf(tmp, LCHeld, thisLog);
            makeSysName(checkHeld, tmp, &cfg.holdArea);
            unlink(checkHeld);
        }

        homeSpace();
        sprintf(work, "user%d.cit", thisLog);
		unlink(work); /* get rid of any user info file if it exists */

        HelpIfPresent("register.blb"); /* to be polite! */
		doRegister(thisLog);  /* gets new user's personal info */

        setupNrCalls(); /* initialize # of calls to 1, we hope! */


        HelpIfPresent(termWidth > 40 ? "notice.blb" : "notice40.blb");

		if (haveCarrier && newLogMethod==FALSE && !onConsole) {
                if ( (access("newuser.bat", 00)) != -1) {
					  /* check to see if batch  files are in place */
        	        sprintf(tempCommand, "newuser.bat %s > nul", fullnm);
					system(tempCommand); /* shell to Q&A Door */
/*					rawModemInit(); */
			        sPrintf(callLogPosting,
    	  			   "New User batch accessed by %s", logBuf.lbname);
					logMessage(19,"",FALSE);
					}
				}

		startTimer(3);
		if (newLogMethod==FALSE) {
			if (!onConsole) HelpIfPresent("policy.hlp");
 	        listRooms(expert ? INT_EXPERT : INT_NOVICE);
			}
    }
    killLogBuf(&lBuf);


hook_toLeave:

	if (newLogMethod && !onConsole) {
		if (!HelpIfPresent("valrules.blb"))
			mPrintf("\n \n New accounts are validated ASAP.\n \n ");
		terminate(TRUE,TRUE);
		}
#endif
}

/************************************************************************/
/*      PWSlot() returns userlog.buf slot password is in, else ERROR    */
/*      NB: we also leave the record for the user in logBuf.            */
/************************************************************************/
int PWSlot(pw, load)
char pw[NAMESIZE], load;
{
    int  h, i;
    int  foundIt, ourSlot;
    logBuffer lBuf;

    if (strLen(pw) < 2)         /* Don't search for these pwds          */
        return ERROR;

    initLogBuf(&lBuf);
    h = hash(pw);

    /* Check all passwords in memory: */
    for(i = 0, foundIt = FALSE;  !foundIt && i < cfg.MAXLOGTAB;  i++) {
        /* check for password match here */

        /* If password matches, check full password                     */
        /* with current newUser code, password hash collisions should   */
        /* not be possible... but this is upward compatable & cheap     */
        if (logTab[i].ltpwhash == h) {
            ourSlot     = logTab[i].ltlogSlot;
            getLog(&lBuf, ourSlot);

            if (strCmpU(pw, lBuf.lbpw) == SAMESTRING) {
                /* found a complete match */
                thisSlot = i   ;
                foundIt  = TRUE;
            }
        }
    }
    if (foundIt) {
        if (load == TRUE) {
            copyLogBuf(&lBuf, &logBuf);
            thisLog = ourSlot;
        }
        killLogBuf(&lBuf);
        return thisSlot;
    }
    killLogBuf(&lBuf);
    return ERROR   ;
}

/************************************************************************/
/*      slideLTab() slides bottom N lots in logTab down.  For sorting.  */
/************************************************************************/
void slideLTab(slot, last)
int slot;
int last;
{
    int i;

    /* open slot up: (movmem isn't guaranteed on overlaps) */
    for (i = last - 1;  i >= slot;  i--)  {
        movmem(&logTab[i], &logTab[i + 1], cfg.sizeLTentry);
    }
}

/************************************************************************/
/*      storeLog() stores the current log record.                       */
/************************************************************************/
void storeLog()
{
    logTab[0].ltnewest    = cfg.newest;
    logBuf.lbvisit[0]     = cfg.newest;
    putLog(&logBuf, thisLog);
}

/************************************************************************/
/*      strCmpU() is strcmp(), but ignoring case distinctions           */
/************************************************************************/
int strCmpU(s, t)
char s[], t[];
{
    int  i;

    i = 0;

    while (toUpper(s[i]) == toUpper(t[i])) {
        if (s[i++] == '\0')  return 0;
    }
    return  toUpper(s[i]) - toUpper(t[i]);
}

/************************************************************************/
/*      terminate() is menu-level routine to exit system                */
/************************************************************************/
void terminate(discon, save)
char discon, save;
{
    extern char heldMess, NewVideo, *READ_TEXT;
    extern MSG_BUF        tempMess;
    int                   i,dumbo;
    char                  StillThere;
    int                   year, day, hours, minutes;
    char                  *month, *m, temp[60];

    outFlag    = IMPERVIOUS;
    StillThere = onLine();

    if (loggedIn)       /* Logout format changed -- K2NE */
        {
	    getCdate(&year, &month, &day, &hours, &minutes);
        civTime(&hours, &m);
        if (!toDoors) {
			shrtColor(colTable.level1 /* A_RED */);
            dumbo=chkTimeSince(3)/60;
			human=TRUE;
			doCR();
			if (onConsole && !haveCarrier) mPrintf(" Console session completed.");
		    else mPrintf(" Session #%d for %s completed.",
											nrCalls, formDate());
			mPrintf("\n Time: %d minute%s with %d message%s saved.\n %s logged out at %d:%02d%s.\n ",
                 dumbo,
		         dumbo==1 ? "" : "s", userMessages,
		         userMessages==1 ? "" : "s", logBuf.lbname, hours, minutes, m);
            nrPosts=nrPosts+userMessages;
			}
		gate_keeper();
        }
    if (StillThere && !toDoors && !blockAllBlurbs) {
        HelpIfPresent("lonotice.blb");
		sleep(3);
		blockAllBlurbs=FALSE;
	    }
    if (discon)  {
        switch (whichIO) {
        case MODEM:
	        if (tradeWars || toDoors) storeLog();
            runHangup();
            modIn();                    /* And now detect carrier loss  */
            pause(50);
			purgeFossilBuffs();			/* clear buffer */
			justOut=TRUE;
            break;
        case CONSOLE:
            whichIO =  MODEM;
            printf("\n'MODEM' mode.\n ");

/* BATCH FILE HOOK to re-enable WATCHKIT or similar timer-guard */
	        if ( (access("OFFLOCAL.BAT", 00)) != -1)
							system("OFFLOCAL.BAT >nul");

			recycleModem();
            break;
        }
    }
    getCdate(&year, &month, &day, &hours, &minutes);
    civTime(&hours, &m);

    if (loggedIn) {

        if (StillThere) logBuf.lbgen[thisRoom]  = roomBuf.rbgen << GENSHIFT;
        if (save)       storeLog();

        strcpy(cfg.lastCaller, logBuf.lbname);

        logBuf.lbname[0] = 0;         /*  For screen display */
		logBuf.lbflags.lflag7 = FALSE; /* swat an ugly one   */
        loggedIn = FALSE;

        if (heldMess)
            SaveInterrupted(&tempMess);
/*        loggedIn=FALSE;   new for K2NE 6.02 TEST */
    }
/*	purgeFossilBuffs();
	if (discon) recycleModem();
    fakeBanner();
    ScrNewUser(); */
    setUp(TRUE);
    barBlock=TRUE;
/*	ScrNewUser(); */

#ifdef AB_ANSI         /* AB_ADD to turn graphics off */
	resetAnsiScreen();  /* AB_ADD to turn graphics off */
	logBuf.lbflags.lflag3 = FALSE;  /* AB_ADD after much experimenting, */
									/* this was unavoidable, honest */
#endif
    ANSI_GRAPHICS = FALSE;
    purgeFossilBuffs();
    for (i = 0; i < MAXROOMS; i++)      /* Clear skip bits */
        roomTab[i].rtflags.SKIP = 0;
    if (discon && !gotCarrier()) doTotals();
    outFlag = OUTOK;
	purgeFossilBuffs();
	getRoom(LOBBY);           /* Make sure user starts in lobby */
}

/* #ifdef NEW */
checkBanned(char thisPerson[NAMESIZE])
{
 FILE *badUsers;
 static char notThis;
 static char notAllowed[NAMESIZE];

 notThis=TRUE;
 if (access("badusers.sys",0)!=0) return notThis;
 badUsers=fopen("badusers.sys", "rt");
 if (badUsers==NULL) {
    fclose(badUsers);
	return TRUE;
	}
 while (!feof(badUsers)) {
	fgets(notAllowed, NAMESIZE, badUsers);
    notAllowed[strlen(notAllowed)-1]='\0';
	if (stricmp(notAllowed,thisPerson)==SAMESTRING) notThis=FALSE;
	}
 fclose(badUsers);
 return notThis;
}
/* #endif */

dayCheck(char onToday[NAMESIZE+5])
{
 FILE *usedIt;
 static char notThis, fileIsNew;
 static char notAllowed[NAMESIZE+5];

 fileIsNew=FALSE;
 notThis=FALSE;
 if (access("dayusers.sys",0)!=0) {
	notThis=TRUE;
	fileIsNew=TRUE;
	}

 usedIt=fopen("dayusers.sys", "a+t");

 if (notThis==FALSE) {
	 notThis=TRUE;
	 while (!feof(usedIt)) {
		fgets(notAllowed, NAMESIZE+5, usedIt);
    	notAllowed[strlen(notAllowed)-1]='\0';
		if (stricmp(notAllowed,onToday)==SAMESTRING) notThis=FALSE;
		}
	 }
 if (fileIsNew==FALSE) fclose(usedIt);
 if (notThis==TRUE) {
	 if (fileIsNew==FALSE) usedIt=fopen("dayusers.sys", "a+t");
	 fprintf(usedIt, "%s\n", onToday);
	 fclose(usedIt);
	 }
 return notThis;
}


dayList()
{
 FILE *usedIt;
 static char nOPE;
 static char userIsHere[NAMESIZE+5];

 nOPE=FALSE;
 usedIt=fopen("dayusers.sys", "rt");
 if (usedIt==NULL) {
    fclose(usedIt);
	nOPE=TRUE;
    mPrintf("\n File missing!");
	return;
	}
 if (nOPE==FALSE) {
	 nOPE=TRUE;
	 mPrintf("\n Today's Users:"
			 "\n ");
	 do {
		fgets(userIsHere, NAMESIZE+5, usedIt);
    	userIsHere[strlen(userIsHere)-1]='\0';
		if (!feof(usedIt)) mPrintf("\n %s", userIsHere);
		} while (!feof(usedIt));
	 }
 fclose(usedIt);
 doCR();
 return;
}

/*
 *  findPartPerson() loads log record for selected person based on
 *                   partial name search/match.
 *  Returns: ERROR if not found, else log record #
 */
int findPartPerson(partName, LBuf)
char *partName;
logBuffer    *LBuf;
{
    int  i, foundIt, LLogNo;
	char /* *thisOne, */ *tempOne, *tempTwo, *partUpper;

    tempOne = GetDynamic(NAMESIZE);
	tempTwo = GetDynamic(NAMESIZE);
    thisOne = GetDynamic(NAMESIZE);
	partUpper = GetDynamic(NAMESIZE);
	strcpy(partUpper, partName);
	strupr(partUpper);
    if (strCmpU(partName, "Citadel") != SAMESTRING
		&& strCmpU(partName, "FlashNET") != SAMESTRING) {
		mPrintf("Searching");
        for (foundIt = i = 0;  i < cfg.MAXLOGTAB && !foundIt;  i++) {
          getLog(LBuf, LLogNo = logTab[i].ltlogSlot);
		  if (LBuf->lbflags.L_INUSE) {
             mPrintf(".");
			 strcpy(tempOne, LBuf->lbname);
			 strcpy(tempTwo, tempOne);
			 strupr(tempTwo);
			 }
		  else continue;

            if (strstr(tempTwo,partUpper)!=NULL) {
/*			   sprintf(thisOne,"%s",tempOne); */
               doCR();
/*			   mPrintf("Found: %s ", thisOne); */
			   mPrintf("Found: %s ", tempOne);
               if (!getYesNo(mailTag==TRUE ? "Send mail" : "This user")) {
                  mPrintf("Searching");
				  continue;
				  }
			   else foundIt = TRUE;
            }
        }
    }
    else foundIt = FALSE;
    if (!foundIt) {
	   doCR();
	   return ERROR;
	   }
    else {
    sPrintf(thisOne, "%s", tempOne);
	return LLogNo;
	}
}
/* #endif */

#ifdef OLD_WAY
doUserDate() /* gets UNIXstyle date of last call and calculates
              * # of days since user last called. */
{
#ifndef ONLYTHENET
 FILE *dateFile;
 long int buffPointer, logRecAsLong;
 char *step1, *lastDate, *step2;

 lastDate = GetDynamic(25);
 step1=GetDynamic(40);
 step2=GetDynamic(40);
 buffPointer=logRecAsLong=0l;

 itoa(thisLog, step1, 10);
 logRecAsLong=atol(step1);
 buffPointer = (22l*logRecAsLong);
 dateFile=fopen("logdates.sys","r+t");

 rewind(dateFile);
 fseek(dateFile, buffPointer, SEEK_SET);
 fgets(lastDate, 20, dateFile);

 rewind(dateFile);
 fseek(dateFile, buffPointer, SEEK_SET);
 human=TRUE;
/* fprintf(dateFile, "%-20s\n", formDate()); */

 sprintf(step2, "%s.", formDate());
 fprintf(dateFile, "%-20s\n", step2);

 fclose(dateFile);

 mPrintf(" You last called on %s\n", lastDate);
/* mPrintf(" Your total of prior calls: %d\n", termNulls++); */
 if (access("logcalls.sys", 00) != -1) doUserCalls();
#endif /* ONLYTHENET */
}
#endif


#ifdef NEWNESS
doUserDate() /* gets UNIXstyle date of last call and calculates
              * # of days since user last called. */
{
#ifndef ONLYTHENET
 FILE *dateFile;
 char *lastDate="", *theFile="";

 sprintf(theFile, "laston.%d", thisLog);

 if (access(theFile,00)!=0) return;

 dateFile=fopen(theFile,"r+t");

 fgets(lastDate, 80, dateFile);
 fgets(lastDate, 80, dateFile);  /* threw out first line */
 fclose(dateFile);

 mPrintf("%s", lastDate);
/* mPrintf(" Your total of prior calls: %d\n", termNulls++); */
 if (access("logcalls.sys", 00) != -1) doUserCalls();
#endif /* ONLYTHENET */
}

#endif




#ifdef OLD_WAY
setupDate() /* initializes LOGDATES.SYS for a new user */
{
#ifndef ONLYTHENET
 FILE *dateFile;
 long int buffPointer, logRecAsLong;
 char *step2="";

 step2=GetDynamic(40);
 buffPointer=logRecAsLong=0l;

 dateFile=fopen("logdates.sys","r+t");

 itoa(thisLog, step2, 10);
 logRecAsLong=atol(step2);
 buffPointer = (22l*logRecAsLong);

 rewind(dateFile);
 fseek(dateFile, buffPointer, SEEK_SET);
 human=TRUE;
 sprintf(step2, "%s.", formDate());
 fprintf(dateFile, "%-20s\n", step2);

 fclose(dateFile);
 termNulls=0; /* set up #lines to default of 0 - disables "-More-" prompt */
#endif /* ONLYTHENET */
}
#endif


/* #ifdef QTEST */
setupNrCalls() /* initializes LOGCALLS.SYS for a new user */
{
#ifndef ONLYTHENET
 FILE *numberFile;
 long int buffPointer, logRecAsLong;
 char *step3="";

 step3=GetDynamic(40);
 buffPointer=logRecAsLong=0l;

 numberFile=fopen("logcalls.sys","r+t");

 itoa(thisLog, step3, 10);
 logRecAsLong=atol(step3);
 buffPointer = (42l*logRecAsLong);

 rewind(numberFile);
 fseek(numberFile, buffPointer, SEEK_SET);
 sprintf(step3, "%d", 1);
 fprintf(numberFile, "%-40s\n", step3);

 fclose(numberFile);
#endif /* ONLYTHENET */
}

doUserCalls() /* gets number of previous calls for this user
               * and updates by incrementing the file */
{
#ifndef ONLYTHENET
 FILE *callerFile;
 int NrTimesCalled, holdCallCount;
 long int buffPointer, logRecAsLong;
 char *step4, *nrOfCalls;

 nrOfCalls=GetDynamic(40);
 step4=GetDynamic(40);
 buffPointer=logRecAsLong=0l;

 itoa(thisLog, step4, 10);
 logRecAsLong=atol(step4);
 buffPointer = (42l*logRecAsLong);
 callerFile=fopen("logcalls.sys","r+t");

 rewind(callerFile);
 fseek(callerFile, buffPointer, SEEK_SET);
 fgets(nrOfCalls, 40, callerFile);
 NrTimesCalled=atoi(nrOfCalls);
 holdCallCount=NrTimesCalled;

 rewind(callerFile);
 fseek(callerFile, buffPointer, SEEK_SET);

 NrTimesCalled++;
 sprintf(step4, "%d", NrTimesCalled);
 fprintf(callerFile, "%-20s\n", step4);

 fclose(callerFile);

 mPrintf("Your prior calls: %d\n", holdCallCount);
#endif /* ONLYTHENET */
}
/* #endif */


void doRegister(int userLogPosition)
{
 char userName[80], userTown[40], userState[20], userZip[20], userPhone[20];
 char userStreet[80];
 char userFileName[20];
 char userIsFinished, isPublic;
 FILE *userRegisterFile;

 userIsFinished=isPublic=FALSE;
 doCR();
 while (!userIsFinished) {
	 specialPrompt=TRUE;

	 getString("Enter REAL name",        userName, 80, FALSE, TRUE);
     getString("Address",              userStreet, 80, FALSE, TRUE);
	 getString("City/Town",              userTown, 40, FALSE, TRUE);
	 getString("State",                 userState,  3, FALSE, TRUE);
	 getString("ZIPcode",                 userZip, 20, FALSE, TRUE);
	 getString("Phone #",               userPhone, 20, FALSE, TRUE);

	 specialPrompt=FALSE;
     doCR();
     mPrintf("You entered:");
	 doCR();

     mPrintf("\n    Name: %s\n Address: %s, %s, %s %s\n   Phone: %s\n ",
				userName, userStreet, userTown, userState, userZip, userPhone);
	 userIsFinished=getYesNo("Is this OK");
	 }

 isPublic=getYesNo("Make visible to others");

 sprintf(userFileName, "ctdluser.%d", userLogPosition);
 homeSpace();
 unlink(userFileName);
 userRegisterFile=fopen(userFileName, "w+t");
 fprintf(userRegisterFile, "%s\n %s\n %s\n %s, %s, %s\n %s",
				isPublic ? "Y" : "N",
				userName, userStreet, userTown, userState, userZip, userPhone);
 fclose(userRegisterFile);

}

