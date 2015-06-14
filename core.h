#ifndef _CORE_H_
#define _CORE_H_

#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL_net.h>

#define SERVER_PORT 8080
#define JOBS_MAX 32

typedef enum {
	JS_DOWNLOADING,
	JS_DOWNLOADING_PAUSED,
	JS_DOWNLOADING_WAITING,
	JS_DOWNLOADING_COMPLETED,
	
	JS_UPLOADING,
	JS_UPLOADING_PAUSED,
	JS_UPLOADING_WAITING,
	JS_UPLOADING_COMPLETED,
} JobState;


typedef struct {
	char *file_name;
	int file_size;
	int piece_size;
	int encryption;
	char *author_name;
	char *author_id;
} PackageInfo;


typedef struct {
	int total;
	int marked;
	char map[];
} ProgressMap;


typedef struct {
	PackageInfo *package;
	JobState state;
	char *file_path;
	FILE *file;
	ProgressMap *progress;
	TCPsocket socket;
} Job;


//int filePut(PackageInfo *);
int filePut(Job *);
//int fileGet();
int fileGet(Job *);

PackageInfo *packageCreate(char *file_name, int piece_size, char *author_name);

bool progressMapGet( ProgressMap *progress, int piece_num );
void progressMapSet( ProgressMap *progress, int piece_num, bool value );

Job *jobCreate( PackageInfo *package );

int handleIncomings( void *ushort_port );
int handleConnection( void *socket );


#endif
