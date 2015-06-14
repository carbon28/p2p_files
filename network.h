#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>

#include "core.h"

typedef struct {
	int length;
	char data[];
} Message;

Message *messageCreatePackage(PackageInfo *package);
PackageInfo *messageParsePackage(Message *msg);

Message *messageCreateDataRequest(int count, int *piece_index);
int messageParseDataRequest(Message *msg, int *piece_index);

Message *messageCreateDataReply(int piece_index, const char *data, int data_length);
int messageParseDataReply(Message *msg, char *buf);

Message *networkReceiveMessage(TCPsocket sock);
int networkSendMessage(TCPsocket sock, Message *msg);

char *nboipToString(int nbo_ip);

enum MessageType {
	MSG_PACKAGE_INFO = 0xa,
	MSG_DATA_REQUEST = 0xb,
	MSG_DATA_REPLY   = 0xc,

	MSG_PACKAGE_INFO_EXT = 0xe,
};

#define MAX_DATA_REQUESTS 64

/**
	Returns value with swapped bytes
*/
#define SWAP_BYTES_SHORT(s) \
	( ((s & 0x00ff) << 8) | ((s >> 8) & 0x00ff) )


#endif
