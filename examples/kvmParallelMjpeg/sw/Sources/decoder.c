/*
 * Copyright (C) 2007 TIMA Laboratory
 *
 * Author(s):
 *   Patrice GERIN, patrice.gerin@imag.fr
 *   Xavier GUERIN, xavier.guerin@imag.fr
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdbool.h>
#include <malloc.h>

#include <Private/Decoder.h>
#include <Private/Computer.h>
#include <Private/Builder.h>

/*
 * Define the ZZ inversion table
 */

const uint8_t G_ZZ[64] =
{
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/*
 * The MJPEG decoder function
 */

int32_t decoder (kpn_channel_t c[2])
{
	bool dispatch_info = true;

	uint8_t marker[2], HT_type = 0, HT_index = 0;
	uint8_t DQT_table[4][64], index = 0, QT_index = 0;
	uint8_t * picture = NULL, * DECODED_FLIT = NULL;

	uint16_t nb_MCU = 0, nb_MCU_sx = 0;

  int32_t MCU[64], * FLIT = NULL;
	uint32_t YH = 0, YV = 0, flit_size = 0, mcu_size = 0;
	uint32_t LB_X = 0, LB_Y = 0;

	jfif_header_t	jfif_header;
	DQT_section_t	DQT_section;
	SOF_section_t	SOF_section;
	SOF_component_t	SOF_component[3];
	DHT_section_t	DHT_section;
	SOS_section_t SOS_section;
	SOS_component_t	SOS_component[3];
	scan_desc_t	scan_desc = {0, 0, {}, {}};
	huff_table_t tables[2][4];

	for (HT_index = 0; HT_index < 4; HT_index++)
  {
		tables[HUFF_DC][HT_index] . table = (uint8_t *) malloc(MAX_SIZE(HUFF_DC));
		if (tables[HUFF_DC][HT_index] . table == NULL)
    {
      printf ("%s,%d: malloc failed\n", __FILE__, __LINE__);
    }

		tables[HUFF_AC][HT_index] . table = (uint8_t *) malloc(MAX_SIZE(HUFF_AC));
		if (tables[HUFF_AC][HT_index] . table == NULL)
    {
      printf ("%s,%d: malloc failed\n", __FILE__, __LINE__);
    }
	} 

  /*
   * Computation loop
   */

	while (1) 
  {
    kpn_channel_read (c[0], marker, 2);

		if (marker[0] == M_SMS)
    {
			switch (marker[1])
      {
				case M_SOF0: {
					IPRINTF("SOF0 marker found\r\n");

					kpn_channel_read (c[0], & SOF_section, sizeof (SOF_section));
					cpu_data_is_big_endian(16, SOF_section . length);
					cpu_data_is_big_endian(16, SOF_section . height);
					cpu_data_is_big_endian(16, SOF_section . width);

					VPRINTF("Data precision = %d\r\n", SOF_section . data_precision);
					VPRINTF("Image height = %d\r\n", SOF_section . height);
					VPRINTF("Image width = %d\r\n", SOF_section . width);
					VPRINTF("%d component%c\r\n", SOF_section . n,
              (SOF_section . n > 1) ? 's' : ' ');

					kpn_channel_read (c[0], & SOF_component,
              sizeof (SOF_component_t) * SOF_section . n);

					YV = SOF_component[0] . HV & 0x0f;
					YH = (SOF_component[0] . HV >> 4) & 0x0f;

					VPRINTF("Subsampling factor = %lux%lu\r\n", YH, YV);

					/*
           * We only dispatch the picture info once, since we suppose
           * all the picture do have the same format
           */

					if (dispatch_info)
          {
						nb_MCU_sx = intceil(SOF_section.width, MCU_sx);
						flit_size = YV * nb_MCU_sx + (SOF_section . n - 1) * nb_MCU_sx / YH;
						mcu_size = YV * YH + SOF_section . n - 1;

            /*
             * We reserve enough memory to work
             */

            FLIT = (int32_t *) malloc (flit_size * 64 * sizeof (int32_t));
            DECODED_FLIT = (uint8_t *) malloc (flit_size * 64);
            picture = (uint8_t *) malloc (SOF_section . width
                * SOF_section . height * 2);

						dispatch_info = false;
					}

					break;
				}

				case M_SOF1: {
					IPRINTF("SOF1 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF2: {
					IPRINTF("SOF2 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF3: {
					IPRINTF("SOF3 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_DHT: {
					IPRINTF("DHT marker found\r\n");

					kpn_channel_read (c[0], & DHT_section, sizeof (DHT_section));
					cpu_data_is_big_endian(16, DHT_section . length);

					HT_index = DHT_section . huff_info & 0x0f;
					HT_type = (DHT_section . huff_info >> 4) & 0x01;

					VPRINTF("Huffman table index is %d\r\n", HT_index);
					VPRINTF("Huffman table type is %s\r\n", HT_type ? "AC" : "DC");

					VPRINTF("Loading Huffman table\r\n");
					load_huffman_table (c[0], & DHT_section,
              & tables[HT_type][HT_index]);

					break;
				}

				case M_SOF5: {
					IPRINTF("SOF5 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF6: {
					IPRINTF("SOF6 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF7: {
					IPRINTF("SOF7 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_JPG: {
					IPRINTF("JPG marker found\r\n");
					skip_segment (c[0]);
					break;
				}
	
				case M_SOF9: {
					IPRINTF("SOF9 marker found\r\n");
					skip_segment (c[0]);
					break;
				}
				case M_SOF10: {
					IPRINTF("SOF10 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF11: {
					IPRINTF("SOF11 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_DAC: {
					IPRINTF("DAC marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF13: {
					IPRINTF("SOF13 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF14: {
					IPRINTF("SOF14 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOF15: {
					IPRINTF("SOF15 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST0: {
					IPRINTF("RST0 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST1: {
					IPRINTF("RST1 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST2: {
					IPRINTF("RST2 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST3: {
					IPRINTF("RST3 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST4: {
					IPRINTF("RST4 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST5: {
					IPRINTF("RST5 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST6: {
					IPRINTF("RST6 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_RST7: {
					IPRINTF("RST7 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SOI: {
					IPRINTF("SOI marker found\r\n");
					break;
				}

				case M_EOI: {
					IPRINTF("EOI marker found\r\n");
					break;
				}

				case M_SOS: {
					IPRINTF("SOS marker found\r\n");

					kpn_channel_read (c[0], & SOS_section, sizeof (SOS_section));
					cpu_data_is_big_endian(16, SOS_section . length);

					kpn_channel_read (c[0], & SOS_component,
              sizeof(SOS_component_t)  * SOS_section . n);
					VPRINTF("Scan with %d components\r\n", SOS_section . n);

					kpn_channel_skip (c[0], SOS_section . n);

					scan_desc . bit_count = 0;
					for (index = 0; index < SOS_section . n; index++)
          {
						scan_desc . pred[index] = 0;
						scan_desc . table[HUFF_AC][index] =
              & tables[HUFF_AC][(SOS_component[index] . acdc >> 4) & 0x0f];
						scan_desc . table[HUFF_DC][index] =
              & tables[HUFF_DC][SOS_component[index] . acdc & 0x0f];
					}

					nb_MCU = intceil (SOF_section . height, MCU_sx) *
            intceil (SOF_section . width, MCU_sy);
					IPRINTF("%d MCU to unpack\r\n", nb_MCU);

					while (nb_MCU)
          {
						for (uint32_t step = 0; step < flit_size; step += mcu_size)
            {
							for (index = 0; index < YV * YH; index++)
              {
								unpack_block (c[0], & scan_desc, 0, MCU);
								iqzz_block (MCU, & FLIT[(step + index) * 64],
                    DQT_table[SOF_component[0] . q_table]);
							}

							for (index =  1; index < SOF_section . n; index++)
              {
								unpack_block (c[0], & scan_desc, index, MCU);
								iqzz_block (MCU, & FLIT[(step + (YV * YH + index - 1)) * 64],
                    DQT_table[SOF_component[index] . q_table]);
							}
						}

            computer (flit_size, FLIT, DECODED_FLIT);
            builder (SOF_section, YV, YH, flit_size, DECODED_FLIT,
                picture, & LB_X, & LB_Y);

						nb_MCU -= YV * nb_MCU_sx;
					}

          kpn_channel_write (c[1], picture, SOF_section . width
              * SOF_section . height * 2);

          kpn_channel_purge (c[0], false);
          break;
				}

				case M_DQT: {
					IPRINTF("DQT marker found\r\n");

					kpn_channel_read (c[0], & DQT_section, sizeof (DQT_section));
					cpu_data_is_big_endian(16, DQT_section . length);

					VPRINTF("Quantization precision is %s\r\n",
              ((DQT_section . pre_quant >> 4) & 0x0f) ? "16 bits" :	"8 bits");

					QT_index = DQT_section . pre_quant & 0x0f;
					VPRINTF("Quantization table index is %d\r\n", QT_index);
				
					VPRINTF("Reading quantization table\r\n");
					kpn_channel_read (c[0], DQT_table[QT_index], 64);

					break;
				}

				case M_DNL: {
					IPRINTF("DNL marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_DRI: {
					IPRINTF("DRI marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_DHP: {
					IPRINTF("DHP marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_EXP: {
					IPRINTF("EXP marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP0: {
					IPRINTF("APP0 marker found\r\n");

					kpn_channel_read (c[0], & jfif_header, sizeof (jfif_header));
					cpu_data_is_big_endian(16, jfif_header.length);
					cpu_data_is_big_endian(16, jfif_header.xdensity);
					cpu_data_is_big_endian(16, jfif_header.ydensity);

					if (jfif_header.identifier[0] != 'J' ||
              jfif_header.identifier[1] != 'F' ||
							jfif_header.identifier[2] != 'I' ||
              jfif_header.identifier[3] != 'F')
          {
						VPRINTF("Not a JFIF file\r\n");
					}

					break;
				}

				case M_APP1: {
					IPRINTF("APP1 marker found\r\n");
					skip_segment (c[0]);
					break;
				}
										 
				case M_APP2: {
					IPRINTF("APP2 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP3: {
					IPRINTF("APP3 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP4: {
					IPRINTF("APP4 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP5: {
					IPRINTF("APP5 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP6: {
					IPRINTF("APP6 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP7: {
					IPRINTF("APP7 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP8: {
					IPRINTF("APP8 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP9: {
					IPRINTF("APP9 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP10: {
					IPRINTF("APP10 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP11: {
					IPRINTF("APP11 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP12: {
					IPRINTF("APP12 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP13: {
					IPRINTF("APP13 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP14: {
					IPRINTF("APP14 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_APP15: {
					IPRINTF("APP14 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_JPG0: {
					IPRINTF("JPG0 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_JPG13: {
					IPRINTF("JPG13 marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_COM: {
					IPRINTF("COM marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_TEM: {
					IPRINTF("TEM marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				case M_SMS: {
					IPRINTF("SMS marker found\r\n");
					skip_segment (c[0]);
					break;
				}

				default: {
					IPRINTF("Error or unknown token: 0x%x\r\n", marker[1]);
					skip_segment (c[0]);
					break;
				}
			}
		}
	}

  return 0;
}

