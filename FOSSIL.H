/* definitions for use with FOSSIL driver */

#define F_SIGNATURE	0x1954

/* Port IDs */
#define F_COM1		0
#define F_COM2		1
#define F_COM3		2
#define F_COM4		3
#define F_CONSOLE	0xFF

/* Baud Rates */
#define F_B300		(0x2 << 5)
#define F_B600		(0x3 << 5)
#define F_B1200		(0x4 << 5)
#define F_B2400		(0x5 << 5)
#define F_B4800		(0x6 << 5)
#define F_B9600		(0x7 << 5)
#define F_B19200	(0x0 << 5)
#define F_B38400	(0x1 << 5)

/* Character Framing */
#define F_NOPARITY	(0x0 << 3)
#define F_ODDPARITY	(0x1 << 3)
#define F_NOPARITY2	(0x2 << 3)
#define F_EVENPARITY	(0x3 << 3)

#define F_STOP1		(0x0 << 2)
#define F_STOP2		(0x1 << 2)

#define F_BITS5		(0x0)
#define F_BITS6		(0x1)
#define F_BITS7		(0x2)
#define F_BITS8		(0x3)

#define F_NORMAL	(F_NOPARITY|F_STOP1|F_BITS8)
#define F_UNIX		(F_EVENPARITY|F_STOP1|F_BITS7)

/* FOSSIL status bits */
#define F_RING      (0x0040)
#define F_DCD		(0x0080)
#define F_RDA		(0x0100)
#define F_OVRN		(0x0200)
#define F_THRE		(0x2000)
#define F_TSRE		(0x4000)

/* Input and Output decoding */
#define CR              (0x000D)     /* Carraige Return */
#define LF              (0x000A)     /* Line Feed */
#define NDBS            (0x0008)     /* BackSpace (Non Destructive) */
/* #define ON 1 */
/* #define OFF 0 */

/* Function prototypes */
unsigned int f_baud(unsigned int, unsigned int);
unsigned int f_tx(unsigned int, char);
unsigned int f_rx(unsigned int);
unsigned int f_status(unsigned int);

#ifdef OPTION_ON
unsigned int f_init(unsigned int, unsigned int *, char far *);
#else
unsigned int f_init(unsigned int);
#endif
void f_exit(unsigned int);
void f_dtr(unsigned int, unsigned int);
unsigned int f_tickparms(unsigned int *);
void f_flusho(unsigned int);
void f_purgeo(unsigned int);
void f_purgei(unsigned int);
unsigned int f_poke(unsigned int, char);
unsigned int f_peek(unsigned int);
unsigned int f_kbdpeek(void);
unsigned int f_kbd(void);
void f_flow(unsigned int, unsigned char);
unsigned int f_control(unsigned int, unsigned char);
void f_gotoxy(unsigned int, unsigned int);
unsigned int f_wherexy(unsigned int *, unsigned int *);
void f_dpyansi(char);
void f_watchdog(unsigned int, int);
void f_dpybios(char);
unsigned int f_tickchain(int, void (far *)());
void f_reboot(unsigned int);
unsigned int f_rxblk(unsigned int, void far *, unsigned int);
unsigned int f_txblk(unsigned int, void far *, unsigned int);
void f_break(unsigned int, unsigned int);
unsigned int f_info(unsigned int, void far *, unsigned int);
unsigned int f_addapp(unsigned int, unsigned int *, void (far *)());
unsigned int f_remapp(unsigned int, unsigned int *, void (far *)());
void f_txputs(unsigned int, char *, unsigned int);
