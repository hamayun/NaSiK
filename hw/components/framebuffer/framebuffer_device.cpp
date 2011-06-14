
#include "framebuffer_device.h"
//#include "assertion.h"
//#include "utils.h"
//#include "debug.h"
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include "errno.h"

framebuffer_device::framebuffer_device(sc_module_name name)
  : slave_device (name),
		_width(NULL),
		_height(NULL),
		_screen(NULL),
		_image(NULL),
    _Y_pixels(NULL),
    _Cr_pixels(NULL),
    _Cb_pixels(NULL)
{
}

framebuffer_device::~framebuffer_device()
{
	SDL_FreeSurface(_screen);
	SDL_Quit();
}

void framebuffer_device::end_of_elaboration()
{

	_width = 256;
	_height = 144;

  	_output_mode = ON;

	_framebuffer = new uint8_t[_width * _height * 2];

  if(_output_mode == ON)
	screen_init(_width,_height);

}

void framebuffer_device::write (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;
	int				offset_dd = 0;
	int				mod = 0;

#ifdef DEBUG
    printf ("%s::%s ofs=0x%X, be=0x%X, ", name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
#endif

	switch (be)
	{
		//byte access uint8_t
		case 0x01: offset_dd = 0; mod = 1; break;
        case 0x02: offset_dd = 1; mod = 1; break;
        case 0x04: offset_dd = 2; mod = 1; break;
        case 0x08: offset_dd = 3; mod = 1; break;
        case 0x10: offset_dd = 4; mod = 1; break;
        case 0x20: offset_dd = 5; mod = 1; break;
        case 0x40: offset_dd = 6; mod = 1; break;
        case 0x80: offset_dd = 7; mod = 1; break;
        //word access uint16_t
        case 0x03: offset_dd = 0; mod = 2; break;
        case 0x0C: offset_dd = 1; mod = 2; break;
        case 0x30: offset_dd = 2; mod = 2; break;
        case 0xC0: offset_dd = 3; mod = 2; break;
        //dword access uint32_t
        case 0x0F: offset_dd = 0; mod = 4; break;
        case 0xF0: offset_dd = 1; mod = 4; break;
        //qword access uint64_t
        case 0xFF: offset_dd = 0; mod = 8; break;
	}

	for( i = 0; i < mod; i++)
	{
		*(_framebuffer + ofs + offset_dd + i) = *(data + offset_dd + i);
		//*(_framebuffer + ofs + offset_dd + i) = *(data + i);
#ifdef DEBUG
		printf("data=0x%X", *(data + offset_dd + i));
#endif
	} 
#ifdef DEBUG
	printf("\n");
#endif

  	if(_output_mode == ON)
    	screen_update();
}

void framebuffer_device::read (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    printf ("Bad %s::%s ofs=0x%X, be=0x%X\n", name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
	exit(1);
}

void framebuffer_device::screen_init(uint32_t width, uint32_t height)
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

  _Y_pixels = _framebuffer;
  _Cb_pixels = _Y_pixels + _y_size;
  _Cr_pixels = _Cb_pixels + _c_size;

  memset(_image->pixels[0], 128, _y_size * 2);

  /* Clean up on exit */
  atexit(SDL_Quit);

}

void framebuffer_device::screen_update()
{
  SDL_LockYUVOverlay(_image);
  memcpy(_image->pixels[0], _Y_pixels,  _y_size);
//  memcpy(_image->pixels[1], _Cb_pixels, _c_size);
//  memcpy(_image->pixels[2], _Cr_pixels, _c_size);
  SDL_UnlockYUVOverlay(_image);
  SDL_DisplayYUVOverlay(_image,&_rect);
}

void framebuffer_device::rcv_rqst (unsigned int ofs, unsigned char be,
                              unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite){
        this->write(ofs, be, data, bErr);
    }else{
        this->read(ofs, be, data, bErr);
    }

    send_rsp(bErr);

    return;
}


