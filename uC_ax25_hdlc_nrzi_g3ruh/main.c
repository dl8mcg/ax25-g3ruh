/*
 * main.c
 *
 * Created: 25.12.2025
 * Modified: 
 * Author: DL8MCG
 */ 

#include <xc.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdint.h>
#include "board.h"
#include "timer.h"
#include "AX25.h"
#include "hdlc.h"

volatile uint8_t packetcnt;

int main(void)
{
	wdt_enable(WDTO_2S);			// Watchdog auf 2 s setzen
	KEY_DDR_OUT;
	FSK_DDR_OUT;
	
	cli();
	init_timer_ctc(9600);			// Datenrate in bit pro sekunde
	InitAX25();
				 
	while(1)
	{
		SetAX25Text(
					0x01,               // type AX.25 V2
					"EU", 0,			// Zielrufzeichen + SSID
					"nocall", 0,        // Quellrufzeichen + SSID
					0x03,               // Control (UI-Frame)
					0xF0,               // PID (No Layer 3)
					"Hallo Test Test Test\n\r"
					"# %d\n\r", packetcnt++
					);
					
		SetAX25Text(
					0x01,               // type AX.25 V2
					"QST", 0,           // Zielrufzeichen + SSID
					"nocall", 0,        // Quellrufzeichen + SSID
					0x03,               // Control (UI-Frame)
					0xF0,               // PID (No Layer 3)
					"The quick brown fox jumps over the lazy dog\n\r"
					"abcdefghijklmnopqrstuvwxyz\n\r"
					"0123456789\n\r"
					"# %d\n\r", packetcnt++
					);
								
		SetAX25Text(
					0x01,               // type AX.25 V2
					"ALL", 0,           // Zielrufzeichen + SSID
					"nocall", 0,        // Quellrufzeichen + SSID
					0x03,               // Control (UI-Frame)
					0xF0,               // PID (No Layer 3)
					"Sample codes at github.com/dl8mcg\n\r"
					"# %d\n\r", packetcnt++
					);	
			
		SetAX25Text(
					0x01,               // type AX.25 V2
					"WORLD", 0,           // Zielrufzeichen + SSID
					"nocall", 0,        // Quellrufzeichen + SSID
					0x03,               // Control (UI-Frame)
					0xF0,               // PID (No Layer 3)
					"Test 0815 Test 4711\n\r"
					"# %d\n\r", packetcnt++
					);
	}
}