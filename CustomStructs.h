/* 
 * CustomStructs.h :
 * Contains Structures to hold the required data.
 * Also Defines constants
 * @author Sujay Paranjape
 */

#ifndef CUSTOMSTRUCTS_H
#define CUSTOMSTRUCTS_H
#include<sys/socket.h>
#include<sys/types.h>
#include<stdint.h>
#include<stdint.h>
#include<iostream>
#include<vector>
#include<string>
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
#include<sys/time.h>

/*
 *  Length of the command that is sent over the network to peers.
 */
#define COMMANDLEN (sizeof(uint32_t))

/*
 * No of maximum connection a client can maintain
 */
#define MAX_CLIENT_CONNECTIONS (3)

/*
 * Buffer size to read data from a socket or to write data into the socket
 */
#define BUFF 1024

/*
 * Enums.
 */

enum HOSTTYPE {CLIENT,SERVER};
enum OPTIONS{INVALID =0,HELP,CONNECT,REGISTER,LIST,EXIT,MYIP,UPLOAD,DOWNLOAD,TERMINATE,MYPORT,CREATOR};
enum COMMAND{PORTINFO=0,UPLOAD_CMD,DOWNLOAD_CMD,IPLIST,INVALID_DOWNLOAD_REQ,DISPLAY_MSG,UPLOAD_COMPLETE};
enum CONNECTIONTYPE{SERVER_CONN,PEER_CONN};
enum STATE{COMMAND_ACCEPT,PARAMETER_ACCEPT,DATA_ACCEPT};
enum TRANSFER_TYPE{TX,RX};

/* 
 * Contains all the information of an IP list element.
 */
struct IpListElement
{
int id;
HOSTTYPE hostType;
char hostName[100];
char hostIp[INET6_ADDRSTRLEN];
char listeningPort[6];
int sockfd;
};


/*
 *  Contains all the information related to a active connection.
 */
struct ConnectionInfo
{
int sockfd;
char remoteHostName[100];
char remoteIp[100];
char remotePort[6];
char localHostName[100];
char localIp[100];
char localPort[6];
struct sockaddr_storage localSock;
struct sockaddr_storage remoteSock;
CONNECTIONTYPE connType;
STATE currentState;
char remoteListeningPort[6];
uint32_t dataSize;
uint32_t bytesReceived;
FILE * filePtr;
int errorFlag ;
struct timeval startTimer;
struct timeval endTimer;
};

#endif
