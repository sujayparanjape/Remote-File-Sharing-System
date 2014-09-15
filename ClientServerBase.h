/* 
 * ClientServerBase.h
 * Declares a base class which has member functions for all the common functionality required by Client/Server versions of the program.
 * @author Sujay Paranjape
 */


#ifndef CLIENTSERVERBASE_H
#define CLIENTSERVERBASE_H
#include "CustomStructs.h"
//#include <vector>
//#include <string>
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
#include <arpa/nameser.h>
#include <resolv.h>


#define BACKLOG 10


using namespace std;

class ClientServerBase
{
	public:
    ClientServerBase();
    ClientServerBase(int);
    ~ClientServerBase();
    void Init();
    void insertIpListElement(struct IpListElement);
    void removeIpListElement(int sockfd);
    void flushIpList();
    void displayIpList();
    string getMyIp();
    int getMyPort();
    int bindAndListen();

    void displayCreator();
    int getListeningSockFd();
    struct ConnectionInfo * getConnectionInfo(int);
    void addActiveConnection(ConnectionInfo);
    void removeActiveConnection(int sockfd);
    void displayActiveConnections();
    struct sockaddr_storage getLocalInfo(int);
    struct sockaddr_storage getRemoteInfo(int);
    int getIp_Port(struct sockaddr_storage *,char *, int *);
    int getHostName(struct sockaddr_storage *, char *, int *);
    vector<IpListElement> * getIpList();
    vector<ConnectionInfo> * getActiveConnections();
    void addtoMasterFdSet(int);
    void removeFromMasterFdSet(int);
    int isPresentInIpList(int searchField,char * dest, char * port,char * ip );
    int isDuplicate(char * dest_char,char * port_char);
    int isSelfConnection(char * dest_char,char * port_char);
    string getMyHostName();
    void sendCommand(int,COMMAND,uint32_t,string);
    void exit_opt();

   // void updatePortInfo(int);

    virtual void help();
    virtual void handleCommand(OPTIONS) =0;
    virtual OPTIONS isValidCommand(string) =0;
    virtual void handleSockInput(int)=0;

	private:
    vector<IpListElement> serverIpList;
    //vector<int> activeConnections;
    vector<ConnectionInfo> activeConnections;
    int listeningPort;
    int listeningSockFd;
    fd_set master,read_fds;
    int fdmax;
    int exitopt;

};

#endif
