#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL_net.h>

#include "network.h"
#include "debug.h"
#include "core.h"


#define READ_INT(ptr) (*((int*)(ptr)))
#define WRITE_INT(ptr,i) do { *((int*)(ptr)) = i; } while(0)

void messageDump(Message *msg, char *file_name)
{
	FILE *fd = fopen(file_name, "wb");
	assert(fd);
	fwrite(msg->data, msg->length, 1, fd);
	fclose(fd);
	verbose1("messageDump -> %s\n", file_name);
}

/**
	creates MSG_PACKAGE_INFO - basic version:
		MSG_PACKAGE_INFO|msg->length|file_size|piece_size|file_name|author_name

	@return NULL     on error
	        Message* on success, dynamicaly allocated
*/
Message *messageCreatePackage(PackageInfo *package)
{
	int tmp;

	assert(package != NULL);
	assert(package->file_size > 0);
	assert(package->piece_size > 0);

	// calculate required space for data
	int len = 0;
	if (package->file_name) len += strlen(package->file_name) +1;
	if (package->author_name) len += strlen(package->author_name) +1;
	len += sizeof(int);    // enum MessageType
	len += sizeof(int);    // total message length
	len += sizeof(int);    // package->piece_size
	len += sizeof(int);    // package->file_size

	verbose1("messageCreatePackage::len = %d\n", len);

	Message *msg = calloc(len + sizeof(Message), 1);
	assert(msg != NULL);

	// copy package to message
	verbose1("messageCreatePackage::copying package to message\n");

	msg->length = len;

	int pos = 0;
	tmp = MSG_PACKAGE_INFO;
	memcpy(msg->data + pos, &tmp, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, &len, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, &package->file_size, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, &package->piece_size, sizeof(int));
	pos += sizeof(int);

	tmp = strlen(package->file_name) +1;
	memcpy(msg->data + pos, package->file_name, tmp);
	pos += tmp;

	tmp = strlen(package->author_name) +1;
	memcpy(msg->data + pos, package->author_name, tmp);
	pos += tmp;

	verbose1("messageCreatePackage::pos = %d\n", pos);

	// debug msg to file
	messageDump(msg, "messageCreatePackage.txt");

	return msg;
}


/**
	requires MSG_PACKAGE_INFO - basic version:
		MSG_PACKAGE_INFO|msg->length|file_size|piece_size|file_name|author_name

	@return NULL     on error
	        Message* on success, dynamicaly allocated
*/
PackageInfo *messageParsePackage(Message *msg)
{
	assert(msg);
	messageDump(msg, "messageParsePackage.txt");

	// allocate package
	PackageInfo *pkg = calloc(sizeof(PackageInfo), 1);
	assert(pkg);

	// read items from message
	int pos = 0;
	pos += 4;   // skip MessageType
	pos += 4;   // skip message length

	pkg->file_size = READ_INT(msg->data + pos);
	verbose1("messageParsePackage::file_size = %d\n", pkg->file_size);
	pos += 4;

	pkg->piece_size = READ_INT(msg->data + pos);
	verbose1("messageParsePackage::piece_size = %d\n", pkg->piece_size);
	pos += 4;

	int len;
	len = strlen(msg->data + pos);
	verbose1("messageParsePackage::file_name::len = %d\n", len);
	pkg->file_name = calloc(len + 1, 1);
	assert(pkg->file_name);
	strcpy(pkg->file_name, msg->data + pos);
	verbose1("messageParsePackage::file_name = %s\n", pkg->file_name);
	pos += len + 1;

	len = strlen(msg->data + pos);
	verbose1("messageParsePackage::author_name::len = %d\n", len);
	pkg->author_name = calloc(len + 1, 1);
	assert(pkg->author_name);
	strcpy(pkg->author_name, msg->data + pos);
	verbose1("messageParsePackage::author_name = %s\n", pkg->author_name);
	pos += len + 1;

	assert(pos == msg->length);

	return pkg;
}


/**
	creates MSG_DATA_REQUEST -  request for one piece:
		MSG_DATA_REQUEST|count|piece_index1|...

	@param piece_index Array of pieces' indices
	@param count Number of indices in piece_index

	@return NULL     on error
	        Message* on success, dynamicaly allocated
*/
Message *messageCreateDataRequest(int count, int *piece_index)
{
	assert(count > 0);
	assert(piece_index);

	// allocate space
	int len = (2 + count) * sizeof(int);
	Message *msg = calloc(len + sizeof(Message), 1);
	assert(msg);

	verbose1("messageCreateDataRequest::copying indices to message\n");

	msg->length = len;

	int pos = 0;
	int tmp = MSG_DATA_REQUEST;
	memcpy(msg->data + pos, &tmp, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, &count, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, piece_index, count * sizeof(int));
	pos += count * sizeof(int);

	assert(pos == msg->length);

	verbose1("messageCreateDataRequest::msg created\n");
	messageDump(msg, "messageCreateDataRequest.txt");

	return msg;
}


/**
	requires MSG_DATA_REQUEST -  request for one piece:
		MSG_DATA_REQUEST|count|piece_index1|...

	@param msg Message to parse
	@param piece_index Array to store pieces' indices

	@return count Number of indices in piece_index
*/
int messageParseDataRequest(Message *msg, int *piece_index)
{
	assert(msg);
	assert(piece_index);

	int count = 0;
	int pos = 0;
	pos += 4;   // skip MessageType

	count = READ_INT(msg->data + pos);
	verbose1("messageParseDataRequest::count = %d\n", count);
	pos += 4;

	for (int i = 0; i < count; ++i) {
		piece_index[i] = READ_INT(msg->data + pos);
		verbose1("messageParseDataRequest::piece = %d\n", piece_index[i]);
		pos += 4;
	}

	assert(pos == msg->length);

	return count;
}


/**
	creates MSG_DATA_REPLY -  reply for one piece:
		MSG_DATA_REPLY|piece_index|data_length|data

	@param piece_index
	@param data_length
	@param data Bytes to send

	@return NULL     on error
	        Message* on success, dynamicaly allocated
*/
Message *messageCreateDataReply(int piece_index, const char *data, int data_length)
{
	assert(piece_index >= 0);
	assert(data);
	assert(data_length > 0);

	// allocate space
	int len = 3 * sizeof(int) + data_length;
	Message *msg = calloc(len + sizeof(Message), 1);
	assert(msg);

	verbose1("messageCreateDataReply::piece_index = %d\n", piece_index);
	verbose1("messageCreateDataReply::data_length = %d\n", data_length);
	verbose1("messageCreateDataReply::copying items to msg\n");

	msg->length = len;

	int pos = 0;
	int tmp = MSG_DATA_REPLY;
	memcpy(msg->data + pos, &tmp, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, &piece_index, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, &data_length, sizeof(int));
	pos += sizeof(int);

	memcpy(msg->data + pos, data, data_length);
	pos += data_length;

	verbose1("messageCreateDataReply::pos, msg->length = %d, %d\n", pos, msg->length);
	assert(pos == msg->length);

	verbose1("messageCreateDataReply::msg created\n");
	messageDump(msg, "messageCreateDataReply.txt");

	return msg;
}


/**
	requires MSG_DATA_REPLY -  reply for one piece:
		MSG_DATA_REPLY|piece_index|data_length|data

	@param msg Message to parse
	@param buf Memory to store data

	@return piece_index
*/
int messageParseDataReply(Message *msg, char *buf)
{
	assert(msg);
	assert(buf);

	messageDump(msg, "messageParseDataReply.txt");

	int piece_index = 0;
	int data_length = 0;
	int pos = 0;
	pos += 4;   // skip MessageType

	piece_index = READ_INT(msg->data + pos);
	verbose1("messageParseDataReply::piece_index = %d\n", piece_index);
	pos += 4;

	data_length = READ_INT(msg->data + pos);
	verbose1("messageParseDataReply::data_length = %d\n", data_length);
	pos += 4;

	memcpy(buf, msg->data + pos, data_length);
	pos += data_length;

	assert(pos == msg->length);

	return piece_index;
}


Message *networkReceiveMessage(TCPsocket sock)
{
	int result;
	int msg_type = 0;
	Message *msg = NULL;

	//  receive MessageType
	result = SDLNet_TCP_Recv(sock, &msg_type, sizeof(int));
	if(result <= 0) {
		puts("thaat");
		error();
	}

	verbose1("networkReceiveMessage::msg_type = %x\n", msg_type);

	assert(msg_type == MSG_PACKAGE_INFO
		|| msg_type == MSG_DATA_REQUEST
		|| msg_type == MSG_DATA_REPLY);

	// package info message
	if (msg_type == MSG_PACKAGE_INFO) {
		verbose1("networkReceiveMessage::MSG_PACKAGE_INFO\n");

		// receive total length
		int msg_len = 0;
		result = SDLNet_TCP_Recv(sock, &msg_len, sizeof(int));
		if (result <= 0)
			error();

		verbose1("networkReceiveMessage::msg_len = %d\n", msg_len);

		// receive the rest of message
		msg = calloc(msg_len + sizeof(Message), 1);
		assert(msg);
		msg->length = msg_len;
		//msg->data[0] = msg_type;
		//msg->data[4] = msg_len;
		WRITE_INT(msg->data + 0, msg_type);
		WRITE_INT(msg->data + 4, msg_len);

		result = SDLNet_TCP_Recv(sock, msg->data + 8, msg_len - 2*sizeof(int));
		if (result <= 0)
			error();

		verbose1("networkReceiveMessage::package received\n");
	} else if (msg_type == MSG_DATA_REQUEST) {
		verbose1("networkReceiveMessage::MSG_DATA_REQUEST\n");

		// receive count of following items
		int count = 0;
		int msg_len = 0;
		result = SDLNet_TCP_Recv(sock, &count, sizeof(int));
		verbose1("result = %d\n", result);
		if (result <= 0)
			error();

		verbose1("networkReceiveMessage::count = %d\n", count);

		// receive the rest of message
		msg_len = (count+2) * sizeof(int);
		msg = calloc(msg_len + sizeof(Message), 1);
		assert(msg);
		msg->length = msg_len;
		verbose1("networkReceiveMessage::msg->length = %d\n", msg->length);

		//msg->data[0] = msg_type;
		//msg->data[4] = count;
		WRITE_INT(msg->data + 0, msg_type);
		WRITE_INT(msg->data + 4, count);

		result = SDLNet_TCP_Recv(sock, msg->data + 8, count * sizeof(int));
		if (result <= 0)
			error();

		verbose1("networkReceiveMessage::package received\n");

		messageDump(msg, "networkReceiveMessage_MSG_DATA_REQUEST.txt");
	} else if (msg_type == MSG_DATA_REPLY) {
		verbose1("networkReceiveMessage::MSG_DATA_REPLY\n");

		// receive piece_index
		int piece_index = -1;

		result = SDLNet_TCP_Recv(sock, &piece_index, sizeof(int));
		verbose1("result = %d\n", result);
		if (result <= 0) {
			puts("_381_");
			error();
		}

		verbose1("networkReceiveMessage::piece_index = %d\n", piece_index);

		// receive length of data
		int data_len = 0;
		result = SDLNet_TCP_Recv(sock, &data_len, sizeof(int));
		verbose1("result = %d\n", result);
		if (result <= 0)
			error();

		verbose1("networkReceiveMessage::data_len = %d\n", data_len);

		// alloc struct for data
		int msg_len = 3 * sizeof(int) + data_len;
		msg = calloc(msg_len + sizeof(Message), 1);
		assert(msg);
		msg->length = msg_len;
		verbose1("networkReceiveMessage::msg->length = %d\n", msg->length);

		//msg->data[0] = msg_type; // zly typ
		//msg->data[4] = piece_index;
		//msg->data[8] = data_len;
		WRITE_INT(msg->data + 0, msg_type);
		WRITE_INT(msg->data + 4, piece_index);
		WRITE_INT(msg->data + 8, data_len);

		// receive data
		result = SDLNet_TCP_Recv(sock, msg->data + 12, data_len);
		if (result <= 0)
			error();

		verbose1("networkReceiveMessage::package received\n");

		messageDump(msg, "networkReceiveMessage_MSG_DATA_REPLY.txt");
	}

	return msg;
}


int networkSendMessage(TCPsocket sock, Message *msg)
{
	return SDLNet_TCP_Send(sock, msg->data, msg->length);
}

/**
	@param nbo_ip IP in network byte order
	          (can be obtained from SDL IPaddress)
	@return char* dynamically allocated string, like "127.0.0.1"
*/
char *nboipToString(int nbo_ip)
{
	char *s = calloc(16, 1);  // max possible length
	assert(s);

	char ip[4];
	memcpy(ip, &nbo_ip, sizeof(int));

	sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	return s;
}

