#include <stdio.h>
#include <assert.h>

#include "core.h"
#include "debug.h"

/**
	Reads requested piece from file.

	@param job Reads file_size, piece_size, file
	@param piece_index Index of piece of file to read
	@param buf Array to store loaded data. Should be at least the same
		size as pkg->piece_size.

	@return Number of bytes read
*/
int fileGetPiece(Job *job, int piece_index, char *buf)
{
	assert(job);
	assert(piece_index >= 0);
	assert(buf);

	PackageInfo *pkg = job->package;

	int pos = pkg->piece_size * piece_index;
	int count = (piece_index == pkg->file_size / pkg->piece_size)
		? pkg->file_size % pkg->piece_size
		: pkg->piece_size;

	verbose1("filePutPiece::pos, count = %d, %d\n", pos, count);

	int read;
	if (fseek(job->file, pos, SEEK_SET) != 0) {
		perror("");
		read = 0;
	}
	else
		read = fread(buf, 1, count, job->file);

	return read;
}



/**
	Writes requested piece to file.

	@param file Reads file_size, piece_size, file
	@param piece_index Index of piece of file to write
	@param buf Buffer with data to store. Should be at least the same
		size as job->package->piece_size.

	@return Number of bytes written
*/
int filePutPiece(Job *job, int piece_index, char *buf)
{
	assert(job);
	assert(piece_index >= 0);
	assert(buf);

	verbose1("filePutPiece::piece_index = %d\n", piece_index);

	PackageInfo *pkg = job->package;

	int pos = pkg->piece_size * piece_index;
	int count = (piece_index == pkg->file_size / pkg->piece_size)
		? pkg->file_size % pkg->piece_size
		: pkg->piece_size;

	verbose1("filePutPiece::pos, count = %d, %d\n", pos, count);

	int written;
	if (fseek(job->file, pos, SEEK_SET) != 0) {
		perror("");
		written = 0;
	}
	else
		written = fwrite(buf, 1, count, job->file);

	return written;
}



long fileGetSize(char *file_name)
{
	FILE *fd = fopen(file_name, "rb");
	if (fd == NULL)
		return -1;

	fseek(fd, 0, SEEK_END);
	long size = ftell(fd);
	fclose(fd);

	return size;
}


int fileGetPieceCount(Job *job)
{
	assert(job);

	int file_size = job->package->file_size;
	int piece_size = job->package->piece_size;

	int count;
	if (file_size % piece_size == 0)
		count = file_size / piece_size;
	else
		count = file_size / piece_size + 1;

	return count;
}
