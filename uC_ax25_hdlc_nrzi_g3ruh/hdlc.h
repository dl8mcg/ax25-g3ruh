/*
 * hdlc.h
 *
 * Created: 26.07.2025 20:50:58
 * Modified: 25.12.2025
 * Author: DL8MCG
 */ 

#ifndef HDLC_H_
#define HDLC_H_

void SetAX25Text(
				uint8_t type,
				const char *dest_call,
				uint8_t dest_ssid,
				const char *src_call,
				uint8_t src_ssid,
				uint8_t control,
				uint8_t pid,
				const char *format,    // Format-String für Payload
				...                    // Variadische Argumente für snprintf
			  );

#endif /* HDLC_H_ */