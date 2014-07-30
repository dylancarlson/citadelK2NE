/************************************************************************/
/*                              libroom.c                               */
/*                        Library for room code                         */
/************************************************************************/

/************************************************************************/
/*                              History                                 */
/*                                                                      */
/* 85Nov15 HAW  Create.                                                 */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      getRoom()               load given room into RAM                */
/*      putRoom()               store room to given disk slot           */
/************************************************************************/

aRoom         roomBuf;               /* Room buffer              */
extern rTable *roomTab;              /* RAM index                */
extern CONFIG cfg;
FILE                 *roomfl;               /* Room file descriptor     */
int                  thisRoom = LOBBY;      /* Current room             */

/************************************************************************/
/*      getRoom()                                                       */
/************************************************************************/
void getRoom(rm)
int rm;
{
    long int s;

    /* load room #rm into memory starting at buf */
    thisRoom    = rm;
    s = (long) ((long) rm * (long) RB_TOTAL_SIZE);
    fseek(roomfl, s, 0);

    if (fread(&roomBuf, RB_SIZE, 1, roomfl) != 1)   {
        crashout(" ?getRoom(): read failed//error or EOF (1)!");
    }
#ifndef NO_CRYPT
    crypte(&roomBuf, RB_SIZE, rm);
#endif

    if (fread(roomBuf.msg, MSG_BULK, 1, roomfl) != 1)   {
        crashout(" ?getRoom(): read failed//error or EOF (2)!");
    }
}

/************************************************************************/
/*      putRoom() stores room in buf into slot rm in room.buf           */
/************************************************************************/
void putRoom(rm)
int rm;
{
    long int s;

    s = (long) ((long) rm * (long) RB_TOTAL_SIZE);
    fseek(roomfl, s, 0);
#ifndef NO_CRYPT
    crypte(&roomBuf, RB_SIZE, rm);
#endif
    if (fwrite(&roomBuf, RB_SIZE, 1, roomfl) != 1)   {
        crashout("?putRoom() crash!//0 returned!!!(1)");
    }

    if (fwrite(roomBuf.msg, MSG_BULK, 1, roomfl) != 1)   {
        crashout("?putRoom() crash!//0 returned!!!(2)");
    }
#ifndef NO_CRYPT
    crypte(&roomBuf, RB_SIZE, rm);
#endif
}
