/************************************************************************
 * Copyright (C) 2008 TIMA Laboratory                                    *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *************************************************************************/

#ifndef EXT_FIFO_PRIVATE_H
#define EXT_FIFO_PRIVATE_H

#include <ExtFifoDriver/Driver.h>
#include <Core/Core.h>
#include <DnaTools/DnaTools.h>

/******************
 * Module defs
 ******************/
typedef enum extfifo_datatype {
	EXTFIFO_INT = 0,
	EXTFIFO_FLOAT = 1,  
} extfifo_datatype_t;

typedef struct extfifo_header_t {
	uint32_t initialized;  //boolean to indicate that the header is already initialized 
	uint32_t head;
	uint32_t tail;
	uint32_t status;
	uint32_t depth;
	uint32_t cell_size;
	extfifo_datatype_t datatype;          
	uint32_t buffer[EXTFIFO_BUFFER_MAX_SIZE];
} extfifo_header_t;

typedef struct _extfifo_config {
	uint32_t depth;
	uint32_t cell_size;

	extfifo_header_t * header_address;
	
	extfifo_datatype_t dtype;
	spinlock_t * lock;
} extfifo_config_t;

/*
 * Definitions
 */

extern uint32_t EXTFIFO_CHANNEL_NDEV;
extern uint32_t EXTFIFO_CHANNELS_PTR;
#define EXTFIFO_CHANNELS ((extfifo_config_t *)EXTFIFO_CHANNELS_PTR)

extern char ** extfifo_devices;

extern device_cmd_t extfifo_commands;
/*
 * Methods
 */

extern status_t extfifo_init_hardware (void);
extern status_t extfifo_init_driver (void);
extern void extfifo_uninit_driver (void);
extern const char ** extfifo_publish_devices (void);
extern device_cmd_t * extfifo_find_device (const char * name);

extern status_t extfifo_open (char * name, int32_t mode, void ** data);
extern status_t extfifo_close (void * data);
extern status_t extfifo_free (void * data);

extern status_t extfifo_read (void * handler, void * destination,
    int64_t offset, int32_t * p_count);
extern status_t extfifo_write (void * handler, void * source,
    int64_t offset, int32_t * p_count);
extern status_t extfifo_control (void * handler, int32_t operation,
    va_list arguments, int32_t * p_res);

#endif


