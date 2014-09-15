/* 
 * Client.h
 * Declares Client Class which has member functions for all the functionality required by the client version of the program.
 * Inherits the ClientServerBase class for common functionality
 * @author Sujay Paranjape
 */

#ifndef CLIENT_H
#define CLIENT_H


#include "CustomStructs.h"
#include "ClientServerBase.h"

#include <iostream>
#include <vector>
#include <string>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <ifaddrs.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>


class Client: public ClientServerBase
{
public:
Client();
Client(int);
~Client();
//virtual OPTIONS isValidCommand(string);
 virtual void handleCommand(OPTIONS);
 virtual OPTIONS isValidCommand(string);
 virtual void handleSockInput(int);
 virtual void help();
 int connectTo(string,string);
 int validateConnectParameters(char * dest,char * port , struct sockaddr_storage *);
 int isHostinIpList(struct sockaddr_storage);
 void sendListeningPortInfo(int new_fd);
 int validateFile(char *filePath_char,char * fileName_char, uint32_t * fileSize);
 void sendFile(int connectionId,string fileName_str);
 string receiveParameters(int sockfd);
 void printTransferRate(ConnectionInfo * connInfo, TRANSFER_TYPE type);
private:
int serverSock;
fd_set download_fds;
int pendingDownloads;
};

#endif
