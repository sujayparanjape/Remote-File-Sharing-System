/* 
 * Client.cpp
 * Defines Client class member functions.
 * @author Sujay Paranjape
 */

#include "CustomStructs.h"
#include "Client.h"
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
#include<iomanip>
#include<sys/time.h>
#include<math.h>

using namespace std;

//Default constructor
Client::Client()
{}

// parameterized constructor
Client::Client(int port):ClientServerBase(port)
{
serverSock = -1 ;
cout<<"Running as a Client/Peer process."<<endl;
}

//destructor
Client::~Client()
{}

/*
 * Validates if command is supported by the client version
 */
OPTIONS Client::isValidCommand(string command)
{

	if(command == "HELP")
		return HELP;
	else if(command == "CONNECT")
		return CONNECT;
	else if(command == "LIST")
		return LIST;
	else if(command == "REGISTER")
		return REGISTER;
	else if(command == "MYIP")
		return MYIP;
	else if(command == "MYPORT")
		return MYPORT;
	else if(command == "UPLOAD")
			return UPLOAD;
	else if(command == "DOWNLOAD")
			return DOWNLOAD;
	else if(command == "TERMINATE")
			return TERMINATE;
	else if(command == "EXIT")
		return EXIT;
	else if(command == "CREATOR")
			return CREATOR;
	else return INVALID;

}

/*
 * Handles the command received by the client version
 */
void Client::handleCommand(OPTIONS opt)
{
	
	switch(opt)
	{

	case CREATOR:
		displayCreator();
		break;

	case CONNECT:
		if (serverSock == -1)
		{
			cout<<"Please register with the server first"<<endl;
			string parameters;
			getline(cin,parameters);
			break;//break out of switch
		}

		{
		string parameters;
		getline(cin,parameters);
		string ip;
		string port;
		char * tmppar ;
		char dest_char[100];
		char * port_char;
		char ip_char[INET6_ADDRSTRLEN];
		char * parameters_char= new char[parameters.size()+1];
		strncpy(parameters_char,parameters.c_str(),parameters.size());
		parameters_char[parameters.size()]= '\0';
		tmppar = strtok(parameters_char," ");
		strcpy(dest_char,tmppar);
		if(dest_char == NULL )
		{	
			cout << "error in parameter scanning\n";
			return ;
		}

		port_char= strtok(NULL," ");
		if(port_char == NULL )
		{	
			cout << "error in parameter scanning\n";
			return ;
		}
		
		struct sockaddr_in tempAddr;

		int searchField = 0;// 0 hostName , 1 IP

		if(inet_pton(AF_INET,dest_char,&tempAddr.sin_addr) == 1 )
		{
			// dest field contains ip
			searchField = 1;
			strcpy(ip_char,dest_char); // can cause seg fault. No length check
		}
		else
		{
			// dest field contains hostname
			searchField= 0;

		}


		//duplicate connection ?
		if(isDuplicate(dest_char,port_char))
		{
			cout<< "Duplicate connections not allowed. Connection aborted."<<endl;
					return;
		}

		/// check if present in IP list
				if(! isPresentInIpList(searchField,dest_char, port_char, ip_char) ) // after this call , dest_char will have hostname.
				{
					cout<< "Given (destination,port) combination not present in the IP list. Connection aborted."<<endl;
					return;
				}


		//self connection request ?
		if(isSelfConnection(dest_char,port_char))
		{
			cout<< "Self connection not allowed. Connection aborted."<<endl;
			return;
		}

		ip.assign(ip_char);
		port.assign(port_char);

		// no of connections > 3 ?
		if((*(getActiveConnections())).size()> MAX_CLIENT_CONNECTIONS)
		{
			cout<<"Already connected to three clients. No more connections permitted."<<endl;
			return;
		}

		int new_fd;
		
		new_fd=connectTo(ip,port);

		if(new_fd>0)
		{
			struct ConnectionInfo conn;
			conn.sockfd=new_fd; 
			conn.localSock = getLocalInfo(new_fd);
			conn.remoteSock = getRemoteInfo(new_fd);

			conn.currentState =COMMAND_ACCEPT;
			char hostName_char[100],remoteIp_char[INET6_ADDRSTRLEN];
			int remotePort;

			int hostNameLen =100;

			if(getHostName(&conn.remoteSock,hostName_char,&hostNameLen) == 0)
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
			strcpy(conn.remoteListeningPort,port_char);
			addActiveConnection(conn);
			sendListeningPortInfo(new_fd);
			addtoMasterFdSet(new_fd);
			cout<<"Connected to a peer "<<conn.remoteHostName<<"( Ip: "<<conn.remoteIp<<", Port:"<<conn.remoteListeningPort<<")."<<endl;
		}
		else
		{
			cout<<"unable to connect to the requested host"<<endl;
		}
		
		delete[] parameters_char;
		}	
		break;

	case HELP:
		help();
		break;

	case MYPORT:
		cout<<"Listening port is "<<getMyPort()<<endl;
		break;

	case LIST:
		displayActiveConnections();
		break;

	case MYIP:
		cout<<"Local Ip is "<<getMyIp()<<endl;
		break;
	case REGISTER:
		{
					if (serverSock > 0)
					{
						cout<<"Already registered with a server."<<endl;
						break;
					}
					string parameters;
					getline(cin,parameters);
					string ip;
					string port;
					char * tmppar ;
					char * parameters_char= new char[parameters.size()+1];
					strncpy(parameters_char,parameters.c_str(),parameters.size());
					parameters_char[parameters.size()]= '\0';

					tmppar = strtok(parameters_char," ");
					if(tmppar == NULL )
					{
						cout << "error in parameter scanning\n";
						return ;
					}
					ip.assign(tmppar);
					tmppar = strtok(NULL," ");
					if(tmppar == NULL )
					{
						cout << "error in parameter scanning\n";
						return ;
					}
					port.assign(tmppar);

					//need to consume remaining input ?

					int new_fd;
					new_fd=connectTo(ip,port);

							if(new_fd>0)
							{

								struct ConnectionInfo conn;
								conn.sockfd=new_fd;

								conn.localSock = getLocalInfo(new_fd);
								conn.remoteSock = getRemoteInfo(new_fd);
								strncpy(conn.remoteIp,ip.c_str(),ip.size());
								conn.remoteIp[ip.size()]='\0';
								conn.connType = SERVER_CONN;
								conn.currentState = COMMAND_ACCEPT;
								strncpy(conn.remoteListeningPort,port.c_str(),port.size());
								conn.remoteListeningPort[port.size()] = '\0';

								char hostName_char[100];
								int hostNameLen=100;

									if(getHostName(&conn.remoteSock,hostName_char,&hostNameLen) == 0)
									{
										strcpy(conn.remoteHostName,hostName_char); // can cause seg fault.
									}
									else
									{
										strncpy(conn.remoteHostName,"unknown",7);
										conn.remoteHostName[7] = '\0';
									}
									memset(hostName_char,'\0',sizeof hostName_char);

									// set local hostName
									if(getHostName(&conn.localSock,hostName_char,&hostNameLen) == 0)
									{
										strcpy(conn.localHostName,hostName_char); // can cause seg fault.
									}
									else
									{
										strncpy(conn.localHostName,"unknown",7);
										conn.localHostName[7] = '\0';
									}

								addActiveConnection(conn);
								addtoMasterFdSet(new_fd);

								//send the linstening port info to server;
								sendListeningPortInfo(new_fd);

								cout<<"Registered with the server "<<conn.remoteHostName<<"( Ip: "<<conn.remoteIp<<", Port:"<<conn.remoteListeningPort<<")."<<endl;

								//any error handling required ?

								serverSock = new_fd;
							}
							else
							{
								cout<<"Unable to register with the requested host."<<endl;
								serverSock = -1;

							}

							delete[] parameters_char;
		}
		break;


	case UPLOAD:
//		if (serverSock == -1)
//				{
//					cout<<"Please register with the server first"<<endl;
//					getline(cin,parameters);
//					break;//break out of switch
//				}
		{
			string parameters;
			getline(cin,parameters);
			char * tmppar, * connectionId_char, * filePath_char;
			char * parameters_char= new char[parameters.size()+1];
			strncpy(parameters_char,parameters.c_str(),parameters.size());
			parameters_char[parameters.size()]= '\0';

			tmppar = strtok(parameters_char," ");
			if(tmppar == NULL )
			{
				cout << "Error in parameter scanning. Invalid parameters for the command.\n";
				return ;
			}
			int connectionId;
			connectionId = strtol(tmppar,NULL,10);
			if(connectionId < 2 || connectionId >  (*getActiveConnections()).size())
			{
				cout<<"Invalid connection for upload command"<<endl;
				return;
			}
			tmppar = strtok(NULL," ");
			if(tmppar == NULL )
			{
				cout << "Error in parameter scanning. Invalid parameters for the command.\n";
				return ;
			}
			filePath_char = tmppar;
			char  fileName_char[100];
			uint32_t fileSize;

			if (validateFile(filePath_char, fileName_char, &fileSize) != 0)
			{
				cout<<" Invalid file."<<endl;
				return;
			}

			ConnectionInfo * connInfo= &(*getActiveConnections())[connectionId-1];
			connInfo->dataSize = fileSize;
			stringstream parameters_ss;

			parameters_ss<<fileSize;
			parameters_ss<<"\n";
			parameters_ss<<fileName_char;
			parameters_ss<<"\n";
			uint32_t paramSize = parameters_ss.str().size();

			sendCommand(connectionId,UPLOAD_CMD,paramSize,parameters_ss.str());

			//sendFile
			string filePath_str(filePath_char);
			cout<<"Uploading the file to a peer "<< connInfo->remoteHostName <<"( Ip: "<<connInfo->remoteIp<< ", PORT: "<<connInfo->remoteListeningPort<<")."<<endl;
			sendFile(connectionId, filePath_str);
			cout<<"Upload Complete."<<endl;

			//print transfer rate
			printTransferRate(connInfo,TX);

			//reset timers  and other counts ?
		}
		break;


	case DOWNLOAD:
//		if (serverSock == -1)
//				{
//					cout<<"Please register with the server first"<<endl;
//					getline(cin,parameters);
//					break;//break out of switch
//				}

		{
		//scan input
		char *tmp;
		string parameters;
		getline(cin,parameters);
		char * parameters_char= new char[parameters.size()+1];
		strncpy(parameters_char,parameters.c_str(),parameters.size());
		parameters_char[parameters.size()]= '\0';

		tmp = strtok(parameters_char," ");
		vector<char*> pCollection;
		int parError =0;
		while (tmp != NULL )
		{
			int connectionId = -1;
			connectionId = (int)strtol(tmp,NULL,10);
			if(connectionId == 1)
			{
				cout<< " Invalid connection Id. Download from Server not allowed."<<endl;
				parError = 1;
				break;
			}
			else
			if(connectionId < 2)
			{
				cout<< " Invalid connection Id."<<endl;
				parError = 1;
				break;
			}
			else
			if(connectionId> (*getActiveConnections()).size())
			{
				cout<< "Invalid connectionId("<<connectionId<<"). Please select valid connection from the connection list ."<<endl;
				parError = 1;
				break;
			}
			else
			{
				pCollection.push_back(tmp);
			}
			tmp = strtok(NULL," ");
			if(tmp!=NULL)
			{
				pCollection.push_back(tmp);
			}
			else
			{
				cout<< "Incomplete parameters."<<endl;
				parError = 1;
				break;
			}
			tmp = strtok(NULL," ");
		}

		if(pCollection.size() > 6 )
		{
			cout<<"No of download parameters exceeded. Cancelling the download."<<endl;
			parError = 1;
			break;
		}

		if(parError)
			{
				return;
			}

		//reset download fd set
		FD_ZERO(&download_fds);
		int i = 0;
		vector<ConnectionInfo> * infoPtr =  getActiveConnections();
		int download_fdmax;

		// send requests to peers for uploads
		while(i<pCollection.size())
		{
			// add fd to the download fd
			int connectionId = (int)strtol(pCollection[i],NULL,10);
			int currentFd = ((*infoPtr)[connectionId-1].sockfd);
			FD_SET(currentFd,&download_fds);
			download_fdmax = (download_fdmax < currentFd ? currentFd:download_fdmax) ;


			stringstream parameters_ss;
			parameters_ss<<pCollection[i+1]; // put filename
			// parameters_ss<<"\n";  		 /// DO NOT DELETE , temp aadjustment to check the code.
			uint32_t paramSize = parameters_ss.str().size();
			sendCommand(connectionId,DOWNLOAD_CMD,paramSize,parameters_ss.str());
			i=i+2;

		}

		//Put select plus exit logic

		fd_set init_fds = download_fds;

		while (pendingDownloads > 0)
		{
			init_fds = download_fds;
			if(select(download_fdmax+1,&download_fds,NULL,NULL,NULL)==-1)
				{
							cout<<"Error in select() while parallel download."<<endl;
							cout.flush();
							return ;
				}
				string opt;
				for(int i= 0;i<download_fdmax+1;i++)
				{
					if(FD_ISSET(i,&download_fds))
						handleSockInput(i);
				}
		}

	}
	break;

	case TERMINATE:
	{
		// scan  & validate parameter
		char *tmp;
		string parameters;
		getline(cin,parameters);
		char * parameters_char= new char[parameters.size()+1];
		strncpy(parameters_char,parameters.c_str(),parameters.size());
		parameters_char[parameters.size()]= '\0';

		tmp = strtok(parameters_char , " ");
		if (tmp ==NULL)
		{
			cout << "Invalid parameters to TERMINATE command"<<endl;
					break; // break switch
		}

		int connId_int = strtol(tmp,NULL,10);

		if(connId_int == 1)
		{
			cout<<"Cannot terminate connection with server. Termination aborted."<<endl;
		}
		else
		if(connId_int < 2  || connId_int> (*getActiveConnections()).size())
		{
			cout<<"Invalid connection selected. Termination aborted."<<endl;
		}
		else
		{
			// close the file pointers if any open
			ConnectionInfo  * connInfo = &((*getActiveConnections())[connId_int-1]);

			if(connInfo->filePtr != NULL)
			{

				fclose(connInfo->filePtr);
			}

			//close the sock
			close(connInfo->sockfd);

			// copy the hostname
			string hostName (connInfo->remoteHostName);

			//terminate the connection and remove connection from connectionList
			removeActiveConnection(connInfo->sockfd);

			cout<<"Connection with the host "<< hostName<< " terminated"<<endl;
			cout.flush();
		}

	}
	break;

	case EXIT:
	{
		exit_opt();
	}
	break;

	case INVALID:
		cout<<"Invalid command"<<endl;
		break;

	default:
		cout<<"Invalid command"<<endl;
		break;

	}
	
}

/*
 * Handles the input on given socket.
 */
void Client::handleSockInput(int sockfd)
{
	cout.flush(); // required ?

	ConnectionInfo * connInfo  =  getConnectionInfo(sockfd);
	switch(connInfo->currentState)
		{
			 case COMMAND_ACCEPT:
			 {
				 	 	 	 char cmd[COMMANDLEN];
				 			 int recvBytes;
				 			 recvBytes = recv(sockfd,cmd,COMMANDLEN,0);// < COMMANDLEN

				 			 if( recvBytes ==0)
				 			 {
				 				 // can put a server exit check and if yes exit

				 				 // client closed the connection
				 				 cout<<"A peer closed the connection. PeerHostname: "<< connInfo->remoteHostName<<" Ip: "<<connInfo->remoteIp<<endl;

				 				 // handle connection close.
				 				 close(sockfd);
				 				 removeFromMasterFdSet(sockfd);
				 				 removeActiveConnection(sockfd);
				 				 break;
				 			 }

				 			while (recvBytes<COMMANDLEN)
				 			{
				 				recvBytes += recv(sockfd,cmd+recvBytes,COMMANDLEN-recvBytes,0);
				 			}

				 			 uint32_t * cmd_nw = reinterpret_cast<uint32_t *>(cmd);
				 			 uint32_t cmd_h =ntohl(*cmd_nw);

				 			switch (cmd_h) // COMMAND enum
				 			{
				 			case IPLIST:
				 				{
				 				 cout<< "New Ip list received from the server."<<endl;
				 				 int recvBytes=0;
				 				 char parameter_c[sizeof(uint32_t)];
				 				 int  parametersize = sizeof(uint32_t);

				 				 while (recvBytes<parametersize)
									{
										recvBytes += recv(sockfd,parameter_c+recvBytes,parametersize-recvBytes,0);
									}
				 				 uint32_t datalen = ntohl(*((uint32_t *)(void *)parameter_c));

				 				 char * data = new char[datalen+1];
				 				 recvBytes = 0;
				 				 while (recvBytes<datalen)
										{
											recvBytes += recv(sockfd,data+recvBytes	,datalen-recvBytes,0);
										}

				 				 data[datalen] = '\0';


				 				 char * token = strtok(data,"\n");
				 				if(token == NULL)
				 				{
				 					cout<< "strtok retured null pointer"<<endl;
				 				}

				 				flushIpList();

				 				while (token != NULL )
				 				{
				 					// problem with null value for a struct
				 					string id(token);

				 					token = strtok(NULL,"\n");
				 					string hostType(token);
				 					char * hostType_char = new char[hostType.size()+1];
				 					strncpy(hostType_char,hostType.c_str(),hostType.size());
				 					hostType_char[hostType.size()] = '\0';

				 					token = strtok(NULL,"\n");
				 					string hostName(token);

				 					token = strtok(NULL,"\n");
				 					string ip(token);

				 					token = strtok(NULL,"\n");
				 					string port(token);

				 					struct IpListElement elem;
				 					elem.hostType = (HOSTTYPE)(int)strtol(hostType_char,NULL,10);

				 					strncpy(elem.hostName,hostName.c_str(),hostName.size());
				 					elem.hostName[hostName.size()] = '\0';

				 					strncpy(elem.hostIp,ip.c_str(),ip.size());
				 					elem.hostIp[ip.size()] = '\0';

				 					strncpy(elem.listeningPort,port.c_str(),port.size());
				 					elem.listeningPort[port.size()] = '\0';


				 					if( (strcmp(elem.hostIp,getMyIp().c_str()) == 0)  &&  ((int)strtol(elem.listeningPort,NULL,10)== getMyPort()) )
				 					{
				 					//self entry in serverIpList. can be removed if required.
				 						insertIpListElement(elem);
				 					}
				 					else
				 					{
				 						insertIpListElement(elem);

				 					}

				 					token = strtok(NULL,"\n");
				 					delete[] hostType_char;
				 				}

				 				//display the new IP list
				 				displayIpList();

				 				delete[] data;

				 				}
				 				break;

				 			case PORTINFO:
				 			{
				 				//receive portinfo
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

								ConnectionInfo * tmpConnInfo = getConnectionInfo(sockfd);
								strncpy(tmpConnInfo->remoteListeningPort,data,datalen+1);

								cout<<"Connected to a peer "<<tmpConnInfo->remoteHostName<<"( Ip: "<<tmpConnInfo->remoteIp<<", Port:"<<tmpConnInfo->remoteListeningPort<<")."<<endl;

				 			}
				 			break;

				 			case UPLOAD_CMD:
				 				cout.flush(); // required ?
				 				{
				 					uint32_t datalen = 0;
									char * ptr =(char *)(void *)(&datalen);

									int  paramLensize = 0;
									while ( paramLensize < sizeof datalen)
									{
									paramLensize += recv(sockfd,ptr+paramLensize,(sizeof datalen)- paramLensize,0);
									}

									datalen = ntohl(datalen);

									char * data = new char[datalen + 1];
									int recvedBytes = 0 ;
									while (recvedBytes < datalen)
									{
										recvedBytes  += recv(sockfd,data+recvedBytes,datalen-recvedBytes,0);
									}


									data[datalen] = '\0';


									char  fileName_char[100];//
									uint32_t dataSize;
									int errorFlag=0;
									char *tmp = strtok(data,"\n");
									dataSize = (uint32_t)strtol(tmp,NULL,10);

									if (dataSize == 0)
									{
										cout<< "datasize parameter cannot be zero."<<endl;//should not happen. Undefined behaviour if this happens.
										errorFlag = 1;
									}
									tmp = strtok(NULL,"\n");
									strcpy(fileName_char,tmp);

									string strtmp(fileName_char);



									// open the file
									FILE * destPtr = fopen(strtmp.c_str(),"wb");
									if(destPtr == NULL)
									{
										cout<< "unable to open the file for write"<<endl;
										errorFlag =1;
									}

									// change the state to download
									connInfo->currentState = DATA_ACCEPT;

									//setup bytecount
									connInfo->dataSize = dataSize;
									connInfo->bytesReceived= 0;

									// set the filePtr
									connInfo->filePtr = destPtr;

									// setup error flag
									connInfo->errorFlag = errorFlag;
									cout.flush();

									//message
									cout<<"Peer is uploading the file. PeerName: "<<connInfo->remoteHostName<< ", FileName: "<<strtmp<<" , FileSize: "<<dataSize<<" bytes."<<endl ;

									//setup the timer ;
									//clock_gettime(CLOCK_MONOTONIC ,&connInfo->startTimer);
									gettimeofday(&connInfo->startTimer,NULL);
				 				}

				 				break;
				 			case DOWNLOAD_CMD:
				 			{

				 				cout<<"Download request received from a peer "<< connInfo->remoteHostName <<"( Ip: "<<connInfo->remoteIp<< ", PORT: "<<connInfo->remoteListeningPort<<")."<<endl;
				 				string str = receiveParameters(sockfd);
				 				cout<<"Requested file: "<<str<<endl;
//				 				cout<<"parameters are "<<str<<endl;
				 				//FILE * ptr = fopen(str.c_str(),"rb");
				 				char  fileName_char[100] ;
				 				uint32_t fileSize = 0 ;
				 				char * filePath_char = new char[str.size()+1];
				 				strncpy(filePath_char, str.c_str(),str.size());
				 				filePath_char[str.size()] = '\0';

				 				if (validateFile(filePath_char, fileName_char, &fileSize) != 0)
				 				{
				 					cout<<"Invalid file requested."<<endl;
				 					stringstream parameters_ss;
									parameters_ss<<"You requested invalid file.";
									parameters_ss<<"\n";
									uint32_t paramSize = parameters_ss.str().size();
									vector<ConnectionInfo> * tmpPtr = getActiveConnections();
									int h;
									for(h = 0; h< (*tmpPtr).size();h++)
									{
										if((*tmpPtr)[h].sockfd == sockfd )
											break;
									}
									sendCommand(h+1,INVALID_DOWNLOAD_REQ,paramSize,parameters_ss.str());

				 				}

				 				else
				 				{
				 					cout<<"Uploading the requested file."<<endl;
									stringstream parameters_ss;

									parameters_ss<<fileSize;
									parameters_ss<<"\n";
									parameters_ss<<fileName_char;
									parameters_ss<<"\n";
									uint32_t paramSize = parameters_ss.str().size();
									vector<ConnectionInfo> * tmpPtr = getActiveConnections();
									int h;
									for(h = 0; h< (*tmpPtr).size();h++)
									{
										if((*tmpPtr)[h].sockfd == sockfd )
											break;
									}

									sendCommand(h+1,UPLOAD_CMD,paramSize,parameters_ss.str());

									//sendFile
									string filePath_str(filePath_char);
									connInfo->dataSize = fileSize;
									sendFile(h+1, filePath_str);
									cout<<"Upload Complete."<<endl;
									paramSize = 31;
									sendCommand(h+1,UPLOAD_COMPLETE,paramSize,"uploaded the file successfully."); // this goes to peer


									printTransferRate(connInfo,TX);

				 				}
				 				cout.flush();

				 			}
				 			break;

				 			case INVALID_DOWNLOAD_REQ:
				 			{
				 				//display response
				 				cout<<"Download failure from a peer "<< connInfo->remoteHostName <<"( Ip: "<<connInfo->remoteIp<< ", PORT: "<<connInfo->remoteListeningPort<<")."<<endl;
				 				string str = receiveParameters(sockfd);
				 				if(str.size()>0 )
				 				cout<<"Message from the peer: "<<str<<endl;

				 				//remove fd from downloadFds.
				 				FD_CLR(sockfd,&download_fds);
				 				pendingDownloads--;
				 				cout.flush();
				 			}
				 				break;
				 			case UPLOAD_COMPLETE:
								{

									string str = receiveParameters(sockfd);

									// no need to display message from uploader
//									if(str.size()>0 )
//									cout<<"Message from the peer: "<<str<<endl;

									//remove fd from downloadFds.
									FD_CLR(sockfd,&download_fds);
									pendingDownloads--;
									cout.flush();
								}
								break;

				 			case DISPLAY_MSG:

				 				break;
							default:
								cout<<"Unknown command from a peer. Command is " <<(int)cmd_h<<endl;
								break;
				 			}
			 }
				 break;

			 case DATA_ACCEPT:

				 cout.flush();//required ?
				 char data[BUFF];
				 if(! (connInfo->errorFlag))
				 {
					 int bytesReceived = 0;
					 uint32_t pendingBytes =connInfo->dataSize - connInfo->bytesReceived; // pending bytes shouldnt evaluate to zero

					 int dataReadSize = ((uint32_t)(BUFF)) < pendingBytes ? (BUFF):(int)pendingBytes;

					 cout.flush();
					 while(bytesReceived < dataReadSize)
					 {
						 bytesReceived += recv(sockfd ,data + bytesReceived, dataReadSize- bytesReceived,0 );
					 }

					 cout.flush();

					 //write data to file
					  fwrite(data,sizeof(char),bytesReceived,connInfo->filePtr);

					  // change the byte counts
					  connInfo->bytesReceived +=bytesReceived;

					 if(connInfo->bytesReceived == connInfo->dataSize)
					 {
						 	//set finish counter
						 	//gettimeofday(&connInfo->endTimer,NULL);
						 	 gettimeofday(&connInfo->endTimer,NULL);
						 	 //clock_gettime(CLOCK_MONOTONIC ,&connInfo->endTimer);

							// change the state to command_accept
							connInfo->currentState = COMMAND_ACCEPT;

							// Close the filePtr
							fclose(connInfo->filePtr);
							connInfo->filePtr = NULL;

							// setup error flag
							connInfo->errorFlag = 0;
							cout<<"Transfer complete."<<endl;

							// calculate transfer speed
							printTransferRate(connInfo,RX);

							//reset bytecount
							connInfo->dataSize =0;
							connInfo->bytesReceived= 0;
							cout.flush();
					 }

				 }
				 else
				 {
					 cout<<"ErrorFlag already set. Some error receiving the file."<<endl;
					 cout.flush();
					 connInfo->dataSize =0;
					 connInfo->bytesReceived= 0;

					// Close the filePtr
					 if(connInfo->filePtr!= NULL)
					fclose(connInfo->filePtr);
					connInfo->filePtr = NULL;


					// Reset error flag after taking the actions.
//					connInfo->errorFlag = 0;
//					cout<<"transfer complete"<<endl;

				 }
				 cout.flush();
				 break;

			 default:
				 cout<<" Connection in unknown state/ out of sync."<<endl;
				 break;
		}
}

/*
 * Connects to a given IP at given port
 */
int Client::connectTo(string ip, string port)
{
	int sockfd,new_fd;
	struct addrinfo *receiverinfo,hints,*p;
	struct in_addr ip4addr;
	struct in6_addr ip6addr;
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];
	int rv, yes =1;
	

	if(inet_pton(AF_INET,ip.c_str(),&ip4addr) <= 0)
		if(inet_pton(AF_INET6,ip.c_str(),&ip6addr) <= 0)
		{
			//invalid ip 
			cout<<"Invalid Ip"<<endl;
			return -1;
		}
	char * port_char = new char[port.size()+1];
	strncpy(port_char,port.c_str(),port.size());
	port_char[port.size()] = '\0';

	long port_int=-1;

	port_int = strtol(port_char,NULL,10);

	if(port_int < 1024 || port_int>65536 )
	{
		cout<<"Invalid Port."<<endl;
		delete[] port_char;
		return -1;
	}
	delete[] port_char;
	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_INET;//AF_UNSPEC;
	hints.ai_socktype= SOCK_STREAM;
	
	
	if((rv=getaddrinfo(ip.c_str(),port.c_str(),&hints,&receiverinfo))!=0)
	{
		printf("Error getting addrinfo\n");
		//exit(1);
		return -1;
	}
	for(p = receiverinfo; p!= NULL; p=p->ai_next)
	{
		if( (sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol)) ==-1)
		{
		 	continue;
		}
		
		if(connect(sockfd,p->ai_addr,p->ai_addrlen) == -1)
			{
			 continue;
			}
		break;
	}	
	
	if (p==NULL)
	{
		cout<<"Unable to connect to the Ip "<< ip <<"at port "<<port<<endl;
		freeaddrinfo(receiverinfo);
		return -1;
	}
	
	freeaddrinfo(receiverinfo);
	return sockfd;

}

/*
 * validates the parameters for the connect command
 */
int Client::validateConnectParameters(char * dest, char * port, struct sockaddr_storage * retValPtr)
{
	struct sockaddr_in temp;
	if(inet_pton(AF_INET,dest,&temp.sin_addr) == 1 )
	{
		retValPtr->ss_family = AF_INET;
		((struct sockaddr_in *)retValPtr)->sin_addr = temp.sin_addr;
		((struct sockaddr_in *)retValPtr)->sin_port = strtol(port,NULL,10);
		return 0;
	}
	else
	{
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, * p;

	// will point to the results
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	// get ready to connect
	status = getaddrinfo(dest, port, &hints, &servinfo);

	if(status != 0 )
		return -1;

	p = servinfo;

	if (p == NULL)
	{
		cout<<"Unable to validate dest by hostname"<<endl;
		return -1;
	}
	else
	{
	 *retValPtr =*((struct sockaddr_storage*)&(p->ai_addr));
	  return 0;
	}
	}


}

/*
 * Checks if the host is present in the Server IP list.
 */
int Client::isHostinIpList(struct sockaddr_storage host)
{
return 1;
}

/*
 * sends the listening port info over the given socket fd.
 */
void Client::sendListeningPortInfo(int new_fd)
{
	//convert listening port to string to send it over socket
	stringstream ss;
	ss << getMyPort();
	string listeningPort_str = ss.str();

	//command PORTINFO
	uint32_t cmd = htonl((uint32_t)PORTINFO);

	// parameterLen = length of Port
	uint32_t parameterLen = htonl((uint32_t)listeningPort_str.size());


	//total datasize = commandsize + size of parametersize + actualparametersize
	int totalDataSize =  sizeof cmd + sizeof parameterLen + listeningPort_str.size();//(sizeof port_char);//+2;
	//cout<<"Total data size "<< totalDataSize<<endl;

	// create a array of data to be sent over the nw
	char * data = new char [totalDataSize];
	memcpy(data,(void *)&cmd,sizeof cmd);
	memcpy(data+(sizeof cmd),(char *)(void *)&parameterLen,(sizeof parameterLen));
	memcpy(data+(sizeof cmd)+ (sizeof parameterLen),listeningPort_str.c_str(),listeningPort_str.size());

	int len,bytes_sent = 0;

	len = totalDataSize;

	while(bytes_sent<len)
	{
		bytes_sent = +send(new_fd, data + bytes_sent,len-bytes_sent,0);
	}
	delete[] data;
}

/*
 * Validates the file and returns the name and size if valid in the given parameters.
 * Returns zero on success. -1 on failure
 */
int Client::validateFile(char *filePath_char,char * fileName_char, uint32_t * fileSize)
{
	//cout<<"Validate the file "<< filePath_char<<endl;
	FILE * sourcePtr;


	int bytesread;

	if((sourcePtr = fopen(filePath_char,"rb")) == NULL)
	{
	  //printf("Unable to open the file. \n");
	  return -1;
	}

	fseek(sourcePtr, 0L, SEEK_END);
	*fileSize = (uint32_t)ftell(sourcePtr);

	if(sourcePtr != NULL )
		fclose(sourcePtr);

	char * tmp;
	char * delim;
	delim  = strchr(filePath_char, '/');
	tmp = delim;
	while(tmp != NULL)
	{
		delim = tmp;
		tmp = strchr(tmp+1, '/');
	}

	if(delim== NULL)
	{
		strcpy(fileName_char,filePath_char);
	}
	else
	strcpy(fileName_char, delim + 1 );
	//cout<<"File Name: "<<fileName_char <<" file size: "<<*fileSize<<endl;
	return 0;

}

/*
 * sends the given file over given connection.
 */
void Client::sendFile(int connectionId,string filePath_str)
{

int sockfd = (*getActiveConnections())[connectionId-1].sockfd;

FILE *ptr = fopen(filePath_str.c_str(),"rb");
if(ptr == NULL)
{
 cout<< "error in opening the file. Send aborted"<<endl ;
 cout.flush();
 return;
}


//log the start time
//clock_gettime(CLOCK_MONOTONIC ,&(*getActiveConnections())[connectionId-1].startTimer);
gettimeofday(&(*getActiveConnections())[connectionId-1].startTimer,NULL);
int bytesread = 0,bytesSent= 0;
char tmp[BUFF];
while ( (bytesread =fread(tmp,sizeof(char),BUFF,ptr)) > 0)
{

	bytesSent  = 0;

	cout.flush();
	while(bytesSent < bytesread)
	{
		bytesSent += send(sockfd, tmp + bytesSent,bytesread-bytesSent,0);
	}
}
fclose(ptr);
//log the end time
//clock_gettime(CLOCK_MONOTONIC ,&(*getActiveConnections())[connectionId-1].endTimer);
gettimeofday(&(*getActiveConnections())[connectionId-1].endTimer,NULL);

//cout<<"send complete"<<endl;
}

/*
 *  Receives fixed size parameters  from the socket.
 */
string Client::receiveParameters(int sockfd)
{

	uint32_t datalen = 0;
	char * ptr =(char *)(void *)(&datalen);

	int  paramLensize = 0;
	while ( paramLensize < sizeof datalen)
	{
	paramLensize += recv(sockfd,ptr+paramLensize,(sizeof datalen)- paramLensize,0);
	}

	datalen = ntohl(datalen);

	char * data = new char[datalen + 1];
	int recvedBytes = 0 ;
	while (recvedBytes < datalen)
	{
	recvedBytes  += recv(sockfd,data+recvedBytes,datalen-recvedBytes,0);
	}



	data[datalen] = '\0';
//	cout<<"Received parameters are :"<< data<< endl;
	string parameters_str(data);
	return parameters_str;
}

/*
 * Prints the transfer rate associated with the given connectionInfo object.
 * @transfer Type : TX=> process was transmitter
 * 					RX=> process was receiver
 */
void Client::printTransferRate(ConnectionInfo * connInfo, TRANSFER_TYPE type)
{
	 // type = TX => transmitter
	 // type = RX=> receiver

	stringstream hname_ss;

	hname_ss<<connInfo->localHostName;
	string localHost_str,remoteHost_str;
	getline(hname_ss,localHost_str,'.');
	hname_ss.str("");
	hname_ss<<connInfo->remoteHostName;
	getline(hname_ss,remoteHost_str,'.');

	double txrate_bps;
	double txrate_Mbps;
	double tx_sec;
	double ten_pow_9 = pow (10,9);
	double ten_pow_6 = pow (10,6);

	//suseconds_t tx_us = connInfo->endTimer.tv_usec -connInfo->startTimer.tv_usec; // microseconds

	long tx_us = connInfo->endTimer.tv_usec -connInfo->startTimer.tv_usec; // microsec
	long tx_s = connInfo->endTimer.tv_sec -connInfo->startTimer.tv_sec;// seconds

	tx_sec = ( double (tx_us)) / ten_pow_6 + (double)tx_s;
	txrate_bps =  ((double)connInfo->dataSize) * 8 /tx_sec;
	txrate_Mbps = ((double)connInfo->dataSize) * 8 /tx_sec/ ten_pow_6;

	if (type == TX )
	{
		printf("Tx(%s):(%s)->(%s), File Size: %u Bytes, Time Taken: %.6f seconds, Ts Rate: %.6f Mbits/second \n"
										,localHost_str.c_str()
										,localHost_str.c_str()
										,remoteHost_str.c_str()
										,connInfo->dataSize
										,tx_sec
										,txrate_Mbps
										);


	}
	else
	if (type =RX )
	{
		printf("Rx(%s):(%s)->(%s), File Size: %u Bytes, Time Taken: %.6f seconds, Ts Rate: %.6f Mbits/second \n"
										,localHost_str.c_str()
										,remoteHost_str.c_str()
										,localHost_str.c_str()
										,connInfo->dataSize
										,tx_sec
										,txrate_Mbps
										);
	}
	cout.flush();
}

/*
 * Prints the help commands specific to client
 */
void Client::help()
{

char line_char[100];
memset(line_char,'_',99);
line_char[99]= '\0';
cout<<line_char<<endl;
cout<<"      "<<"This is a peer-to-peer Client."<<endl;
cout<<line_char<<endl;
cout<<"      "<<"Following commands are available on this Client"<<endl;
cout<<line_char<<endl;
cout<<"      "<<"1)HELP :"<<endl;
cout<<"      "<<"   Displayes available options :"<<endl<<endl;
cout<<"      "<<"2)REGISTER <<serverIp>> <<port>> :"<<endl;
cout<<"      "<<"   To register with the server with given Ip and port"<<endl<<endl;
cout<<"      "<<"3)CONNECT <<[hostname|Ip]>> <<port>> :"<<endl;
cout<<"      "<<"   Connect to the Ip/hostname on the given port. This should be present in the Server-IP-List"<<endl<<endl;
cout<<"      "<<"4)LIST :"<<endl;
cout<<"      "<<"   List all active connections."<<endl<<endl;
cout<<"      "<<"5)MYIP :"<<endl;
cout<<"      "<<"   Display the IP of the local machine."<<endl<<endl;
cout<<"      "<<"6)MYPORT :"<<endl;
cout<<"      "<<"   Display the port on which current process is listening for connectioins."<<endl<<endl;
cout<<"      "<<"7)UPLOAD <<connectionId>> <<filename>>:"<<endl;
cout<<"      "<<"   Upload the file with given name to the peer at given Id in the active connections. Only 1 upload at time is permitted."<<endl<<endl;
cout<<"      "<<"8)DOWNLOAD <<connectionId1>> <<filename1>>[ <<connectionId2>> <<filename2>>][ <<connectionId3>> <<filename3>>] :"<<endl;
cout<<"      "<<"    Download the respective files from the peers at given Id in active connection List. Max 3 downloads allowed at a time."<<endl<<endl;
cout<<"      "<<"9)TERMINATE <<connectionId>>:"<<endl;
cout<<"      "<<"   Terminate connection with the peer at given Id."<<endl<<endl;
cout<<"      "<<"10)EXIT :"<<endl;
cout<<"      "<<"   To exit the process. All the active connections will be closed."<<endl<<endl;
cout<<"      "<<"11)CREATOR :"<<endl;
cout<<"      "<<"     Display the creator of this program."<<endl<<endl;
cout<<line_char<<endl;

}
