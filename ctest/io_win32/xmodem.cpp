#include "stdafx.h"
#include "FC_ERROR.h"
#include "FCPipeUSART.h"

typedef struct XMODEM
{
	// Public Methods:
	int (*AT91F_XMD_Xup)(char *pData, unsigned int length);
	int (*AT91F_XMD_Xdown)(char *pData, unsigned int length, unsigned int loopsPerSecond);
} XMODEM, *P_XMODEM;


typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;

CFCPipeUSART uart;
unsigned int sizeOfData;
char timeOutError;

/* CRC-CCITT crc16 used primarily by Xmodem...
 */


ushort xcrc16tab[] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};


//*----------------------------------------------------------------------------
//* \fn    gotachar
//* \brief return 0 if no char 1 else.
//*----------------------------------------------------------------------------
static char gotachar(char *buf)
{
	return (uart.GotAChar((LPVOID)buf));
}

//*----------------------------------------------------------------------------
//* \fn    putchar
//* \brief
//*----------------------------------------------------------------------------
int putcharX(char data)
{
	if (uart.Write((LPVOID)&data, 1) != FC_OK)
			return -1;
	else return 0;
}

//*----------------------------------------------------------------------------
//* \fn    putchar
//* \brief
//*----------------------------------------------------------------------------
char getcharX(void)
{
	char msg;

	if (uart.Read((LPVOID)&msg, 1) != FC_OK){
		timeOutError = 1;
		return -2;
	}
	else return msg;

}

/* Runtime flags: */
#define USECRC  (1<<0)
#define VERIFY  (1<<1)
#define YMODEM  (1<<2)

/* Current xmodem operation: */
#define XNULL   0
#define XUP     1
#define XDOWN   2

/* X/Ymodem protocol: */
#define SOH     0x01
#define STX     0x02
#define EOT     0x04
#define ACK     0x06
#define NAK     0x15
#define CAN     0x18
#define ESC     0x1b

#define PKTLEN_128  128
#define PKTLEN_1K   1024


//*----------------------------------------------------------------------------
//* \fn    getbytes
//* \brief
//*----------------------------------------------------------------------------
static ushort getbytes(char *pData, unsigned int length)
{
	unsigned int cpt,i=0;
    ushort  crc = 0;
    char c;
	
	for (cpt = 0; cpt < length; ++cpt) {
		c = (uchar)getcharX();
		if(timeOutError)
			return -2;
		crc = (crc << 8) ^ xcrc16tab[0xff & ((crc >> 8) ^ c)];
		if(sizeOfData) {
			*pData++ = c;
			if(length == PKTLEN_128) sizeOfData--;
		}
	}
	return crc;
}




//*----------------------------------------------------------------------------
//* \fn    putPacket
//* \brief Used by Xup to send packets.
//*----------------------------------------------------------------------------
static int putPacket(uchar *tmppkt, uchar sno)
{
	int     i, timeout = 10;
	ushort  chksm;
	char temp = 'C', data;

	chksm = 0;

	putcharX(SOH);

	putcharX(sno);
	putcharX((uchar)~(sno));

	for(i=0;i<PKTLEN_128;i++) {
		if(sizeOfData) {
	    	data = *tmppkt++;
	    	sizeOfData--;
	    }
		else data = 0x00;
	    putcharX(data);

		chksm = (chksm << 8) ^ xcrc16tab[0xff & ((chksm >> 8) ^ data)];
	}
	/* An "endian independent way to extract the CRC bytes. */
	putcharX((char)(chksm >> 8));
	putcharX((char)(chksm));


	// Flush buffer
	while(temp == 'C')
		if(!uart.GotAChar((LPVOID)&temp)){
			timeout --;
			if(!timeout){
				timeOutError = 1;
				break;
			}
		}


	return(temp);  /* Wait for ack */
}

//*----------------------------------------------------------------------------
//* \fn    getPacket
//* \brief Used by Xdown to retrieve packets.
//*----------------------------------------------------------------------------
static int getPacket(char *pData, uchar sno)
{
    uchar   seq[2];
    ushort  crc, xcrc;

    getbytes((char *)seq,2);
	if(timeOutError)
			return -2;
    xcrc = getbytes(pData,PKTLEN_128);
	if(timeOutError)
			return -2;

    /* An "endian independent way to combine the CRC bytes. */
    crc  = (ushort)getcharX() << 8;
    crc |= (0xff & getcharX());
    if ((crc != xcrc) || (seq[0] !=  sno) || (seq[1] != (uchar) (~sno))) {
        putcharX(CAN);
        return(-1);
    }

    putcharX(ACK);
    return(0);
}

//*----------------------------------------------------------------------------
//* \fn    Xup
//* \brief Called when a transfer from host to target is being made (considered
//*        an upload).
//*----------------------------------------------------------------------------
static int Xup(char *pData, unsigned int length)
{
    uchar   c, sno = 1;
    int     done;

	sizeOfData = length;
	timeOutError = 0;

    if (length & (PKTLEN_128-1)) {
        length += PKTLEN_128;
        length &= ~(PKTLEN_128-1);
    }

    /* Startup synchronization... */
    /* Wait to receive a NAK or 'C' from receiver. */
    done = 0;
    while(!done) {
        c = (uchar)getcharX();
		if(timeOutError)
			return -2;
        switch(c) {
        case NAK:
            done = 1;
            // ("CSM");
            break;
        case 'C':
            done = 1;
            // ("CRC");
            break;
        case 'q':   /* ELS addition, not part of XMODEM spec. */
            return(0);
        default:
            break;
        }
    }


    done = 0;
    sno = 1;
    while(!done) {
        c = (uchar)putPacket((uchar *)pData, sno); 
		if(timeOutError)
			return -2;
        switch(c) {
        case ACK:
        	++sno;
            length -= PKTLEN_128;
            pData += PKTLEN_128;
            // ("A");
            break;
        case NAK:
            // ("N");
            break;
        case CAN:
			return -1;
        case EOT:
        default:
            done = -1;
            break;
        }
        if (length == 0) {
            putcharX(EOT);
            getcharX();  /* Flush the ACK */
            break;
        }
        // ("!");
    }

    // ("Xup_done.");
    return(0);
}


//*----------------------------------------------------------------------------
//* \fn    Xdown
//* \brief Called when a transfer from target to host is being made (considered
//*        an download).
//*----------------------------------------------------------------------------
static int Xdown(char *pData, unsigned int length, unsigned int loopsPerSecond)
{
	long    timeout;
	char    c, firsttime = 0;
	int     done;
	uchar   sno = 0x01;
	int     dataTransfered = 0;

	sizeOfData = length;
	timeOutError = 0;


    /* Startup synchronization... */
    /* Continuously send NAK or 'C' until sender responds. */
	// ("Xdown");
	while(1) {
	    putcharX('C');
		if(timeOutError)
			return -2;
		timeout = loopsPerSecond;
		while(!gotachar(&c) && timeout)
		    timeout--;
		if (timeout)
		    break;
		firsttime++;
		if(firsttime > 3) return -2;
	}
	firsttime = 0;
    done = 0;
    // ("Got response");
    while(done == 0) {
		if(firsttime){ 
			c = (char)getcharX(); 
			if(timeOutError)
					return -2;
		}
		else firsttime = 1;
        switch(c) {
        case SOH:               /* 128-byte incoming packet */
            // ("O");
            done = getPacket(pData, sno);
            if (done == 0) {
            	++sno;
            	pData += PKTLEN_128;
            	dataTransfered += PKTLEN_128;
            }
            break;
        case EOT:   // ("E");
            putcharX(ACK);
             if(length > (unsigned int)dataTransfered)
				 done = dataTransfered;
			 else done = length;
             break;
        case CAN:       // ("C");
        case ESC:       /* "X" User-invoked abort */
        default:
            done = -1;
            break;
        }
        // ("!");

    }
    return(done);
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_XMODEM_Open
//* \brief Initialize XMODEM service
//*----------------------------------------------------------------------------
P_XMODEM XMODEM_Open(P_XMODEM pXmodem, CFCPipeUSART *pUsart)
{
	uart = *pUsart;
	pXmodem->AT91F_XMD_Xup   = Xup;
	pXmodem->AT91F_XMD_Xdown = Xdown;
	
	return pXmodem;
}

