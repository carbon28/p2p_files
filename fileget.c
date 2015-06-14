#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include "core.h"
#include "debug.h"


int main(int argc, char *argv[])
{
	// init
	opt_verbose = 1;

	if(SDL_Init(0) == -1)
		error();

	if(SDLNet_Init() != 0)
		error();
	
	
	// get file
	Job job;

	fileGet(&job);
	
	verbose1("main::exit");

	// clean up
	SDLNet_Quit();
	SDL_Quit();
	
	return 0;
}
