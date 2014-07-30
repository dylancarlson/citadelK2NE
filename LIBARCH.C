/************************************************************************/
/*                              libarch.c                               */
/*      Archive handling for Citadel bulletin board system              */
/************************************************************************/

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 86Aug01 HAW  Created.                                                */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      addArchiveList()        add new archive to room archive list    */
/*      addToList()             common for addAr.. and initArch...      */
/*      findArchiveName()       gets requested archive name             */
/*      initArchiveList()       eat archive list                        */
/************************************************************************/
struct any_list Arch_base;

char *ArchFileName = "ctdlarch.sys";

extern CONFIG cfg;

/************************************************************************/
/*      findArchiveName() gets archive name for given room              */
/************************************************************************/
char *findArchiveName(i)
int i;
{
    return findListName(&Arch_base, i);
}

/************************************************************************/
/*      findListName() find target                                      */
/************************************************************************/
char *findListName(base, i)
struct any_list *base;
int i;
{
    struct any_list *rover;

    for (rover = base->next; rover != NULL; rover = rover->next) {
        if (rover->roomNo == i)
            return rover->archName;
    }
    return NULL;
}

/************************************************************************/
/*      initArchiveList() set up archive list                           */
/************************************************************************/
void initArchiveList()
{
    SYS_FILE filename;

    makeSysName(filename, ArchFileName, &cfg.roomArea);
    initList(filename, &Arch_base);
    SeparateValues(&Arch_base);
}

/************************************************************************/
/*      SeparateValues() separate #s from values                        */
/************************************************************************/
void SeparateValues(BaseList)
struct any_list *BaseList;
{
    struct any_list *rover;
    char            *runner, *b;

    for (rover = BaseList->next; rover != NULL; rover = rover->next) {
        rover->roomNo = atoi(rover->archName);
        for (runner = rover->archName; isdigit(*runner); runner++)
                ;
        runner++;       /* Just over space */
        b = GetDynamic(strLen(runner) + 1);
        strCpy(b, runner);
        free(rover->archName);
        rover->archName = b;
    }
}

/************************************************************************/
/*      initList() set up list                                          */
/************************************************************************/
char initList(fileName, base)
char *fileName;
struct any_list *base;
{
    FILE *fd;
    char name[120];
    int room;
    extern char *READ_TEXT;

    base->next = NULL;

    if ((fd = safeopen(fileName, READ_TEXT)) == NULL)
        return FALSE;

    room = 0;
    while (fgets(name, 118, fd) != NULL) {
        name[strLen(name) - 1] = 0;     /* Kill off excess newline */
        LaddToList(base, room++, name);
    }

    fclose(fd);
    return TRUE;
}

/************************************************************************/
/*      addArchiveList() add to archive list                            */
/************************************************************************/
char addArchiveList(room, fn)
int room;
char *fn;
{
    SYS_FILE filename;

    makeSysName(filename, ArchFileName, &cfg.roomArea);
    return AddElement(&Arch_base, filename, room, fn);
}

/************************************************************************/
/*      AddElement() Add a new member to the list                       */
/************************************************************************/
char AddElement(BaseList, filename, room, fn)
struct any_list *BaseList;
SYS_FILE filename;
int room;
char *fn;
{
    FILE        *fd;
    struct      any_list *rover;
    char        *format = "%d %s\n";
    char        replace;
    extern char *WRITE_TEXT, *APPEND_TEXT;

    replace = LaddToList(BaseList, room, fn);

    if ((fd = safeopen(filename, replace ? WRITE_TEXT : APPEND_TEXT))
                                                               == NULL) {
        mPrintf("?Couldn't open %s!\n ", filename);
        return FALSE;
    }

    if (replace) {
        for (rover = BaseList->next; rover != NULL; rover = rover->next) {
            fprintf(fd, format, rover->roomNo, rover->archName);
        }
    }
    else {
        fprintf(fd, format, room, fn);
    }

    fclose(fd);

    return TRUE;
}

/************************************************************************/
/*      addToList() Adds filename to archive list                       */
/************************************************************************/
int LaddToList(base, room, fn)
struct any_list *base;
int room;
char *fn;
{
    struct any_list *rover;
    char            found;

    for (rover = base, found = FALSE; rover->next != NULL;
                                                     rover = rover->next) {
        if (rover->next->roomNo == room) {
            found = TRUE;
            break;
        }
    }

    if (!found) {
        rover->next = (struct any_list *) GetDynamic(sizeof Arch_base);
        rover = rover->next;
        rover->next = NULL;
    }
    else {
        rover = rover->next;
        free(rover->archName);
    }

    rover->roomNo = room;
    rover->archName = GetDynamic(strLen(fn) + 1);
    strCpy(rover->archName, fn);
    return found;
}
