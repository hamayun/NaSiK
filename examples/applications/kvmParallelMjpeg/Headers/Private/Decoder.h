/*
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Patrice GERIN patrice.gerin@imag.fr
 * Bug Fixer(s) :   Xavier GUERIN xavier.guerin@imag.fr
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>
#include <Private/Mjpeg.h>
#include <KahnProcessNetwork/KahnProcessNetwork.h>
#include <Processor/Processor.h>

/*
 * Definition of the thread signature
 */

extern int32_t decoder (kpn_channel_t c[2]);

/*
 * UnZZ table
 */

extern const uint8_t G_ZZ[64]; 

/*
 * Decompression functions
 */

static inline void skip_segment (kpn_channel_t channel)
{
	union
  {
		uint16_t segment_size;
		uint8_t size[2];
	}
  u;

	kpn_channel_read (channel, & u . size[0], 1);
	kpn_channel_read (channel, & u . size[1], 1);
	cpu_data_is_big_endian (16, u . segment_size);

	IPRINTF("Skip segment (%d data)\r\n",(unsigned int) u . segment_size);
	kpn_channel_skip (channel, u . segment_size - 2); 
}

static inline void load_huffman_table (kpn_channel_t channel,
    DHT_section_t *DHT_section, huff_table_t * ht)
{
	uint8_t buffer = 0;
	int32_t size = 0, max = 0;
	int32_t LeavesN = 0, LeavesT = 0, i = 0;
	int32_t AuxCode = 0;

	for (i = 0; i < 16; i++)
  {
		kpn_channel_read (channel, & buffer, 1);
		LeavesN = buffer;
		ht -> ValPtr[i] = LeavesT;
		ht -> MinCode[i] = AuxCode * 2;
		AuxCode = ht -> MinCode[i] + LeavesN;
		ht -> MaxCode[i] = (LeavesN) ? (AuxCode - 1) : (-1);
		LeavesT += LeavesN;
	}

	size = DHT_section -> length - 2 - 1 - 16;
	if (LeavesT > MAX_SIZE((DHT_section -> huff_info & 0x10)))
  {
		max = MAX_SIZE((DHT_section -> huff_info & 0x10));
		VPRINTF("WARNING: Truncating Table by %lu symbols\r\n", LeavesT - max);
	}
	else max = LeavesT;

	kpn_channel_read (channel, ht -> table, max);
	kpn_channel_skip (channel, LeavesT - max);
}

static inline uint32_t get_bits (kpn_channel_t channel,
    scan_desc_t *scan_desc, uint8_t number)
{
	int32_t i = 0, newbit = 0;
	uint32_t result = 0;
	uint8_t wwindow = 0, aux = 0;

	if (number == 0) return 0;

	for (i = 0; i < number; i++)
  {
		if (scan_desc -> bit_count == 0)
    {
			kpn_channel_read (channel, & wwindow, 1);
			scan_desc -> bit_count = 8;
			if (wwindow == 0xFF) kpn_channel_read (channel, & aux, 1); 
		}
		else wwindow = scan_desc->window;

		newbit = (wwindow >> 7) & 1;
		scan_desc -> window = wwindow << 1;
		scan_desc -> bit_count -= 1;
		result = (result << 1) | newbit;
	}

	return result;
}

static inline uint8_t get_symbol (kpn_channel_t channel, scan_desc_t *scan_desc,
    uint32_t acdc, uint32_t component)
{
	uint8_t temp = 0;
	int32_t code = 0;
	uint32_t length = 0, index = 0;
	huff_table_t * HT = scan_desc -> table[acdc][component];

	for (length = 0; length < 16; length++)
  {
		temp = get_bits (channel, scan_desc, 1);
		code = (2 * code) | temp;
		if (code <= HT -> MaxCode[length]) break;
	}

	if (length != 16)
  {
		index = HT -> ValPtr[length] + code - HT -> MinCode[length];
		if (index < MAX_SIZE(acdc)) return HT -> table[index];
	}

	return 0;
}

static inline void unpack_block (kpn_channel_t channel, scan_desc_t * scan_desc,
    uint32_t index, int32_t T[64])
{
	uint32_t temp = 0, i = 0, run = 0, cat = 0;
	int32_t value = 0;
	uint8_t symbol = 0;

	memset ((void *) T, 0, 64 * sizeof (int32_t));
	symbol = get_symbol (channel, scan_desc, HUFF_DC, index);
	temp = get_bits (channel, scan_desc, symbol);

	value = reformat (temp, symbol);
	value += scan_desc -> pred[index];
	scan_desc -> pred[index] = value;

	T[0] = value ;

	for (i = 1; i < 64; i++)
  {
		symbol = get_symbol (channel, scan_desc, HUFF_AC, index);

		if (symbol == HUFF_EOB) 
    {
			break;
		}

		if (symbol == HUFF_ZRL)
    {
			i += 15;
			continue;
		}

		cat = symbol & 15;
		run = (symbol >> 4) & 15;
		i += run;
		temp = get_bits (channel, scan_desc, cat);
		value = reformat(temp, cat);
		T[i] = value ;    
	}
}

static inline void iqzz_block(int32_t in[64],
    int32_t out[64], uint8_t table[64])
{
	uint32_t index;

	for( index = 0; index < 64 ; index++)
  {
		out[G_ZZ[index]] = in[index] * table[index];
  }
}

#endif
