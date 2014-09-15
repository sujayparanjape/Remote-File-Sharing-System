/* 
 * Server.h
 * Declares Server Class which has member functions for all the functionality required by the Server version of the program.
 * Inherits the ClientServerBase class for common functionality
 * @author Sujay Paranjape
 */

#ifndef SERVER_H
#define SERVER_H

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
#include<ifaddrs.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

class Server: public ClientServerBase
{
public:
Server();
Server(int);
~Server();
void broadcastIpList();
//virtual OPTIONS isValidCommand(string);
 virtual void handleCommand(OPTIONS);
 virtual OPTIONS isValidCommand(string);
 virtual void handleSockInput(int);
 virtual void help();
// int connectTo(string,string);
//private:
//int serverSock;
};

#endif
