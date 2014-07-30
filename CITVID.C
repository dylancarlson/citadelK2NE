/* Citadel-86 BBS video routines...
 * (c) Copyright Pete Gontier (NO CARRIER), Santa Barbara, 1988.
 * These routines can be called 'freeware' in the sense that any use at all
 * is encouraged. The one exclusion is CompuServe - they will only
 * end up suing someone for downloading this and putting it on his BBS.
 * CompuServe may never have any claim to this file whatsoever.
 *
 * Modified January 1990 by Alan Bowen and Vince Quaresima for support
 * of ANSI colors to user and console.  Same (c) Copyright restrictions
 * apply as stated above.
 */

#include <dos.h>
#include <stdio.h> /* AB_ADD */
#define VMODULE
#include <stdlib.h>
#include "citvid.h"

#ifdef ANSI
	#define BRIGHT 1
	#define DIM 0
	#define SAME -1
	#define A_BLACK 0
	#define A_RED 1
	#define A_GREEN 2
	#define A_BROWN 3
	#define A_BLUE 4
	#define A_MAGENTA 5
	#define A_CYAN 6
	#define A_WHITE 7
#endif

#define TRUE 1

typedef struct {        /* Principal color levels defined     */
	short int level0;   /* 0 main text stuff and messages     */
	short int level1;   /* 1 headers and helps, mail and aide */
	short int level2;   /* 2 directories and other goodies    */
	short int level3;   /* 3 some minor prompts               */
	short int level4;   /* 4 rest reserved for later use      */
	short int level5;   /* 5         "      "    "    "       */
	short int level6;   /* 6         "      "    "    "       */
	short int level7;   /* 7         "      "    "    "       */
} paintBrush ;


paintBrush colTable;      /* the ANSI rainbow             */
extern char InvVideo, ansi;
char *temp;
int NORM;
int CONTRAST;

SCOPE byte vatt,
           vrow, vcol,            /* (autoset to 0) */
           vtop         = 0,      /* defaults */
		   vbot         = 24,     /* was 24   */
           vleft        = 0,
           vright       = 79,
           vwherey      = 20;

void video ( char *tagline )
{
   int k;
   extern int CITCOLOR, CITSTATUS; /* new stuff */
/*
   let's try this stuff at the command line instead!
   NORM = ( (temp=getenv("CITCOLOR"))==0) ? 0x007 : atoi(temp);
   CONTRAST = ( (temp=getenv("CITSTATUS"))==0) ? 0x070 : atoi(temp);
*/

   NORM = (CITCOLOR==0) ? 0x007 : CITCOLOR;
   CONTRAST = (CITSTATUS==0) ? 0x070 : CITSTATUS;

   vatt=NORM;
   vsetup ();
#ifdef DOUBLE_BAR
   vlocate ( 23, 0 );              /* AB */
   vatt = (InvVideo) ? CONTRAST : NORM;
   for ( k = 0; k < 80; k++ )
      vputch ( ' ' );
#endif
   vlocate ( 24, 0 );              /* AB */
   vatt = (InvVideo) ? CONTRAST : NORM;
   for ( k = 0; k < 80; k++ )
      vputch ( ' ' );
   vlocate ( 24, 0 );              /* AB */
   if (ansi) {
		clearStatusLine();            /* erase line to keep solid color AB_ADD */
		setAnsiColor(BRIGHT, A_WHITE, A_BLUE);         /* set new color AB_ADD */
		}
   vputs ( tagline );
   vatt = NORM;
   vlocate ( 23, 0 );              /* AB was 23, 0*/
   vatt = NORM; vbot=23; vtop = 0; /* AB vbot was 23 */
}

#ifdef NEEDED
void statusline ( char *string )
{
   byte ocol = vcol,
        orow = vrow;
   vlocate ( 1, vwherey );
      vatt = CONTRAST;
         vputs ( string );
	  vatt = NORM;
   vlocate ( orow, ocol );
}
#endif

void vsetmode ( byte mode )
{
   _AH = 0;
   _AL = mode;
   geninterrupt ( VIDEO );
}

byte vgetmode ( void )
{
   _AH = 15;
   geninterrupt ( VIDEO );
   return ( _AL );
}

void vsetpage ( byte page )
{
   _AH = 5;
   _AL = page;
   geninterrupt ( VIDEO );
}

#ifdef TEST
void vsetup ( void )
{
   byte omode = vgetmode ();

   vsetmode(omode);
}
#endif

void vsetup ( void )
{
   byte omode = vgetmode ();
   if ( omode == 7 )
      vsetmode ( 7 );
   else {
      vsetmode ( 3 );
      vsetpage ( 0 );
   }
}


void vlocate ( byte row, byte col )
{
   _AH = 2;
   _DH = row;
   _DL = col;
   _BH = 0;
   geninterrupt ( VIDEO );
   vrow = row; vcol = col;
}

int vputs ( char *string )
{
   char *old; old = string;
   while ( *string )
      vputch ( *string++ );
   return ( old - string );
}

void vbump ( void ) {
   _AH = 6;         /* scroll */
   _CH = vtop;
   _DH = vbot;
   _CL = vleft;
   _DL = vright;
   _BH = vatt;
   _AL = 1;         /* number of lines */
   geninterrupt ( VIDEO );
}

#ifdef NEW_VIDEO_THING
byte vputch ( byte c)
{
 f_dpyansi( c);

}
#endif

/* #ifdef OLD_WAY */
byte vputch ( byte c )
{
   switch ( c ) {
      case '\a' :
         _AH = 14;
         _AL = c;
         _BH = 0;
         geninterrupt ( VIDEO );
         break;
      case '\r' :
         vcol = vleft;
         break;
      case '\n' :
         if ( vrow == vbot )
            vbump ( );
         else
            vrow++;
         break;
      case '\t' : { int kount;
         for ( kount = 0; kount < 5; kount++ )
            vputch ( ' ' ); }
         break;
      case '\b' :
         if ( vcol == vleft ) {
            if ( vrow != vtop ) {
               vlocate ( vrow - 1, vright );
               vputch ( ' ' );
               vlocate ( vrow - 1, vright );
            }
         }
         else {
            vlocate ( vrow, vcol - 1 );
            vputch ( ' ' );
            vlocate ( vrow, vcol - 1 );
         }
         break;
	  default :
/* #ifdef ANSI */ /* AB_ADD Begin */
if (ansi) {
		 putchar(c);
         if ( vcol == vright ) {
            vcol = vleft;
			if ( vrow == vbot )
               vbump ( );
            else
               vrow++;
         	}
         else
            vcol++;
		 break;
		 }
/* #else */ /* AB_ADD End */
else {
		 _AH = 9;
         _AL = c;
         _CX = 1;
         _BL = vatt;
         _BH = 0;
         geninterrupt ( VIDEO );
         if ( vcol == vright ) {
            vcol = vleft;
            if ( vrow == vbot )
               vbump ( );
            else
               vrow++;
         }
         else
            vcol++;
		 break;
 }
/* #endif */
   }
   vlocate ( vrow, vcol );
   return ( c );
}
/* #endif */

void statusline ( char *string )
{
	byte ocol = vcol,
         orow = vrow;
	int k;
	extern char roomLevelFlag;

    vlocate ( 24, (roomLevelFlag == TRUE) ? 0 : vwherey );        /* AB */
    vatt = (InvVideo) ? CONTRAST : NORM;
    if (!ansi) {
	for (k = (roomLevelFlag == TRUE) ? 0 : vwherey; k < 80; k++)
		vputch ( ' ' );
		}
	vlocate ( 24, (roomLevelFlag == TRUE) ? 0 : vwherey );  /* AB */
#ifdef ANSI
    if (ansi) {
		setAnsiColor(BRIGHT, A_WHITE, A_BLUE);         /* set new color AB_ADD */
		clearStatusLine();            /* erase line to keep solid color AB_ADD */
		setAnsiColor(BRIGHT, A_WHITE, A_BLUE);         /* set new color AB_ADD */
		}
#endif
	vputs ( string );
    vatt = NORM;
#ifdef ANSI                      /* picky, picky this Ansi driver!  AB_ADD */
	if (ansi) shrtColor(colTable.level0 /* A_GREEN */); /* reset to other color AB_ADD */
#endif
	vlocate ( orow, ocol );
}

/* #ifdef NOISE*/

extern char noNoises;
#ifdef OLD_NOISES
upWin()
{
 static int timing, freq;

 if (noNoises==TRUE) {
	return;
	}

 timing=0;
 freq=0;
 do {
	 sound(freq);
	 delay(5);
	 timing++;
	 freq = freq+25;
	 } while (timing<80);
  nosound();
  return;
}

downWin()
{
 static int timing, freq;

 if (noNoises==TRUE) {
	return;
	}

 timing=0;
 freq=1500;
 sound(freq);
 do {
	 sound(freq);
	 delay(5);
	 timing++;
	 freq = freq-20;
	 } while (timing<100);
 nosound();
}
#endif
/* #endif */
upWin()
{
 static int timing, freq;

 if (noNoises==TRUE) {
	return;
	}

 timing=0;
 freq=600;
 do {
	 sound(freq);
	 delay(50);
	 timing++;
	 freq = freq+600;
	 nosound();
	 delay(10);
	 } while (timing<2);
  nosound();
  return;
}

downWin()
{
 static int timing, freq;

 if (noNoises==TRUE) {
	return;
	}

 timing=0;
 freq=1200;
 sound(freq);
 do {
	 sound(freq);
	 delay(50);
	 timing++;
	 freq = freq-600;
	 nosound();
	 delay(10);
	 } while (timing<2);
 nosound();
}
