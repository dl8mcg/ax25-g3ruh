/*
 * board.h
 *
 * Created: 29.06.2024 
 * Modified: 25.12.2025
 * Author: DL8MCG
 */ 

#ifndef BOARD_H_
#define BOARD_H_

#define F_CPU 14745600UL   // Quarz-Frequenz

/*****  Pin-Zuordnung  *******************************************************/
/*****************************************************************************/

#define FSK					_BV(PB4)
#define FSK_DDR_OUT			DDRB  |= FSK
#define FSK_HIGH			PORTB |= FSK
#define FSK_LOW				PORTB &= ~FSK
#define FSK_TOGGLE			PORTB ^= FSK
#define FSK_DDR_IN			DDRB  &= ~FSK
#define FSK_DDR_IN_HighZ	do{DDRB  &= ~FSK; PORTB &= ~FSK;}while(0)

#define KEY					_BV(PD4)				
#define KEY_DDR_OUT			DDRD  |= KEY
#define KEY_ON				PORTD |= KEY
#define KEY_OFF				PORTD &= ~KEY
#define KEY_TOGGLE			PORTD ^= KEY

void (* volatile smFSK)(void);		// Funktionszeiger

#endif /* BOARD_H_ */