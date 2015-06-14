#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include "core.h"
#include "network.h"
#include "files.h"
#include "debug.h"


/**
    filePut: CLIENT
 */
int filePut(Job *job)
{
	// connect to server
	IPaddress ip;
	if (SDLNet_ResolveHost(&ip, "localhost", SERVER_PORT) != 0)
		error();

	printf("Waiting for receiver...\n");

	//TCPsocket sock;
	while (1) {
		job->socket = SDLNet_TCP_Open(&ip);
		if (job->socket)
			break;

		SDL_Delay(1000);
	}


	verbose1("Connected to server.\n");
	verbose1("Server port = %d\n", SWAP_BYTES_SHORT(ip.port));

	// create & send package info message
	verbose1("Creating package message.\n");

	Message *msg = NULL;
	msg = messageCreatePackage(job->package);
	verbose1("filePut::msg = %p\n", msg);

	int result;
	result = networkSendMessage(job->socket, msg);
	if(result < msg->length)
		error();
	free(msg);

	
	char *buf = calloc(job->package->piece_size, 1);
	assert(buf);

	// receive requests & send requested pieces
	for (int i=0; i<fileGetPieceCount(job); i++) {
		// receive & parse data request message
		msg = networkReceiveMessage(job->socket);
		assert(msg);

		verbose1("received data request\n");

		//int pieces[MAX_DATA_REQUESTS];
		int piece_index;
		int pieces_count;
		pieces_count = messageParseDataRequest(msg, &piece_index);
		verbose1("filePut::pieces_count = %d (should be 1)\n", pieces_count);
		if (pieces_count <= 0)
			error();
		free(msg);

		// send required pieces of file
		///char buf[1024];

		result = fileGetPiece(job, piece_index, buf);
		assert(result);

		msg = messageCreateDataReply(piece_index, buf, result);
		assert(msg);

		result = networkSendMessage(job->socket, msg);
		if(result < msg->length)
			error();
		free(msg);
	}


	SDL_Delay(500);

	SDLNet_TCP_Close(job->socket);

	return 0;
}


/**
    fileGet: SERVER
 */
int fileGet(Job *job)
{
	// make me server
	IPaddress ip;
	if (SDLNet_ResolveHost(&ip, NULL, SERVER_PORT) != 0)
		error();
	
	TCPsocket sock_server;
	sock_server = SDLNet_TCP_Open(&ip);
	if (! sock_server)
		error();

	// wait for client's connection
	printf("Waiting for sender...\n");

	//TCPsocket sock_client;
	while (1) {
		job->socket = SDLNet_TCP_Accept(sock_server);
		if(! job->socket) {
		    printf("SDLNet_TCP_Accept: %s\n", SDLNet_GetError());
		}
		else
		    break;

		SDL_Delay(1000);
	}

	verbose1("Peer connected.\n");

	// receive message
	Message *msg = networkReceiveMessage(job->socket);
	assert(msg);

	// parse message to package
	job->package = messageParsePackage(msg);
	free(msg);
	verbose1("fileGet::msg = %p\n", job->package);

	job->file_path = "received/received.txt";
	verbose1("fileGet::file_path temporarily set to %s\n", job->file_path);

	job->file = fopen( job->file_path, "wb+" );
	if (job->file == NULL) {
		perror("fopen()");
		exit(1);
	}
	verbose1("output file opened\n");


	// print package info & prompt user for receiving
	IPaddress *peer_ip;
	peer_ip = SDLNet_TCP_GetPeerAddress(job->socket);
	if (! peer_ip)
		error();
	
	char *ip_str = nboipToString(peer_ip->host);

	printf("     -- Incoming file --\n");
	printf("File:   %s\n", job->package->file_name);
	printf("Sender: %s (%s)\n", job->package->author_name, ip_str);
	printf("Size:   %d B\n", job->package->file_size);
	printf("     Download the file? y/n: ");

	int answer = getchar();
	if (answer != 'y' && answer != 'Y') {
		printf("Quiting\n");
		exit(0);
	}

	char *buf = calloc(job->package->piece_size, 1);
	assert(buf);

	// request & receive all pieces
	for (int i=0; i<fileGetPieceCount(job); i++) {
		// request for pieces
		//int pieces[5] = {0,1,2,3,4};
		msg = messageCreateDataRequest(1, &i);
		assert(msg);

		int result;
		result = networkSendMessage(job->socket, msg);
		verbose1("result = %d\n", result);
		if(result < msg->length)
			error();
		free(msg);

		// receive data
		msg = networkReceiveMessage(job->socket);
		assert(msg);

		verbose1("received data reply\n");

		int piece_index = messageParseDataReply(msg, buf);
		verbose1("fileGet::piece_index = %d\n", piece_index);
		if (piece_index < 0)
			error();
		free(msg);

		///verbose1("fileGet::buf = %*s\n", job->package->piece_size, buf);

		filePutPiece(job, piece_index, buf);
	}



	SDL_Delay(500);

	fclose( job->file );
	SDLNet_TCP_Close(sock_server);
	
	return 0;
	
}





























#if 0
int handleIncomings( void *ushort_port )
{
	// Make me server
	IPaddress ip;
	if ( SDLNet_ResolveHost(&ip, NULL, 8080) != 0 ) {
		fprintf(stderr, "%s\n", SDLNet_GetError());
		exit(2);
	}
	
	TCPsocket sock_client;
	TCPsocket sock_server;
	sock_server = SDLNet_TCP_Open(&ip);
	if ( ! sock_server ) {
		fprintf(stderr, "%s\n", SDLNet_GetError());
		exit(2);
	}
	
	
	// Create a socket set to handle up to <JOBS_MAX> sockets
	SDLNet_SocketSet set;
	set = SDLNet_AllocSocketSet(JOBS_MAX);
	if(! set) {
		printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
		exit(1);
	}
	
	// Add sock_server to set
	int numused, numready;
	numused = SDLNet_UDP_AddSocket(set, sock_server);
	if (numused == -1) {
		printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
		exit(1);
	}
	
	// Forever, check for activity on sockets
	while (1) {
		numready = SDLNet_CheckSockets(set, 1000);
		if (numready == -1) {
			printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
			perror("SDLNet_CheckSockets");
		}
		else if (numready) {
			printf("There are %d sockets with activity!\n", numready);
			
			// check all sockets with SDLNet_SocketReady and handle the active ones.
			if (SDLNet_SocketReady(sock_server)) {
				TCPsocket client;
				client = SDLNet_TCP_Accept(sock_server);
				if (client) {
					// play with the client.
				}
			}
		}
	}
	
	
	// accept new connections
	/*while (1) {
		TCPsocket sock_client;
		sock_client = SDLNet_TCP_Accept(sock_server);
		if( ! sock_client ) {
			printf("SDLNet_TCP_Accept: %s\n", SDLNet_GetError());
		}
		else {
			SDL_Thread *thread_peer;
			thread_peer = SDL_CreateThread( handleConnection, "", NULL );
			if (thread_peer) {
				printf("\nSDL_CreateThread failed: %s\n", SDL_GetError());
				exit(2);
			}
			threads[ threadsCount++ ] = thread_peer;
		}
		
		SDL_Delay(100);
	}
	
	// wait for threads to finish
	for (int i=0; i<THREADS_MAX; i++)
		SDL_WaitThread(threads[i], NULL);*/
	
	return 0;
}
#endif
