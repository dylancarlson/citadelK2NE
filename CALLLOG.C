/************************************************************************/
/*                              calllog.c                               */
/*                      handles call log of Citadel-86                  */
/************************************************************************/

#include "ctdl.h"

#ifdef DSZ
#define UL_FLAG 11
#define DL_FLAG 12
#endif
#define BACKUP_FLAG 13
#define DOOR_FLAG 14
#define DOOR_CLOSE_FLAG 15
#define CARRIER_ON 16
#define CARRIER_OFF 17
#define L_STAY 18
#define MISC_MSG 19
/************************************************************************/
/*                              history                                 */
/*                                                                      */
/* 88Jul02 VAQ  Started code for download file reporting.				*/
/* 88Jul01 VAQ  Added upload file reporting.							*/
/* 88Jun29 VAQ  Format of file output changed for improved readability. */
/* 86Mar07 HAW  New users and .ts signals.                              */
/* 86Feb09 HAW  System up and down times.                               */
/* 86Jan22 HAW  Set extern var so entire system knows baud.             */
/* 85Dec08 HAW  Put blank lines in file.                                */
/* 85Nov?? HAW  Created.                                                */
/************************************************************************/

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      logMessage()            Put out str to file.                    */
/************************************************************************/
extern CONFIG cfg;
extern int    byteRate, userMessages;
extern char   frontEnd; /* BinkleyTerm stuff */

static SYS_FILE CallFn = "";
static char     CallCrash = FALSE;
static int timeHere;
char *curBaud;

/************************************************************************/
/*      logMessage() Puts message out.  Also, on date change, and       */
/*                   first output of system, insert blank line          */
/************************************************************************/
int hr_in;
char *m_in;
char currentUserName[30];

void logMessage(char val, char strVar[26], char sig)
/* char strVar[26];
char sig;
char val; */
{
    struct timeData {
        int y, d, h, m;
        char  *month;
        label person;
        char  newuser;
        char  evil;
    };

    static int oldDay = 0;
    static struct timeData lgin;

    int        yr, dy, hr, mn, year, day, hours, minutes;  /* K2NE */
    char       *mon, buf[255], *m, dat_str[20]; /* K2NE look at this */
	char       job_str[10];

    extern 	   char uploadFile[NAMESIZE], downloadFile[NAMESIZE]; /* K2NE */
               /* ain't we getting sophisticated!   */
	extern     char callLogPosting[800], doorLogFlag;
	extern     int thisRoom, nrCalls;

    if (CallCrash) return;

    if (!cfg.BoolFlags.Calllog) return;

    getCdate(&yr, &mon, &dy, &hr, &mn);
	if ( (val != INTO_NET) && (val != OUTOF_NET) ) {
		civTime(&hr,&m);
		sPrintf(dat_str, " %s  %2d:%02d%s: ",formDate(),hr,mn,m);
        sPrintf(job_str, "[Job B%d]", nrCalls);
		}


/************************************************************************/
/*             +++ Whither goest thou, CallLog? +++                     */
/*                                                                      */
/*   In the SWITCH structure that follows, various messages are passed  */
/* to the CALLLOG.SYS file.  CALLLOG.SYS records various events that    */
/* happen as the Citadel is used.  Most of the cases in the SWITCH are  */
/* rather self-explanatory.  The case 'MISC_MSG' is in place as a       */
/* general "catch-all" for miscellaneous event recording, the text of   */
/* which is pre-written and passed to logMessage() via the string       */
/* we have named "callLogPosting" (800 bytes allocated).                */
/*                                                                      */
/*   To make it more readable, the format of the strings passed to      */
/* CALLLOG.SYS is NOT the same as that used by the so-called "Hue-Style"*/
/* Citadel-86 systems.  It is our belief that the method we are using   */
/* is much more readable, less cryptic and, naturally, more informative */
/* as we are reporting things such as chat requests and file transfers  */
/* which are not handled at all by other DOS manifestations of Citadel  */
/* or are handled clumsily through separately generated report files.   */
/*                                                                      */
/*   Because the format of the CALLLOG.SYS file has been changed, those */
/* utilities that depend on the format of CALLLOG.SYS will not work     */
/* properly (if at all).  Specifically, if you have been using the      */
/* CALLSTAT utility, you will not be able to continue using it unless   */
/* you want to modify it yourself.  The source code for CALLSTAT is     */
/* available at Jersey Devil and you are free to grab it.               */
/*                                                                      */
/*   A great deal of the original K2NE code in this section was cleaned */
/* up on 88Nov29 during a 'hack by phone' chat with Brian Riley.  Some  */
/* small (but annoying) bugs were removed that same day, and what you   */
/* see here is the result.                                              */
/************************************************************************/
    makeSysName(CallFn, "calllog.sys", &cfg.call_log);
                         /* si non habet memoria... */
	if (doorLogFlag || frontEnd) {
		oldDay = dy; /* eliminate those annoying blank lines! */
		}

    switch (val) {
        case FIRST_IN:
                oldDay = dy;
                makeSysName(CallFn, "calllog.sys", &cfg.call_log);
        case CRASH_OUT:
        case LAST_OUT:
                sPrintf(buf,"%sSystem %sabled", dat_str,
					 (val == FIRST_IN) ? "en" : "dis",
    	             (val == CRASH_OUT) ? "(crash exit!)" : "");
                CallMsg(buf);
				if (val == FIRST_IN) break;
                return;
        case BAUD:
	        if      (strCmp(strVar, "300" ) == SAMESTRING)
	        		byteRate = 30 ;
    	    else if (strCmp(strVar, "1200") == SAMESTRING)
    	    		byteRate = 120;
        	else if (strCmp(strVar, "2400") == SAMESTRING)
        			byteRate = 240;
        	else if (strCmp(strVar, "4800") == SAMESTRING)
        			byteRate = 480;
        	else if (strCmp(strVar, "9600") == SAMESTRING)
        			byteRate = 960;
        	else    byteRate = 0  ;

        	curBaud        = strVar;
			strcpy(cfg.whatRate, curBaud);
            lgin.person[0] = 0;
            return;
        case L_IN: lgin.y = yr;
                lgin.d = dy;
                lgin.h = hr;
                lgin.m = mn;
                lgin.newuser = sig;
                strCpy(lgin.person, strVar);
			    strCpy(currentUserName, lgin.person);
                lgin.month = mon;
				if (oldDay != dy) {
					CallMsg("");
					oldDay = dy;
					}
				sPrintf(buf,
                   "%s%s logged in %s %c %c %c",
                	    dat_str, lgin.person,
		                (curBaud == NULL) ? "at Console" : job_str,
        		        (lgin.newuser) ? lgin.newuser : ' ',
						(sig == 0) ? ' ' : sig, lgin.evil ? 'E' : ' ');
        	    CallMsg(buf);
                lgin.evil  = FALSE;
                break;
        case EVIL_SIGNAL:
                lgin.evil  = TRUE;
                break;
		case CHAT_FLAG:
                if (oldDay != dy)
					{
					CallMsg("");
					oldDay = dy;
					}
				sPrintf(buf, "%sOperator paged by %s",
                    dat_str, lgin.person[0] ? lgin.person : "unlogged caller");
                CallMsg(buf);
                break;
        case CARRLOSS:
                homeSpace();
				break;
        case L_OUT:
                if (!lgin.person[0]) {
                     curBaud = NULL;
                     break;
                }
                if (oldDay != dy)
					{
                    CallMsg("");
					oldDay = dy;
                    }
                timeHere = chkTimeSince(3)/60;
                sPrintf(buf,
                    "%s%s logged out [%d message%ssaved in %d minute%s]",
					  dat_str, lgin.person,
						 userMessages, userMessages==1 ? " " : "s ",
						 timeHere, timeHere==1 ? "" : "s");
                CallMsg(buf);
                lgin.person[0] = 0;
                oldDay = dy;
                if (val == CARRLOSS) curBaud = NULL;
                break;
        case INTO_NET:
                lgin.y = yr;
                lgin.d = dy;
                lgin.h = hr;
                lgin.m = mn;
                lgin.month = mon;
                hr_in = hr;
				civTime(&hr_in, &m_in);  /* look at this */
                break;
        case OUTOF_NET:
				civTime(&hr, &m);
				sPrintf(buf,
				  " %d%s%02d  %2d:%02d%s: System in Network mode until %d:%02d%s",
				  lgin.y, lgin.month, lgin.d, hr_in, lgin.m, m_in, hr, mn, m);
                CallMsg(buf);
                break;
#ifdef DSZ
		case DL_FLAG:
		case UL_FLAG:    /* user uploaded so let's report it to the log */
				sPrintf(buf, "%sFile \"%s\" in %s %sloaded by %s",
					dat_str,
					(val == UL_FLAG) ? uploadFile : downloadFile,
					formRoom(thisRoom, FALSE, FALSE),
					(val == UL_FLAG) ? "up" : "down",
					lgin.person);
                CallMsg(buf);
				break;
#endif
		case DOOR_FLAG:
                sPrintf(buf, "%sDoor [%s] opened by %s [%d message%ssaved]",
						dat_str,
						uploadFile,
						lgin.person,
                        userMessages, userMessages==1 ? " " : "s ");
				CallMsg(buf);
				userMessages = 0;
				break;
		case DOOR_CLOSE_FLAG:
                sPrintf(buf, "%sDoor [%s] closed by %s",
						dat_str,
						uploadFile,
						lgin.person);
				CallMsg(buf);
				break;
		case BACKUP_FLAG:  /* report backing up of file */
                if (oldDay != dy)
					{
					CallMsg("");
					oldDay = dy;
					}
				sPrintf(buf, "%sLog file backed up [%s]",
                     dat_str, lgin.person[0] ? lgin.person : "Console");
                CallMsg(buf);
                break;
		case CARRIER_ON:
		case CARRIER_OFF:
		case L_STAY:
				if (oldDay != dy) {
					CallMsg("");
					oldDay = dy;
					}
				if (val == L_STAY)
					sPrintf(buf, "%sSession continued", dat_str);
				else sPrintf(buf, "%s%s%s",
					dat_str,
                    (val == CARRIER_ON) ? curBaud : "",
					(val == CARRIER_ON) ? " bps carrier detected" : "Carrier lost");
				CallMsg(buf);
				break;
		case MISC_MSG:
				if (oldDay != dy) {
					CallMsg("");
					oldDay = dy;
					}
				sPrintf(buf,"%s%s",
					dat_str,
					callLogPosting);
				CallMsg(buf);
				break;
    }
}

static void CallMsg(char *str)
/* char *str; */
{
    FILE        *fd;
    extern char *A_C_TEXT;

    if ((fd = safeopen(CallFn, A_C_TEXT)) != NULL) {
        fprintf(fd, "%s\n", str);
        fclose(fd);
    }
    else {
        CallCrash = TRUE;
        crashout("Call log error!");
    }
}

