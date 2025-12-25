/*
 * timer.c
 *
 * Created: 18.04.2024 21:40:32
 * Modified: 25.12.2025
 * Author: DL8MCG
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "board.h"

#define prescaler 8.0

volatile static uint16_t normal_divider;
volatile static uint16_t ctc_divider;

void init_timer_ctc(float firq)
{
	// Timer1 konfigurieren (16-Bit Timer) im CTC-Modus
	TCCR1B |= (1 << WGM12);		// CTC-Modus aktivieren
	TCCR1B |= (1 << CS11);		// Prescaler auf 8 setzen, CS10 und CS12 auf 0 setzen
	TIMSK |= (1 << OCIE1A);		// Output Compare Match A Interrupt aktivieren
	
	// Vergleichswert für CTC-Modus setzen, damit alle 1/firq s ein Interrupt ausgelöst wird
	
	ctc_divider = F_CPU/prescaler/firq - 1;
	OCR1A = ctc_divider;
}

ISR(TIMER1_COMPA_vect)			// IRQ CTC-Timer
{
	smFSK();
}

void init_timer_normal(float firq)		
{
	// Timer1 konfigurieren (16-Bit Timer)
	TCCR1B |= (1 << CS11);	// Prescaler auf 8 setzen
	TIMSK |= (1 << TOIE1);  // Overflow Interrupt für Timer1 aktivieren

	// TCNT1 auf den Wert setzen, damit nach 1/firq s ein Interrupt ausgelöst wird
	normal_divider = (65536 - F_CPU/prescaler/firq);  
	TCNT1 = normal_divider; 
}

ISR(TIMER1_OVF_vect)			// IRQ normal-Timer	
{
	// TCNT1 neu setzen, damit wieder nach 1/firq s ein Interrupt ausgelöst wird
	TCNT1 = normal_divider;
	smFSK();
}
