/* 
 * ClientServerBase.cpp
 * Defines member functions of a ClientServerBase class.
 * Defines all the common functioinality required by any peer to peer process.(Client or Server)
 * @author Sujay Paranjape
 */

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
#include "ClientServerBase.h"
#include "CustomStructs.h"
#include<iomanip>
#include<sstream>

using namespace std;

//default constructor
ClientServerBase::ClientServerBase()
{

}

// parameterized constructor
/*
 * @port: Port on which process wishes to listen for incoming connections.
 */
ClientServerBase::ClientServerBase(int port)
{
	exitopt = 0;
	listeningPort= port;
	if(bindAndListen()==0)
	{
	printf("Successfully bound and listening on port %d\n",listeningPort);
	cout<<"Localhost name: "<<getMyHostName()<<", Ip: "<< getMyIp()<<endl;
	}
	else
	{
	 cout<<"Exiting the program."<<endl;
	 exit(1);
	}

}

//destructor
ClientServerBase::~ClientServerBase()
{

}

// initialize the client/server 
void ClientServerBase::Init()
{

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(0,&master);
	FD_SET(listeningSockFd,&master);
	fdmax= listeningSockFd;

	
	while(1)
	{
		if(exitopt==1 )
			break;
		read_fds= master;
		if(select(fdmax+1,&read_fds,NULL,NULL,NULL)==-1)
				{
					printf("Error in select. Exiting the process.\n");
					exit(1);
				}
		
		string opt;
		for(int i= 0;i<fdmax+1;i++)
		{
			if(FD_ISSET(i,&read_fds))
			{
				if(i == 0)
				{
					// handle stdin

					cin>>opt;
					OPTIONS commandOpt = isValidCommand(opt);

					if(commandOpt)
					{
						handleCommand(commandOpt);
					}
					else
					{
						cout<<"Invalid command"<<endl;
					}
						
					continue;	
				}
				else if (i==listeningSockFd)
				{

					// handle new connection request
					socklen_t sin_size;
					int new_fd;
					struct sockaddr_storage clientaddr;
					sin_size = sizeof clientaddr;
					new_fd= accept(listeningSockFd,(struct sockaddr*)&clientaddr,&sin_size);
					FD_SET(new_fd,&master);
					fdmax = fdmax<new_fd?new_fd:fdmax;
					struct sockaddr_storage localInfo; 
					localInfo = getLocalInfo(new_fd);

					struct ConnectionInfo conn;
					conn.sockfd=new_fd;
					conn.remoteSock = clientaddr;
					conn.localSock = getLocalInfo(new_fd);
					conn.currentState =COMMAND_ACCEPT;
					char hostName_char[100],remoteIp_char[INET6_ADDRSTRLEN];
					int remotePort;
					int hostNameLen =100;

					//set remotehostName
					if(getHostName(&clientaddr,hostName_char,&hostNameLen) == 0)
					{
						strcpy(conn.remoteHostName,hostName_char); // can cause seg fault.
					}
					else
					{
						strncpy(conn.remoteHostName,"unknown",7);
						conn.remoteHostName[7] = '\0';
					}
					memset(hostName_char,'\0',sizeof hostName_char);

					//set localhostName
					if(getHostName(&conn.localSock,hostName_char,&hostNameLen) == 0)
					{
						strcpy(conn.localHostName,hostName_char); // can cause seg fault.
					}
					else
					{
						strncpy(conn.localHostName,"unknown",7);
						conn.localHostName[7] = '\0';
					}

					if(getIp_Port(&conn.remoteSock,remoteIp_char,&remotePort)==0 )
					{

						stringstream ss;
						ss<<remotePort;
						strncpy(conn.remoteIp,remoteIp_char,INET6_ADDRSTRLEN);
						strncpy(conn.remotePort,ss.str().c_str(),ss.str().size());
						conn.remotePort[ss.str().size()] = '\0';
					}

					addActiveConnection(conn); //should this call add fd to fd set ?

					continue;

				}
				else
				{
					// handle inputs from one of the clients 
					handleSockInput(i);
					continue;
				}
					
			}
			
		}
		
		if(exitopt)
			break;
			
	}

	cout<<"You selected to exit. Good bye !\n";
	
}

/*
 * Inserts an element into the IP Server List
 * @elem: element to be inserted into the IP list
 */
void ClientServerBase::insertIpListElement(struct IpListElement elem)
{
	// update the id based on the count
	int currentId = this->serverIpList.size();
	currentId++ ;
	elem.id =currentId;

	//insert into the vector.
	this->serverIpList.push_back(elem);
}

/*
 * Removes an element from Server Ip List
 * @sockfd: Element associated with this sock fd will be removed
 */
void ClientServerBase::removeIpListElement(int sockfd)
{
	int i;
	for (i = 0 ; i<serverIpList.size() ; i ++ )
	{
		if (serverIpList[i].sockfd ==sockfd )
			break;
	}
	if(i == serverIpList.size())
	{
		cout<< "IplistElement with given sockfd not found"<<endl;
	}
	else
	{
		serverIpList.erase(serverIpList.begin()+i);
		for(int j = 0 ; j<serverIpList.size(); j ++)
		serverIpList[j].id = j +1;
	}

}

/*
 * return the external IP of the machine on which process is running.
 */
string ClientServerBase::getMyIp()
{
			int sockfd;
			if ((sockfd = socket(AF_INET, SOCK_STREAM,0))==-1)
					{
						cout <<"Error creating the socket."<< endl;
					}
			else
			{
				struct sockaddr_in addr;
				inet_pton(AF_INET,"8.8.8.8",&addr.sin_addr);
				addr.sin_family = AF_INET;
				addr.sin_port =htons( 53);
				if(connect(sockfd,(const sockaddr *)&addr,sizeof addr) == -1 )
					{
					 	 cout<<"Error in connect in getMyIp()"<<endl;
					}
			}

			struct sockaddr_storage localInfo = getLocalInfo(sockfd);

			char s[INET6_ADDRSTRLEN];

			if(localInfo.ss_family == AF_INET )
			{
				inet_ntop(localInfo.ss_family,&((struct sockaddr_in *)&localInfo)->sin_addr,s,sizeof s );
			}
			else
			if(localInfo.ss_family == AF_INET6 )
			{
				inet_ntop(localInfo.ss_family,&((struct sockaddr_in6 *)&localInfo)->sin6_addr,s,sizeof s );
			}

			string ip_str(s);

			return ip_str;

}

/*
 * Returns hostname of the machine on which process is running
 */
string ClientServerBase::getMyHostName()
{
	struct sockaddr_in tmp;
	tmp.sin_family = AF_INET;
	inet_pton(AF_INET,getMyIp().c_str(),&tmp.sin_addr);
	char hostName_char[100];
	int hostNameLen = 100;
	if(getHostName((struct sockaddr_storage *)(&tmp),hostName_char,&hostNameLen) == 0)
	{
		string hostName (hostName_char);
		return hostName;
	}
	else
	{
		return "unknown.";
	}


}

/*
 * Retuens the port no on which process is listening
 */
int ClientServerBase::getMyPort()
{
	return listeningPort;	
}


/*
 * Bind and listen on the socket given in class member ListeningPort
 * Returns 0 on soccess 1 on failure
 */
int ClientServerBase::bindAndListen()
{
	struct addrinfo *serverinfo, hints,*p;
	int sockfd,new_fd;
	int rv;
	int yes = 1;
	char strPort[6];
	strPort[5]= '\0';

	snprintf(strPort, sizeof strPort, "%d", listeningPort);
	memset(&hints,0,sizeof hints);
	hints.ai_family =AF_INET; //AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((rv = getaddrinfo(NULL,strPort,&hints,&serverinfo) )!= 0 )
		{
			printf("Some error in getaddrinfo\n");
			return 1;
		}
	
	for (p = serverinfo;p!= NULL; p=p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol))==-1)
				{
					continue;
				}
		
		if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
				{
					printf("Unable to set socketopt\n");
					close(sockfd);
					return 1;
				}
		
		if((bind(sockfd,p->ai_addr,p->ai_addrlen))==-1)
				{
					printf("Error binding on the socket\n");
					close(sockfd);
					return 1;
				}
	
		
		break;
		
	}

	if(p==NULL)
		{
			printf("Couldnt bind to any server address.\n");
			return 1;
		}

	listeningSockFd = sockfd;
	freeaddrinfo(serverinfo);
	


	if(listen(listeningSockFd,BACKLOG)==-1)
		{
		 printf("Unable to listen on the socket\n");
		 return 1;
		}

	return 0;
	
}

/*
 * Displayes the creator info
 */
void ClientServerBase::displayCreator()
{
	cout<< "********Creator*******"<<endl;
	cout<< setw(10)<<"Name :"<<"Sujay Paranjape"<<endl;
	cout<< setw(10)<<"UBIT :"<<"sujaypar"<<endl;
	cout<< setw(10)<<"UB# :"<<"50097994"<<endl;
	cout<< setw(10)<<"Email :"<<"sujaypar@buffalo.edu"<<endl;
	cout<< "***************"<<endl;
}

/*
 *  returns the listeningSockFd member .
 */
int ClientServerBase::getListeningSockFd()
{
	return listeningSockFd; 
}

/*
 * Returns the connectioninfo object associated with the given socket.
 * @sockfd: socket Fd
 */
struct ConnectionInfo * ClientServerBase::getConnectionInfo(int sockfd)
{
	
	for (int j = 0 ; j< activeConnections.size() ; j++ )
	{
		if(activeConnections[j].sockfd == sockfd)
		return &activeConnections[j];
	}
	

	cout<< "ConnectionInfo object not found for given sockfd"<<endl;

	//return empty obj. Should throw error .
	return NULL;
}

/*
 *  Dummy place holder for help. Specialized functions will be implemented by the inheriting class.
 */

void ClientServerBase::help()
{
	cout<<"No info available."<<endl;
}

/*
 * Returns the socket_storage info object filled with local information for given socket
 */
struct sockaddr_storage ClientServerBase::getLocalInfo(int sock)
{
	struct sockaddr_storage localAddrInfo;
	socklen_t len = sizeof localAddrInfo;	// can set all bits to zero , just in case of errors


	if(getsockname(sock,(struct sockaddr *)&localAddrInfo,&len)==0)
	{
		return localAddrInfo;
	}
	else
	{	
		cout<<"Error in getLocalInfo"<<endl; 
		return  localAddrInfo;
	}
	
}

/*
 * Returns the socket_storage info object filled with remote end details for given socket
 */
struct sockaddr_storage ClientServerBase::getRemoteInfo(int sock)
{

	struct sockaddr_storage remoteAddrInfo;
	socklen_t len = sizeof remoteAddrInfo;	// can set all bits to zero , just in case of errors


	if(getpeername(sock,(struct sockaddr *)&remoteAddrInfo,&len)==0)
	{
		return remoteAddrInfo;
	}
	else
	{
		cout<<"Error in getRemoteInfo"<<endl;
		return  remoteAddrInfo;
	}

}

/*
 * Adds ConnectionInfo object into the active connections list.
 */
void ClientServerBase::addActiveConnection(ConnectionInfo conn)
{
	if(conn.filePtr != NULL)
			conn.filePtr = NULL;
	activeConnections.push_back(conn);	
}


/*
 * Displayes the active connections.
 */
void ClientServerBase::displayActiveConnections()
{
int  remotePort;
char remoteIp[INET6_ADDRSTRLEN] ;

if(activeConnections.size() == 0)
{
	cout<< "No active connections."<<endl;
	return;
}

	char line_char[100];
	memset(line_char,'_',99);
	line_char[99]= '\0';
	cout<<line_char<<endl;
	cout<<setw(48)<<"Active Connections"<<endl;
	cout<<line_char<<endl;
	cout<<setw(2)<<"id"<<" "<<setw(40)<<"Hostname"<<" "<<setw(25)<<"Ip"<<" "<<setw(25)<<"ListeningPort"<<endl;
	cout<<line_char<<endl;
	int i;
	for (i = 0 ; i<activeConnections.size() ; i ++ )
	{
		cout<<setw(2)<< i+1<<" "
			<<setw(40)<<activeConnections[i].remoteHostName<<" "
			<<setw(25)<<activeConnections[i].remoteIp<<" "
			<<setw(25)<<activeConnections[i].remoteListeningPort
			<<endl;

	}
	cout<<line_char<<endl;

}

/*
 * Stores the ip and port information of the given sockaddr_storage object into the passed parameters
 */
int ClientServerBase::getIp_Port(struct sockaddr_storage *ptr,char *ip, int * port)
{
	socklen_t size = INET6_ADDRSTRLEN;

	if (ptr->ss_family == AF_INET)
	{
			if(inet_ntop(AF_INET,&(((struct sockaddr_in*)ptr)->sin_addr),ip,size) != 0)
			{
				*port = ((struct sockaddr_in*)ptr)->sin_port;
				return 0;
			}
			cout<<"failed to get ip4 from sockaddr_storage"<<endl;
			return -1;

	}
	else
	if(ptr->ss_family == AF_INET6)
	{
		if(inet_ntop(AF_INET6,&(((struct sockaddr_in6*)ptr)->sin6_addr),ip,size)!= 0)
			{
				*port = ((struct sockaddr_in6*)ptr)->sin6_port;
				return 0;
			}
			cout<<"failed to get ip6 from sockaddr_storage"<<endl;
			return -1;

	}

	else
		cout<<"unknown addr family"<<endl;
		return -1;
}

/*
 * Fills in the hostname information of the sockaddr_storage object into the hostname parameter of length HostNameLength
 */
int ClientServerBase::getHostName(struct sockaddr_storage * ptr , char * hostname,int * hostNameLength )
{

		int flags =NI_NAMEREQD;

		char  serv[512];
		size_t socklen;
		size_t servlen = sizeof serv;

		if(ptr->ss_family == AF_INET )
			socklen = sizeof(struct sockaddr_in);
		else if(ptr->ss_family == AF_INET6 )
			socklen = sizeof(struct sockaddr_in6);

		int statusCode = getnameinfo((sockaddr *)ptr, socklen,hostname,*hostNameLength,serv, servlen, flags);
		if(statusCode== 0 )
		{
			//do nothing
		}
		else
		{

			const char * error =  gai_strerror(statusCode);
			cout<< "unable to get nameinfo."<<endl;
			cout<<error<<endl;
			return -1 ;
		}

		return statusCode;

}

/*
 * Returns the pointer to the ServerIPList vector
 */
vector<IpListElement> * ClientServerBase::getIpList()
{

	return &serverIpList;

}

/*
 * Returns the pointer to the vector of active connectionInfo objects
 */
vector<ConnectionInfo> * ClientServerBase::getActiveConnections()
		{
	return &activeConnections;
		}


/*
 * Adds the given file discriptor into the MasterFdSet
 */
void ClientServerBase::addtoMasterFdSet(int fd)
{
	FD_SET(fd,&master);
	fdmax= fdmax<fd?fd:fdmax;

}

/*
 * Removes given fd from master Fd set
 */
void ClientServerBase::removeFromMasterFdSet(int fd)
{
	FD_CLR(fd,&master);

}

/*
 * removes connectionInfo object associated with given sockFd from the active connections list
 */
void ClientServerBase::removeActiveConnection(int sockfd)
{
	int i = 0;
	for(i = 0; i<activeConnections.size();i++)
	{
		if (activeConnections[i].sockfd == sockfd)
			break;
	}

	if(i == activeConnections.size())
	{
		cout<< "Unable to find the activeConnection with given sockfd."<<endl;
	}
	else
	{
		//remove sock from master fd
		removeFromMasterFdSet(activeConnections[i].sockfd);
		activeConnections.erase(activeConnections.begin() + i );

	}


}

/*
 * Clears the ServerIpList
 */

void ClientServerBase::flushIpList()
{
	serverIpList.clear();
}

/*
 * Displayes the IP list
 */
void ClientServerBase::displayIpList()
{
	char line_char[100];
	memset(line_char,'_',99);
	line_char[99]= '\0';
	cout<<line_char<<endl;
	cout<<setw(48)<<"Server IP List"<<endl;
	cout<<line_char<<endl;
	cout<<setw(2)<<"Id"<<" "<<setw(40)<<"Hostname"<<" "<<setw(25)<<"Ip"<<" "<<setw(25)<<"ListeningPort"<<endl;
	cout<<line_char<<endl;
	int i;
	for (i = 0 ; i<serverIpList.size() ; i ++ )
	{
		cout<<setw(2)<<serverIpList[i].id<<" "
			<<setw(40)<<serverIpList[i].hostName<<" "
			<<setw(25)<<serverIpList[i].hostIp<<" "
			<<setw(25)<<serverIpList[i].listeningPort
			<<endl;
	}
	cout<<line_char<<endl;

}

/*
 * Checks if given element present in the Server Ip list. If [resent then fills in the missing parameter
 * @searchField : 0 => dest contains hostName. Procedure needs to Fill the IP,port info from the this.
 * 				  1 => dest contains Ip, Procedure needs to Fill the HostName,port info from the this.
 */
int ClientServerBase::isPresentInIpList(int searchField,char * dest, char * port, char *ip) //not taking dest size parameter . Can cause segmentatioin fault
{
	if(searchField == 0) // dest is hostName
	{

		int i;
		for (i = 0 ; i<serverIpList.size() ; i ++)
			{

			 if( strcmp( serverIpList[i].hostName, dest) == 0 && strcmp(serverIpList[i].listeningPort, port) == 0 )
			//if( strcmp( serverIpList[i].hostName, dest) == 0 )
			 {
				 strcpy(ip,serverIpList[i].hostIp); //length?
				 return 1;
			 }
			}

		if(i==serverIpList.size())
			return 0;

	}
	else
	if( searchField == 1)//dest is IP
	{

		int i;
		for (i = 0 ; i<serverIpList.size() ; i ++)
			{
			 if( strcmp( serverIpList[i].hostIp, dest) == 0 && strcmp(serverIpList[i].listeningPort, port) == 0 )
//			 if( strcmp( serverIpList[i].hostIp, dest) == 0)
			 {
				 strcpy(dest,serverIpList[i].hostName); //length?
				 return 1;
			 }
			}
		if(i==serverIpList.size())
					return 0;
	}

}


/*
 * Checks if given dest present in the active connections.
 * Returns 0 for no duplicate
 * 		   1 for duplicate
 */
int ClientServerBase::isDuplicate(char * dest_char,char * port_char)
{

int  remotePort;
char remoteIp[INET6_ADDRSTRLEN] ;

int i;
for( i = 0; i<activeConnections.size();i++)
{
		//if(strcmp(activeConnections[i].remoteHostName, dest_char) == 0 && strcmp( activeConnections[i].remoteListeningPort, port_char)==0)
		if(strcmp(activeConnections[i].remoteHostName, dest_char) == 0)
		{
			break;
		}
		else
		{
			//do nothing
		}

}

if( i == activeConnections.size())
return 0 ;// no duplicate
else
return 1; // duplicate
}

/*
 * Checks if given dest,port represents selfconnection
 * returns 1 on self connection else 0
 */

int ClientServerBase::isSelfConnection(char * dest_char,char * port_char)
{
	struct sockaddr_in tmpsock;

			inet_pton(AF_INET,getMyIp().c_str(),&tmpsock.sin_addr);
			tmpsock.sin_family = AF_INET;
			char hostName_char[100];
			int hostNameLen = 100;
if(getHostName((struct sockaddr_storage *)(&tmpsock),hostName_char,&hostNameLen) == 0)
{
	//do nothing
}
else
{
	strncpy(hostName_char,"unknown",7);
	hostName_char[7] = '\0';
}
stringstream ss ;
ss<<getMyPort();
char listeningport_char[6] ;
strcpy(listeningport_char,ss.str().c_str());
listeningport_char[6] = '\0';
if(strcmp(hostName_char, dest_char)==0 && strcmp(listeningport_char,port_char)==0)
{
return 1; // self conn
}
else
{
return 0; // not a self connection
}

}

/*
 * Sends given commands and parameters to the given connection.
 */
void ClientServerBase::sendCommand(int connectionId,COMMAND cmd,uint32_t paramSize,string par_str)
{
//cout<<"Send command: "<<cmd<<" parameter size: "<<paramSize<<" actual parameters :"<<par_str<<endl;
cout.flush();


	//command to uint32
	uint32_t cmd_u = htonl((uint32_t)cmd);

	// parameterLen = length of Port
	paramSize= htonl(paramSize);


	//total datasize = commandsize + size of parametersize + actualparametersize
	int totalDataSize =  sizeof cmd_u + sizeof paramSize + par_str.size();//(sizeof port_char);//+2;

	// create a array of data to be sent over the nw
	char * data = new char [totalDataSize];
	memcpy(data,(void *)&cmd_u,sizeof cmd_u);
	memcpy(data+(sizeof cmd_u),(char *)(void *)&paramSize,(sizeof paramSize));
	memcpy(data+(sizeof cmd_u)+ (sizeof paramSize),par_str.c_str(),par_str.size());

	int len,bytes_sent = 0;

	len = totalDataSize;

	int new_fd = (*getActiveConnections())[connectionId-1].sockfd;
	while(bytes_sent<len)
	{
		bytes_sent = +send(new_fd, data + bytes_sent,len-bytes_sent,0);
	}

	delete[] data;

	cout.flush();
}

/*
 *  Cleans up all the connection and objects before exiting and sets Exitopt =1
 */
void ClientServerBase::exit_opt()
{
	vector<ConnectionInfo>  * connList = getActiveConnections();

	for (int i= 0 ; i< (*connList).size() ; i ++ )
	{
		int j = (i+1)%(*connList).size(); // this is to make sure that peer connections are closed before server connections.
		if((*connList)[j].filePtr != NULL)
					{
						fclose((*connList)[j].filePtr);
					}


					//close the sock
					close((*connList)[j].sockfd);
					removeFromMasterFdSet((*connList)[j].sockfd);

				cout<<"Closed the connection with Host "
					<<(*connList)[j].remoteHostName
					<<"( Ip: "<<(*connList)[j].remoteIp
					<<", Port: "<<(*connList)[j].remoteListeningPort
					<<")."<<endl;
	}


	//clear the vector
	(*connList).clear();

	// close the listening sockfd
	removeFromMasterFdSet(getListeningSockFd());
	close(getListeningSockFd());

	exitopt = 1;
}

