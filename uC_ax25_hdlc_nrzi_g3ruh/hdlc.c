/*
 * hdlc.c
 *
 * Created: 24.07.2024 21:23:46
 * Modified: 25.12.2025
 * Author: DL8MCG
 */ 

#include <avr/pgmspace.h>
#include <string.h>
#include <stdio.h>
#include "hdlc.h"
#include "AX25.h"

#define AX25_ADDR_LEN 7
#define MAX_CALLSIGN_LEN 7
#define FLAG 0x7E
#define MAX_FRAME_LEN 375 //275
#define MAX_PAYLOAD_LEN 375 //275

// AX.25 Frame Struktur
typedef struct {
	uint8_t type;                           // AX.25 Version (0x01 = V2)
	char dest_call[MAX_CALLSIGN_LEN];       // Ziel-Rufzeichen
	uint8_t dest_ssid;                      // Ziel-SSID
	char src_call[MAX_CALLSIGN_LEN];        // Quell-Rufzeichen
	uint8_t src_ssid;                       // Quell-SSID
	uint8_t control;                        // Control-Feld
	uint8_t pid;                            // Protocol ID
	char payload[MAX_PAYLOAD_LEN];          // Nutzdaten
	uint16_t payload_len;                   // Länge der Nutzdaten
} AX25_Frame_t;

// HDLC Frame Struktur
typedef struct {
	uint8_t data[MAX_FRAME_LEN];           // Encoded HDLC Daten
	uint16_t length;                       // Länge des HDLC Frames
} HDLC_Frame_t;

static AX25_Frame_t ax25_frame;
static HDLC_Frame_t hdlc_frame;
	
int AX25_EncodeHDLC(const AX25_Frame_t* ax25_frame, HDLC_Frame_t* hdlc_frame);
void encode_callsign(uint8_t type, uint8_t* dest, const char* callsign, uint8_t ssid, uint8_t last);
int HDLCEncode(const uint8_t* frame, int len, uint8_t* output);

// Statische CRC-16-CCITT-Tabelle (MSB-first, Polynom 0x1021, Initialwert 0xFFFF)
static const uint16_t crc_table[256] PROGMEM =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

// --- CRC-Berechnungsfunktion per Tabelle ---
uint16_t CRCCalculation(const uint8_t* frame, size_t len)
{
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < len; ++i)
	{
		uint8_t j = (frame[i] ^ (crc >> 8)) & 0xFF;
		//crc = crc_table[j] ^ (crc << 8);
		crc = pgm_read_word(&crc_table[j]) ^ (crc << 8);
		
	}
	return crc ^ 0xFFFF;
}

// --- Hilfsfunktion: Bit-Reversal eines Bytes ---
uint8_t ReverseBits(uint8_t byte)
{
	byte = ((byte >> 1) & 0x55) | ((byte & 0x55) << 1);
	byte = ((byte >> 2) & 0x33) | ((byte & 0x33) << 2);
	byte = ((byte >> 4) & 0x0F) | ((byte & 0x0F) << 4);
	return byte;
}

// --- HDLC-Encoding mit Bit-Stuffing und Flag-Frames ---
int HDLCEncode(const uint8_t* frame, int len, uint8_t* output)
{
	static uint8_t temp[MAX_FRAME_LEN];
	int temp_len = 0;

	// Schritt 1: Bit-Reversal jedes Bytes
	for (int i = 0; i < len; i++)
	{
		temp[temp_len++] = ReverseBits(frame[i]);
	}

	// Schritt 2: CRC-Berechnung
	uint16_t crc = CRCCalculation(temp, temp_len);  // schnelle Version über Tabelle

	temp[temp_len++] = ((crc >> 8) & 0xFF);         // CRC MSB
	temp[temp_len++] = (crc & 0xFF);                // CRC LSB

	// Schritt 3: Startflag setzen
	int out_len = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = 0;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	
	
	// Schritt 4: Bit-Stuffing
	int bit_count = 0;
	int bit_pos = 7;
	uint8_t byte = 0;
	
	    for (int i = 0; i < temp_len; i++)
	    {
		    for (int b = 0; b < 8; b++)
		    {
			    uint8_t bit = (temp[i] >> (7 - b)) & 0x01;

			    if (bit)
			    {
				    byte |= (1 << bit_pos);
				    bit_count++;
			    }
			    else
			    {
				    bit_count = 0;
			    }

			    bit_pos--;

			    if (bit_pos < 0)
			    {
				    output[out_len++] = byte;
				    byte = 0;
				    bit_pos = 7;
			    }

			    if (bit_count == 5)
			    {
				    // Bit-Stuffing: füge eine 0 ein 
				    bit_pos--;

				    if (bit_pos < 0)
				    {
					    output[out_len++] = byte;
					    byte = 0;
					    bit_pos = 7;
				    }

				    bit_count = 0;
			    }
		    }
	    }
		
		// Schritt 5: Endflag setzen
		if (bit_pos != 7)
		{
			uint16_t ende = 0x007E << (bit_pos + 1);
			ende |= byte << 8;
			output[out_len++] = (ende >> 8) & 0xFF;
			output[out_len++] = (ende & 0xFF);
		}
		else
			output[out_len++] = FLAG;

	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;
	output[out_len++] = FLAG;


	    return out_len;
    }
	
	// AX.25 Rufzeichen in Adressfeld umwandeln
	void encode_callsign(uint8_t type, uint8_t* dest, const char* callsign, uint8_t ssid, uint8_t last)
	{
		int i;
		for (i = 0; i < 6 && callsign[i]; i++)
		{
			dest[i] = callsign[i] << 1;
		}
		while (i < 6) dest[i++] = ' ' << 1;
		dest[6] = ((ssid & 0x0F) << 1) | 0x60 | (last ? 0x01 : 0x00) | (type ? 0x80 : 0x00);
	}

	// AX25-frame erzeugen
	void SetAX25Text(
					uint8_t type,
					const char *dest_call,
					uint8_t dest_ssid,
					const char *src_call,
					uint8_t src_ssid,
					uint8_t control,
					uint8_t pid,
					const char *format,    // Format-String für Payload
					...
				  )
	 {
		uint16_t len = 0;
		
		// Grunddaten setzen
		ax25_frame.type = type;

		// Callsigns kopieren 
		strncpy(ax25_frame.dest_call, dest_call, MAX_CALLSIGN_LEN - 1);
		ax25_frame.dest_call[MAX_CALLSIGN_LEN - 1] = '\0';

		strncpy(ax25_frame.src_call, src_call, MAX_CALLSIGN_LEN - 1);
		ax25_frame.src_call[MAX_CALLSIGN_LEN - 1] = '\0';

		ax25_frame.dest_ssid = dest_ssid;
		ax25_frame.src_ssid = src_ssid;
		ax25_frame.control = control;
		ax25_frame.pid = pid;

		// Variadische Argumente für Payload formatieren
		va_list args;
		va_start(args, format);
		vsnprintf(ax25_frame.payload, sizeof(ax25_frame.payload), format, args);
		va_end(args);

		// Payload-Länge berechnen
		ax25_frame.payload_len = strlen(ax25_frame.payload);
		
		len = AX25_EncodeHDLC(&ax25_frame, &hdlc_frame);
		
		SendAX25(hdlc_frame.data, len);
	}
	
int AX25_BuildFrame(const AX25_Frame_t* frame, uint8_t* output, size_t output_size)
	{
		if (!frame || !output) return 0;

		uint16_t len = 0;

		// 1) Zieladresse
		if (len + AX25_ADDR_LEN > output_size) return 0;
		encode_callsign(1, output + len, frame->dest_call, frame->dest_ssid, 0);
		len += AX25_ADDR_LEN;

		// 2) Quelladresse
		if (len + AX25_ADDR_LEN > output_size) return 0;
		encode_callsign(0, output + len, frame->src_call, frame->src_ssid, 1);
		len += AX25_ADDR_LEN;

		// 3) Control und PID
		if (len + 2 > output_size) return 0;
		output[len++] = frame->control;
		output[len++] = frame->pid;

		// 4) Payload
		if (frame->payload_len > 0) 
		{
			// Falls payload_len größer ist als der Rest des Puffers ? Fehler
			if (len + frame->payload_len > output_size) return 0;

			memcpy(output + len, frame->payload, frame->payload_len);
			len += frame->payload_len;
		}

		return len;
	}
	
	
	// HDLC Frame aus AX.25 Frame
int AX25_EncodeHDLC(const AX25_Frame_t* ax25_frame, HDLC_Frame_t* hdlc_frame)
	{
		if (!ax25_frame || !hdlc_frame) 
			return 0;

		static uint8_t raw_frame[MAX_FRAME_LEN];
		int raw_len = AX25_BuildFrame(ax25_frame, raw_frame, sizeof(raw_frame));

		if (raw_len == 0) 
			return 0;

		hdlc_frame->length = HDLCEncode(raw_frame, raw_len, hdlc_frame->data);

		return hdlc_frame->length;
	}

