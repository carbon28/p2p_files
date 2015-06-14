#ifndef DEBUG_H
#define DEBUG_H

void error();

#define VERBOSE_FILE stderr

// verbose level: (none)0,1,2,3(max)
extern int opt_verbose;

void verbose1(const char *fmt, ...);

#endif
