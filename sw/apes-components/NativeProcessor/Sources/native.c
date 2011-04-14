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

#include <Processor/Processor.h>
#include <Platform/Platform.h>

#include "stdint.h"

uint32_t OS_N_DRIVERS __attribute__  ((section (".data")));
uint32_t OS_DRIVERS_LIST[32] __attribute__  ((section (".data")));

uint32_t OS_N_FILESYSTEMS __attribute__  ((section (".data")));
uint32_t OS_FILESYSTEMS_LIST[32] __attribute__  ((section (".data")));

uint32_t OS_THREAD_STACK_SIZE __attribute__  ((section (".data")));

uint32_t OS_KERNEL_HEAP_ADDRESS __attribute__  ((section (".data")));
uint32_t OS_KERNEL_HEAP_SIZE __attribute__  ((section (".data")));

uint32_t OS_USER_HEAP_ADDRESS __attribute__  ((section (".data")));

uint32_t OS_GLOBAL_MEM_ADDRESS __attribute__  ((section (".data")));

uint32_t PLATFORM_N_NATIVE __attribute__  ((section (".data")));
uint32_t APP_ENTRY_POINT __attribute__  ((section (".data")));

uint32_t CPU_OS_ENTRY_POINT __attribute__  ((section (".data")));
uint32_t CPU_SVC_STACK_ADDR __attribute__  ((section (".data")));

uint32_t CPU_BSS_START __attribute__  ((section (".data")));
uint32_t CPU_BSS_END __attribute__  ((section (".data")));

uint32_t SOCLIB_TIMER_NDEV __attribute__  ((section (".data")));
uint32_t SOCLIB_TIMER_DEVICES[32] __attribute__  ((section (".data")));

uint32_t SOCLIB_TTY_NDEV __attribute__  ((section (".data")));
uint32_t SOCLIB_TTY_DEVICES[32] __attribute__  ((section (".data")));

uint32_t SOCLIB_FDACCESS_NDEV __attribute__  ((section (".data")));
uint32_t SOCLIB_FDACCESS_DEVICES[32] __attribute__  ((section (".data")));

uint32_t CHANNEL_RDV_NDEV __attribute__  ((section (".data")));

uint32_t SOCLIB_FB_BASE __attribute__ ((section(".data"))); 

uint32_t IT_GEN_DEVICE __attribute__ ((section(".data"))); 

soclib_timer_port_t PLATFORM_TIMER_BASE __attribute__ ((section(".data")));
int32_t PLATFORM_TIMER_INTN __attribute__ ((section(".data")));

soclib_ipi_port_t PLATFORM_IPI_BASE __attribute__ ((section(".data")));
int32_t PLATFORM_IPI_INTN __attribute__ ((section(".data")));

volatile unsigned long int cpu_mp_synchro __attribute__ ((section(".data")));

char * PLATFORM_DEBUG_CHARPORT __attribute__ ((section(".data")));

int32_t OS_MODULES_LIST[10] __attribute__ ((section(".data")));
int32_t OS_N_MODULES __attribute__ ((section(".data")));

uint32_t SOCLIB_FB_DEVICES[3*10] __attribute__ ((section(".data")));
uint32_t SOCLIB_FB_NDEV __attribute__ ((section(".data")));

uint32_t EXTFIFO_CHANNEL_NDEV __attribute__ ((section(".data")));
uint32_t EXTFIFO_CHANNELS_PTR __attribute__ ((section(".data")));

uint32_t SOCLIB_BLOCK_DEVICES_NDEV __attribute__ ((section(".data")));
uint32_t SOCLIB_BLOCK_DEVICES[3*3] __attribute__ ((section(".data")));

uint32_t PLATFORM_HOSTTIME_BASE		__attribute__ ((section(".data"))); 
