
#define LD_DIR          "backbone"
#define LOCAL_DIR       "peon"

#define LD_CHANGE       0x01    /* Bit settings */
#define LOC_CHANGE      0x02
#define SENT_DATA       0x04

typedef struct {
    label       vrName;         /* Name of the fiendish entity */
                        /* If this one not in use, make strLen(vrName) == 0 */
    MSG_NUMBER  vrHiLocal,      /* # of current highest from local msg-file */
                vrHiLD,         /* # of current highest from LD msg-file */
                vrLoLocal,      /* # of current lowest from local msg-file */
                vrLoLD;         /* # of current lowest from LD msg-file */
   char         vrChanged;      /* temporary for changes */
} VirtualRoom;

#define VRoomInuse(x)           strLen(VRoomTab[x].vrName)
#define VRoomKill(x)            VRoomTab[x].vrName[0] = 0

typedef struct {
    int         WhichVirt;      /* Which virtual room? */
    MSG_NUMBER  LocSent, LDSent;/* last msg-file sent */
    char        mode;           /* Host or backbone of some sort */
} VirtPoint;

#define VIRT_LIMIT 15           /* CTDLCNFG.SYS on next major version     */

typedef struct {
    VirtPoint VirtList[VIRT_LIMIT];
} VirtNet;

#define NORMAL          0
#define BATCH           1

#ifdef V_ADMIN

typedef struct {
    char uses, mode;
    MSG_NUMBER WhenNormal;
    int VirtIndex;
} TempData;

#ifdef ANSI_PROTOTYPING

TempData *SetNtoVList(int roomNo, int VNo);

void ConNorVa(void);
void GenInit(void);
void UpdVirtStuff(void);
void AnalyzeArguments(int argc, char **argv);
void InteractMode(void);
void BatchMode(void);
void AddRoom(void);
void InitVirtual(void);
void Delete(void);
void Modify(void);
void Display(void);
void ResetNodes(int slot);
void RunList(int index, char *prompt, int (*func)());

int searchNameNet(label name);
int RemoveNodes(int index, label name);
int AddNodes(int index, label name);
int VirtualExists(label name);
int DoesShare(int NodeNo, int RoomNo);
int NorToVirtual(int VirtNo, TempData *list);
int ChgNodes(int index, label name);

char ReadMessage(int rover);
char GetConString(char *prompt, char *buffer, int length);

#endif

#endif
