#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include "core.h"
#include "files.h"
#include "debug.h"


int main(int argc, char *argv[])
{
	// init
	opt_verbose = 0;

	if(SDL_Init(0) == -1)
		error();
	
	if(SDLNet_Init() != 0)
		error();

	// init package
	PackageInfo pkg;
	pkg.file_name = "sample.jpg";
	pkg.author_name = "lukas";
	pkg.piece_size = 32*1024;
	pkg.file_size = fileGetSize( "for_sending/sample.jpg" );
	verbose1("main::pkg.file_size = %d\n", pkg.file_size);
	pkg.encryption = 0;  // none

	// init job
	Job job;
	job.package = &pkg;
	job.state = JS_UPLOADING_WAITING;
	job.file_path = "for_sending/sample.jpg";

	job.file = fopen(job.file_path, "rb+");
	if (job.file == NULL) {
		perror("fopen()");
		exit(1);
	}
	job.progress = NULL;  // unused yet

	///filePut(&pkg);
	filePut(&job);
	
	verbose1("main::exit");

	// clean up
	SDLNet_Quit();
	SDL_Quit();
	
	return 0;
}
