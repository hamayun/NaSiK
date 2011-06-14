#ifndef __FRAMEBUFFER_DEVICE_H__
#define __FRAMEBUFFER_DEVICE_H__

#include <slave_device.h>
#include "SDL/SDL.h"

class framebuffer_device : public slave_device
{
public:
	framebuffer_device(sc_module_name name);
	virtual ~framebuffer_device();
	void end_of_elaboration();

public:
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr);
     */
    virtual void rcv_rqst (unsigned int ofs, unsigned char be,
                           unsigned char *data, bool bWrite);

	// IO interfaces methods
    virtual void write (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr);
    virtual void read  (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr);

private:
    void screen_init(uint32_t width, uint32_t height);
    void screen_update();

		typedef enum {
			ON = 0,
			OFF = 1,
		} OUTPUT_MODE;

private:

    uint32_t	_width;
    uint32_t	_height;

    OUTPUT_MODE _output_mode;

    SDL_Surface   *_screen;
    SDL_Overlay   *_image;
    SDL_Rect      _rect;
    uint32_t      _y_size;
    uint32_t      _c_size;
    uint8_t       *_Y_pixels;
    uint8_t       *_Cr_pixels;
    uint8_t       *_Cb_pixels;

	uint8_t		  *_framebuffer;
  };

#endif				// __FRAMEBUFFER_DEVICE_H__
