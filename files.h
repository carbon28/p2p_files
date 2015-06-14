#ifndef FILES_H
#define FILES_H

int fileGetPiece(Job *job, int piece_index, char *buf);
int filePutPiece(Job *job, int piece_index, char *buf);

long fileGetSize(char *file_name);
int fileGetPieceCount(Job *job);

#endif /* FILES_H */
