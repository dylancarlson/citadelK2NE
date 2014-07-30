
typedef unsigned char byte;

#define VIDEO 16

#ifdef VMODULE
#   define SCOPE /* */
#else
#   define SCOPE extern
#endif

/* #define NORM     0x01D  */    /* BF */             /* normal text  07*/
/* #define CONTRAST 0x01F  */    /* BF */             /* set-off text 70 */

void vsetmode ( byte mode );
byte vgetmode ( void );
void vsetpage ( byte page );
void vsetup ( void );
void vlocate ( byte row, byte col );
int  vputs ( char *string );

void video ( char *tagline );
void statusline ( char *string );

byte vputch ( byte c );
