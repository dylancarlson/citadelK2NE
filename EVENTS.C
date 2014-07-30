/************************************************************************/
/*                              events.c                                */
/*                    Event handling code for Citadel                   */
/************************************************************************/

#include "ctdl.h"
/* #define EVENT_DEBUG */

/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 87Jun13 HAW  Created.                                                */
/************************************************************************/

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      InitEvents()            Initialize event stuff                  */
/*      eventSort()             Sorts events                            */
/*      setPtrs()               Set up pointers                         */
/*      DoTimeouts()            Handles timeouts                        */
/************************************************************************/
#define Cur     EventTab[i]

/************************************************************************/
/*                              Variables                               */
/************************************************************************/
EVENT         EvBuf;

#ifdef SYSTEM_CLOCK
char          ShowClock = TRUE;
#endif

static int    ThisDay, ThisMinute;
static long   ThisSecond;
static int    NextPre = -1;
static int    NextQuiet = -1;
static char   warned = FALSE;
static long   NextAbs;

long   EndAnytime = -1l;
long   DeadTime = 1800l;        /* Default to 30 minutes */
long   AnyNetLen = 180l;        /* Default to 3 minutes  */
MULTI_NET_DATA AnyTimeNets;

extern EVENT  *EventTab;
extern CONFIG cfg;
extern char   outFlag;
extern char   haveCarrier;
extern char   onConsole;
extern char   ExitToMsdos, runStandbyScreen, autoNet;
extern char   killScreen;
extern int    exitValue, evMode;
extern long   End_Dl_Limit, Dl_Limit;    /* Yuck, but necessary */
extern logBuffer logBuf;

/************************************************************************/
/*      InitEvents()  Initializes events stuff                          */
/************************************************************************/
void InitEvents(char SetRel)
/* char SetRel; */
{
    int      i;
    int      yr, dy, hr, mn, mon, secs, milli;
    long     temp;

    getRawDate(&yr, &mon, &dy, &hr, &mn, &secs, &milli);
    ThisDay = WhatDay();
    ThisMinute = (ThisDay * 1440) + (hr * 60) + mn;
    temp = (long) ThisMinute;
    ThisSecond = (temp * 60l) + secs;
    if (cfg.EvNumber > 0) {
        if (SetRel)
            for (i = 0; i < cfg.EvNumber; i++)
                if (EventTab[i].EvClass == CLREL) {
                    EventTab[i].EvMinutes = 
                              (ThisMinute + EventTab[i].EvDur) % 10080;
                }
        qsort(EventTab, cfg.EvNumber, sizeof *EventTab, eventSort);
        setPtrs();
    }
}

/************************************************************************/
/*      eventSort() Sort a pair of events                               */
/************************************************************************/
static int eventSort(EVENT *s1, EVENT *s2)
/* EVENT *s1, *s2; */
{
#ifdef OLD_STYLE
    if (during(s1) && (s1->EvClass != CL_DL_TIME || Dl_Limit == -1l))
        return -1;
    if (during(s2) && (s2->EvClass != CL_DL_TIME || Dl_Limit == -1l))
        return  1;
#else
    if (during(s1) && !AlreadyProcessed(s1))
        return -1;
    if (during(s2) && !AlreadyProcessed(s2))
        return  1;
#endif

    if ((passed(s1) && passed(s2)) ||
       (!passed(s1) && !passed(s2))) {
        if (s1->EvMinutes < s2->EvMinutes) return -1;
        if (s1->EvMinutes > s2->EvMinutes) return  1;
        if (s1->EvType == TYNON && s1->EvType == TYPREEMPT) return -1;
        if (s1->EvType == TYPREEMPT && s1->EvType == TYNON) return  1;
        return 0;
    }
    if (s1->EvMinutes < s2->EvMinutes) return 1;
    return -1;
}

static char AlreadyProcessed(EVENT *s)
/* EVENT *s; */
{
    if (s->EvClass == CL_DL_TIME)
        return (Dl_Limit != -1l);

    if (s->EvClass == CL_ANYTIME_NET)
        return (EndAnytime != -1l);

    return 0;
}

/************************************************************************/
/*      during() Are we "during" this event?                            */
/************************************************************************/
int during(EVENT *x)
/* EVENT *x; */
{
    if (ThisMinute >= x->EvMinutes && ThisMinute < x->EvMinutes + x->EvDur)
        return TRUE;

    if (x->EvMinutes + x->EvDur > 10080 &&
        ThisMinute < (x->EvMinutes + x->EvDur) % 10080)
        return TRUE;
    return FALSE;
}

/************************************************************************/
/*      passed() has this event "passed" in terms of the week?          */
/************************************************************************/
int passed(EVENT *x)
/* EVENT *x; */
{
#ifndef OLD_STYLE
    if (((x->EvDur == 0 ||
         (x->EvClass == CL_DL_TIME && Dl_Limit != 0l) ||
         (x->EvClass == CL_ANYTIME_NET && EndAnytime != -1l) ||
          x->EvClass == CLREL) &&
          x->EvMinutes <= ThisMinute) ||
         (x->EvDur != 0 && x->EvMinutes < ThisMinute))
         return TRUE;
    return FALSE;
#else

#endif
}

/************************************************************************/
/*      setPtrs() Initializes pointers for events                       */
/************************************************************************/
void setPtrs()
{
    int  rover;
    long InSeconds, temp;

    NextPre = NextQuiet = -1;
    for (rover = 0; rover < cfg.EvNumber; rover++) {
        if (EventTab[rover].EvType == TYPREEMPT && NextPre == -1) {
            NextPre = rover;
        }
        if (EventTab[rover].EvType == TYQUIET && NextQuiet == -1) {
            NextQuiet = rover;
        }
    }

    if (cfg.EvNumber != 0) {
        temp = (long) EventTab[0].EvMinutes;
        InSeconds = temp * 60l;
        if (during(&EventTab[0]))
            NextAbs = CurAbsolute() - WeekDiff(ThisSecond, InSeconds);
        else
            NextAbs = CurAbsolute() + WeekDiff(InSeconds, ThisSecond);
    }
}

/************************************************************************/
/*      WeekDiff() Figures out difference in time between events        */
/************************************************************************/
long WeekDiff(long future, long now)
/* long future, now; */
{
    if (now > future)
        return (long) (604800l - now + future);
    return (long) (future - now);
}

/************************************************************************/
/*      ChkPreempt() will estimated time to d/l interfere with next     */
/*                   preemptive event?                                  */
/************************************************************************/
char *ChkPreempt(long estimated)
/* long estimated; */
{
    int      yr, dy, hr, mn, mon, secs, milli;
    long     temp, InSeconds;

    if (NextPre == -1) return NULL;

    getRawDate(&yr, &mon, &dy, &hr, &mn, &secs, &milli);
    ThisDay = WhatDay();
    ThisMinute = (ThisDay * 1440) + (hr * 60) + mn;
    temp = (long) ThisMinute;
    ThisSecond = (temp * 60l) + secs; /* Current second in week */
    temp = (long) EventTab[NextPre].EvMinutes;
    InSeconds = temp * 60l;
    if (estimated >= WeekDiff(InSeconds, ThisSecond)) {
        return cfg.codeBuf + EventTab[NextPre].EvWarn;
    }
    return NULL;
}

/************************************************************************/
/*      DoTimeouts() Code that does actual checking of timeouts         */
/*                   returns TRUE if you want modIn to break out, too   */
/* Clean up this code someday, it's not good. -- Hue                    */
/* It's not even pretty. -- Hue
/************************************************************************/
char DoTimeouts()
{
    int  yr, dy, hr, mn, temp, mon, secs, milli;
    char oldState = TRUE;
    long ThisAbsolute;
    extern char QuickNet;
#ifdef SYSTEM_CLOCK
    static int LastMinute = -1;
#endif

    getRawDate(&yr, &mon, &dy, &hr, &mn, &secs, &milli);

#ifdef SYSTEM_CLOCK
    if (LastMinute != mn && ShowClock) {
        ScrTimeUpdate(hr, mn);
        LastMinute = mn;
    }
#endif
    if (cfg.EvNumber == 0) return FALSE;

    ThisMinute = (WhatDay() * 1440) + (hr * 60) + mn;
    ThisAbsolute = (long) ThisMinute;
    ThisSecond = ThisAbsolute * 60 + secs;

    ThisAbsolute = CurAbsolute();

    if (Dl_Limit != -1l) {
        if (ThisAbsolute >= End_Dl_Limit) {
            Dl_Limit = End_Dl_Limit = -1l;
/*            printf("D-L limits OFF.\n"); */
        }
    }
    if (EndAnytime != -1l) {
        if (ThisAbsolute >= EndAnytime) {
/*            printf("Anytime Netting OFF.\n",
            ThisAbsolute, EndAnytime); */
            EndAnytime = -1l;
        }
        else if (chkTimeSince(1) > DeadTime) {
		    if (!onLine() || QuickNet ) {
				autoNet=TRUE;  /* a fakeout maneuver! */
				if (killScreen==TRUE)  resurrect();
				netController((hr * 60) + mn,
							AnyNetLen, AnyTimeNets, ANYTIME_NET);
				}
            startTimer(1);
        }
    }
        /*
         * Preemptive events checker.  At T-5 minutes, give 1 warning.
         */
    if (NextPre != -1) {
        if (!warned &&
            WeekDiff(EventTab[NextPre].EvMinutes, ThisMinute)  <= 5 &&
                              onLine()) {
            temp = EventTab[NextPre].EvMinutes % 1440;
            warned = TRUE;
            outFlag = IMPERVIOUS;

            mPrintf("\n %cNOTE: Going down at %d:%02d for %s.\n ",
               BELL, temp/60, temp%60, EventTab[NextPre].EvWarn + cfg.codeBuf);
			if (!expert) {
				expert = TRUE;
				oldState = FALSE;
				}
			else oldState = TRUE;
            HelpIfPresent("warning.blb");
			expert = oldState;
            outFlag = OUTOK;
            return FALSE;
        }
    }

        /*
         * This handles quiet events
         */
    if (NextQuiet != -1) {
        if (during(EventTab + NextQuiet))
            if (!HandleQuiet(NextQuiet)) return FALSE;
    }

#ifdef EVENT_DEBUG
    if (cfg.BoolFlags.debug) {
        printf("EventTab[0].EvMinutes<=ThisMinute == %d\n",
             EventTab[0].EvMinutes <= ThisMinute);
        printf("EvDur=%d\n", EventTab[0].EvDur);
        printf("ThisMinute==%d\n", ThisMinute);
    }
#endif

        /*
         * Check for the next chronological event occuring.
         */
    if (ThisAbsolute >= NextAbs) {
#ifdef EVENT_DEBUG
        printf("In big if, Type is %d\n", EventTab[0].EvType);
#endif

        if (onLine()) {
            if (EventTab[0].EvType == TYPREEMPT) {
                outFlag = IMPERVIOUS;
				expert = TRUE;
				HelpIfPresent("schedule.blb");
                mPrintf("\n %cGoing to %s, bye!\n ", BELL,
                                      EventTab[0].EvWarn + cfg.codeBuf);

                if (onConsole) {        /* Ugly cheat */
                    terminate(TRUE, TRUE);
                }
                else
                    runHangup();

                outFlag = OUTOK;
                return FALSE;
            }
#ifdef EVENT_DEBUG
            if (cfg.BoolFlags.debug) printf("Returning FALSE at 1\n");
#endif
            return FALSE;
        }

        switch (EventTab[0].EvClass) {
        case CLNET:
            netController(EventTab[0].EvMinutes % 1440, EventTab[0].EvDur,
                                EventTab[0].EvExitVal, NORMAL_NET);
            warned = FALSE;
            InitEvents(FALSE);
            return TRUE;
        case CLEXTERN:
        case CLREL:
        EventShow();
            ExitToMsdos = TRUE;
            exitValue = (int) EventTab[0].EvExitVal;
            warned = FALSE;
            return TRUE;
        default:        /* do nothing */
        }
    }
/*    if (runStandbyScreen==TRUE) return TRUE;
	else */
	return FALSE;
}

/************************************************************************/
/*      HandleQuiet() Handles events of type TYQUIET                    */
/************************************************************************/
char HandleQuiet(int index)
/* int index; */
{
    long ThisAbs, temp, InSeconds;

    temp = (long) EventTab[index].EvMinutes;
    InSeconds = temp * 60l;
    ThisAbs = CurAbsolute() - WeekDiff(ThisSecond, InSeconds);

    temp = (long) EventTab[index].EvDur;
    switch (EventTab[index].EvClass) {
        case CL_DL_TIME:
            if (Dl_Limit == -1l) {
                End_Dl_Limit = ThisAbs + temp * 60;
                Dl_Limit = EventTab[index].EvExitVal;
/*                printf("D-L limit now %ld minutes.\n", Dl_Limit); */
                if (index == 0) InitEvents(FALSE);
                return FALSE;
            }
            else return TRUE;
        case CL_ANYTIME_NET:
            if (EndAnytime == -1l) {
                EndAnytime = ThisAbs + temp * 60;
                                /* gets eligible nets */
                AnyTimeNets = EventTab[index].EvExitVal;
/*                printf("Anytime net ON.\n"); */
            }
            if (index == 0) InitEvents(FALSE);
            return FALSE;
    }
}

/***********************************************************/
#ifdef EVENT_DEBUG

static char *cl[] =
        { "network", "extern", "relative", "dl-time", "anytime-net" } ;
static char *ty[] =
        { "preempt", "non-preempt", "quiet" } ;
static char *dy[] =
        { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" } ;

void EventShow()
{
    int i;
    int temp;

    mPrintf("\n Dl_Limit: %ld, CurAbs=%ld, NextAbs=%ld End_Dl_Limit=%ld\n ",
        Dl_Limit, CurAbsolute(), NextAbs, End_Dl_Limit);
    mPrintf("NextQuiet=%d\n ", NextQuiet);
/*printf("ThisMinute=%d\n", ThisMinute);*/
    for (i = 0; i < cfg.EvNumber; i++) {
        mPrintf("Event%3d: ", i);
        mPrintf("%s %s ", cl[Cur.EvClass], ty[Cur.EvType]);
        mPrintf("%s ", dy[Cur.EvMinutes / 1440]);
        temp = Cur.EvMinutes % 1440;
        mPrintf("%d:%02d %lx\n ", temp/60, temp%60,
                Cur.EvExitVal);
    }
}

#else

void EventShow()
{
 if (evMode==0) {
	    if (Dl_Limit != -1l)
    	    cprintf("\n DL_Limit: %ld minutes.", Dl_Limit);

    	if (EndAnytime != -1l) {
        	cprintf("\n Anytime-net: %ld minutes left.",
                       (EndAnytime - CurAbsolute())/60);
/*			cprintf("\n %ld seconds until next session-layer check.", */
			cprintf("\n %ld seconds until next traffic check.",
						DeadTime - chkTimeSince(1));
        	cprintf("\n Networking%s needed.",
                AnyCallsNeeded(AnyTimeNets) ? "" : " NOT");
		    }
 		}
 else {
	    if (Dl_Limit != -1l)
    	    mPrintf("\n DL_Limit: %ld minutes.", Dl_Limit);

   	 if (EndAnytime != -1l) {
    	    mPrintf("\n Anytime-net: %ld minutes left.",
                       (EndAnytime - CurAbsolute())/60);
/*			mPrintf("\n %ld seconds until next session-layer check.", */
			mPrintf("\n %ld seconds until next traffic check.",
						DeadTime - chkTimeSince(1));
        	mPrintf("\n Networking%s needed.",
                AnyCallsNeeded(AnyTimeNets) ? "" : " NOT");
    	}
	}
}

#endif

#ifndef MAJOR_RELEASE
/* #ifdef OLD_WAY */
AnyKludge(char *str)
/* char *str; */
{
    char s[8];
    int i;

    for (i = 0; *str != ',' && *str; str++, i++)
        s[i] = *str;
    s[i] = 0;
    DeadTime = atol(s) * 60l;
    if (*str) {
        for (i = 0, str++; *str != ',' && *str; str++, i++)
            s[i] = *str;
        s[i] = 0;
        AnyNetLen = atol(s);
    }
}
/* #endif */
#endif
/***********************************************************/
