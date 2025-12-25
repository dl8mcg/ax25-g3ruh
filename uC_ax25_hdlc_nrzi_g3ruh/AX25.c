/*
 * AX25.c
 *
 * Created: 12.05.2024 21:18:55
 * Modified: 25.12.2025
 * Author: DL8MCG
 */ 

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <util/atomic.h>
#include "AX25.h"
#include "board.h"

#define lenAX25Buf 275

uint8_t AX25Buf[lenAX25Buf];

uint8_t character;
uint8_t AX25byte;

void AX25Stop();
void AX25GetNextCharacter();
void AX25SendNextBits();

bool isAX25();

uint8_t syncbyte;
uint8_t syncnt;
uint8_t bitcnt;
uint16_t bytecnt;

static volatile bool freq;
static volatile uint32_t g3ruh_lsr = 0x21000;


void InitAX25()
{
	freq = 0;
	cli();
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{smFSK=AX25Stop;}		
}

void SendAX25(const uint8_t * buf, uint16_t size)
{
	uint16_t i = 0;
	while(i<size)
	{
		AX25Buf[i++] = *buf++;
	}
	AX25Buf[i] = 0;
	character = 0;								// start of buffer
	cli();
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{smFSK = AX25GetNextCharacter;}				// hole erstes Zeichen
	bytecnt = size;
	sei();
	while(isAX25())
	{
	}
}
		
void AX25Stop()
{

}

bool isAX25()
{
	wdt_reset();
	return !(smFSK == AX25Stop);
}

void scramble_g3ruh()
{
	bool bit_in;
	
	if(AX25byte & 0x80)
		bit_in = 1;
	else
		bit_in = 0;
	
    // Feedback-Bits aus dem Shiftregister:
    // Positionen 12 und 17 -> Index 11 und 16 (wenn wir bei 0 zählen)
    bool fb12 = (g3ruh_lsr >> 11) & 0x01;
    bool fb17 = (g3ruh_lsr >> 16) & 0x01;

    // Scrambled Bit
    bool bit_out = bit_in ^ fb12 ^ fb17;

    // Shiftregister aktualisieren: neues Ausgangsbit hinein schieben
    g3ruh_lsr = ((g3ruh_lsr << 1) | bit_out) & 0x1FFFF; // 17 Bits maske

	if(bit_out == 0)
	{
		freq = !freq;
	}
	
	if(freq == 0)				// bei 0-bits Flankenwechsel (NRZI)
		FSK_LOW;
	else
		FSK_HIGH;	
}

void AX25GetNextCharacter()
{
	if(!bytecnt)									// end of buffer ?
	{
		KEY_OFF;
		FSK_DDR_IN_HighZ;
		smFSK = AX25Stop;
		return;
	}	
	KEY_ON;
	FSK_DDR_OUT;
	bytecnt--;
	AX25byte = AX25Buf[character++];				// get next character
	
	scramble_g3ruh();

	AX25byte <<= 1;
	bitcnt = 1;
	smFSK = AX25SendNextBits;
}

void AX25SendNextBits()
{
	scramble_g3ruh();
	
	AX25byte <<= 1;
	bitcnt++;
	if(bitcnt >= 8)
		smFSK = AX25GetNextCharacter;
	return;
}



