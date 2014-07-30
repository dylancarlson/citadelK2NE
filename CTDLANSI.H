        /*
         * Use these only if we're (semi) ANSI capable, with prototypes.
         */
#ifdef ANSI_PROTOTYPING

        /* These functions are located in CTDL.C */

void doChat(char moreYet, char first);
void doEnter(char moreYet, char first);
void doForget(char expand);
void doGoto(char expand);
void doHelp(char expand);
void doKnown(char expand);
void doLogin(char moreYet);
void doLogout(char expand, char first);
void doRead(char moreYet, char first);
void greeting(void);
void doSkip(char expand);
void doBackupToRoom(char moreYet);
void doDirectory(char doDir, label fileName, char *phrase);

char doRegular(char x, char c);
char doSysop(char first);
char getCommand(char *c);
char doAide(char moreYet, char first);

        /* These functions are located in LOG.C */
void doInviteDisplay(void);
void login(char *password);
void newPW(void);
void newUser(void);
void terminate(char discon, char save);
void slideLTab(int slot, int last);
void storeLog(void);

int PWSlot(char pw[NAMESIZE], char load);
int strCmpU(char s[], char t[]);
int findPerson(char *name, logBuffer *lBuf);

        /* These functions are located in MSG.C */
void aideMessage(char noteDeletedMessage);
void doFlush(FILE *whichmsg, struct mBuf *mFile);
void fakeFullCase(char *text);
void flushMsgBuf(void);
void makeMessage(char uploading);
void mFormat(char *string);
void mPeek(void);
void msgToDisk(char *filename, MSG_NUMBER id, SECTOR_ID loc);
void noteLogMessage(logBuffer *lBuf, int logNo);
void noteMessage(logBuffer *lBuf, int logNo);
void note2Message(MSG_NUMBER id, SECTOR_ID loc);
void note3message(int logNo, logBuffer *lBuf);
void printMessage(SECTOR_ID loc, MSG_NUMBER id, char net_format);
void netMailProcess(int netPlace);
void netMailOut(void);
void putWord(char *st);
void showMessages(char whichMess, char revOrder, char allOrLocal,
                                     label date, label user, char *phrase);
void CheckForwarding(logBuffer *lbuf);
void prNetStyle(int (*SourceFn)());

char redirect(int);
char deleteMessage(int m);
char canRespond(void);
char dGetWord(char *dest, int lim);
char doActualWrite(FILE *whichmsg, struct mBuf *mFile, char c);
char mAbort(void);
char getRecipient(logBuffer *lBuf, int *logNo);
char replyMessage(void);
char hldMessage(void);
char idiotMessage(void);
char procMessage(char uploading);
char moveMessage(int m);
char findMessage(SECTOR_ID loc, MSG_NUMBER id);
char pullIt(int m);
char putMessage(void);

int getWord(char *dest, char *source, int offset, int lim);
int putMsgChar(char c);

        /* These functions are located in MISC.C */
void changeDate(void);
void civTime(int *hours, char **which);
void configure(char showVals);
void crashout(char *message);
void doCR(void);
void download(char whichMess, char revOrder, char whatkind, char allOrLocal,
                                        label date, label user, char *phrase);
void doGlobal(char revOrder, char allOrLocal, label date, label user,
                                                        char *phrase);
void getCdate(int *year, char **month, int *day, int *hours, int *minutes);
void ingestFile(char *name);
void reconfigure(void);
void SaveInterrupted(MSG_BUF *SomeMsg);
void TranFiles(int protocol, char *phrase);
void TranSend(int protocol, int (*fn)(char *fn), char *filespec, char *phrase,
                                                        char NeedToMove);
void upLoad(char WC);
void writeTutorial(FILE *fd, char noviceWarning);
void XTime(long size, long *seconds);
void YTime(long size, long *seconds);
void WXTime(long size, long *seconds);

char *formHeader(void);
char HelpIfPresent(char *filename);
char *lbyte(char *l);
char *formRoom(int roomNo, int showPriv, int noDiscrimination);
char *formDate(void);
char tutorial(char *filename, char addHelpArea);
char visible(AN_UNSIGNED c);
char CheckDLimit(long estimated);
char TranAdmin(int protocol, int NumFiles);

int ARCDir(char *fn);
int doFormatted(char *fn);
int putBufChar(int c);
int putFLChar(int c);
int transmitFile(char *filename);

unsigned int calcrc(unsigned char *ptr, int count);

        /* These functions are located in ROOMA.C */
void dumpRoom(char ShowFloor);
void fillMailRoom(void);
void initCitadel(void);
void retRoom(char *roomName);
void searchRooms(void);
void setUp(char justIn);
void systat(void);
void UngotoMaintain(int lRoom);
void listRooms(int mode);

char legalMatch(int i, label target);
char knowRoom(int i);

int fDir(char *fileName);
int SkippedNewRoom(int i);
int gotoRoom(char *nam, char mode);
int tableRunner(int (*func)(), char OnlyKnown);
int partialExist(label target);
int roomCheck(int (*checker)(), char *nam);
int roomExists(char *room);
int ShowVerbose(char *fn);
int sortDir(struct dirList *s1, struct dirList *s2);
int wildCard(int (*fn)(), char *filename, char needToMove, char *phrase);
int wild2Card(struct dirList list[], int fileCount, int (*fn)(char *fn),
                                                                char *phrase);

        /* These functions are located in ROOMB.C */
char conGetYesNo(char *prompt);
char coreGetYesNo(char *prompt, int consoleOnly);
char getText(char uploading);
char getYesNo(char *prompt);
char *matchString(char *buf, char *pattern, char *bufEnd);
char renameRoom(void);
char *formatSummary(char *buffer);
char
getXString(char *prompt, char *target, int targetSize, char *CR_str, 
                                                                char *dft);

int editText(char *buf, int lim);
int findRoom(void);
int knownHosts(char *name);
int addToList(char *name);
int makeKnown(char *user);
int makeUnknown(char *user);
int doMakeWork(char *user, int val);
int killFromList(char *sysName);
int searchForRoom(void);

long getNumber(char *prompt, long bottom, long top);

void
getString(char *prompt, char *buf, int lim, char QuestIsSpecial, char doEcho);
void givePrompt(void);
void indexRooms(void);
void insertParagraph(char *buf, int lim);
void makeRoom(void);
void getNormStr(char *prompt, char *s, int size, char doEcho);
void noteRoom(void);
void getList(int (*fn)(), char *prompt);
void replaceString(char *buf, int lim);
void initialArchive(char *fn);

        /* These functions are located in FLOORS.C */
int CheckFloor(int i);
int FindFloor(label name, char doPartial);
int FloorRunner(int start, int (*func)());
int NewRoom(void);
int FirstRoom(int FloorNo);
int RoomHasNew(int i);
int NSRoomHasNew(int i);
int DoFloors(void);
int Zroom(int i);
int FSroom(int i);
int FindAny(int i);
int FAide(void);
int MoveToFloor(char *name);
int DispRoom(int i);
int MaimOrKill(int i);

void DispFloorName(int FloorNo);
void FSkipped(void);
void FForget(void);
void FConfigure(void);
void FGotoSkip(char mode);
void DeleteFloors(void);
void MoveRooms(void);
void RenameFloor(void);
void CreateFloor(void);
void putFloor(int i);
void KillFloor(void);
void FKnown(char mode);

        /* These functions are located in EVENTS.C */
void InitEvents(char SetRel);
void setPtrs(void);
void EventShow(void);

char DoTimeouts(void);
char HandleQuiet(int index);
char *ChkPreempt(long estimated);
char AlreadyProcessed(EVENT *s);

long WeekDiff(long future, long now);

int during(EVENT *x);
int passed(EVENT *x);
int eventSort(EVENT *s1, EVENT *s2);

#ifdef CONFIGURE
        /* These functions are located in CONFG2.C */
int EatEvent(char *line, int offset);
int GetStoreQuote(char *line, int *rover, int *offset);
int AddString(char *str, int offset);
int FigureDays(char *vals);

char *getLVal(char *line, int *rover, char fin);

MULTI_NET_DATA FigureNets(char *str);

void putEvent(void);

        /* These functions are located in CONFG.C */
void init(int attended);
void readString(char *source, char *destination, char doProc);
void xlatfmt( char *s );
void illegal(char *errorstring);
void msgInit(void);
void indexRooms(void);
void noteRoom(void);
void logInit(void);
void noteLog(void);
void slideLTab(int slot, int last);
void wrapup(char onlyParams);
void netInit(void);
void crashout(char *str);
void CheckFloors(void);

char isoctal( int c );
char dGetWord(char *dest, int lim);
char zapMsgFile(void);
char realZap(void);
char zapRoomFile(void);
char zapLogFile(void);
char findMessage(SECTOR_ID loc, MSG_NUMBER id);

int logSort(LogTable *s1, LogTable *s2);
int msgSort(theMessages *s1, theMessages *s2);
int strCmpU(char s[], char t[]);

MSG_NUMBER findHighestNative(void);

#endif

        /* These functions are located in MODEM.C */
char BBSCharReady(void);
char getMod(void);
char iChar(void);
char JumpStart(int tries, int timeout, int Starter, int t1, int t2,
        char (*Method)(int (*wrt)(int c)), int (*WriteFn)(int c));
char Reception(char protocol, int (*WriteFn)(int c));
char recWX(int (*WriteFn)(int c));
char recXYmodem(int (*WriteFn)(int c));
char sendWXchar(int data);
char Transmission(char protocol, char mode);
char XYBlock(int mode, int size);
char YMHdr(long fileSize, char *filename);

void FlowControl(void);
void GenTrInit(void);
void initTransfers(void);
void interact(char ask);
void interOut(char c);
void modemInit(void);
void oChar(char c);
void ringSysop(void);
void runHangup(void);
void SendCmnBlk(char type, TransferBlock *block, char (*SendFn)(int c),
                                                                int size);
void SummonSysop(void);
void WXResponses(void);

int ClearWX(void);
int CommonPacket(char type, int size, int (*recFn)(int t), int *Sector);
int CommonWrite(int (*WriteFn)(int c), int size);
int getSize(char *fileName);
int recWXchar(int ErrorTime);
int sendWCChar(int c);
int sendWXModem(int c);
int sendYMChar(int c);
int XYClear(void);

AN_UNSIGNED modIn(void);

        /* These functions are located in LIBROOM.C */
void getRoom(int rm);
void putRoom(int rm);

        /* These functions are located in LIBTABL.C */
char readSysTab(char kill, char showMsg);
char *GetDynamic(unsigned size);

int writeSysTab(void);
int common_read(void *block, int size, int elements, FILE *fd, char showMsg);

void openFile(char *filename, FILE **fd);

        /* These functions are located in LIBLOG.C */
void getLog(logBuffer *lBuf, int n);
void putLog(logBuffer *lBuf, int n);

        /* These functions are located in LIBCRYP.C */
void crypte(AN_UNSIGNED *buf, unsigned len, unsigned seed);
int hash(char *str);

        /* These functions are located in LIBMSG.C */
void getMessage(void);
void getMsgStr(char *dest, int lim);
void startAt(FILE *whichmsg, struct mBuf *mFile, SECTOR_ID sect, int byt);
void unGetMsgChar(char c);

int getMsgChar(void);

        /* These functions are located in LIBNET.C */
void getNet(int n);
void putNet(int n);

char normId(label source, label dest);

        /* These functions are located in LIBARCH.C */
char *findArchiveName(int i);
char *findListName(struct any_list *base, int i);
char initList(char *fileName, struct any_list *base);
char addArchiveList(int room, char *fn);
char AddElement(struct any_list *BaseList, SYS_FILE filename, int room, 
                                                                char *fn);

void initArchiveList(void);
void SeparateValues(struct any_list *BaseList);

int  LaddToList(struct any_list *base, int room, char *fn);

        /* These functions are located in CALLLOG.C */
void logMessage(char val, char *str, char sig);
void CallMsg(char *str);

        /* These functions are located in NETMISC.C */
char called_stabilize(void);
char check_for_init(char mode);
char getNetMessage(char all);
char callOut(int i);
char roomsShared(int slot);
char netInfo(void);
char AnyCallsNeeded(MULTI_NET_DATA whichNets);

int ARCread(char *fn);
int AckStabilize(void);
int searchNet(char *forId);
int timeLeft(void);
int searchNameNet(label name);
int addNetMem(char *netnum);
int makeCall(char checkkbd);
int subNetMem(char *netnum);
int getNetChar(void);
int needToCall(int system, MULTI_NET_DATA CurrentNets);

void NodeValues(void);
void sendWCFile(FILE *fd);
void readMail(char zap, void (*procFn)());
void getNetStr(char *place, int length);
void inMail(void);
void netController(int NetStart, int NetLength, MULTI_NET_DATA whichNets,
                                                        char mode);
void initNetRooms(void);
void setTime(int NetStart, int NetLength);
void moPuts(char *s);
void netMessage(char uploading);
void writeNet(char idsAlso);
void netStuff(void);
void getSendFiles(void);
void addNetNode(void);
void editNode(void);
void fileRequest(void);
#ifdef NET_BUG
void dumpNodeRoom(char file);
#else
void dumpNodeRoom();
#endif
void netResult(char *msg);
void killConnection(void);
void setPoll(void);
void EvalNeed(int searcher, MULTI_NET_DATA whichNets);
void parseBadRes(char *c);      /* temporary until next major release */

        /* These functions are located in NETCALL.C */
void caller(void);
void sendStuff(char reversed, char SureDoIt);
void SendPwd(void);
void roleReversal(char reversed);
void caller_stabilize(void);
void sendId(void);
void sendMail(void);
void checkMail(void);
void readNegMail(void);
void sendSharedRooms(void);
void findAndSend(int commnd, char *send1, char *send2, char *send3, int rover,
int (*MsgSender)(), label roomName, int (*MsgReceiver)());
void doSendFiles(void);
void askFiles(void);
void multiReceive(struct fl_req *file_data);
void sendHangUp(void);
void no_good(char *str, char hup);
void SendHostFile(char *fn);

char sendNetCommand(struct cmd_data *cmds, char *error);

int netSendFile(char *fn);
int s_m_n(void);
int RoomSend(int rover, char *send1, char *send2, char *send3);
int RoomReceive(int rover);

        /* These functions are located in NETRCV.C */
void called(void);
void rcvStuff(char reversed);
void netPwd(struct cmd_data *cmds);
void doResults(void);
void getId(void);
void getNextCommand(struct cmd_data *cmds);
void grabCommand(struct cmd_data *cmds, char *sect);
void reply(char state, char *reason);
void reqReversal(struct cmd_data *cmds, char reversed);
void reqCheckMail(struct cmd_data *cmds);
void targetCheck(void);
void doNetRooms(void);
void getMail(struct cmd_data *cmds);
void reqSendFile(struct cmd_data *cmds);
void netFileReq(struct cmd_data *cmds);
void netRRReq(struct cmd_data *cmds);
void netRoomReq(struct cmd_data *cmds);
void recNetMessages(int arraySlot, char *name, int RoomNo);

int netMultiSend(char *fn);
int roomRoutable(char *name, char *reason, int *arraySlot);
int chkSharing(int i);

        /* These functions are located in VIRT.C */
char VNeedCall(VirtPoint *VirtP);
char VirtualNeed(int NetNo);

void VirtSummary(void);
void V_Listing(void);
void InitVNode(int slot);
void VirtInit(void);
void UpdVirtStuff(void);
void DoVirtuals(void);

int ThrowAll(int which, char *distance, MSG_NUMBER start, MSG_NUMBER end);
int RecVirtualRoom(int VirtIndex);
int SendVirtual(int VirtIndex, char *d1, char *d2, char *d3);
int VirtualShared(int NetNo, label name);
int VirtualExists(label name);

                /* These functions reside in NETITL.C */
char ITL_Receive(char *FileName, char ReplyFirst);
char ITL_Send(char mode);
char sendITLchar(int c);

int increment(int c);

void ITL_InitCall(void);
void ITL_DeInit(void);
void ITL_optimize(void);
void ITL_rec_optimize(struct cmd_data *cmds);


#else
        /*
         * Else, we need these in place.
         * NOTE: This file has not been compiled with ANSI_PROTOTYPING
         * not defined!
         */

char *GetDynamic(),
     *formRoom(),
     *formDate(),
     *ChkPreempt(),
     *lbyte(),
     *matchString(),
     *formatSummary(),
     *findArchiveName(),
     *formHeader(),
     *findListName();

        /*
         * "standard functions"
         */
char *fgets(),
     *strchr(),
     *strcat();

char iChar(),
     BBSCharReady(),
     getCommand(),
     getYesNo(),
     mAbort(),
     getMod(),
     coreGetYesNo(),
     dGetWord(),
     normId(),
     getNetMessage(),
     callOut(),
     putMessage(),
     conGetYesNo();

long getNumber(),
     WeekDiff();

int  roomExists(),
     Xtime(),
     Ytime(),
     eventSort(),
     increment(),
     partialExist(),
     ShowZF(),
     sortDir(),
     inMail(),
     targetCheck(),
     netMultiSend(),
     RoomSend(),
     RoomReceive(),
     mPrintf(),
     getNetChar();

        /*
         * More "standard functions"
         */
int  printf();

        /* 
         * These are used (mostly) with the wildCard() function
         */
int  fDir(),
     ARCdir(),
     putFLChar(),
     putBufChar(),
     getSize(),
     netSendFile(),
     ShowVerbose();

        /*
         * These are used with the floorRunner() function.
         */
int  NSRoomHasNew(),
     SkippedNewRoom(),
     Zroom(),
     FindAny(),
     FSroom(),
     MoveToFloor(),
     MaimOrKill(),
     DispRoom(),
     RoomHasNew(),
     CheckFloor();

        /*
         * These are used for getList() calls
         */
int  addToList(),
     makeKnown(),
     makeUnknown(),
     knownHosts(),
     killFromList(),
     addNetMem(),
     subNetMem();

AN_UNSIGNED modIn();

void *realloc();        /* This may not be necessary. */

#ifdef CONFIGURE

MSG_NUMBER findHighestNative();

int msgSort(), logSort();

char *getLVal();

MULTI_NET_DATA FigureNets();

#endif

#endif
