/*
 * Copyright (C) 2007 TIMA Laboratory                                    
 *                                                                       
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, either version 3 of the License, or     
 * (at your option) any later version.                                   
 *                                                                       
 * This program is distributed in the hope that it will be useful,       
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
 * GNU General Public License for more details.                          
 *                                                                       
 * You should have received a copy of the GNU General Public License     
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef SOCLIB_FDACCESS_PRIVATE_H
#define SOCLIB_FDACCESS_PRIVATE_H

#include <SoclibTgDriver/Driver.h>
#include <Core/Core.h>
#include <DnaTools/DnaTools.h>

extern device_cmd_t tg_commands;
char ** tg_devices;

/*
 * Methods
 */

extern status_t tg_init_hardware (void);
extern status_t tg_init_driver (void);
extern void tg_uninit_driver (void);
extern const char ** tg_publish_devices (void);
extern device_cmd_t * tg_find_device (const char * name);

extern status_t tg_open (char * name, int32_t mode, void ** data);
extern status_t tg_close (void * data);
extern status_t tg_free (void * data);

extern status_t tg_read (void * handler, void * destination,
    int64_t offset, int32_t * p_count);
extern status_t tg_write (void * handler, void * source,
    int64_t offset, int32_t * p_count);
extern status_t tg_control (void * handler, int32_t operation,
    va_list arguments, int32_t * p_res);

#endif

