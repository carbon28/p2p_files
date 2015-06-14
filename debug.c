#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <SDL2/SDL_net.h>

#include "debug.h"

int opt_verbose = 0;

void error()
{
	fprintf(stderr, "[error] %s\n", SDLNet_GetError());
	exit(-1);
}

void verbose1(const char *fmt, ...)
{
	if (opt_verbose >= 1) {
		va_list args;
		va_start(args, fmt);
		fprintf(VERBOSE_FILE, "[verbose] ");
		vfprintf(VERBOSE_FILE, fmt, args);
		va_end(args);
	}
}
