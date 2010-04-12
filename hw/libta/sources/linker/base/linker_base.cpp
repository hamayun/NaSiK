/*************************************************************************************
 * File   : linker_base.cpp,     
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

#define DEBUG_OPTION "linker"

#include "base/linker_base.h"
#include "utils.h"
#include "assertion.h"
#include "debug.h"
#include "errno.h"
#include "dlfcn.h"
#include <err.h>
#include <fcntl.h>
#include <libelf.h>
#include <sysexits.h>

using namespace libta;

linker_base::linker_base(sc_module_name name) 
	: sc_module(name),
	exp_linker_loader("exp_linker_loader"),
	p_linker("p_linker"),
	_application(NULL),
	_end_of_elaboration_flag(false),
	_sw_image(NULL)
{
	DOUT_CTOR << this->name() << std::endl;
	exp_linker_loader(*this);
}

linker_base::~linker_base()
{
	DOUT_DTOR << this->name() << std::endl;
}

void * linker_base::load(const char * filename)
{
  ASSERT_MSG(_end_of_elaboration_flag != false, "should not be called before end of elaboration!!!");

  if(_sw_image == NULL)
  {
      dlerror();
      _sw_image = dlopen(filename, (RTLD_DEEPBIND | RTLD_NOW));
      ASSERT_MSG(_sw_image != NULL, dlerror());
      DOUT_NAME << " application loaded at " << std::hex << _sw_image << std::endl;
      load_elf_sections(filename);
  }
  return(_sw_image);
}

link_map* linker_base::get_link_map()
{
  link_map    *linkmap;

  ASSERT(_sw_image != NULL);
  dlinfo(_sw_image, RTLD_DI_LINKMAP, &linkmap);
  return(linkmap);
}

void linker_base::end_of_elaboration()
{
  _end_of_elaboration_flag = true;

  // Load/find the software image.
  GET_ATTRIBUTE("APPLICATION", _application, char*, false);
  DOUT << name() << ": APPLICATION = " << _application->value << std::endl;

  load((const char*)_application->value);

}

void linker_base::start_of_simulation()
{
  int port_index;
  unsigned int symbol_index;
  std::vector< symbol_base* > *	symbols_vector_ptr;

  DOUT << name() << " start dynamic link" << std::endl;

  for( port_index = 0 ; port_index < p_linker.size(); port_index++)
  {
    symbols_vector_ptr = p_linker[port_index]->get_symbols();
    for( symbol_index = 0 ; symbol_index < symbols_vector_ptr->size(); symbol_index++)
    {
      (*symbols_vector_ptr)[symbol_index]->link(_sw_image);
    }
  }
}

mapping::segment_t * linker_base::get_section(const char * section_name)
{
  unsigned int index;

  ASSERT_MSG(_end_of_elaboration_flag != false, "should not be called before end of elaboration!!!");

  for(index = 0 ; index < _sections_vector.size(); index++)
  {
    if(strcmp(_sections_vector[index]->name,section_name) == 0)
    {
      DOUT << name() << " section " << section_name << " found @ 0x" << std::hex << _sections_vector[index]->base_addr << std::endl;
      return(_sections_vector[index]);
    }
  }

  ASSERT_MSG( NULL, "section not found" );
  return((mapping::segment_t*)NULL);
}

void linker_base::load_elf_sections(const char * filename)
{
  link_map *linkmap_ptr;
  int fd;
  Elf *elf;
  Elf_Scn *scn;
  Elf32_Ehdr * ehdr32;
  Elf32_Shdr *shdr;
  const char* section_name;
  mapping::segment_t* segment_ptr;

  ASSERT_MSG(_end_of_elaboration_flag != false, "should not be called before end of elaboration!!!");

  dlinfo(_sw_image, RTLD_DI_LINKMAP, &linkmap_ptr);
  DOUT_NAME << " Application base address is 0x" << std::hex << linkmap_ptr->l_addr << std::endl;

  if (elf_version(EV_CURRENT) == EV_NONE)
    errx(EX_SOFTWARE, "ELF library initialization failed: %s",elf_errmsg(-1));

  if ((fd = open(filename, O_RDONLY, 0)) < 0)
    err(EX_NOINPUT, "open \%s\" failed", filename);

  if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
    errx(EX_SOFTWARE, "elf_begin() failed: %s.",elf_errmsg(-1));

  if (elf_kind(elf) != ELF_K_ELF)
    errx(EX_DATAERR, "%s is not an ELF object.", filename);

  ehdr32 = elf32_getehdr(elf);

  scn = NULL;
  while( (scn = elf_nextscn(elf,scn)) != NULL)
  {
    shdr = elf32_getshdr(scn);
    if( (shdr->sh_flags & SHF_ALLOC))
    {
      section_name = elf_strptr(elf, ehdr32->e_shstrndx, (size_t)shdr->sh_name);
      segment_ptr = mapping::init(section_name,(uintptr_t)(linkmap_ptr->l_addr + shdr->sh_addr),shdr->sh_size);
      segment_ptr->flags = shdr->sh_flags;
      _sections_vector.push_back(segment_ptr);
    }
  }

  (void) elf_end(elf);
  (void) close(fd);

}


