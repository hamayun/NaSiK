/*************************************************************************************
 * File   : framebuffer.cpp,     
 *
 * Copyright (C) 2008 TIMA Laboratory
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

#define DEBUG_OPTION "framebuffer"

#include "framebuffer.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "errno.h"

namespace libta
{
    framebuffer::framebuffer(sc_module_name name)
      : device_slave(name),
      _width(NULL),
      _height(NULL)
    {
      _generic_name = "Framebuffer";
      DOUT_CTOR << this->name() << std::endl;
    }

    framebuffer::~framebuffer()
    {
      DOUT_DTOR << this->name() << std::endl;
      /* Shutdown all subsystems */
      SDL_Quit();
    }

    void framebuffer::end_of_elaboration()
    {

      device_slave::end_of_elaboration();

      GET_ATTRIBUTE("WIDTH",_width,uint32_t,false);
      GET_ATTRIBUTE("HEIGHT",_height,uint32_t,false);
      DOUT_NAME << "WIDTH = " << _width->value << std::endl;
      DOUT_NAME << "HEIGHT = " << _height->value << std::endl;

      init();

      _segments.push_back(mapping::init(name(),_registers.ptr, _width->value*_height->value*3));

      symbol<uintptr_t> * symbol_value;
      symbol_value = new symbol<uintptr_t>("SOCLIB_FB_BASE");
      symbol_value->push_back(_registers.ptr);
      _symbols.push_back(symbol_value);

    }

    void framebuffer::slv_write (uint32_t  *addr, uint32_t  data)
    {
      *addr = data;
    }

    void framebuffer::slv_write (uint16_t  *addr, uint16_t  data)
    {
      *addr = data;
    }

    void framebuffer::slv_write (uint8_t  *addr, uint8_t  data)
    {
      *addr = data;
    }

    void framebuffer::slv_read (uint32_t  *addr, uint32_t  *data)
    {
      *data = *addr;
    }

    void framebuffer::init()
    {

      /* Initialize defaults, Video and Audio */
      if((SDL_Init(SDL_INIT_VIDEO)==-1)) { 
        printf("Could not initialize SDL: %s.\n", SDL_GetError());
        exit(-1);
      }

      /* Initialize the SDL library */
      if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        fprintf(stderr,
            "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
      }

      /* Clean up on exit */
      atexit(SDL_Quit);

      _screen = SDL_SetVideoMode(_width->value, _height->value, 24, SDL_SWSURFACE);
      if ( _screen == NULL ) {
        fprintf(stderr, "Couldn't set %dx%dx24 video mode: %s\n", _width->value,_height->value,
            SDL_GetError());
        exit(1);
      }

      _registers.reg8 = (uint8_t *) malloc(_width->value * _height->value * 3);
      pthread_create(&_thread,NULL, framebuffer::screen_thread, (void*)this);

    }

    void * framebuffer::screen_thread(void *arg)
    {
      framebuffer *_fb;
      _fb = (framebuffer*)arg;
      _fb->_screen_thread();
      return(NULL);
    }

    void * framebuffer::_screen_thread()
    {
      uint8_t   *ptr_dst;
      uint8_t   *ptr_src;
      uint32_t  i_dst;
      uint32_t  i_src;

      while(1)
      {
        usleep(500000);
        SDL_LockSurface(_screen);
        for(i_src = 0, i_dst = (_height->value-1); i_src < _height->value; i_src++, i_dst--)
        {
          ptr_src = (uint8_t*)((uint32_t)i_src * _width->value * 3 + _registers.reg8);
          ptr_dst = (uint8_t*)((uint32_t)i_dst * _width->value * 3 + (uint32_t)_screen->pixels);
          memcpy(ptr_dst,ptr_src,_width->value * 3);
        }
        SDL_UnlockSurface(_screen);
        SDL_UpdateRect(_screen,0,0,0,0);
      }
      return(NULL);
    }

}

