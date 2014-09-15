/* 
 * Server.cpp
 * Defines Server Class member functions.
  * @author Sujay Paranjape
 */

#include "CustomStructs.h"
#include "Server.h"
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
#include<sstream>

//default constructor
Server::Server()
{}

//parameterized constructor
Server::Server(int port):ClientServerBase(port)
{


	struct IpListElement  elem;
	struct sockaddr_storage localSock;//  = getLocalInfo(getListeningSockFd());

	localSock.ss_family = AF_INET;

	if(inet_pton(AF_INET,getMyIp().c_str(),&((struct sockaddr_in*)&localSock)->sin_addr)!= 1)
	{
		 if(inet_pton(AF_INET6,getMyIp().c_str(),&((struct sockaddr_in6*)&localSock)->sin6_addr)!= 1)
			 cout<<"unable to get struct sin_addr(6) from IP"<<endl;
		 else
			 localSock.ss_family = AF_INET6;
	}
	if(localSock.ss_family == AF_INET)
	{
		((struct sockaddr_in*)&localSock)->sin_port= htons(getMyPort());
	}
	else
	{
		((struct sockaddr_in6*)&localSock)->sin6_port= htons(getMyPort());
	}

	char hostName[512];
	int hnameSize = sizeof hostName;
	if(getHostName(&localSock, hostName,&hnameSize)< 0)
	{
		strncpy(hostName,"unknown",7);
		hostName[7] = '\0';
	}
	else
	{

	}

	elem.hostType = SERVER;

	strncpy(elem.hostName,hostName,sizeof elem.hostName);

	string ip = getMyIp();

	strncpy(elem.hostIp,ip.c_str(),INET6_ADDRSTRLEN);


	stringstream ss;
	ss << getMyPort();
	string listeningPort_str = ss.str();


	strncpy(elem.listeningPort,listeningPort_str.c_str(),listeningPort_str.size());
	elem.listeningPort[listeningPort_str.size()] = '\0';
	insertIpListElement(elem);
	cout<<"Running as a Server process."<<endl;

}

//destructor
Server::~Server()
{}

/*
 * handles the command received by the server version of the program
 */
void Server::handleCommand(OPTIONS opt)
 {
	switch (opt)
	{
	 case LIST:
		displayActiveConnections();
		break;
	 case MYIP:
		cout<<"Local ip is "<<getMyIp()<<endl;
		break;
	 case HELP:
		help();
		break;
	 case MYPORT:
		cout<<"Listening port is "<<getMyPort()<<endl;
		break;
	 case EXIT:
		exit_opt();
		break;
	 case CREATOR:
		 displayCreator();
		break;
	 case INVALID:
		 cout<<"Invalid command."<<endl;
 		break;
	 default:
		 cout<<"Invalid command."<<endl;
 		break;
	}


 }
 
/*
 * Checks if the given command is valid for the server version
 */
OPTIONS Server::isValidCommand(string command)
 {
		if(command == "HELP")
			return HELP;
		else if(command == "MYIP")
			return MYIP;
		else if(command == "LIST")
			return LIST;
		else if(command == "MYPORT")
			return MYPORT;
		else if(command == "EXIT")
			return EXIT;
		else if(command == "CREATOR")
			return CREATOR;
		else return INVALID;
	
 }
 
/*
 * Hanldes the input from a given socket as a server.
 */
void Server::handleSockInput(int sockfd)
 {
	ConnectionInfo * connInfo  =  getConnectionInfo(sockfd);
	switch(connInfo->currentState)
	{
		 case COMMAND_ACCEPT:
		// read data of size COMMANDLEN
		 {
			 char cmd[COMMANDLEN];
			 int recvBytes;
			 recvBytes = recv(sockfd,cmd,COMMANDLEN,0);// < COMMANDLEN

			 if( recvBytes ==0)
			 {
				 // client closed the connection
				 cout<<"A client closed the connection. "
				 << connInfo->remoteHostName <<"( Ip: "<<connInfo->remoteIp<< ", PORT: "<<connInfo->remoteListeningPort<<")."<<endl;

				 // handle connection close.
				 close(sockfd);
				 removeFromMasterFdSet(sockfd);

				 //update and send Ip list to all clients
				 removeIpListElement(sockfd);
				 removeActiveConnection(sockfd);
				 broadcastIpList();
				 break;
			 }

			while (recvBytes<COMMANDLEN)
			{
				recvBytes += recv(sockfd,cmd+recvBytes,COMMANDLEN-recvBytes,0);
			}

		 uint32_t * cmd_nw = reinterpret_cast<uint32_t *>(cmd);
		 uint32_t cmd_h =ntohl(*cmd_nw);
		 switch ((int)cmd_h)
		 {
		 case PORTINFO:
		 {
			 	 // read data actual port data
		 	 	 uint32_t datalen;
		 	 	 char * ptr =reinterpret_cast<char *>(&datalen);

		 	 	 int  paramLensize = 0;
		 	 	 while ( paramLensize < sizeof datalen )
		 	 	 {
		 	 		paramLensize += recv(sockfd,ptr+paramLensize,(sizeof datalen)- paramLensize,0);
		 	 	 }

		 	 	 datalen = ntohl(datalen);

		 	 	 char * data = new char[datalen + 1];
		 	 	 recv(sockfd,data,datalen,0);

		 	 	 data[datalen] = '\0';

		 	 	 struct IpListElement elem;

		 	 	 elem.hostType = CLIENT;

		 	 	// get Ip
		 	 	struct sockaddr_storage remoteaddr = getRemoteInfo(sockfd);
		 	 	int hnamelength =sizeof elem.hostName;
		 	 	if(getHostName(&remoteaddr,elem.
		 	 		hostName,&hnamelength) < 0)
		 	 		{
		 	 			cout<<"unable to lookup client hostname";
		 	 			strncpy(elem.hostName,"unknown", 7);
		 	 			elem.hostName[7]= '\0';
		 	 		}

		 	 	if(remoteaddr.ss_family == AF_INET)
		 	 	{

		 	 		if ( inet_ntop(remoteaddr.ss_family, &(((struct sockaddr_in *)&remoteaddr)->sin_addr), elem.hostIp , sizeof elem.hostIp) == NULL )
		 	 		{
		 	 			cout<<"unable to get remote ip4"<<endl;
		 	 		}

		 	 	}
		 	 	else if (remoteaddr.ss_family == AF_INET6)
		 	 	{

		 	 		if (inet_ntop(remoteaddr.ss_family, &(((struct sockaddr_in6 *)&remoteaddr)->sin6_addr), elem.hostIp , sizeof elem.hostIp)== NULL )
		 	 				 	 		{
		 	 				 	 			cout<<"unable to get remote ip6"<<endl;
		 	 				 	 		}
		 	 	}

		 	 	strncpy(elem.listeningPort,data,datalen+1);//can cause issues if datalen exceeds listenign port size.
		 	 	elem.sockfd = sockfd;
		 	 	insertIpListElement(elem);
		 	 	delete[] data;

		 	 	// update listening portinfo of a connection.
		 	 	ConnectionInfo * tmpConnInfo = getConnectionInfo(sockfd);
		 	 	strncpy(tmpConnInfo->remoteListeningPort,elem.listeningPort,datalen+1);

		 	 	//print message
		 	 	cout<<"New client registered. Hostname: "<<tmpConnInfo->remoteHostName<<" Ip: "<< tmpConnInfo->remoteIp<< " ListeningPort: "<<tmpConnInfo->remoteListeningPort <<endl;

		 	 	//send ip list to all the clients.
		 	 	broadcastIpList();

		 }
		 break;

		 case UPLOAD:
			 cout<< "Upload command received. No action."<<endl;
		 break;

		 case DOWNLOAD:
			 cout<<"Download command received. No action."<<endl;
		 break;

		 default:
			 cout<<"Unknown command received. No action"<<endl;
			 break;

		 }

		 }
		break;
	case PARAMETER_ACCEPT:

		break;
	case DATA_ACCEPT:

		break;
	}
	
	
 }
 
/*
 *  Broadcast the IP list to all the connected clients
 */
void Server::broadcastIpList()
{
// total data = command + IPListLength+ IPList

//preparedata


	stringstream ss;
	stringstream data;
	vector<IpListElement> *listPtr =  getIpList();

	uint32_t u_cmd= htonl((uint32_t)IPLIST);

	ss.write((char *)(void *)&u_cmd,sizeof  u_cmd);//command IPLIST , 1st 4 bytes



	for(int i =0 ;i<(*listPtr).size();i++)
	{
		data<<((*listPtr)[i]).id;
		data<<"\n";
		data<<((*listPtr)[i]).hostType;
		data<<"\n";
		data<<((*listPtr)[i]).hostName;
		data<<"\n";
		data<<((*listPtr)[i]).hostIp;
		data<<"\n";
		data<<((*listPtr)[i]).listeningPort;
		data<<"\n";
	}

		data.seekg(0, data.end);
		int dataLen =data.tellg();
		data.seekg (0, data.beg);
		 uint32_t u_lenParameter  = htonl((uint32_t)dataLen);
		 ss.write((char *)(void *)&u_lenParameter,sizeof  u_lenParameter);//datalength , 2nd 4 bytes
		 ss.seekg (0, ss.end);
		 int commandLen = ss.tellg();
		 ss.seekg (0, ss.beg);

     char * finaldata = new char[commandLen +dataLen];
     char * command_char = new char[commandLen];
     char * data_char = new char[dataLen];

     ss.read(finaldata,commandLen);
     data.read(finaldata+commandLen,dataLen);

 	 vector<ConnectionInfo> * connPtr =getActiveConnections();

 	 for(int i =0 ; i <(*connPtr).size();i++)
 	 {

 		int bytesSent = 0;
 		int totalBytes = commandLen +dataLen;
 		while(bytesSent<totalBytes)
 		{
 			bytesSent  += send( (*connPtr)[i].sockfd,finaldata+ bytesSent,totalBytes-bytesSent,0 );
 		}
 	 }

 	 delete[] finaldata;
 	 delete[] command_char;
 	 delete[] data_char;

}

/*
 * Prints the Help optioins specific to server version.
 */
void Server::help()
{
	char line_char[100];
	memset(line_char,'_',99);
	line_char[99]= '\0';
	cout<<line_char<<endl;
	cout<<"      "<<"This is a Server."<<endl;
	cout<<line_char<<endl;
	cout<<"      "<<"Following commands are available on this Server."<<endl;
	cout<<line_char<<endl;
	cout<<"      "<<"1)HELP :"<<endl;
	cout<<"      "<<"   Displayes available options :"<<endl<<endl;
	cout<<"      "<<"2)LIST :"<<endl;
	cout<<"      "<<"   List all active connections."<<endl<<endl;
	cout<<"      "<<"3)MYIP :"<<endl;
	cout<<"      "<<"   Display the IP of the local machine."<<endl<<endl;
	cout<<"      "<<"4)MYPORT :"<<endl;
	cout<<"      "<<"   Display the port on which current process is listening for connectioins."<<endl<<endl;
	cout<<"      "<<"5)EXIT :"<<endl;
	cout<<"      "<<"   To exit the process. All the active connections will be closed."<<endl<<endl;
	cout<<"      "<<"6)CREATOR :"<<endl;
	cout<<"      "<<"     Display the creator of this program."<<endl<<endl;
	cout<<line_char<<endl;


}

