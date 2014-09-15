/* 
 * Main.cpp
 * Takes input parameters and runs the corresponding version of  
 * peer to peer program. (i.e. As a Client or as Server ) 
 * @author Sujay Paranjape
 */


#include "CustomStructs.h"
#include <iostream>
#include "Client.h"
#include "Server.h"
#include<sstream>

void printUsage(char *);
int validatePort(char *);

using namespace std;
int main(int argc, char *argv[])
{

if(argc < 2)
	{
		cout<< "Incomplete parameters"<<endl;
		printUsage(argv[0]);
		exit(1);
	}

int listeningPort ;

if(strcmp(argv[1],"c") == 0 && (listeningPort =validatePort(argv[2]) ) > 0 )
{
	ClientServerBase * ptr = new Client(listeningPort);
	ptr->Init();
	delete ptr;
}
else if(strcmp(argv[1],"s") == 0 && (listeningPort =validatePort(argv[2]) ) > 0 )
	{
		ClientServerBase * ptr = new Server(listeningPort);
		ptr->Init();
		delete ptr;
	}
else
{
 cout<<"Invalid parameters"<<endl;
 printUsage(argv[0]);
}

cout<< "Exiting the program."<<endl;
return 0;
}

void printUsage(char * progName)
{
cout<<"Please type one of the following. Port should be a valid port between 1024 and 65536."<<endl;
cout<<progName<< " c " << "<<port>>"<<endl;
cout<<progName<< " s " << "<<port>>"<<endl;
}

int validatePort(char * port)
{
int port_int = -1;
port_int = strtol(port,NULL,10);
if(port_int < 1024 || port_int>65536 )
return -1;
else return port_int;
}
