CC = gcc
SDL_LIB = -LC:/SDL2-2.0.3/i686-w64-mingw32/lib -lSDL2main -lSDL2 -lSDL2_net
SDL_INCLUDE = -IC:/SDL2-2.0.3/i686-w64-mingw32/include
CFLAGS = -Wall -c -std=c99 $(SDL_INCLUDE)
LDFLAGS = -lmingw32 -mwindows -mconsole $(SDL_LIB)
BIN = core.o fileget.o fileput.o debug.o network.o files.o

all: fileget fileput

# ----------
fileget: fileget.o core.o debug.o network.o files.o
	$(CC) $^ $(LDFLAGS) -o $@.exe

fileput: fileput.o core.o debug.o network.o files.o
	$(CC) $^ $(LDFLAGS) -o $@.exe

# ----------
core.o: core.c core.h
	$(CC) $< $(CFLAGS) -c

fileget.o: fileget.c core.h
	$(CC) $< $(CFLAGS) -c
	
fileput.o: fileput.c core.h
	$(CC) $< $(CFLAGS) -c

debug.o: debug.c debug.h
	$(CC) $< $(CFLAGS) -c

network.o: network.c network.h
	$(CC) $< $(CFLAGS) -c

files.o: files.c files.h
	$(CC) $< $(CFLAGS) -c
	
# ----------
clean:
	rm -f $(BIN)
	rm -f fileget.exe fileput.exe
