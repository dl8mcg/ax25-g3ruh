/*
 * AX25.h
 *
 * Created: 12.05.2024 21:19:11
 * Modified: 25.12.2025
 * Author: DL8MCG
 */ 

#ifndef AX25_H_
#define AX25_H_

void InitAX25();

void SendAX25(const uint8_t * buf, uint16_t size);

#endif /* AX25_H_ */