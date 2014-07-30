/******
 *   Module: LIBFOSSL.C
 *  Program: CTDL.EXE and CONFG.EXE - Citadel:K2NE BBS
 *
 *  This is a very generalized interface to the physical layer
 *  via the so-called "FOSSIL" driver software, of which there
 *  are several public domain variants, all of which should be
 *  mutually interchangeable.   The "function numbering" is
 *  intended to be consistent with the FOSSIL facility definitions
 *  as released by IFNA in their FIDO Technical Standards Committee
 *  series.  We make no guarantees about the future, but it works
 *  today!
 *
 *  A few "extended FOSSIL functions" are also included in this
 *  library module.  They are peculiar to Citadel:K2NE and handle,
 *  for the most part, the job of reading modem result codes at the
 *  beginning of each incoming call.  We make the somewhat rude
 *  assumption that all modems are really Hayes result-code format
 *  compatible.  Please note that Everex modems which support
 *  2400 bps MIGHT have some problems.
 ******/



#include <dos.h>
#include <ctype.h>
#include "fossil.h"

#define FALSE 0
#define TRUE 1

unsigned int status;
int GENERAL;

char IMPERVIOUS, skipRest;
char toCitadelNet;
char netSignal;

/* Function 0x00 - Set baud rate */

unsigned int f_baud(unsigned int port, unsigned int baud)
{
	_AH = 0x00;
	_AL = baud;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x01 Transmit character with wait */

unsigned int f_tx(unsigned int port, char c)
{
	_AH = 0x01;
	_AL = c;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x02 Receive character with wait  */

unsigned int f_rx(unsigned int port)
{
	_AH = 0x02;
	_DX = port;
	geninterrupt(0x14);
    return(_AL);
}

/* Function 0x03 Request status */
unsigned int f_status(unsigned int port)
{
	_AH = 0x03;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);

/*
 * The following are placed here for reference by Citadel developers.
 * They are also #defined as to their respective bit-values in FOSSIL.H.
 *
 *      Bit 0 = F_RDA  - input data is available in buffer
 *      Bit 1 = F_OVRN - the input buffer has been overrun.  All
 *                        characters received after the buffer is
 *                        full should be discarded.
 *      Bit 5 = F_THRE - room is available in output buffer
 *      Bit 6 = F_TSRE - output buffer is empty
 *
 * If you need to check any of these STATUS values, you should use the
 * #define method, example  (f_status(port) & F_RDA) will be TRUE if
 * data is available and false otherwise.
 */





}

/* Function 0x04 Initialize driver */
#ifdef OPTION_ON
unsigned int f_init(unsigned int port, unsigned int *flag, char far *flagbyte)
#else
unsigned int f_init(unsigned int port)
#endif
{
	_AH = 0x04;
#ifdef OPTION_ON
	_BX = *flag;
    if(_BX == 0x4F50)
    {
      _ES = FP_SEG(flagbyte);
      _CX = FP_OFF(flagbyte);
    }
#endif
	_DX = port;
	geninterrupt(0x14);
#ifdef OPTION_ON
	_CX = _BX;
	*flag = _CX;
#endif
	return(_AX);
}

/* Function 0x05 Deinitialize driver */
void f_exit(unsigned int port)
{
	_AH = 0x05;
	_DX = port;
	geninterrupt(0x14);
}

/* Function 0x06 Raise/lower DTR */
void f_dtr(unsigned int port, unsigned int dtr)
{
	_AH = 0x06;
	_AL = dtr;
	_DX = port;
	geninterrupt(0x14);
}

/* Function 0x07 Return timer tick parameters */
unsigned int f_tickparms(unsigned int *mspertick)
{
	_AH = 0x07;
	geninterrupt(0x14);
	*mspertick = _DX;
	return(_AX);
}

/* Function 0x08 Flush output buffer */
void f_flusho(unsigned int port)
{                       /* Not used in present Citadel implementation.     */
	_AH = 0x08;         /* Developers take NOTE!  This is a VERY dangerous */
	_DX = port;         /* function.  Can lead to most mysterious system   */
	geninterrupt(0x14); /* hangups.  Use with EXTREME caution, if at all!  */
}

/* Function 0x09 Purge output buffer */
void f_purgeo(unsigned int port)
{
	_AH = 0x09;
	_DX = port;
	geninterrupt(0x14);
}

/* Function 0x0A Purge input buffer */
void f_purgei(unsigned int port)
{
	_AH = 0x0A;
	_DX = port;
	geninterrupt(0x14);
}

/* Function 0x0B Transmit character without wait */
unsigned int f_poke(unsigned int port, char c)
{
	_AH = 0x0B;
	_AL = c;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x0C Nondestructive read-ahead */
unsigned int f_peek(unsigned int port)
{
	_AH = 0x0C;
	_DX = port;
	geninterrupt(0x14);
    return(_AL);
}

/* Function 0x0D Keyboard read without wait */
unsigned int f_kbdpeek(void)
{
	_AH = 0x0D;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x0E Keyboard read with wait */
unsigned int f_kbd(void)
{
	_AH = 0x0E;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x0F Enable or disable flow control */
void f_flow(unsigned int port, unsigned char mode)
{
	_AH = 0x0F;
	_AL = mode;
	_DX = port;
	geninterrupt(0x14);
}

/* Function 0x10 Extended control-C/-K checking and transmit on/off */
unsigned int f_control(unsigned int port, unsigned char mask)
{
	_AH = 0x10;
	_AL = mask;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x11 Set current cursor location */
void f_gotoxy(unsigned int col, unsigned int row)
{
	_AH = 0x11;
	_DL = col;
	_DH = row;
	geninterrupt(0x14);
}

/* Function 0x12 Read current cursor location */
unsigned int f_wherexy(unsigned int *col, unsigned int *row)
{
	_AH = 0x12;
	geninterrupt(0x14);
    if(col)
    {
      *col = _DL;
    }
    if(row)
    {
      *row = _DH;
    }
	return(_DX);
}

/* Function 0x13 Single character ANSI write to screen */
void f_dpyansi(char c)
{
	_AH = 0x13;
	_AL = c;
	geninterrupt(0x14);
}

/* Function 0x14 Enable or disable carrier watchdog processing */
void f_watchdog(unsigned int port, int onoff)
{
	_AH = 0x14;
	_AL = onoff;
	_DX = port;
	geninterrupt(0x14);
}

/* Function 0x15 Write character to screen via BIOS */
void f_dpybios(char c)
{
	_AH = 0x15;
	_AL = c;
	geninterrupt(0x14);
}

/* Function 0x16 Insert or delete a function from the timer tick chain */
unsigned int f_tickchain(int addrem, void (far *function)())
{
	_AH = 0x16;
	_AL = addrem;
	_ES = FP_SEG(function);
	_DX = FP_OFF(function);
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x17 Reboot system */
void f_reboot(unsigned int warm)
{
	_AH = 0x17;
	_AL = warm;
	geninterrupt(0x14);
}

/* Function 0x18 Read block transfer */
unsigned int f_rxblk(unsigned int port, void far *buffer, unsigned int size)
{
	_AH = 0x18;
	_ES = FP_SEG(buffer);
	_DI = FP_OFF(buffer);
	_CX = size;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x19 Write block transfer */
unsigned int f_txblk(unsigned int port, void far *buffer, unsigned int size)
{
	_AH = 0x19;
	_ES = FP_SEG(buffer);
	_DI = FP_OFF(buffer);
	_CX = size;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x1A Break begin or end */
void f_break(unsigned int port, unsigned int onoff)
{
	_AH = 0x1A;
	_AL = onoff;
	_DX = port;
	geninterrupt(0x14);
}

/* Function 0x1B Return information about driver */
unsigned int f_info(unsigned int port, void far *buffer, unsigned int size)
{
	_AH = 0x1B;
	_ES = FP_SEG(buffer);
	_DI = FP_OFF(buffer);
	_CX = size;
	_DX = port;
	geninterrupt(0x14);
	return(_AX);
}

/* Function 0x1F RTS control ** this is an "extended" FOSSIL function */
void f_rtsctrl(unsigned int port, int mode)
{
    _AH = 0x1f;  /* FOSSIL function 1F */
	_AL = 0x01;  /* Subfunction 01     */
	_DX = port;  /* Fossil "port" is one less than PC BIOS "COM" number */
	_BL = mode==0 ? 0x0D : 0x0F;      /* 0d for off  0f for on */
							  /* we pass mode=TRUE to start communication */
							  /* and mode=FALSE to stop communication.    */
							  /* This is purely a PHYSICAL LAYER function */
							  /* between host machine and host modem!     */
	geninterrupt(0x14);
}



/* Function 0x7E Install an "external application" function */
unsigned int f_addapp(unsigned int port, unsigned int *code, void (far *function)())
{
	_AH = 0x7E;
	_AL = *code;
	_ES = FP_SEG(function);
	_DX = FP_OFF(function);
	_DX = port;
	geninterrupt(0x14);
	*code = _BX;
	return(_AX);
}

/* Function 0x7F Remove an "external application" function */
unsigned int f_remapp(unsigned int port, unsigned int *code, void (far *function)())
{
	_AH = 0x7F;
	_AL = *code;
	_ES = FP_SEG(function);
	_DX = FP_OFF(function);
	_DX = port;
	geninterrupt(0x14);
	*code = _BX;
	return(_AX);
}

/* Extended Functions */


/* checks for S N or A and returns TRUE otherwise returns FALSE */
int getSNA(unsigned int port, int Local)
{
   unsigned int c;
   int returnvalue = 0;

      if((c = f_peek(port)) != 0xFF) {
         c = f_rx(port);                 /* got a char from port */
         switch(toupper(c)) {
			case   7:
				if (!netSignal) returnvalue=0;
				else {
					returnvalue=1;
					toCitadelNet=TRUE;
					}
				break;
			case 'S':
            case 'N':
			case 'A':
            returnvalue = 1;
            break;

            default:                     /* Nope, return 0 */
            returnvalue = 0;
            break;
            }
         }
      if((c = f_kbdpeek()) != 0xFFFF) {
         c = f_kbd();                    /* got a char from kybd */
         switch(toupper(c)) {
			case 'S':
            case 'N':
            case 'A':
            returnvalue = 1;
            break;

            default:                     /* nope, return 0 */
            returnvalue = 0;
            break;
            }
         }
   return(returnvalue);
}

int getFromModem(unsigned int port)
{
 unsigned int c;
 int returnvalue = 0;
 char modemString[20];
 char exitTag=FALSE;

 setmem(modemString, 20, 0);
 returnvalue=100;
 c=0;
 do {
 	f_rxhide(port, modemString, 20, 0);
/*     printf("TEST -- MODEMSTRING IS %s\n", modemString); */
   	if ( (stricmp(modemString, "1")==0)  /* 300 baud */
						  ||
		 (stricmp(modemString, "5")==0)  /* 1200 bps */
       	                  ||
       	 (stricmp(modemString, "10")==0) /* 2400 bps */
                          ||
         (stricmp(modemString, "11")==0) /* 4800 bps */
                          ||
         (stricmp(modemString, "12")==0) /* 9600 bps */
                          ||
		 (stricmp(modemString, "15")==0) /* 14400 bps */
       	                  ||
       	 (stricmp(modemString, "16")==0) /* 19200 bps */
                          ||
		 (strnicmp(modemString+1, "CONNECT", 7)==0)  /* 300 baud */
						  ||
		 (strnicmp(modemString+1, "CONNECT 1200", 12)==0)  /* 1200 bps */
       	                  ||
       	 (strnicmp(modemString+1, "CONNECT 2400", 12)==0) /* 2400 bps */
                          ||
         (strnicmp(modemString+1, "CONNECT 4800", 12)==0) /* 4800 bps */
                          ||
         (strnicmp(modemString+1, "CONNECT 9600", 12)==0) /* 9600 bps */
                          ||
		 (strnicmp(modemString+1, "CONNECT 14400", 13)==0) /* 14400 bps */
       	                  ||
       	 (strnicmp(modemString+1, "CONNECT 19200", 13)==0) /* 19200 bps */

	   ) exitTag=TRUE;

	c++;

     	} while (exitTag==FALSE && c<50);

    returnvalue=100; /* triggers default to 2400bps if we can't find reality */

	if ( stricmp(modemString, "1")==0 || strnicmp(modemString+1, "CONNECT", 7)==0 ) {
		returnvalue=0;
		}
	if ( stricmp(modemString, "5")==0 || strnicmp(modemString+1, "CONNECT 1200", 12)==0) {
		returnvalue=1;
		}
	if ( stricmp(modemString, "10")==0 || strnicmp(modemString+1, "CONNECT 2400", 12)==0) {
		returnvalue=2;
		}

	if ( stricmp(modemString, "11")==0 || strnicmp(modemString+1, "CONNECT 4800", 12)==0) {  /* 4800 bps */
		returnvalue=3;
		}
	if ( stricmp(modemString, "12")==0 || strnicmp(modemString+1, "CONNECT 9600", 12)==0) {  /* The FreakDog's Orgasmic 9600 */
		returnvalue=4;
		}
    if ( stricmp(modemString, "15")==0 || strnicmp(modemString+1, "CONNECT 14400", 13)==0) {  /* 14400 bps */
		returnvalue=5;
		}
	if ( stricmp(modemString, "16")==0 || strnicmp(modemString+1, "CONNECT 19200", 13)==0) {  /* 19200 bps */
		returnvalue=6;
		}
    if (returnvalue==100) returnvalue=2; /* could not read baudrate so assume 2400 */

 f_purgei(port);
 return(returnvalue);
}

int f_rxhide(port, buf, maxsize, Local)
unsigned int port;     /* com port */
char *buf;             /* buffer to store string */
unsigned int maxsize;  /* max len of string */
unsigned int Local;    /* Local only if 1 */
{
   int i;
   unsigned int c;
   int STOP = 0;
   int fill;
   status = f_status(port);

   if(!Local) {
   	  if(!(status & F_DCD)) {
       	 return 0;
       	 }
      }


   i=0;

   /* get string, stop if CR received, watch for max chars */

   startTimer(4); /* we only want to watch for ONE MINUTE */

   while(!STOP && chkTimeSince(4) < 60l)
   {
      if((c = f_peek(port)) != 0xFF)
      {
         c = f_rx(port);
         switch(c)
         {
            case NDBS:                 /* process Backspace properly */
            if(i > 0)
            {
               if(!Local)
               {
                  f_tx(port,0xE08);
                  f_tx(port,' ');
                  f_tx(port,0xE08);
               }
               f_dpyansi(0xE08);
               f_dpyansi(0x0020);
               f_dpyansi(0xE08);
               i--;
            }
            else
            {
               if(!Local)
               f_tx(port,0x0007);      /* ring callers bell if no chars in */
            }                          /* in string and BS hit */
            break;

            case 0x0D:
            STOP = 1;
            break;

            default:
            buf[i++] = c;              /* place char in buffer */
            if(i>=maxsize)             /* check for too many chars */
            {
               if(!Local)
               {
                  f_tx(port,0x0007);   /* ring callers bell if too many chars */
                  f_tx(port,0xE08);
                  f_tx(port,' ');
                  f_tx(port,0xE08);
               }
               f_dpyansi(0xE08);
               f_dpyansi(0x0020);
               f_dpyansi(0xE08);
               i--;
            }
            break;
         }
      }


   }
   buf[i] = '\0';            /* Make it a 'C' style string */

   return(1);
}

int ringDetect(int port)
{
 if (f_status(port) & F_RING) return 1;
 else return 0;
}


int getRingFromModem(unsigned int port, int dlay)
{
 int returnValue;

 returnValue=0;

/* if (f_status(port) & F_RING) { */
 if (ringDetect(port)) {
	returnValue=1;
/* 	f_status(port); */
 	delay(dlay);
	}
 return(returnValue);
}

#ifdef DUMB
int f_rxspecial(port, buf, maxsize, Local)
unsigned int port;     /* com port */
char *buf;             /* buffer to store string */
unsigned int maxsize;  /* max len of string */
unsigned int Local;    /* Local only if 1 */
{
   int i;
   unsigned int c;
   int STOP = 0;
   int fill;
   status = f_status(port);

   i=0;

   /* get char, stop if CR received, watch for max chars */

      if((c = f_peek(port)) != 0xFF)
      {
         c = f_rx(port);
         switch(c)
         {
            case 0:
			break;

            case 0x0D:
            STOP = 1;
            break;

            default:
            buf[i++] = c;              /* place char in buffer */
            if(i>=maxsize)             /* check for too many chars */
            {
               if(!Local)
               {
                  f_tx(port,0x0007);   /* ring callers bell if too many chars */
                  f_tx(port,0xE08);
                  f_tx(port,' ');
                  f_tx(port,0xE08);
               }
               f_dpyansi(0xE08);
               f_dpyansi(0x0020);
               f_dpyansi(0xE08);
               i--;
            }
            break;
         }
      }


   buf[i] = '\0';            /* Make it a 'C' style string */

   return(1);
}
#endif