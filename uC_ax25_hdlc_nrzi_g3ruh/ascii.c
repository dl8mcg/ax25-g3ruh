/*
 * ascii.c
 *
 * Created: 05.05.2024 07:51:47
 * Modified: 3/8/2025
 * Author: Hans
 */ 
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <util/atomic.h>
#include "board.h"
//#include "ascii.h"
#include "SerProg.h"

//char AsciiBuf[AsciiTextLen];
uint8_t Asciibyte;

uint8_t character;

uint8_t bitcnt;
uint8_t nullcnt;


void AsciiGetNextCharacter();
void AsciiSendNextBits();
void AsciiStopBit1();
void AsciiStopBit2();
void AsciiStop();

void AsciiSendNullStart();
void AsciiSendNull();
void AsciiSendMark1();
void AsciiSendMark2();


bool isAscii();

void InitAscii()
{
	smFSK=AsciiStop;
}

void SetAsciiIdleNull()
{
	nullcnt = 64;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{smFSK=AsciiSendNullStart;}
	sei();
	while(isAscii())
	{
	}
}

void AsciiStop()
{

}

void SetSyn()
{
	uint8_t i = 0;
	for(i=0; i<64; i++)
	{
		AsciiBuf[i] = 0x02;							// Ascii STX
	}
	
	AsciiBuf[i] = 0;
	character = 0;									// start of buffer
	cli();
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{smFSK = AsciiGetNextCharacter;}				// hole erstes Zeichen
	sei();
	while(isAscii())
	{
	}
}


void SetAsciiText(const char * buf)
{
	uint8_t i = 0;
	while(*buf !=0)
	{
		AsciiBuf[i++] = *buf++;
	}
	AsciiBuf[i] = 0;
	character = 0;									// start of buffer
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{smFSK = AsciiGetNextCharacter;}				// hole erstes Zeichen
	sei();
	while(isAscii())
	{
	}
}

void AsciiGetNextCharacter()
{
	Asciibyte = AsciiBuf[character++];					// get CW-pattern of next text-character

	if(Asciibyte == 0)									// end of buffer ?
	{
		smFSK = AsciiStop;
		return;
	}

	FskSpace();											//start-bit
	
	bitcnt = 8;

	smFSK = AsciiSendNextBits;
}

void AsciiSendNextBits()
{
	if(Asciibyte & 0x01)		// lsb first
	{
		FskMark();
	}
	else
	{
		FskSpace();
	}
	
	Asciibyte >>= 1;
	
	if(--bitcnt)
		return;
	smFSK = AsciiStopBit1;
}

void AsciiStopBit1()
{
	FskMark();											//stop-bit
	smFSK = AsciiStopBit2;	
}

void AsciiStopBit2()
{
	FskMark();											//stop-bit
	smFSK = AsciiGetNextCharacter;
}

bool isAscii()
{
	wdt_reset();
	return !(smFSK == AsciiStop);
}


void AsciiSendNullStart()
{
	FskSpace();
	bitcnt = 8;
	smFSK=AsciiSendNull;
}

void AsciiSendNull()
{
	if(--bitcnt)
		return;	
	smFSK=AsciiSendMark1;
}

void AsciiSendMark1()
{
	FskMark();
	smFSK=AsciiSendMark2;
}

void AsciiSendMark2()
{
	if(--nullcnt)
	{
		smFSK= AsciiSendNullStart;
		return;
	}
	smFSK = AsciiStop;
}
