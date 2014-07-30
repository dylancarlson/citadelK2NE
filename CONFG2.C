/************************************************************************/
/*                              confg2.c                                */
/*      configuration program for Citadel bulletin board system.        */
/************************************************************************/

#define CONFIGURE

#include "ctdl.h"

/************************************************************************/
/*                              History                                 */
/*                                                                      */
/* 87Oct02 HAW  Created.                                                */
/************************************************************************/

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/************************************************************************/



#define PARA1 "\nWARNING: An event of Class dl-time should only use type quiet\n"
#define PARA2 "\nWARNING: An event of Class anytime-net should only use type quiet\n"

struct GenList {
    char *GenName;
    int  GenVal;
} ;

static struct GenList EvnDays[] = {
        { "Sun", SUNDAYS },
        { "Mon", MONDAYS },
        { "Tue", TUESDAYS },
        { "Wed", WEDNESDAYS },
        { "Thu", THURSDAYS },
        { "Fri", FRIDAYS },
        { "Sat", SATURDAYS },
        { "All", ALL_DAYS }
} ;

static struct GenList EvnTypes[] = {
        { "preempt", TYPREEMPT },
        { "non-preempt", TYNON },
        { "quiet", TYQUIET }
} ;

static struct GenList EvCls[] = {
        { "network", CLNET },
        { "external", CLEXTERN },
        { "relative", CLREL },
        { "dl-time", CL_DL_TIME },
        { "anytime-net", CL_ANYTIME_NET }
} ;

EVENT EvBuf;
extern CONFIG    cfg;       /* The configuration variable   */
extern EVENT     *EventTab;

/************************************************************************/
/*      EatEvent() assimilates event parameters.  Format:               */
/*#event <days> <time> <class> <type> <duration> <warning string> <dep> */
/************************************************************************/
int EatEvent(line, offset)
int  offset;
char *line;
{
    char *ptr;
    int rover, i, Day;
    int EvHour, EvMin;

    rover = 6;
    Day             = FigureDays(getLVal(line, &rover, ' '));
    EvHour = atoi(getLVal(line, &rover, ':'));
    EvMin = atoi(getLVal(line, &rover, ' '));
    EvBuf.EvMinutes = EvHour * 60 + EvMin;
    ptr             = getLVal(line, &rover, ' ');
    EvBuf.EvClass   = checkList(ptr, EvCls, NumElems(EvCls));
    if (EvBuf.EvClass != CLREL) {
        if (EvHour > 23 || EvMin > 59)
            illegal("Bad time specified for an event!\n");
        EvBuf.EvMinutes = EvHour * 60 + EvMin;
    }
    ptr             = getLVal(line, &rover, ' ');
    EvBuf.EvType    = checkList(ptr, EvnTypes, NumElems(EvnTypes));
    EvBuf.EvDur     = atoi(getLVal(line, &rover, ' '));
    EvBuf.EvWarn    = GetStoreQuote(line, &rover, &offset);
    switch (EvBuf.EvClass) {
        case CL_ANYTIME_NET:
        case CLNET:
            EvBuf.EvExitVal = FigureNets(getLVal(line, &rover, ' '));
            if (EvBuf.EvClass == CL_ANYTIME_NET && EvBuf.EvType != TYQUIET) {
                printf(PARA2);
                EvBuf.EvType = TYQUIET;
            }
            break;
        case CLREL:
        case CLEXTERN:
        case CL_DL_TIME:
            EvBuf.EvExitVal = (MULTI_NET_DATA) atoi(getLVal(line, &rover, ' '));
            if (EvBuf.EvType != TYQUIET && EvBuf.EvClass == CL_DL_TIME) {
                printf(PARA1);
                EvBuf.EvType = TYQUIET;
            }
            if (EvBuf.EvClass != CL_DL_TIME) {
                if (EvBuf.EvExitVal >=0 && EvBuf.EvExitVal < 5)
                    printf("\n\007WARNING: Event ERRORLEVEL value is "
                           "between 0 and 4, all of which are used by "
                           "Citadel.\007\n");
            }
            break;
    }

    if (EvBuf.EvClass != CLREL) {
        for (rover = 0, i = 1; rover < 7; EvBuf.EvMinutes += 1440, rover++) {
            if (Day & i) {
                putEvent();
            }
            i = i << 1;
        }
    }
    else {
        EvBuf.EvDur = EvHour * 60 + EvMin;
        putEvent();
    }

    return offset;
}

int checkList(ptr, listing, elements)
char *ptr;
struct GenList listing[];
int elements;
{
    int rover;
    char message[100];

    for (rover = 0; rover < elements; rover++)
        if (strCmpU(ptr, listing[rover].GenName) == SAMESTRING)
            return listing[rover].GenVal;

    sPrintf(message, "'%s' is not recognized!\n", ptr);
    illegal(message);
    return ERROR;
}

char *getLVal(line, rover, fin)
char *line, fin;
int  *rover;
{
    static char retVal[75];
    int         i;

    if (!line[*rover]) {
        retVal[0] = 0;
        return retVal;
    }
    if (line[*rover] != '\n')
        (*rover)++;
    while (line[*rover] == ' ') (*rover)++;
    i = 0;
    while (line[*rover] != fin && line[*rover] != '\n') {
        retVal[i++] = line[*rover];
        (*rover)++;
    }
    retVal[i] = 0;
    return retVal;
}

MULTI_NET_DATA FigureNets(str)
char *str;
{
    MULTI_NET_DATA retVal, r;
    int temp;

    retVal = 0l;
    while (*str) {
        temp = atoi(str);
        if (temp < 1 || temp > 31) illegal("Bad net value");
        r = 1l;
        retVal = retVal + (r << (temp - 1));
        while (*str != ',' && *str) str++;
        if (*str) str++;
    }
    return retVal;
}

int GetStoreQuote(line, rover, offset)
char *line;
int  *rover, *offset;
{
    int OldOffset;

    OldOffset = *offset;
    while (line[*rover] == ' ') (*rover)++;
    if (line[*rover] != '\"')
        illegal("Expecting a quote mark in event processor!\n");
    (*rover)++;
    if (line[*rover] == '\"') return ERROR;
    while (line[*rover] != '\"' && line[*rover] != '\r') {
        cfg.codeBuf[(*offset)++] = line[*rover];
        (*rover)++;
    }
    cfg.codeBuf[(*offset)++] = 0;
    return OldOffset;
}

int AddString(str, offset)
char *str;
int  offset;
{
    for (; *str; str++)
        cfg.codeBuf[offset++] = *str;
    cfg.codeBuf[offset++] = 0;
    return offset;
}

int FigureDays(vals)
char *vals;
{
    int results, rover;
    char val[4];

    results = 0;
    while (*vals) {
        for (rover = 0; rover < 3; rover++)
            val[rover] = *vals++;
        val[3] = 0;
        results += checkList(val, EvnDays, NumElems(EvnDays));
        if (*vals) vals++;      /* bypass ',' */
    }
    return results;
}


void putEvent()
{
    if (EventTab == NULL) {
        EventTab = (EVENT *) GetDynamic(sizeof EvBuf);
        cfg.EvNumber = 0;
    }
    else {
        EventTab = (EVENT *) realloc(EventTab,
                                        (cfg.EvNumber + 1) * sizeof EvBuf);
if (EventTab == NULL) exit(printf("Ran out of memory for events"));
    }
    movmem(&EvBuf, EventTab + cfg.EvNumber, sizeof EvBuf);
    cfg.EvNumber++;
}
