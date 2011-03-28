/*************************************************************************************
 * File   : dna_linker.cpp,     
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Patrice, GERIN patrice.gerin@imag.fr
 * Bug Fixer(s) :   
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************************/

#define DEBUG_OPTION "dna_linker"

#include "dna/dna_linker.h"
#include "utils.h"
#include "assertion.h"
#include "debug.h"
#include "dlfcn.h"

using namespace native;

dna_linker::dna_linker(sc_module_name name) 
  : linker_base(name) 
{}
dna_linker::~dna_linker() 
{}

void dna_linker::end_of_elaboration()
{
	char * token;
	std::vector< char * > sections_name;
	unsigned int index;
	int base;
	uintptr_t pointer;
	symbol< uintptr_t > * symbol_ptr;
	symbol< uint32_t > * symbol_value;
	mapping::segment_t * segment_ptr;

	linker_base::end_of_elaboration();
	
	DOUT << "---------------------------------------------------------" << std::endl;
	DOUT << name() << "       DNA OS configuration" << std::endl;

	GET_ATTRIBUTE("CPU_OS_ENTRY_POINT",_cpu_os_entry_point,char*,false);
	DOUT << name() << ": CPU_OS_ENTRY_POINT = " << _cpu_os_entry_point << std::endl;

	GET_ATTRIBUTE("SECTIONS_DECL",_sections_decl,char*,false);
	GET_ATTRIBUTE("SECTIONS_SIZE",_sections_size,char*,false);
	DOUT << name() << ": SECTIONS_DECL = " << _sections_decl->value << std::endl;
	DOUT << name() << ": SECTIONS_SIZE = " << _sections_size->value << std::endl;

	token = strtok(_sections_decl->value, " ");
	while(token != NULL)
	{
		sections_name.push_back(token);
		token = strtok(NULL, " ");
	}

	token = strtok(_sections_size->value, " ");
	for(index = 0; index < sections_name.size();index++)
	{
		ASSERT_MSG(token != NULL, "SECTIONS_DECL and SECTIONS_SIZE doesn't match");

		base = 10;
		if( token[0] == '0' && ( token[1] == 'x' || token[1] == 'X'))
		{
			base = 16;
			token = &(token[2]);
		}
		_sections_vector.push_back(mapping::init(sections_name[index], strtol(token,NULL,base)));
		token = strtok(NULL, " ");
	}

	GET_ATTRIBUTE("OS_DRIVERS_LIST",_os_drivers_list,char*,false);
	DOUT << name() << ": OS_DRIVERS_LIST = " << _os_drivers_list->value << std::endl;

	// Compute devices list
	symbol_ptr = new symbol<uintptr_t>("OS_DRIVERS_LIST");
	token = strtok(_os_drivers_list->value, " ");
	while(token != NULL)
	{
		pointer = (uintptr_t)dlsym(_sw_image,token);
		ASSERT_MSG(pointer != (uintptr_t)NULL, token );
		symbol_ptr->push_back(pointer);
		token = strtok(NULL, " ");
	}
	_symbols_vector.push_back(symbol_ptr);

	symbol_value = new symbol< uint32_t >("OS_N_DRIVERS");
	symbol_value->push_back(symbol_ptr->size());
	_symbols_vector.push_back(symbol_value);

	GET_ATTRIBUTE("OS_FILESYSTEMS_LIST",_os_filesystems_list,char*,false);
	DOUT << name() << ": OS_FILESYSTEMS_LIST = " << _os_filesystems_list->value << std::endl;

	// Compute filesystems list
	symbol_ptr = new symbol<uintptr_t>("OS_FILESYSTEMS_LIST");
	token = strtok(_os_filesystems_list->value, " ");
	while(token != NULL)
	{
		pointer = (uintptr_t)dlsym(_sw_image,token);
		ASSERT_MSG(pointer != (uintptr_t)NULL, token );
		symbol_ptr->push_back(pointer);
		token = strtok(NULL, " ");
	}
	_symbols_vector.push_back(symbol_ptr);

	symbol_value = new symbol< uint32_t >("OS_N_FILESYSTEMS");
	symbol_value->push_back(symbol_ptr->size());
	_symbols_vector.push_back(symbol_value);

	GET_ATTRIBUTE("OS_THREAD_STACK_SIZE",_os_thread_stack_size,uint32_t,false);
	DOUT << name() << ": OS_THREAD_STACK_SIZE = " << _os_thread_stack_size->value << std::endl;
	symbol_value = new symbol< uint32_t >("OS_THREAD_STACK_SIZE");
	symbol_value->push_back(_os_thread_stack_size->value);
	_symbols_vector.push_back(symbol_value);

	GET_ATTRIBUTE("OS_KERNEL_HEAP_SECTION",_os_kernel_heap_section,char*,false);
	DOUT << name() << ": OS_KERNEL_HEAP_SECTION = " << _os_kernel_heap_section->value << std::endl;

	segment_ptr = get_section(_os_kernel_heap_section->value);
	symbol_ptr = new symbol< uintptr_t >("OS_KERNEL_HEAP_ADDRESS");
	symbol_ptr->push_back(segment_ptr->base_addr);
	_symbols_vector.push_back(symbol_ptr);

	symbol_value = new symbol< uint32_t >("OS_KERNEL_HEAP_SIZE");
	symbol_value->push_back(segment_ptr->size);
	_symbols_vector.push_back(symbol_value);

	GET_ATTRIBUTE("OS_USER_HEAP_SECTION",_os_user_heap_section,char*,false);
	DOUT << name() << ": OS_USER_HEAP_SECTION = " << _os_user_heap_section->value << std::endl;

	segment_ptr = get_section(_os_user_heap_section->value);
	symbol_ptr = new symbol< uintptr_t >("OS_USER_HEAP_ADDRESS");
	symbol_ptr->push_back(segment_ptr->base_addr);
	_symbols_vector.push_back(symbol_ptr);

	GET_ATTRIBUTE("OS_KERNEL_BSS_SECTION",_os_kernel_bss_section,char*,false);
	DOUT << name() << ": OS_KERNEL_BSS_SECTION = " << _os_kernel_bss_section->value << std::endl;
	segment_ptr = get_section(_os_kernel_bss_section->value);
	symbol_ptr = new symbol< uintptr_t >("CPU_BSS_START");
	symbol_ptr->push_back(segment_ptr->base_addr);
	_symbols_vector.push_back(symbol_ptr);

	symbol_ptr = new symbol< uintptr_t >("CPU_BSS_END");
	symbol_ptr->push_back(segment_ptr->end_addr);
	_symbols_vector.push_back(symbol_ptr);

	GET_ATTRIBUTE("PLATFORM_N_NATIVE",_platform_n_native,uint32_t,false);
	DOUT << name() << ": PLATFORM_N_NATIVE = " << _platform_n_native->value << std::endl;
	symbol_value = new symbol< uint32_t >("PLATFORM_N_NATIVE");
	symbol_value->push_back(_platform_n_native->value);
	_symbols_vector.push_back(symbol_value);

	GET_ATTRIBUTE("APP_ENTRY_POINT",_app_entry_point,char *,false);
	DOUT << name() << ": APP_ENTRY_POINT = " << _app_entry_point->value << std::endl;
	pointer = (uintptr_t)dlsym(_sw_image,_app_entry_point->value);
	symbol_ptr = new symbol< uintptr_t >("APP_ENTRY_POINT");
	symbol_ptr->push_back(pointer);
	_symbols_vector.push_back(symbol_ptr);

	GET_ATTRIBUTE("CHANNEL_RDV_NDEV",_channel_rdv_ndev,uint32_t,false);
	DOUT << name() << ": CHANNEL_RDV_NDEV = " << _channel_rdv_ndev->value << std::endl;
	symbol_value = new symbol< uint32_t >("CHANNEL_RDV_NDEV");
	symbol_value->push_back(_channel_rdv_ndev->value);
	_symbols_vector.push_back(symbol_value);

#if 0
	//GET_ATTRIBUTE("AICU_PLATFORM_BASE",_aicu_platform_base,char*,false);
        GET_ATTRIBUTE("EXTFIFO_MEM_SECTION",_extfifo_mem_section,char*,false);
	GET_ATTRIBUTE("EXTFIFO_CHANNEL_NDEV",_extfifo_channel_ndev,uint32_t,false);
	GET_ATTRIBUTE("EXTFIFO_CHANNELS_LIST",_extfifo_channels_list,char *,false);
	DOUT << name() << ": EXTFIFO_MEM_SECTION = " << _extfifo_mem_section->value << std::endl;
	DOUT << name() << ": EXTFIFO_CHANNEL_NDEV = " << _extfifo_channel_ndev->value << std::endl;
	DOUT << name() << ": EXTFIFO_CHANNELS = " << _extfifo_channels_list->value << std::endl;
	segment_ptr = get_section(_extfifo_mem_section->value);
	symbol_value = new symbol< uint32_t >("EXTFIFO_CHANNEL_NDEV");
	symbol_value->push_back(_extfifo_channel_ndev->value);
	_symbols_vector.push_back(symbol_value);

	extfifo_config_t * header = (extfifo_config_t *)malloc(sizeof(extfifo_config_t)*(_extfifo_channel_ndev->value));
	token = strtok(_extfifo_channels_list->value, " ");
	for(index = 0; index < _extfifo_channel_ndev->value; index++)
	{
		ASSERT_MSG(token != NULL, "EXTFIFO_CHANNEL_NDEV AND EXTFIFO_CHANNELS_LIST do not match");
		header[index].depth = atoi(token);

		token = strtok(NULL, " ");
		ASSERT_MSG(token != NULL, "EXTFIFO_CHANNEL_NDEV AND EXTFIFO_CHANNELS_LIST do not match");
		header[index].cell_size = atoi(token);
		header[index].header_address = (void *)(segment_ptr + index*0x10000);

		token = strtok(NULL, " ");
		ASSERT_MSG(token != NULL, "EXTFIFO_CHANNEL_NDEV AND EXTFIFO_CHANNELS_LIST do not match");
		header[index].dtype = atoi(token);
		header[index].lock = (void *)(segment_ptr + index*0x10000 - 0x10);

		token = strtok(NULL, " ");
	}
	symbol_value = new symbol<uint32_t>("EXTFIFO_CHANNELS_PTR");
	symbol_value->push_back((uint32_t)header);
	_symbols_vector.push_back(symbol_value);
#endif
	DOUT << "---------------------------------------------------------" << std::endl;
}

void dna_linker::start_of_simulation()
{
	unsigned int symbol_index;

	// Link platform dependent symbols
	linker_base::start_of_simulation();

	// Link DNA OS symbols
	DOUT << name() << ": DNA specific link" << std::endl;
	DOUT << name() << ":     " << _symbols_vector.size() << " symbols to link" << std::endl;
	for( symbol_index = 0 ; symbol_index < _symbols_vector.size(); symbol_index++)
	{
		_symbols_vector[symbol_index]->link(_sw_image);
	}
}

uintptr_t dna_linker::get_start_addr()
{
	uintptr_t start;

	ASSERT_MSG(_end_of_elaboration_flag != false, "should not be called before end of elaboration!!!");

	start = (uintptr_t)dlsym(_sw_image,_cpu_os_entry_point->value);
	ASSERT_MSG(start != (uintptr_t)NULL, "Symbol not found");

	return(start);

}

