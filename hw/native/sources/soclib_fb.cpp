/*************************************************************************************
 * File   : soclib_fb.cpp,     
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

#define DEBUG_OPTION "soclib_fb"

#include "soclib_fb.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include "errno.h"

using namespace native;
using namespace mapping;

soclib_fb::soclib_fb(sc_module_name name)
  : device_slave(name),
		_width(NULL),
		_height(NULL),
		_screen(NULL),
		_image(NULL),
    _Y_pixels(NULL),
    _Cr_pixels(NULL),
    _Cb_pixels(NULL)
{
  _generic_name = "Soclib FB";
	DOUT_CTOR << this->name() << std::endl;
}

soclib_fb::~soclib_fb()
{
	DOUT_DTOR << this->name() << std::endl;
  SDL_FreeSurface(_screen);
	SDL_Quit();
}

void soclib_fb::end_of_elaboration()
{

	device_slave::end_of_elaboration();

	GET_ATTRIBUTE("WIDTH",_width,uint32_t,false);
	GET_ATTRIBUTE("HEIGHT",_height,uint32_t,false);
	DOUT_NAME << "WIDTH = " << _width->value << std::endl;
	DOUT_NAME << "HEIGHT = " << _height->value << std::endl;

	GET_ATTRIBUTE("MODE",_mode,char*,false);
	DOUT << name() << ": MODE = " << _mode->value << std::endl;
  if(strcmp(_mode->value,"ON") == 0) _output_mode = ON;
  if(strcmp(_mode->value,"OFF") == 0) _output_mode = OFF;
  if(_output_mode == OFF)
    cerr << "\033[01;31m" << name() << ": No image will be displayed" << "\033[00m" << "\n";


	_segments.push_back(mapping::init(name(), _width->value*_height->value*2));
	_registers.ptr = _segments[0]->base_addr;

  if(_output_mode == ON)
	screen_init(_width->value,_height->value);

	symbol<uintptr_t> * symbol_value;
	//symbol_value = new symbol<uintptr_t>("SOCLIB_FB_BASE");
	symbol_value = new symbol<uintptr_t>("SOCLIB_FB_DEVICES");
	//symbol_value->push_back(_registers.ptr);
	symbol_value->push_back((uint32_t)(_width->value));
	symbol_value->push_back((uint32_t)(_height->value));
	symbol_value->push_back((uint32_t)(_registers.ptr));
	_symbols.push_back(symbol_value);

	symbol_value = new symbol<uintptr_t>("SOCLIB_FB_NDEV");
	symbol_value->push_back((uint32_t)1);
	_symbols.push_back(symbol_value);

}

void soclib_fb::slv_write (uint32_t  *addr, uint32_t  data)
{
	*addr = data;
}

void soclib_fb::slv_write (uint16_t  *addr, uint16_t  data)
{
	*addr = data;
}

void soclib_fb::slv_write (uint8_t  *addr, uint8_t  data)
{
	*addr = data;
  if(_output_mode == ON)
    screen_update();
}

void soclib_fb::slv_read (uint32_t  *addr, uint32_t  *data)
{
  if((_output_mode == ON) && (addr == (uint32_t *)_registers.ptr))
    screen_update();
  *data = *addr;
}

void soclib_fb::slv_read (uint16_t  *addr, uint16_t  *data)
{
  if((_output_mode == ON) && (addr == (uint16_t *)_registers.ptr))
    screen_update();
  *data = *addr;
}

void soclib_fb::slv_read (uint8_t  *addr, uint8_t  *data)
{
  if((_output_mode == ON) && (addr == (uint8_t *)_registers.ptr))
    screen_update();
  *data = *addr;
}

void soclib_fb::screen_init(uint32_t width, uint32_t height)
{

  /* Initialize defaults, Video and Audio */
  if((SDL_Init(SDL_INIT_VIDEO)==-1)) { 
    printf("Could not initialize SDL: %s.\n", SDL_GetError());
    exit(-1);
  }

  _screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
  if ( _screen == NULL ) {
    fprintf(stderr, "Couldn't set %dx%dx32 video mode: %s\n", width,height,SDL_GetError());
    exit(1);
  }

  _image = SDL_CreateYUVOverlay(width,height, SDL_IYUV_OVERLAY, _screen);
  if ( _screen == NULL ) {
    fprintf(stderr, "Couldn't create YUV overlay: %s\n", SDL_GetError());
    exit(1);
  }

  _rect.x = _rect.y = 0;
  _rect.w = width;
  _rect.h = height;
  _y_size = width * height;
  _c_size = _y_size >> 2;

  _Y_pixels = (uint8_t*)_registers.ptr;
  _Cb_pixels = _Y_pixels + _y_size;
  _Cr_pixels = _Cb_pixels + _c_size;

  memset(_image->pixels[0], 128, _y_size * 2);

  /* Clean up on exit */
  atexit(SDL_Quit);

}

void soclib_fb::screen_update()
{
  SDL_LockYUVOverlay(_image);
  memcpy(_image->pixels[0], _Y_pixels,  _y_size);
//  memcpy(_image->pixels[1], _Cb_pixels, _c_size);
//  memcpy(_image->pixels[2], _Cr_pixels, _c_size);
  SDL_UnlockYUVOverlay(_image);
  SDL_DisplayYUVOverlay(_image,&_rect);
}
