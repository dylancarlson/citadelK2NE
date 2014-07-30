/************************************************************************/
/*                              libnet.c                                */
/*                                                                      */
/*                       Library net functions                          */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 85Nov15 HAW  Created.                                                */
/************************************************************************/

#include "ctdl.h"

NetBuffer netBuf;
extern NetTable  *netTab;
extern CONFIG    cfg;
int              thisNet;
FILE             *netfl;

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      getNet()                gets a node from CTDLNET.SYS            */
/*      putNet()                puts a node to CTDLNET.SYS              */
/*      normId()                normalizes the ID                       */
/************************************************************************/

/************************************************************************/
/*      getNet() Gets a net node from the file.                         */
/************************************************************************/
void getNet(n)
int n;
{
    long int r, s;

    thisNet = n;
    r = NB_TOTAL_SIZE;
    s = n * r;

    fseek(netfl, s, 0);
    if (fread(&netBuf, NB_SIZE, 1, netfl) != 1) {
        crashout("?getNet-read fail(1)!!");
    }
#ifndef NO_CRYPT
    crypte(&netBuf, NB_SIZE, n);
#endif
    if (SR_BULK != 0) {
        if (fread(netBuf.netRooms, SR_BULK, 1, netfl) != 1) {
            crashout("?getNet-read fail(2)!!");
        }
    }

    if (NA_BULK != 0) {
        if (fread(netBuf.nbArchRooms, NA_BULK, 1, netfl) != 1) {
            crashout("?getNet-read fail(3)!!");
        }
    }
}

/************************************************************************/
/*      putNet() put node to file                                       */
/************************************************************************/
void putNet(n)
int n;
{
    long int r, s;
    label temp;

    thisNet = n;

    copy_struct(netBuf.nbflags, netTab[n].ntflags);
    movmem(netBuf.netRooms, netTab[n].netTRooms, SR_BULK);
    movmem(netBuf.nbArchRooms, netTab[n].ntArchRooms, NA_BULK);
    netTab[n].ntnmhash = hash(netBuf.netName);
    normId(netBuf.netId, temp);
    netTab[n].ntidhash = hash(temp);
    netTab[n].ntMemberNets = netBuf.MemberNets;

    r = NB_TOTAL_SIZE;
    s = n * r;
#ifndef NO_CRYPT
    crypte(&netBuf, NB_SIZE, n);
#endif
    fseek(netfl, s, 0);

    if (fwrite(&netBuf, NB_SIZE, 1, netfl) != 1) {
        crashout("?putNet-write fail(1)!!");
    }

    if (SR_BULK != 0)
        if (fwrite(netBuf.netRooms, SR_BULK, 1, netfl) != 1) {
            crashout("?putNet-write fail(2)!!");
        }

    if (NA_BULK != 0)
        if (fwrite(netBuf.nbArchRooms, NA_BULK, 1, netfl) != 1) {
            crashout("?putNet-write fail(3)!!");
        }
#ifndef NO_CRYPT
    crypte(&netBuf, NB_SIZE, n);
#endif
}

/************************************************************************/
/*      normId() Normalizes a node id.                                  */
/************************************************************************/
char normId(source, dest)
label source, dest;
{
    while (!isalpha(*source) && *source)
        source++;
    if (!*source) return FALSE;
    *dest++ = toUpper(*source++);
    while (!isalpha(*source) && *source)
        source++;
    if (!*source) return FALSE;
    *dest++ = toUpper(*source++);
    while (*source) {
        if (isdigit(*source))
            *dest++ = *source;
        source++;
    }
    *dest = '\0';
    return TRUE;
}
