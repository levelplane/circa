
/* Simple program:  Create a blank window, wait for keypress, quit.

   Please see the SDL documentation for details on using the SDL API:
   /Developer/Documentation/SDL/docs.html
*/
   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "SDL.h"
#include "SDL_gfxPrimitives.h"

#include "circa.h"

SDL_Surface *gScreen;

void draw_line(circa::Term* term)
{
    float x1 = circa::as_float(term->inputs[0]);
    float y1 = circa::as_float(term->inputs[1]);
    float x2 = circa::as_float(term->inputs[2]);
    float y2 = circa::as_float(term->inputs[3]);
    int color = circa::as_int(term->inputs[4]);

    lineColor(gScreen, x1, y1, x2, y2, color);
}

void initialize_circa()
{
    // Load a bunch of functions
    circa::import_c_function(draw_line, "draw-line(float,float,float,float,int)");
}

void initialize_sdl()
{
	Uint32 initflags = SDL_INIT_VIDEO;  /* See documentation for details */
	Uint8  video_bpp = 0;
	Uint32 videoflags = SDL_SWSURFACE;
    int WIDTH = 640;
    int HEIGHT = 480;

	/* Initialize the SDL library */
	if ( SDL_Init(initflags) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
			SDL_GetError());
		exit(1);
	}

	/* Set 640x480 video mode */
	gScreen=SDL_SetVideoMode(WIDTH,HEIGHT, video_bpp, videoflags);
        if (gScreen == NULL) {
		fprintf(stderr, "Couldn't set 640x480x%d video mode: %s\n",
                        video_bpp, SDL_GetError());
		SDL_Quit();
		exit(2);
	}
}

void main_loop()
{
	bool done;
    SDL_Event event;

    SDL_LockSurface(gScreen);
    lineColor(gScreen, 20, 20, 50, 50, 0xff0000ff);
    SDL_UnlockSurface(gScreen);
    SDL_Flip(gScreen);

	done = false;
	while ( !done ) {

		/* Check for events */
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {

				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_KEYDOWN:
					/* Any keypress quits the app... */
				case SDL_QUIT:
					done = 1;
					break;
				default:
					break;
			}
		}
	}
}

void uninitialize_sdl()
{
	/* Clean up the SDL library */
	SDL_Quit();
}

int main(int argc, char *argv[])
{
    initialize_sdl();
    main_loop();
    uninitialize_sdl();
	return(0);
}
