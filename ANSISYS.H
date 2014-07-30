
        /*
         * Use these only if we are ANSI capable...
         */
#ifdef ANSI_PROTOTYPING

/*
 * These functions are the interface calls to the system dependencies.
 * Since some of these functions can be implemented as parameterized
 * defines, you must go through these and comment out those that should
 * not be here.
 */

/*
 * 3.1. Modem stuff
 */

AN_UNSIGNED inp(void);

int MIReady(void);

void modemClose(void);
char outMod(int c);
char fastMod(int c);
void rawModemInit(void);

/* int gotCarrier(void); */
/* int firstModemInit(void); */
/* int changeBauds(void); */

/* void DisableModem(void);    */
/* void EnableModem(void);     */

char getNetBaud(void);

/* int setNetCallBaud(int targetBaudCode); */


/*
 * 3.2. Disk Stuff
 */

/* int mvToHomeDisk(int x); */

/*
 * 3.3. Console Stuff
 */

/* char getCh(void);   */
/* char KBReady(void); */
void mputChar(char c);
/* int simpleGetch(void); */

void ScreenUser(void);
void ScrNewUser(void);
void ScrTimeUpdate(int hr, int mn);
void SpecialMessage(char *message);

/*
 * 3.4. Area Stuff
 */

int CitGetFileList(char *mask, struct dirList list[], int max);

void freeFileList(struct dirList *list, int count);

char getArea(aRoom *roomData);
void homeSpace(void);

int netGetArea(struct fl_req *file_data, char ambiguous);
char netSetNewArea(NET_AREA *file_data);
/* int printArea(ROOM_AREA *x); */
char *prtNetArea(NET_AREA *netArea);
/* int RoomSys(int roomNo); */
char setSpace(aRoom *roomData);
char sysGetSendFiles(struct fl_send *sendWhat);
long sysRoomLeft(void);
void sysSendFiles(struct fl_send *sendWhat);
void updFiletag(char *fileName, char *desc);

/* 
 * 3.5. Baud handling
 */

char Find_baud(char **whatRate);

/*
 * 3.6. File Stuff
 */

/* void dirString(char *target, ROOM_AREA *area); */
void makeHelpFileName(char *new, char *original);
/* void makeSysName(SYS_FILE target, char *name, SYS_AREA *area); */
FILE *safeopen(char *fn, char *mode);
/* void totalBytes(long *size, FILE *fd); */
/* unopenSize(long *size, char *name);    */

/*
 * 3.7. System Formatting Functions
 */
int  mPrintf(char *format, ...);

void dPrintf(char *format, ...);
void splitF(FILE *diskFile, char *format, ...);
void mWCprintf(char *format, ...);
void mYMprintf(char *format, ...);
void mTrPrintf(char *format, ...);

/*
 * 3.8. Timers
 */

long chkTimeSince(int TimerId);

void pause(int i);

void startTimer(int TimerId);

void getRawDate(int *year, int *month, int *day, int *hours, int *minutes,
                        int *seconds, int *milli);
char setRawDate(int year, int month, int day, int hour, int min);

int  ReadDate(char *date, long *RetTime);

long CurAbsolute(void);

/*
 * 3.9. Miscellaneous
 */

/* void copy_struct(char *src, char *dest); */
/* void copy_array(char *src, char *dest); */
/* void copy_ptr(char *src, char *dest, int s); */
void giveSpaceLeft(aRoom *roomData);
void systemCommands(void);
void systemInit(void);
void systemShutdown(void);
int receive(int seconds);
void runPCPdial(void);
int WhatDay(void);
/* void zero_struct(target); */
/* void zero_array(char *target); */


/*
 * 3.11 File Comments
 */
int StFileComSearch(void);
int FindFileComment(char *fn);
void EndFileComment(void);

/*
 * 3.12 deARCing prototypes.
 */
void ArcInit(void);
void SendArcFiles(int protocol);
void MakeTempDir(void);

/*
 * These functions are in SysCfg.C.
 */

#ifdef CONFIGURE

void initSysSpec(void);
int sysSpecs(char *line, int offset, char *status);
char SysDepIntegrity(int *offset);

#ifdef SYSTEM_DEPENDENT

void dirExists(char disk, char *theDir);
int  doAreaCommon(char *var, char *line, SYS_AREA *area, int offset, int which);
void anyArea(char *var, char *line, char *disk, char *target);
int  doResCodes(char *var, char *line, int offset, int which);
void MSDOSparse(char *theDir, char *drive);

#endif

#endif

/*
 * These functions are in SysUtil.C.
 */

void getUtilDate(int *year, int *month, int *day, int *hours, int *minutes);

/*
 * These functions are for macro definitions.
 */
AN_UNSIGNED interpret(int instruction);
void writeArea(char rightNow, aRoom *roomData, char *buf);

#ifdef SYSTEM_DEPENDENT
/*
 * These functions are support functions for the SYSDEP stuff.
 */

int SearchFileComments(char *FileName);
char realSetSpace(char disk, char *dir);
void MSDOSparse(char *theDir, char *drive);
void doSendWork(char *filename, int (*fn)(char *fn));
int getModemId(void);
void initDirList(void);
int nodie(void);
int Control_C(void);
void SysWork(void (*form)(), char *cmdLine);
int specCmpU(char *f1, char *f2);
void setup_nocccb(void);
void VideoInit(void);
int fileType(char *drive, char dir);
int goodArea(char *prompt, char *dir, char *drive, char fullPath);

#ifdef TIMER_FUNCTIONS_NEEDED
        /* Timer support function prototypes */
long timeSince(struct timePacket *Slast);
long milliTimeSince(struct timePacket *Slast);

void setTimer(struct timePacket *Slast);

#endif
        /* IBM support function prototypes */
void setInterrupts(void);

        /* IBM video function prototypes (see CITVID.H) */
char vputch(unsigned char c);
void video(char *tagline);
void statusline(char *tagline);

        /* Z-100 support function prototypes */
int mGetch(void);
int mHasch(void);
int mPutch(int c);
int mHasout(void);
int mInit(int port, int baud, int parity, int stopbits, int wordlen, int xon);

void Zreset_video(void);
void Zsline(char *data);
void Zvideo(char *tagline);
void Zreset_video(void);
void mClose(void);

        /* Miscellaneous prototypes */
void diskSpaceLeft(char drive, long *sectors, long *bytes);
int  check_CR(void);

#endif

#else

FILE *safeopen();

long CurAbsolute(),
     sysRoomLeft(),
     chkTimeSince();

AN_UNSIGNED inp();

char *prtNetArea();

#ifdef SYSTEM_DEPENDENT

int  nodie(),
     Control_C(),
     video(),
     statusline(),
     Zvideo(),
     Zsline(),
     Zreset_video();

char *gcdir(),

AN_UNSIGNED interpret();

long milliTimeSince(),
     timeSince(),
     dostounix();

#ifdef CONFIGURE

char *getcwd();

#endif

#endif

#endif
