/************************************************************************/
/*                              virt.c                                  */
/*      Virtual room handler for Citadel-86.                            */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 88Apr01 HAW Final cleanup before release.                            */
/************************************************************************/

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*              (not available yet)                                     */
/************************************************************************/

VirtualRoom *VRoomTab;
VirtNet     *VirtNetList;

static char VirtualInUse = FALSE;
static int  VirtSize, VNetSize;

extern NetBuffer netBuf;
extern CONFIG    cfg;
extern int       thisNet;
extern FILE      *netLog;
extern char      inNet, netDebug;

/************************************************************************/
/*      VirtInit() Initializes the virtual room stuff, if available.    */
/*      The virtual room stuff is created by the virtadmn utility, so   */
/*      this is a non-fatal, non-warning failure -- the sysop doesn't   */
/*      even know that this stuff ain't here.                           */
/************************************************************************/
void VirtInit()
{
#ifndef NO_VIRTUAL_ROOMS
    FILE *fd;
    SYS_FILE fn;
    long size;
    extern char *R_W_ANY;

    makeVASysName(fn, "ctdlvrm.sys");
    if ((fd = safeopen(fn, R_W_ANY)) == NULL)
        return;         /* Depend on initializer to handle VirtualInUse */

    totalBytes(&size, fd);
    VRoomTab = (VirtualRoom *) GetDynamic((int) size);
    fread((char *) VRoomTab, (int) size, 1, fd);
    fclose(fd);
    VirtSize = (int) size / sizeof *VRoomTab;
    VirtualInUse = TRUE;

    makeVASysName(fn, "ctdlvnet.sys");
    if ((fd = safeopen(fn, R_W_ANY)) == NULL)
        crashout("ctdlvnet.sys is missing!!");

    totalBytes(&size, fd);
    VirtNetList = (VirtNet *) GetDynamic((int) size);
    VNetSize = size / sizeof *VirtNetList;
    fread(VirtNetList, (int) size, 1, fd);
    fclose(fd);
#endif
}

/************************************************************************/
/*      InitVNode() When a new node is added to the net list, this      */
/*      function initializes the virtual part of the new node.  This    */
/*      consists of enlarging the vnet table size if necessary, and     */
/*      initializing the room pointers to -1, indicating that none of   */
/*      them are in use.  Finally, the virtual tables on disk are       */
/*      updated.                                                        */
/************************************************************************/
void InitVNode(slot)
int slot;
{
#ifndef NO_VIRTUAL_ROOMS
    int rover;

    if (!VirtualInUse) return ;

    if (slot >= VNetSize) {
        VirtNetList = realloc(VirtNetList, (slot+1) * sizeof *VirtNetList);
        VNetSize = slot + 1;
    }
    for (rover = 0; rover < VIRT_LIMIT; rover++)
        VirtNetList[slot].VirtList[rover].WhichVirt = -1;
    UpdVirtStuff();
#endif
}

/************************************************************************/
/*      UpdVirtStuff() Updates the virtual data on disk.                */
/************************************************************************/
void UpdVirtStuff()
{
#ifndef NO_VIRTUAL_ROOMS
    FILE *fd;
    SYS_FILE fn;
    extern char *R_W_ANY;

    if (!VirtualInUse) return ;

    VirtSummary();

    makeVASysName(fn, "ctdlvrm.sys");
    if ((fd = safeopen(fn, R_W_ANY)) == NULL)
        crashout("ctdlvrm.sys is missing!");

    fwrite(VRoomTab, VirtSize, sizeof *VRoomTab, fd);
    fclose(fd);

    makeVASysName(fn, "ctdlvnet.sys");
    if ((fd = safeopen(fn, R_W_ANY)) == NULL)
        crashout("ctdlvnet.sys is missing!!");

    fwrite(VirtNetList, cfg.netSize, sizeof *VirtNetList,  fd);
    fclose(fd);
#endif
}

/************************************************************************/
/*      VirtualNeed() Checks to see if node needs calling.              */
/************************************************************************/
char VirtualNeed(NetNo)
int NetNo;              /* Assume node is active */
{
#ifndef NO_VIRTUAL_ROOMS
    int rover;

    if (!VirtualInUse) return FALSE;

    for (rover = 0; rover < VIRT_LIMIT; rover++) {
        if (VNeedCall(VirtNetList[NetNo].VirtList + rover)) {
if (inNet != NON_NET && netDebug) {
ExpVirt(VirtNetList[NetNo].VirtList + rover);
}
            return TRUE;
        }
    }
    return FALSE;
#else
    return FALSE;
#endif
}

#ifndef NO_VIRTUAL_ROOMS
ExpVirt(VirtP)
VirtPoint *VirtP;
{
splitF(netLog, "Returning TRUE for %s\n", VRoomTab[VirtP->WhichVirt].vrName);
if (VirtP->mode == ACTIVE_BACKBONE)
splitF(netLog, "is ACTIVE BACKBONE\n");
else {
    splitF(netLog, "P->LocSent=%ld, VrHiLocal=%ld\n", VirtP->LocSent,
                 VRoomTab[VirtP->WhichVirt].vrHiLocal);
    splitF(netLog, "P->LDSent=%ld, VrHiLD=%ld\n", VirtP->LDSent,
                 VRoomTab[VirtP->WhichVirt].vrHiLD);
}
}
#endif

/************************************************************************/
/*      VNeedCall() Checks to see if list element needs calling         */
/************************************************************************/
char VNeedCall(VirtP)
VirtPoint *VirtP;
{
#ifndef NO_VIRTUAL_ROOMS
    if (VirtP->WhichVirt == -1 ||
        !VRoomInuse(VirtP->WhichVirt)) return FALSE;

    switch (VirtP->mode) {
        /*
         * For active backbone, we assume that we are always called via
         * roomsShared(), which will keep us from calling more than once.
         * Since an Active should always call once, we return TRUE here
         * without actually checking values.
         */
        case ACTIVE_BACKBONE:
            return TRUE;
        case PASS_BACKBONE:   return FALSE;
        case PEON:      /* This doesn't seem right! */
#ifdef OLD_STYLE
                return (VirtP->LocSent < VRoomTab[VirtP->WhichVirt].vrHiLocal ||
                          VirtP->LDSent < VRoomTab[VirtP->WhichVirt].vrHiLD);
#else
                return (VirtP->LDSent < VRoomTab[VirtP->WhichVirt].vrHiLD);
#endif
    }
    return FALSE;
#else
    return FALSE;
#endif
}

/************************************************************************/
/*      VirtualShared() returns index into current net's virtual index  */
/************************************************************************/
int VirtualShared(NetNo, name)
int NetNo;
label name;
{
#ifndef NO_VIRTUAL_ROOMS
    int rover, VirtNo;

    if ((VirtNo = VirtualExists(name)) == ERROR) return ERROR;

    for (rover = 0; rover < VIRT_LIMIT; rover++)
        if (VirtNetList[NetNo].VirtList[rover].WhichVirt == VirtNo)
            return rover;
    return ERROR;
#else
    return ERROR;
#endif
}

/************************************************************************/
/*      VirtualExists() Returns index to given room, if exists          */
/************************************************************************/
int VirtualExists(name)
label name;
{
#ifndef NO_VIRTUAL_ROOMS
    int rover;

    for (rover = 0; rover < VirtSize; rover++)
        if (strCmpU(VRoomTab[rover].vrName, name) == SAMESTRING) return rover;

    return ERROR;
#else
    return ERROR;
#endif
}

/************************************************************************/
/*      SendVirtual() Manages sending a room to another system          */
/************************************************************************/
int SendVirtual(VirtIndex, d1, d2, d3)                /* roomshare */
char *d1, *d2, *d3;     /* Dummies -- addresses for CTDLMSG msgs */
int  VirtIndex;
{
#ifndef NO_VIRTUAL_ROOMS
    int        VirtNo, count;
    MSG_NUMBER StartMsg;

    VirtNo = VirtNetList[thisNet].VirtList[VirtIndex].WhichVirt;

    splitF(netLog, "Sending %s (virtual) ", VRoomTab[VirtNo].vrName);

            /* Unless we are host, send all of the new LD messages received */
    if (VirtNetList[thisNet].VirtList[VirtIndex].mode != REG_HOST) {
        StartMsg = VirtNetList[thisNet].VirtList[VirtIndex].LDSent;
        count = ThrowAll(VirtNo, LD_DIR, StartMsg, VRoomTab[VirtNo].vrHiLD);
        VirtNetList[thisNet].VirtList[VirtIndex].LDSent =
                                 VRoomTab[VirtNo].vrHiLD;
    }

    if (VirtNetList[thisNet].VirtList[VirtIndex].mode != PEON) {
        StartMsg = VirtNetList[thisNet].VirtList[VirtIndex].LocSent;
        count += ThrowAll(VirtNo, LOCAL_DIR, StartMsg,
                                                VRoomTab[VirtNo].vrHiLocal);
        VirtNetList[thisNet].VirtList[VirtIndex].LocSent =
                                 VRoomTab[VirtNo].vrHiLocal;
    }
    VRoomTab[VirtNo].vrChanged |= SENT_DATA;
    return count;
#else
    return 0;
#endif
}

/************************************************************************/
/*      ThrowAll() Sends a virtual room to another system.              */
/************************************************************************/
int ThrowAll(which, distance, start, end)
int which;
char *distance;
MSG_NUMBER start, end;
{
#ifndef NO_VIRTUAL_ROOMS
    MSG_NUMBER  rover;
    int         count=0;
    char        fn[100];
    extern char *READ_ANY;
    extern FILE *netMisc;

    for (rover = start + 1; rover <= end; rover++) {
        CreateVAName(fn, which, distance, rover);
        if ((netMisc = safeopen(fn, READ_ANY)) != NULL) {
            while (getNetMessage(FALSE)) {
                count++;
                prNetStyle(getNetChar);
            }
            fclose(netMisc);
        }
    }
    return count;
#else
    return 0;
#endif
}

/************************************************************************/
/*      RecVirtualRoom() receives a virtual room from another system.   */
/************************************************************************/
int RecVirtualRoom(VirtIndex)
int VirtIndex;
{
#ifndef NO_VIRTUAL_ROOMS
    int         VirtNo;
    MSG_NUMBER  rover;
    char        *distance, fn[50];
    extern FILE *upfd;
    extern char *W_R_ANY;

    VirtNo = VirtNetList[thisNet].VirtList[VirtIndex].WhichVirt;

    if (VirtNetList[thisNet].VirtList[VirtIndex].mode != PEON) {
        distance = LD_DIR;
        rover = VRoomTab[VirtNo].vrHiLD + 1l;
        VRoomTab[VirtNo].vrChanged |= LD_CHANGE;
    }
    else {
        distance = LOCAL_DIR;
        rover = VRoomTab[VirtNo].vrHiLocal + 1l;
        VRoomTab[VirtNo].vrChanged |= LOC_CHANGE;
    }
    CreateVAName(fn, VirtNo, distance, rover);
    return (ITL_Receive(fn, FALSE) == ITL_SUCCESS) ? TRUE : FALSE;
#else
    return TRUE;
#endif
}

/************************************************************************/
/*      DoVirtuals() Sends rooms to another system, if needed.          */
/************************************************************************/
void DoVirtuals()
{
#ifndef NO_VIRTUAL_ROOMS
    int rover, cmd_mode, which;
    char doit;
    extern char inReceive;

    if (!VirtualInUse) return ;

    for (rover = 0; rover < VIRT_LIMIT; rover++) {
        doit = TRUE;
        if (VirtNetList[thisNet].VirtList[rover].WhichVirt != -1) {
            which = VirtNetList[thisNet].VirtList[rover].WhichVirt;
            switch (VirtNetList[thisNet].VirtList[rover].mode) {
                case PEON:
                    cmd_mode = NET_ROOM;
                    if (VirtNetList[thisNet].VirtList[rover].LDSent >=
                                VRoomTab[which].vrHiLD) doit = FALSE;
                    break;
                case PASS_BACKBONE:
                    if (!inReceive || VRoomTab[which].vrChanged & SENT_DATA)
                        doit = FALSE;
                    else
                        cmd_mode = NET_ROOM;
                    break;
                case ACTIVE_BACKBONE:
                    cmd_mode = (netBuf.nbflags.local) ?
                                          NET_ROOM : NET_ROUTE_ROOM;
                    break;
                default: splitF(netLog, "Error in virtuals!\n");
            }

            if (doit)
                findAndSend(cmd_mode, NULL, NULL, NULL, rover, SendVirtual,
                            VRoomTab[which].vrName, RecVirtualRoom);
        }
    }
#endif
}

/************************************************************************/
/*      VirtSummary() Handles post-call cleanup.                        */
/************************************************************************/
void VirtSummary()
{
#ifndef NO_VIRTUAL_ROOMS
    int rover, another;

    for (rover = 0; rover < VirtSize; rover++) {
        if (VRoomTab[rover].vrChanged & LD_CHANGE) {
            VRoomTab[rover].vrHiLD++;
            for (another = 0; another < VIRT_LIMIT; another++)
                if (VirtNetList[thisNet].VirtList[another].WhichVirt == rover) {
                    VirtNetList[thisNet].VirtList[another].LDSent =
                                                VRoomTab[rover].vrHiLD;
                    break;
                }
if (another == VIRT_LIMIT) printf("ERROR in LDVirtSummary!\n");
        }
        if (VRoomTab[rover].vrChanged & LOC_CHANGE) {
            VRoomTab[rover].vrHiLocal++;
            for (another = 0; another < VIRT_LIMIT; another++)
                if (VirtNetList[thisNet].VirtList[another].WhichVirt == rover) {
                    VirtNetList[thisNet].VirtList[another].LocSent =
                                                VRoomTab[rover].vrHiLocal;
                    break;
                }
if (another == VIRT_LIMIT) printf("ERROR in LocVirtSummary!\n");
        }
        VRoomTab[rover].vrChanged = 0;
    }
#endif
}

/************************************************************************/
/*      V_Listing() Lists virtual rooms shared with some node.          */
/************************************************************************/
void V_Listing()
{
#ifndef NO_VIRTUAL_ROOMS
    int rover, x;

    if (!VirtualInUse) return ;

    for (rover = 0; rover < VIRT_LIMIT; rover++) {
        if ((x = VirtNetList[thisNet].VirtList[rover].WhichVirt) != -1 &&
            VRoomInuse(VirtNetList[thisNet].VirtList[rover].WhichVirt)) {
            mPrintf("%s: ", VRoomTab[x].vrName);
            switch (VirtNetList[thisNet].VirtList[rover].mode) {
                case PEON: mPrintf("PEON relationship "); break;
                case ACTIVE_BACKBONE:
                        mPrintf("We are an ACTIVE BACKBONE "); break;
                case PASS_BACKBONE:
                        mPrintf("We are a PASSIVE BACKBONE "); break;
            }
            mPrintf("(last bb sent %ld, hi bb %ld",
VirtNetList[thisNet].VirtList[rover].LDSent, VRoomTab[x].vrHiLD);
            if (VirtNetList[thisNet].VirtList[rover].mode != PEON) {
                mPrintf(", last peon sent %ld, hi peon %ld",
VirtNetList[thisNet].VirtList[rover].LocSent, VRoomTab[x].vrHiLocal);
            }
            mPrintf(")\n ");
        }
    }
#else
    mPrintf("Virtual Rooms are not supported in this release.\n");
/*          " not support Virtual Rooms.\n"); */
#endif
}
