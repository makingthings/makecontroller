//*----------------------------------------------------------------------------
//*         ATMEL Microcontroller Software Support  -  ROUSSET  -
//*----------------------------------------------------------------------------
//* The software is delivered "AS IS" without warranty or condition of any
//* kind, either express, implied or statutory. This includes without
//* limitation any warranty or condition with respect to merchantability or
//* fitness for any particular purpose, or against the infringements of
//* intellectual property rights of others.
//*----------------------------------------------------------------------------
//* File Name           : init.c
//* Object              : Low level initialisations written in C
//* Creation            : ODi   08/12/2003
//*
//*----------------------------------------------------------------------------
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;

typedef struct XMODEM
{
	// Public Methods:
	int (*AT91F_XMD_Xup)(char *pData, unsigned int length);
	int (*AT91F_XMD_Xdown)(char *pData, unsigned int length, unsigned int loopsPerSecond);
} XMODEM, *P_XMODEM;

P_XMODEM XMODEM_Open(P_XMODEM pXmodem,CFCPipeUSART *pUsart);

char gotachar(char *buf);
void putcharX(char data);
char getcharX();
ushort getbytes(char *pData, unsigned int length);
int putPacket(uchar *tmppkt, uchar sno);
int getPacket(char *pData, uchar sno);
int Xup(char *pData, unsigned int length);
int Xdown(char *pData, unsigned int length, unsigned int loopsPerSecond);


