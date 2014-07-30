/************************************************************************/
/*                          K2NEDOOR.C                                  */
/*                                                                      */
/*                        - An Overview -                               */
/*  The idea for "doorways" in the Citadel software belongs totally to  */
/*  Alan Bowen.  If it were not for his experience in the use of the    */
/*  other PC bulletin boards that allow for running applications from   */
/*  within the BBS itself, Doors for Citadel:K2NE probably would never  */
/*  have come to pass.  The original code was hacked by Alan during the */
/*  last week in August (1988) and quickly taught us that we had a lot  */
/*  to learn about how the PC family of machines handles RS232 during   */
/*  "shell" applications.  The solution we chose may not be the best of */
/*  all possible worlds, but it works.  Later came the desire to more   */
/*  closely emulate the way other BBS programs handle the "layered      */
/*  handoff" to the actual door applications, so I patched in a bunch   */
/*  kludgey code to cause Citadel to create two files (dorinfo1.def and */
/*  pcboard.sys) which should help make it easier to run "doorware"     */
/*  originally designed for RBBS-PC and PCBoard use.  The RBBS stuff is */
/*  still (88Oct30) a bit flakey, but "we're working on it."  (VAQ)     */
/************************************************************************/

 #include "ctdl.h"
 #include "process.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/************************************************************************/

char toDoors = FALSE;  /* set a flag for the logout msg to caller       */
char binkleyBaudRate[10], doorTag[5];
char infoBannerCut;
extern CONFIG cfg;


/************************************************************************/
/*                           History				                	*/
/*                                                                      */
/* 88Sep15 VAQ  See local [K2NE] increm.xxx files for further history   */
/* 88Aug26 AAB  First work done                                         */
/* 88Aug26 AAB  Door stuff dreamed up                                   */
/************************************************************************/

/************************************************************************/
/*              Externals needed for this Door stuff!                   */
/*                                                                      */
/************************************************************************/
extern char intSet;
extern char onConsole, specialPrompt;
extern char *indexTable;
extern char human, loggedIn, stickingAround;
extern char hostDomainName[80];
extern void     (*StartVideo)(char *info),
       	 	    (*ChangeStatus)(char *info),
		        (*StopVideo)(void);
extern logBuffer logBuf;
extern int oldDayVal, nrCalls, nrPosts, linkLayerCount,
           netLogCount, netMessageTotal;
extern struct date today;


#ifdef BRIAN
#else
/**************************************************************************/
/*                  doDoors()                                             */
/*                                                                        */
/**************************************************************************/

int tradeWars=FALSE;
int  timeInDoors;      /* passed from command-line to enable/time Doors */
                       /* If this is missing, doors do NOT function!!!  */
void doDoors()
{
#ifndef ONLYTHENET
  FILE *fd;
  SYS_FILE name;
  char doorCommand[60], doorName[30], path[50], doorBaud[5];
  char doorUserName[30], realFirstName[30], doorFirstName[30], PCBS[129];
  char ourUserName[30], ourLastCallerName[30];
  char foundSpace, foundReturn;
  int nameIndex, firstNamePointer;
  extern char uploadFile[NAMESIZE]; /* kludge! use for calllog - K2NE */
  extern char currentUserName[30];
  extern char *curBaud, haveCarrier, onConsole;

    strcpy(path, "door\\");
    if (!loggedIn) /* no doors until logged in */
	{
	   mPrintf("\n Log in!\n");
	   return;
	}

	if (logBuf.lbflags.lflag2) {  /* make sure user has door privs */
		mPrintf("\n No access.");
		return;
		}
    mPrintf("\bDoors\n");

    if ( (chdir("door"))==-1 || timeInDoors == 0) { /* #1 - check to see if */
        mPrintf(" \n No Doors.\n ");     /* system allows door.  Must have  */
		homeSpace();                     /* a homespace/door directory and  */
        return;                          /* nonzero value for timeInDoors.  */
		}

    homeSpace();
    setmem(doorName, strlen(doorName), '\0');
    tutorial("door.blb", TRUE); /* prints list of doors */
    specialPrompt = TRUE;       /* get choice from user */
    getString("Your choice or <CR> to abort", doorName, 3, FALSE, ECHO);
								/* changed from 2 to 3 ^^ in V6.03 */
	specialPrompt = FALSE;

    if ((strlen(doorName)<1)
		||
		(doorName[0] == 13)
		||
        (doorName[0] == 10)
		||
		(doorName[0] == SPECIAL)) {
			homeSpace();
			mPrintf("Aborting\n ");
			return;
 			}


  	strcpy(uploadFile,doorName);  /* K2NE */
    doorTag[0]=toupper(doorName[0]);
	doorTag[1]='\0';
    if (doorName[1]) doorTag[1]=toupper(doorName[1]);
	doorTag[2]='\0';
	if (onConsole && !haveCarrier) {
		strcat(doorName, "L.BAT");
		tradeWars = FALSE;
		}
    else strcat(doorName, ".BAT");

    chdir("door");
    if ( (access(doorName, 00)) == -1) { /* #3 - check to see if door exists */
		mPrintf(" Invalid\n ");
		homeSpace();
		return;
		}

/* at this point we have 'passed all the tests' and are ready for door */

    homeSpace();
    mPrintf(" \n Door %s.\n ", doorTag);

   	logMessage(14,"",FALSE);   /* K2NE -- See, easy! */
    (*StopVideo)();

    chdir("door");                        /* puts us back in door dir */

    fd=fopen("doorname.bat", "wt");       /* puts door name to temp file */
	fprintf(fd, "ctdlkey.exe\n%s", doorName);
	fclose(fd);

	makeDoorFiles(); /* sets up datafile for CTDLKEY.EXE */
					 /* which does the real work after CTDL drops off! */

	doDoorSystemClose(); /* we are outta here! ERRORLEVEL set to 4 */

#endif
}

/******************************************************************/
/*   doWarsReset()  Very kludgey exit of Citadel on exitlevel 8.  */
/*                  You must support IF ERRORLEVEL 8 in your main */
/*                  BBS batchfile and execute a system ReBoot!!!  */
/******************************************************************/

doWarsReset()

{
#ifndef ONLYTHENET
    writeSysTab();
    systemShutdown();
    exit(8);
#endif
}

/******************************************************************/
/*   doDoorSystemClose() makes Citadel exit on exilevel 4.  This  */
/*         copies current CtdlTabl.Sys files, logs out            */
/*         updated user and returns control to your main          */
/*         BBS batch file.  You support IF ERRORLEVEL 4           */
/*         to handle your doors and after your doors,             */
/*         call the BBS back up using the "+doors" parameter      */
/*         to avoid kicking the current user offline!             */
/*         See the sample main BBS batchfile that we included     */
/*         in the runtime file-set for a neat way of doing this!  */
/******************************************************************/

doDoorSystemClose()  /* K2NE -- Horrible kludge! */

{
#ifndef ONLYTHENET
	homeSpace(); /* now we know where we are! */
	toDoors = TRUE;  /* flag makes sure other functions handle this right. */
    terminate( /* hangUp == */ FALSE, TRUE); /* log the user out */
    writeSysTab();   /* copy CtdlTabl.Sys! */
	gate_keeper();
    exit(4);
#endif
}
#endif

#ifdef WHYISTHISHERE
/* #ifdef QTEST * problems galore here! */
doNetSystemClose()  /* K2NE -- Horrible kludge! */
{
#ifndef ONLYTHENET
	homeSpace(); /* now we know where we are! */
	gate_keeper();
    purgeFossilBuffs(); /* clr modem buf */
    system("copy netsig.txt netsig.sys >nul");

    writeSysTab();
	modemClose();
    makeLinkedRoomList();
    systemShutdown();
    exit(0);


#endif
}
/* #endif */
#endif

doNetSystemClose()  /* K2NE -- Horrible kludge! */
{
#ifndef ONLYTHENET
    exit(0);


#endif
}




/*****
	doInfobanner() - collects the system info into one call for several areas
******/

doInfobanner()
{
	extern char *VERSION, *netVersion, *FLASH_MAIL_VERSION,
	            *NET_SWITCH_VERSION, *DOOR_VERSION, *NETgateVer;
#ifdef ONLYTHENET
    extern char *CITANETver;
#endif
    extern int  fixVers, majorVers;
    int   i, year, day, hours, minutes;
    char  *month, *m;

    getCdate(&year, &month, &day, &hours, &minutes);
    civTime(&hours, &m);

    if (infoBannerCut==TRUE) {
/*		mPrintf(" [%d:%02d%s]", hours, minutes, m);
		doCR(); */
		infoBannerCut=FALSE;
		return;
        }

    human=TRUE;
    mPrintf("\n %sThis is %s\n %s%s at %d:%02d%s\n ",
		loggedIn ? "" : "\t\t\t\t\t",
    	&cfg.codeBuf[cfg.nodeTitle],
		loggedIn ? "" : "\t\t\t\t\t",
		formDate(), hours, minutes, m);

#ifndef ONLYTHENET
    mPrintf("\n Citadel:K2NE ver: %s      Doors ver: %s%s\n CitaNet <tm> ver: %s   Telnet ver: 1.03%s",
    	VERSION, DOOR_VERSION,loggedIn ? "" : "\t   CITADEL:K2NE", netVersion, loggedIn ? "" : "\tDEVELOPMENT PROJECT");
#endif
#ifdef ONLYTHENET
    mPrintf("\n Networker    ver: %s", CITANETver);
#endif
	mPrintf("\n FlashMail!   ver: %s%s\n Net_Switch   ver: %s%s",
		FLASH_MAIL_VERSION, loggedIn ? "" : "\t\t\t\t\t    Vince Quaresima", NET_SWITCH_VERSION, loggedIn ? "" : "\t\t\t\t\t    Steve Williams");
#ifndef ONLYTHENET
    mPrintf("\n Citadel_Gate ver: %s%s", NETgateVer, loggedIn ? "" : "\t\t\t\t\t    Mike Burger");
#endif
    mPrintf("\n Configure    ver: %d    ", cfg.paramVers);
    if (!loggedIn && hostDomainName[0])
		mPrintf("FQDN: %s", hostDomainName);


/*    mPrintf("\n Commands   ver: %d.%d\n Configure  ver: %d",
    	majorVers, fixVers, cfg.paramVers); */

#ifndef ONLYTHENET
    doCR();
    mPrintf("\n %s** A PUBLIC DOMAIN release by K2NE SOFTWARE **\n ",
			loggedIn ? "" : "\t\t  ");
/*    mPrintf("\n * Presented by K2NE SOFTWARE *\n "); */
	ScrNewUser();
#endif
#ifdef ONLYTHENET
    mPrintf("\n ** CitaNet <tm> Network Handler for Citadel:K2NE **\n ");
#endif

}

getSessionData()
{
/*
 * The method used here may look like a kludge but it saves almost
 * 2K over using fscanf()!!!!  (vaq)
*/
 FILE *getInfo;
 char numberCalls[10], numberPosts[10], numberNetw[10], links[10];
 char dayt[9];
 char netMsgCount[10];

 getInfo = fopen("sysdata.usr","rt");
 if (getInfo!=NULL) {
	fgets(dayt, 9, getInfo);
	fgets(numberCalls, 9, getInfo);
	fgets(numberPosts, 9, getInfo);
    fgets(numberNetw, 9, getInfo);
    fgets(links, 9, getInfo);
	fgets(netMsgCount, 9, getInfo);
    fclose(getInfo);
	oldDayVal=atoi(dayt);
    nrCalls=atoi(numberCalls);
    nrPosts=atoi(numberPosts);
    netLogCount=atoi(numberNetw);
    linkLayerCount=atoi(links);
	netMessageTotal=atoi(netMsgCount);
	fclose(getInfo);
	}
 else { /* if it's missing we fix */
	fclose(getInfo);
	nrCalls=nrPosts=netLogCount=linkLayerCount=netMessageTotal=0;
	noteSessionData();
	}
}

dataBackUp()
{
 FILE *backInfo;
 char  backThisUp[80];

	homeSpace();
	unlink("sysdata.bak");
    backInfo = fopen("sysdata.bak","wt");

    sprintf(backThisUp, "%d\n%d\n%d\n%d\n%d\n%d", today.da_day,
				nrCalls, nrPosts, netLogCount,
			    linkLayerCount, netMessageTotal);
    fputs(backThisUp, backInfo);

	fclose(backInfo);
}

/*****
 *   getBinkleyBaud() gets baudrate from BinkleyTerm
 *****/

getBinkleyBaud()
{

 FILE *getBaud;
 char binkLine[10];
 extern char *curBaud;

 getBaud = fopen("bauds.usr","rt");
	fgets(binkleyBaudRate, 9, getBaud);
    fclose(getBaud);
    strcpy(cfg.whatRate, curBaud);
    stickingAround=FALSE;
}

doPP()
{
 cprintf("\n...");
 pause(75);
 cprintf("Pregnant ");
 pause(75);
 cprintf("Pause");
 pause(75);
 cprintf("...\n");
 pause(75);
}

